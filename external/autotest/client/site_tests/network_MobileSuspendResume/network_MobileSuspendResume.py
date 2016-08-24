# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
from random import choice, randint
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_suspend, rtc
from autotest_lib.client.cros.networking.chrome_testing \
        import chrome_networking_test_context as cntc

# Special import to define the location of the flimflam library.
from autotest_lib.client.cros import flimflam_test_path
import flimflam

SHILL_LOG_SCOPES = 'cellular+dbus+device+dhcp+manager+modem+portal+service'

class network_MobileSuspendResume(test.test):
    version = 1
    TIMEOUT = 60

    device_okerrors = [
        # Setting of device power can sometimes result with InProgress error
        # if it is in the process of already doing so.
        'org.chromium.flimflam.Error.InProgress',
    ]

    service_okerrors = [
        'org.chromium.flimflam.Error.InProgress',
        'org.chromium.flimflam.Error.AlreadyConnected',
    ]

    scenarios = {
        'all': [
            'scenario_suspend_mobile_enabled',
            'scenario_suspend_mobile_disabled',
            'scenario_suspend_mobile_disabled_twice',
            'scenario_autoconnect',
        ],
        'stress': [
            'scenario_suspend_mobile_random',
        ],
    }

    modem_status_checks = [
        lambda s: ('org/chromium/ModemManager' in s) or
                  ('org/freedesktop/ModemManager' in s) or
                  ('org/freedesktop/ModemManager1' in s),
        lambda s: ('meid' in s) or ('EquipmentIdentifier' in s),
        lambda s: 'Manufacturer' in s,
        lambda s: 'Device' in s
    ]

    def filterexns(self, function, exn_list):
        try:
            function()
        except dbus.exceptions.DBusException, e:
            if e._dbus_error_name not in exn_list:
                raise e

    # This function returns True when mobile service is available.  Otherwise,
    # if the timeout period has been hit, it returns false.
    def mobile_service_available(self, timeout=60):
        service = self.FindMobileService(timeout)
        if service:
            logging.info('Mobile service is available.')
            return service
        logging.info('Mobile service is not available.')
        return None

    def get_powered(self, device):
        properties = device.GetProperties(utf8_strings=True)
        logging.debug(properties)
        logging.info('Power state of mobile device is %s.',
                     ['off', 'on'][properties['Powered']])
        return properties['Powered']

    def _check_powered(self, device, check_enabled):
        properties = device.GetProperties(utf8_strings=True)
        power_state = (properties['Powered'] == 1)
        return power_state if check_enabled else not power_state

    def check_powered(self, device, check_enabled):
        logging.info('Polling to check device state is %s.',
                     'enabled' if check_enabled else 'disabled')
        utils.poll_for_condition(
            lambda: self._check_powered(device, check_enabled),
            exception=error.TestFail(
                'Failed to verify the device is in power state %s.',
                'enabled' if check_enabled else 'disabled'),
            timeout=self.TIMEOUT)
        logging.info('Verified device power state.')

    def enable_device(self, device, enable):
        lambda_func = lambda: device.Enable() if enable else device.Disable()
        self.filterexns(lambda_func,
                        network_MobileSuspendResume.device_okerrors)
        # Sometimes if we disable the modem then immediately enable the modem
        # we hit a condition where the modem seems to ignore the enable command
        # and keep the modem disabled.  This is to prevent that from happening.
        time.sleep(4)
        return self.get_powered(device) == enable

    def suspend_resume(self, duration=10):
        suspender = power_suspend.Suspender(self.resultsdir, throw=True)
        suspender.suspend(duration)
        logging.info('Machine resumed')

        # Race condition hack alert: Before we added this sleep, this
        # test was very sensitive to the relative timing of the test
        # and modem resumption.  There is a window where flimflam has
        # not yet learned that the old modem has gone away (it doesn't
        # find this out until seconds after we resume) and the test is
        # running.  If the test finds and attempts to use the old
        # modem, those operations will fail.  There's no good
        # hardware-independent way to see the modem go away and come
        # back, so instead we sleep
        time.sleep(4)

    # __get_mobile_device is a hack wrapper around the FindMobileDevice
    # that verifies that GetProperties can be called before proceeding.
    # There appears to be an issue after suspend/resume where GetProperties
    # returns with UnknownMethod called until some time later.
    def __get_mobile_device(self, timeout=TIMEOUT):
        properties = None
        start_time = time.time()
        timeout = start_time + timeout
        while properties is None and time.time() < timeout:
            try:
                device = self.FindMobileDevice(timeout)
                properties = device.GetProperties(utf8_strings=True)
            except dbus.exceptions.DBusException:
                logging.debug('Mobile device not ready yet')
                properties = None

            time.sleep(1)
        if not device:
            # If device is not found, spit the output of lsusb for debugging.
            lsusb_output = utils.system_output('lsusb', timeout=self.TIMEOUT)
            logging.debug('Mobile device not found. lsusb output:')
            logging.debug(lsusb_output)
            raise error.TestError('Mobile device not found.')
        return device

    # The suspend_mobile_enabled test suspends, then resumes the machine while
    # mobile is enabled.
    def scenario_suspend_mobile_enabled(self, **kwargs):
        device = self.__get_mobile_device()
        self.enable_device(device, True)
        if not self.mobile_service_available():
            raise error.TestError('Unable to find mobile service.')
        self.suspend_resume(20)

    # The suspend_mobile_disabled test suspends, then resumes the machine
    # while mobile is disabled.
    def scenario_suspend_mobile_disabled(self, **kwargs):
        device = self.__get_mobile_device()
        self.enable_device(device, False)
        self.suspend_resume(20)

        # This verifies that the device is in the same state before and after
        # the device is suspended/resumed.
        device = self.__get_mobile_device()
        logging.info('Checking to see if device is in the same state as prior '
                     'to suspend/resume')
        self.check_powered(device, False)

        # Turn on the device to make sure we can bring it back up.
        self.enable_device(device, True)

    # The suspend_mobile_disabled_twice subroutine is here because
    # of bug 9405.  The test will suspend/resume the device twice
    # while mobile is disabled.  We will then verify that mobile can be
    # enabled thereafter.
    def scenario_suspend_mobile_disabled_twice(self, **kwargs):
        device = self.__get_mobile_device()
        self.enable_device(device, False)

        for _ in [0, 1]:
            self.suspend_resume(20)

            # This verifies that the device is in the same state before
            # and after the device is suspended/resumed.
            device = self.__get_mobile_device()
            logging.info('Checking to see if device is in the same state as '
                         'prior to suspend/resume')
            self.check_powered(device, False)

        # Turn on the device to make sure we can bring it back up.
        self.enable_device(device, True)

    # Special override for connecting to wimax devices since it requires
    # EAP parameters.
    def connect_wimax(self, service=None, identity='test',
                      password='test', **kwargs):
      service.SetProperty('EAP.Identity', identity)
      service.SetProperty('EAP.Password', identity)
      self.flim.ConnectService(service=service, **kwargs)

    # This test randomly enables or disables the modem.  This is mainly used
    # for stress tests as it does not check the power state of the modem before
    # and after suspend/resume.
    def scenario_suspend_mobile_random(self, stress_iterations=10, **kwargs):
        logging.debug('Running suspend_mobile_random %d times' %
                      stress_iterations)
        device = self.__get_mobile_device()
        self.enable_device(device, choice([True, False]))

        # Suspend the device for a random duration, wake it,
        # wait for the service to appear, then wait for
        # some random duration before suspending again.
        for i in range(stress_iterations):
            logging.debug('Running iteration %d' % (i+1))
            self.suspend_resume(randint(10, 40))
            device = self.__get_mobile_device()
            self.enable_device(device, True)
            if not self.FindMobileService(self.TIMEOUT*2):
                raise error.TestError('Unable to find mobile service')
            time.sleep(randint(1, 30))


    # This verifies that autoconnect works.
    def scenario_autoconnect(self, **kwargs):
        device = self.__get_mobile_device()
        self.enable_device(device, True)
        service = self.FindMobileService(self.TIMEOUT)
        if not service:
            raise error.TestError('Unable to find mobile service')

        props = service.GetProperties(utf8_strings=True)
        if props['AutoConnect']:
            expected_states = ['ready', 'online', 'portal']
        else:
            expected_states = ['idle']

        for _ in xrange(5):
            # Must wait at least 20 seconds to ensure that the suspend occurs
            self.suspend_resume(20)

            # wait for the device to come back
            device = self.__get_mobile_device()

            # verify the service state is correct
            service = self.FindMobileService(self.TIMEOUT)
            if not service:
                raise error.TestFail('Cannot find mobile service')

            state, _ = self.flim.WaitForServiceState(service,
                                                     expected_states,
                                                     self.TIMEOUT)
            if not state in expected_states:
                raise error.TestFail('Mobile state %s not in %s as expected'
                                     % (state, ', '.join(expected_states)))

    # Running modem status is not supported by all modems, specifically wimax
    # type modems.
    def _skip_modem_status(self, *args, **kwargs):
        return 1

    # Returns 1 if modem_status returned output within duration.
    # otherwise, returns 0
    def _get_modem_status(self, duration=TIMEOUT):
        time_end = time.time() + duration
        while time.time() < time_end:
            status = utils.system_output('modem status', timeout=self.TIMEOUT)
            if reduce(lambda x, y: x & y(status),
                      network_MobileSuspendResume.modem_status_checks,
                      True):
                break
        else:
            return 0
        return 1

    # This is the wrapper around the running of each scenario with
    # initialization steps and final checks.
    def run_scenario(self, function_name, **kwargs):
        device = self.__get_mobile_device()

        # Initialize all tests with the power off.
        self.enable_device(device, False)

        function = getattr(self, function_name)
        logging.info('Running %s' % function_name)
        function(**kwargs)

        # By the end of each test, the mobile device should be up.
        # Here we verify that the power state of the device is up, and
        # that the mobile service can be found.
        device = self.__get_mobile_device()
        logging.info('Checking that modem is powered on after scenario %s.',
                     function_name)
        self.check_powered(device, True)

        logging.info('Scenario complete: %s.' % function_name)

        if not self.modem_status():
            raise error.TestFail('Failed to get modem_status after %s.'
                              % function_name)
        service = self.mobile_service_available()
        if not service:
            raise error.TestFail('Could not find mobile service at the end '
                                 'of test %s.' % function_name)

    def init_flimflam(self, device_type):
        # Initialize flimflam and device type specific functions.
        self.flim = flimflam.FlimFlam(dbus.SystemBus())
        self.flim.SetDebugTags(SHILL_LOG_SCOPES)

        logging.debug('Using device type: %s' % device_type)
        if device_type == flimflam.FlimFlam.DEVICE_WIMAX:
            self.FindMobileService = self.flim.FindWimaxService
            self.FindMobileDevice = self.flim.FindWimaxDevice
            self.modem_status = self._skip_modem_status
            self.connect_mobile_service= self.connect_wimax
        elif device_type == flimflam.FlimFlam.DEVICE_CELLULAR:
            self.FindMobileService = self.flim.FindCellularService
            self.FindMobileDevice = self.flim.FindCellularDevice
            self.modem_status = self._get_modem_status
            self.connect_mobile_service = self.flim.ConnectService
        else:
            raise error.TestError('Device type %s not supported yet.' %
                                  device_type)

    def run_once(self, scenario_group='all', autoconnect=False,
                 device_type=flimflam.FlimFlam.DEVICE_CELLULAR, **kwargs):

        with cntc.ChromeNetworkingTestContext():
            # Replace the test type with the list of tests
            if (scenario_group not in
                    network_MobileSuspendResume.scenarios.keys()):
                scenario_group = 'all'
            logging.info('Running scenario group: %s' % scenario_group)
            scenarios = network_MobileSuspendResume.scenarios[scenario_group]

            self.init_flimflam(device_type)

            device = self.__get_mobile_device()
            if not device:
                raise error.TestFail('Cannot find mobile device.')
            self.enable_device(device, True)

            service = self.FindMobileService(self.TIMEOUT)
            if not service:
                raise error.TestFail('Cannot find mobile service.')

            service.SetProperty('AutoConnect', dbus.Boolean(autoconnect))

            logging.info('Running scenarios with autoconnect %s.' % autoconnect)

            for t in scenarios:
                self.run_scenario(t, **kwargs)

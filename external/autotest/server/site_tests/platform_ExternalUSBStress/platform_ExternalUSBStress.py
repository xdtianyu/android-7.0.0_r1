# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, threading, time

from autotest_lib.server import autotest, test
from autotest_lib.client.common_lib import error

_WAIT_DELAY = 10
_LONG_TIMEOUT = 60
_WAKE_PRESS_IN_SEC=0.2

class platform_ExternalUSBStress(test.test):
    """Uses servo to repeatedly connect/remove USB devices."""
    version = 1

    def run_once(self, host, client_autotest, repeat, network_debug):
        self.has_lid = True

        # Check if DUT has lid.
        if host.servo.get('lid_open') == 'not_applicable':
            self.has_lid = False
        else:
            # Check if
            host.servo.lid_open()
            if host.servo.get('lid_open') != 'yes':
                raise error.TestError('SERVO has a bad lid_open control')

        autotest_client = autotest.Autotest(host)
        diff_list = []
        off_list = []
        # The servo hubs come up as diffs in connected components. These
        # should be ignored for this test.
        servo_hardware_prefix = 'Standard Microsystems Corp.'
        self.is_suspended = False

        def strip_lsusb_output(lsusb_output):
            """Finds the external USB devices plugged

            @param lsusb_output: lsusb command output to parse

            @returns plugged_list: List of plugged usb devices names

            """
            items = lsusb_output.split('\n')
            named_list = []
            unnamed_device_count = 0
            for item in items:
                columns = item.split(' ')
                if len(columns) == 6 or len(' '.join(columns[6:]).strip()) == 0:
                    logging.debug('Unnamed device located, adding generic name.')
                    name = 'Unnamed device %d' % unnamed_device_count
                    unnamed_device_count += 1
                else:
                    name = ' '.join(columns[6:]).strip()
                if not name.startswith(servo_hardware_prefix):
                    named_list.append(name)
            return named_list


        def set_hub_power(on=True):
            """Turns on or off the USB hub (dut_hub1_rst1).

            @param on: To power on the servo-usb hub or not

            @returns usb devices list if not suspended, None if suspended
            """
            reset = 'off'
            if not on:
                reset = 'on'
            host.servo.set('dut_hub1_rst1', reset)
            time.sleep(_WAIT_DELAY)


        def wait_to_detect(timeout=_LONG_TIMEOUT):
            """Waits till timeout for set of peripherals in lsusb output.

            @param timeout: timeout in seconds

            @raise error.TestFail: if timeout is reached

            """
            start_time = int(time.time())
            while True:
                connected = strip_lsusb_output(host.run('lsusb').stdout.strip())
                if diff_list.issubset(connected):
                    break
                elif int(time.time()) - start_time > timeout:
                    raise error.TestFail('USB peripherals not detected: %s' %
                                          str(diff_list.difference(connected)))
                time.sleep(1)


        def test_suspend(plugged_before_suspended=False,
                         plugged_before_resume=False):
            """Close and open lid while different USB plug status.

            @param plugged_before_suspended: USB plugged before suspended
            @param plugged_before_resume: USB plugged after suspended


            @raise error.TestFail: if USB peripherals do not match expectations.

            """
            set_hub_power(plugged_before_suspended)

            # Suspend
            boot_id = host.get_boot_id()
            if self.has_lid:
                host.servo.lid_close()
            else:
                thread = threading.Thread(target = host.suspend)
                thread.start()
            host.test_wait_for_sleep(_LONG_TIMEOUT)
            logging.debug(' --DUT suspended')
            self.is_suspended = True

            if plugged_before_resume is not plugged_before_suspended:
                set_hub_power(plugged_before_resume)

            # Resume
            if self.has_lid:
                host.servo.lid_open()
            else:
                host.servo.power_key(_WAKE_PRESS_IN_SEC)
            host.test_wait_for_resume(boot_id, _LONG_TIMEOUT)
            logging.debug(' --DUT resumed')
            self.is_suspended = False

            if not plugged_before_resume:
                time.sleep(_WAIT_DELAY)
                connected = strip_lsusb_output(host.run('lsusb').stdout.strip())
                if connected != off_list:
                    raise error.TestFail('Devices were not removed on wake.')
            else:
                wait_to_detect(_LONG_TIMEOUT)


        def test_hotplug():
            """Testing unplug-plug and check for expected peripherals.

             @raise error.TestFail: if USB peripherals do not match expectations.

            """
            set_hub_power(False)
            set_hub_power(True)
            wait_to_detect(_LONG_TIMEOUT)


        def stress_external_usb():
            """Test procedures in one iteration."""

            # Unplug/plug
            test_hotplug()

            # Suspend/resume as unplugged
            test_suspend()

            # Plug/close_lid/unplug/open_lid
            test_suspend(plugged_before_suspended=True)

            #Unplug/close_lid/plug/open_lid
            test_suspend(plugged_before_resume=True)

            # Suspend/resume as plugged
            test_suspend(plugged_before_suspended=True,
                         plugged_before_resume=True)


        host.servo.switch_usbkey('dut')
        host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')

        # There are some mice that need the data and power connection to both
        # be removed, otherwise they won't come back up.  This means that the
        # external devices should only use the usb connections labeled:
        # USB_KEY and DUT_HUB1_USB.
        set_hub_power(False)
        time.sleep(_WAIT_DELAY)
        off_list = strip_lsusb_output(host.run('lsusb').stdout.strip())
        set_hub_power(True)
        time.sleep(_WAIT_DELAY * 2)
        connected = strip_lsusb_output(host.run('lsusb').stdout.strip())
        diff_list = set(connected).difference(set(off_list))
        if len(diff_list) == 0:
            raise error.TestError('No connected devices were detected.  Make '
                                  'sure the devices are connected to USB_KEY '
                                  'and DUT_HUB1_USB on the servo board.')
        logging.debug('Connected devices list: %s', diff_list)

        autotest_client.run_test(client_autotest,
                                 exit_without_logout=True)
        for iteration in xrange(1, repeat + 1):
            logging.debug('---Iteration %d/%d' % (iteration, repeat))
            stress_external_usb()

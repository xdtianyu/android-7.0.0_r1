# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re, time
from autotest_lib.server import autotest, test
from autotest_lib.client.common_lib import error

_CHARGING = 'CHARGING'
_DISCHARGING = 'DISCHARGING'
_WAIT_SECS_AFTER_SWITCH = 5
_LONG_TIMEOUT = 120
_CLIENT_LOGIN = 'desktopui_SimpleLogin'
_WAKE_PRESS_IN_SEC = 0.2
_SUSPEND_TIME = 10

class platform_PowerStatusStress(test.test):
    """Uses RPM and servo to test the power_supply_info output. """
    version = 1


    def action_login(self):
        """Login i.e. runs running client test

        @exception TestFail failed to login within timeout.

        """
        self.autotest_client.run_test(_CLIENT_LOGIN,
                                      exit_without_logout=True)


    def wait_to_suspend(self, suspend_timeout = _LONG_TIMEOUT):
        """Wait for DUT to suspend.

        @param suspend_timeout: Time in seconds to wait to disconnect

        @exception TestFail if fail to suspend/disconnect in time

        """
        if not self.host.ping_wait_down(timeout=suspend_timeout):
            raise error.TestFail("Unable to suspend in %s sec" %
                                 suspend_timeout)


    def wait_to_come_up(self, resume_timeout = _LONG_TIMEOUT):
        """Wait for DUT to resume.

        @param resume_timeout: Time in seconds to wait to come up

        @exception TestFail if fail to come_up in time

        """
        if not self.host.wait_up(timeout=resume_timeout):
            raise error.TestFail("Unable to resume in %s sec" %
                                 resume_timeout)


    def do_suspend_resume(self):
        """ Suspends the DUT through powerd_dbus_suspend
        """
        #Suspend
        logging.debug('Suspending...')
        if self.has_lid:
            self.host.servo.lid_close()
            self.wait_to_suspend()
            time.sleep(_SUSPEND_TIME)
        else:
            self.host.suspend(suspend_time=_SUSPEND_TIME)

        #Resume
        logging.debug('Resuming...')
        if self.has_lid:
            self.host.servo.lid_open()
        else:
            self.host.servo.power_key(_WAKE_PRESS_IN_SEC)
        self.wait_to_come_up()


    def cleanup(self):
        """ Finish as powered on and lid open"""
        self.host.power_on()
        self.host.servo.lid_open()


    def switch_power_and_verify(self, powered_on, expected):
        """ Main action on switching the power state, and verifying status

        @param powered_on: a boolean ON if True, OFF else
        @param expected: touple of cmd and values to verify

        @exception TestFail  if line_power or battery state do not match
        """
        bat_state = _CHARGING if powered_on else _DISCHARGING,
        logging.info('Switching status to %s ', bat_state)
        if powered_on:
            self.host.power_on()
        else:
            self.host.power_off()
        time.sleep(_WAIT_SECS_AFTER_SWITCH)

        # Get power_supply_info output
        psi_output = self.host.run('power_supply_info').stdout.strip()
        psi_output = psi_output.replace('\n', '')

        exp_psi_online, exp_psi_enum_type, exp_psi_bat_state = expected

        is_psi_online = re.match(r'.+online:\s+%s.+' % exp_psi_online,
                                 psi_output) is not None
        is_psi_enum_type = re.match(r'.+enum type:\s+%s.+' % exp_psi_enum_type,
                                    psi_output) is not None
        is_psi_bat_state = re.match(r'.+state:\s+%s.+' % exp_psi_bat_state,
                                    psi_output) is not None

        if not all([is_psi_online, is_psi_enum_type, is_psi_bat_state]):
            raise error.TestFail('Bad %s state!' % bat_state)


    def run_once(self, host, loop_count):
        self.host = host
        dut_type = host.get_board_type()
        if dut_type != 'CHROMEBOOK':
            raise error.TestNAError(
                    'This test is not supported on %s' %  dut_type)
        self.autotest_client = autotest.Autotest(self.host)

        # Start as powered on
        if self.host.has_power():
            self.host.power_on()
        else:
            raise error.TestFail('No RPM is setup to device')

        # Check if DUT has lid.
        self.has_lid = True
        if self.host.servo.get('lid_open') == 'not_applicable':
            self.has_lid = False
        else:
            # Check if lid_open control is good.
            self.host.servo.lid_open()
            if self.host.servo.get('lid_open') != 'yes':
                raise error.TestError('BAD lid_open control. Reset servo!')

        # Login to device
        self.action_login()

        pdu_connected = True
        for i in xrange(loop_count):
            logging.info('--- Iteration %d', (i + 1))

            # Suspend/resume
            self.do_suspend_resume()

            # Discharging state
            expected = ('no', 'Disconnected', 'Discharging')
            self.switch_power_and_verify(False, expected)

            # Suspend/resume
            self.do_suspend_resume()

            # Charging state - it could be any of the three below
            expected = ('yes', 'AC', '(Charging|Fully charged|Discharging)')
            self.switch_power_and_verify(True, expected)

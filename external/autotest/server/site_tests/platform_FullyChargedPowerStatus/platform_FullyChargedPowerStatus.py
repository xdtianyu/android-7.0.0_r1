# copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, threading, time

from autotest_lib.server import autotest, test
from autotest_lib.client.common_lib import error

_LONG_TIMEOUT = 120
_WAIT_DELAY = 5
_CHROME_PATH = '/opt/google/chrome/chrome'

class platform_FullyChargedPowerStatus(test.test):
    version = 1

    def cleanup(self):
        """ Power on RPM on cleanup.

        """
        self.host.power_on()


    def get_power_supply_parameters(self):
        """ Retrieve power supply info

        @returns a list of power supply info paramenters

        """
        power_supply_info = self.host.get_power_supply_info()
        online = power_supply_info['Line Power']['online']
        state = power_supply_info['Battery']['state']
        percentage = power_supply_info['Battery']['display percentage']
        current = power_supply_info['Battery']['current (A)']
        return (online, state, int(float(percentage)), float(current))


    def check_power_charge_status(self, status):
        """ Check any power status strings are not returned as expected

        @param status: record power status set when fail

        """
        errors = list()
        online, state, percentage, current = self.get_power_supply_parameters()

        if state != 'Fully charged' and state != 'Charging' and current != 0.0:
            errors.append('Bad state %s at %s' % (state, status))

        if percentage < 95 :
            errors.append('Bad percentage %d at %s' % (percentage, status))

        if online != 'yes':
            errors.append('Bad online %s at %s' % (online, status))

        if errors:
            raise error.TestFail('; '.join(errors))


    def action_login(self):
        """Login i.e. runs running client test"""
        self.autotest_client.run_test('desktopui_SimpleLogin',
                                      exit_without_logout=True)


    def is_chrome_available(self):
        """check if _CHROME_PATH exists

        @returns true if _CHROME_PATH no exists

        """
        return self.host.run('ls %s' % _CHROME_PATH,
                             ignore_status=True).exit_status == 0


    def action_suspend(self):
        """Suspend i.e. powerd_dbus_suspend and wait

        @returns boot_id for the following resume

        """
        boot_id = self.host.get_boot_id()
        thread = threading.Thread(target = self.host.suspend)
        thread.start()
        self.host.test_wait_for_sleep(_LONG_TIMEOUT)
        logging.debug('--- Suspended')
        return boot_id


    def run_once(self, host, power_status_sets):
        self.host = host
        self.autotest_client = autotest.Autotest(self.host)

        if not self.is_chrome_available():
            raise error.TestNAError('Chrome does not reside on DUT. Test Skipped')

        if not self.host.get_board_type() == 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped')

        if self.host.has_power():
            self.host.power_on()
        else:
            raise error.TestError('No RPM is setup to device')

        online, state, percentage, current = self.get_power_supply_parameters()
        if not ( online == 'yes' and percentage > 95 ):
            raise error.TestError('The DUT is not on AC or Battery charge is low ')

        self.action_login()

        for power_status_set in power_status_sets:
            before_suspend, after_suspend, before_resume = power_status_set
            logging.info('Power status set: %s', str(power_status_set))

            # Set power before suspend
            if not before_suspend:
                self.host.power_off()
            time.sleep(_WAIT_DELAY)

            # Suspend DUT(powerd_dbus_suspend)
            boot_id = self.action_suspend()
            logging.info('DUT suspended')

            # Set power after suspend
            if after_suspend:
                self.host.power_on()
            else:
                self.host.power_off()
                time.sleep(_WAIT_DELAY)
            time.sleep(_WAIT_DELAY)

            # Set power before resume
            if before_resume:
                self.host.power_on()
            else:
                self.host.power_off()
                time.sleep(_WAIT_DELAY)
            time.sleep(_WAIT_DELAY)

            # Wait to resume DUT
            self.host.test_wait_for_resume(boot_id, _LONG_TIMEOUT)
            logging.info('DUT resumed')

            # Set power to on after resume if needed
            if not before_resume:
                self.host.power_on()
            time.sleep(_WAIT_DELAY)

            self.check_power_charge_status(str(power_status_set))

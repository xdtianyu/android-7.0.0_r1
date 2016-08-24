# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.cros.bluetooth import bluetooth_semiauto_helper


class bluetooth_RegressionClient(
        bluetooth_semiauto_helper.BluetoothSemiAutoHelper):
    """Implement Bluetooth Regression Tests with some interaction."""
    version = 1

    def _test_init(self, test_type):
        """Init test by collecting intial logs, starting dump, etc.

        @param: test_type: short string label for log files and messages
        """
        self._test_type = test_type
        logging.info('Beginning test of type %s.', test_type)
        self.start_dump()
        self.collect_logs(message=('Before %s.' % test_type))

    def _power_off(self):
        self._test_init('power_off')

    def _os_idle(self):
        self._test_init('os_idle')
        self.ask_user('OS Idle test: after pressing PASS, the OS will idle '
                      'after a short delay.  Do not prevent it from idling.'
                      '<br>After OS has idled for at least 10 seconds, use '
                      'a Bluetooth device to wake machine (or use onboard '
                      'inputs if no Bluetooth device is capable).<br>'
                      'Make sure audio continues to play over Bluetooth.')
        self.os_idle_time_set()
        self.tell_user('Going to sleep now...')
        time.sleep(20)
        self.check_working()
        self.os_idle_time_set(reset=True)
        self.collect_logs(message='After idle.')

    def _suspend(self):
        self._test_init('suspend')
        self.ask_user('OS Suspend test: after pressing PASS, the OS will '
                      'suspend.<br>It will wake on its own after some time.'
                      '<br>Audio will stop playing.')
        self.os_suspend()
        self.check_working()
        self.collect_logs(message='After suspend.')

    def _log_off(self):
        self._test_init('log_off')
        self.close_browser()
        self.login_and_open_browser()
        self.check_working()
        self.collect_logs(message='After login.')

    def _disconnect(self):
        self._test_init('disconnect')
        self.tell_user('Please disconnect all Bluetooth devices using (x).')
        self.wait_for_adapter(adapter_status=True)
        self.wait_for_connections(paired_status=True, connected_status=False)
        self.ask_user('Audio NOT playing through onboard speakers?<br>'
                      'Audio NOT playing through Bluetooth device?')
        self.collect_logs(message='After disconnect.')
        self.check_working()
        self.collect_logs(message='After reconnect.')

    def _device_off(self):
        self._test_init('device_off')
        self.tell_user('Please turn off all Bluetooth devices.<br>'
                       'Disconnect them on the Settings page if needed.')
        self.wait_for_adapter(adapter_status=True)
        self.wait_for_connections(paired_status=True, connected_status=False)
        self.ask_user('Audio NOT playing through onboard speakers?')
        self.collect_logs(message='After device turned off.')
        self.check_working(message='Please turn devices back on and connect.')
        self.collect_logs(message='After device on.')

    def _unpair(self):
        self._test_init('unpair')
        self.tell_user('Please unpair all Bluetooth devices (using (x))')
        self.wait_for_adapter(adapter_status=True)
        self.wait_for_connections(paired_status=False, connected_status=False)
        self.ask_user('No Bluetooth devices work.<br> Audio is NOT playing '
                      'through onboard speakers or wired headphones.')
        self.collect_logs(message='After unpair.')
        self.check_working(message='Please re-pair and connect devices.')
        self.collect_logs(message='After re-pair.')

    def _disable(self):
        self._test_init('disable')
        self.tell_user('Please disable Bluetooth (uncheck Enable Bluetooth).')
        self.wait_for_adapter(adapter_status=False)
        self.collect_logs(message='While disabled')
        self.wait_for_connections(paired_status=True, connected_status=False)
        self.ask_user('No Bluetooth devices work?<br> Audio is NOT playing '
                      'through onboard speakers or wired headphones?')
        self.tell_user('Please enable Bluetooth (check Enable Bluetooth).<br>'
                       'Make sure all devices are still listed after enable.')
        self.wait_for_adapter(adapter_status=True)
        self.check_working()
        self.collect_logs(message='After re-enable.')

    def run_once(self):
        """Runs Regression tests for Bluetooth.

        Two phases: before and after reboot by server. Called by run_test.
        """
        self.check_working()

        if self._test_phase == 'reboot':
            self._disable()
            self._power_off()
        elif self._test_phase == 'client':
            self._power_off()
            self._os_idle()
            self._suspend()
            self._log_off()
            self._disconnect()
            self._device_off()
            self._unpair()


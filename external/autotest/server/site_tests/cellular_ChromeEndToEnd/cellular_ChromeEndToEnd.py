# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.server import autotest, test


class cellular_ChromeEndToEnd(test.test):
    """Reboots the DUT and runs clients side tests to test cellular UI.

    """
    version = 1


    def _cold_reboot_dut(self, boot_id):
        """Cold reboot the dut.

        @param boot_id: DUT boot_id.

        """
        self._servo.get_power_state_controller().power_off()
        self._servo.get_power_state_controller().power_on()
        time.sleep(self._servo.BOOT_DELAY)
        self._client.wait_for_restart(old_boot_id=boot_id)


    def run_once(self, host, test):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param test: Cellular UI test to execute.

        """

        self._client = host
        self._servo = host.servo

        if not self._servo:
            logging.info('Host %s does not have a servo.', host.hostname)
            return

        boot_id = self._client.get_boot_id()
        self._cold_reboot_dut(boot_id)

        client_at = autotest.Autotest(self._client)
        client_at.run_test('network_ChromeCellularEndToEnd',
                           test=test)

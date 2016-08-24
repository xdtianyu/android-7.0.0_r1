# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class platform_ServoPowerStateController(test.test):
    """Test servo can power on and off DUT in recovery and non-recovery mode."""
    version = 1


    def initialize(self, host):
        """Initialize DUT for testing."""
        pass


    def cleanup(self):
        """Clean up DUT after servo actions."""
        if not self.host.ssh_ping():
            # Power off, then power on DUT from internal storage.
            self.controller.power_off()
            self.host.servo.switch_usbkey('off')
            self.controller.power_on(self.controller.REC_OFF)


    def assert_dut_on(self, rec_on=False):
        """Confirm DUT is powered on, claim test failure if DUT is off.

        @param rec_on: True if DUT should boot from external USB stick as in
                       recovery mode.

        @raise TestFail: If DUT is off or DUT boot from wrong source.
        """
        if not self.host.wait_up(timeout=300):
            raise error.TestFail('power_state:%s did not turn DUT on.' %
                                 ('rec' if rec_on else 'on'))

        # Check boot source. Raise TestFail if DUT boot from wrong source.
        boot_from_usb = self.host.is_boot_from_usb()
        if boot_from_usb != rec_on:
            boot_source = ('USB' if boot_from_usb else
                           'non-removable storage')
            raise error.TestFail('power_state:%s booted from %s.' %
                                 ('rec' if rec_on else 'on', boot_source))


    def assert_dut_off(self, error_message):
        """Confirm DUT is off and does not turn back on after 30 seconds.

        @param error_message: Error message to raise if DUT stays on.
        @raise TestFail: If DUT stays on.
        """
        if not self.host.ping_wait_down(timeout=10):
            raise error.TestFail(error_message)

        if self.host.ping_wait_up(timeout=30):
            raise error.TestFail('%s. %s' % (error_message, 'DUT turns back on'
                                             ' after it is turned off.'))


    def test_with_usb_plugged_in(self):
        """Run test when USB stick is plugged in to servo."""
        logging.info('Power off DUT')
        self.controller.power_off()
        self.assert_dut_off('power_state:off did not turn DUT off.')

        logging.info('Power DUT on in recovery mode, DUT shall boot from USB.')
        self.host.servo.switch_usbkey('off')
        self.controller.power_on(self.controller.REC_ON)
        self.assert_dut_off('power_state:rec didn\'t stay at recovery screen.')

        self.host.servo.switch_usbkey('dut')
        time.sleep(30)
        self.assert_dut_on(rec_on=True)

        logging.info('Power off DUT which is up in recovery mode.')
        self.controller.power_off()
        self.assert_dut_off('power_state:off failed after boot from external '
                            'USB stick.')

        logging.info('Power DUT off in recovery mode without booting.')
        self.host.servo.switch_usbkey('off')
        self.controller.power_on(self.controller.REC_ON)
        time.sleep(10)
        self.controller.power_off()
        self.assert_dut_off('power_state:off failed at recovery screen ')

        # Power DUT on in non-recovery mode with USB stick plugged in.
        # DUT shall boot from internal storage.
        logging.info('Power on DUT in non-recovery mode.')
        self.host.servo.switch_usbkey('dut')
        self.controller.power_on(self.controller.REC_OFF)
        self.assert_dut_on()
        self.host.servo.switch_usbkey('off')


    def test_with_usb_unplugged(self):
        """Run test when USB stick is not plugged in servo."""
        # Power off DUT regardless its current status.
        logging.info('Power off DUT.')
        self.controller.power_off()
        self.assert_dut_off('power_state:off did not turn DUT off.')

        # Try to power off the DUT again, make sure the DUT stays off.
        logging.info('Power off DUT which is already off.')
        self.controller.power_off()
        self.assert_dut_off('power_state:off turned DUT on.')

        # USB stick should be unplugged before the test.
        self.host.servo.switch_usbkey('off')

        logging.info('Power on in non-recovery mode.')
        self.controller.power_on(self.controller.REC_OFF)
        self.assert_dut_on(rec_on=False)

        logging.info('Power DUT off and on without delay. DUT should be '
                     'on after power_on is completed.')
        self.controller.power_off()
        self.controller.power_on(self.controller.REC_OFF)
        self.assert_dut_on(rec_on=False)

        logging.info('Power off DUT which is up in non-recovery mode.')
        self.controller.power_off()
        self.assert_dut_off('power_state:off failed after boot from '
                            'internal storage.')

        logging.info('Power DUT off and reset. DUT should be on after '
                     'reset is completed.')
        self.controller.reset()
        self.assert_dut_on(rec_on=False)

        logging.info('Reset DUT when it\'s on. DUT should be on after '
                     'reset is completed.')
        boot_id = self.host.get_boot_id()
        self.controller.reset()
        self.assert_dut_on(rec_on=False)
        new_boot_id = self.host.get_boot_id()
        if not new_boot_id or boot_id == new_boot_id:
            raise error.TestFail('power_state:reset failed to reboot DUT.')


    def run_once(self, host, usb_available=True):
        """Run the test.

        @param host: host object of tested DUT.
        @param usb_plugged_in: True if USB stick is plugged in servo.
        """
        self.host = host
        self.controller = host.servo.get_power_state_controller()

        self.test_with_usb_unplugged()
        if usb_available:
            self.test_with_usb_plugged_in()

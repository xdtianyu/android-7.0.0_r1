# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from threading import Timer
import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


def delayed(seconds):
    def decorator(f):
        def wrapper(*args, **kargs):
            t = Timer(seconds, f, args, kargs)
            t.start()
        return wrapper
    return decorator


class firmware_ECLidSwitch(FirmwareTest):
    """
    Servo based EC lid switch test.
    """
    version = 1

    # Delay between closing and opening the lid
    LID_DELAY = 1

    # Delay to allow FAFT client receive command
    RPC_DELAY = 2

    # Delay between shutdown and wake by lid switch including kernel
    # shutdown time
    LONG_WAKE_DELAY = 25
    SHORT_WAKE_DELAY = 15

    def initialize(self, host, cmdline_args):
        super(firmware_ECLidSwitch, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')

    def _open_lid(self):
        """Open lid by servo."""
        self.servo.set('lid_open', 'yes')

    def _close_lid(self):
        """Close lid by servo."""
        self.servo.set('lid_open', 'no')

    @delayed(RPC_DELAY)
    def delayed_open_lid(self):
        """Delay by RPC_DELAY and then open lid by servo."""
        self._open_lid()

    @delayed(RPC_DELAY)
    def delayed_close_lid(self):
        """Delay by RPC_DELAY and then close lid by servo."""
        self._close_lid()

    def _wake_by_lid_switch(self):
        """Wake DUT with lid switch."""
        self._close_lid()
        time.sleep(self.LID_DELAY)
        self._open_lid()

    @delayed(LONG_WAKE_DELAY)
    def long_delayed_wake(self):
        """Delay for LONG_WAKE_DELAY and then wake DUT with lid switch."""
        self._wake_by_lid_switch()

    @delayed(SHORT_WAKE_DELAY)
    def short_delayed_wake(self):
        """Delay for SHORT_WAKE_DELAY and then wake DUT with lid switch."""
        self._wake_by_lid_switch()

    def shutdown_and_wake(self, wake_func):
        """Software shutdown and delay. Then wake by lid switch.

        Args:
          wake_func: Delayed function to wake DUT.
        """
        self.faft_client.system.run_shell_command('shutdown -P now')
        wake_func()

    def _get_keyboard_backlight(self):
        """Get keyboard backlight brightness.

        Returns:
          Backlight brightness percentage 0~100. If it is disabled, 0 is
            returned.
        """
        cmd = 'ectool pwmgetkblight'
        pattern_percent = re.compile(
            'Current keyboard backlight percent: (\d*)')
        pattern_disable = re.compile('Keyboard backlight disabled.')
        lines = self.faft_client.system.run_shell_command_get_output(cmd)
        for line in lines:
            matched_percent = pattern_percent.match(line)
            if matched_percent is not None:
                return int(matched_percent.group(1))
            matched_disable = pattern_disable.match(line)
            if matched_disable is not None:
                return 0
        raise error.TestError('Cannot get keyboard backlight status.')

    def _set_keyboard_backlight(self, value):
        """Set keyboard backlight brightness.

        Args:
          value: Backlight brightness percentage 0~100.
        """
        cmd = 'ectool pwmsetkblight %d' % value
        self.faft_client.system.run_shell_command(cmd)

    def check_keycode(self):
        """Check that lid open/close do not send power button keycode.

        Returns:
          True if no power button keycode is captured. Otherwise, False.
        """
        self._open_lid()
        self.delayed_close_lid()
        if self.faft_client.system.check_keys([]) < 0:
            return False
        self.delayed_open_lid()
        if self.faft_client.system.check_keys([]) < 0:
            return False
        return True

    def check_backlight(self):
        """Check if lid open/close controls keyboard backlight as expected.

        Returns:
          True if keyboard backlight is turned off when lid close and on when
           lid open.
        """
        if not self.check_ec_capability(['kblight'], suppress_warning=True):
            return True
        ok = True
        original_value = self._get_keyboard_backlight()
        self._set_keyboard_backlight(100)

        self._close_lid()
        if self._get_keyboard_backlight() != 0:
            logging.error("Keyboard backlight still on when lid close.")
            ok = False
        self._open_lid()
        if self._get_keyboard_backlight() == 0:
            logging.error("Keyboard backlight still off when lid open.")
            ok = False

        self._set_keyboard_backlight(original_value)
        return ok

    def check_keycode_and_backlight(self):
        """
        Disable powerd to prevent DUT shutting down during test. Then check
        if lid switch event controls keycode and backlight as we expected.
        """
        ok = True
        logging.info("Stopping powerd")
        self.faft_client.system.run_shell_command('stop powerd')
        if not self.check_keycode():
            logging.error("check_keycode failed.")
            ok = False
        if not self.check_backlight():
            logging.error("check_backlight failed.")
            ok = False
        logging.info("Restarting powerd")
        self.faft_client.system.run_shell_command('start powerd')
        return ok

    def run_once(self):
        if not self.check_ec_capability(['lid']):
            raise error.TestNAError("Nothing needs to be tested on this device")

        logging.info("Shutdown and long delayed wake.")
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.shutdown_and_wake(self.long_delayed_wake))

        logging.info("Shutdown and short delayed wake.")
        self.switcher.mode_aware_reboot(
                'custom',
                lambda:self.shutdown_and_wake(self.short_delayed_wake))

        logging.info("Check keycode and backlight.")
        self.check_state(self.check_keycode_and_backlight)

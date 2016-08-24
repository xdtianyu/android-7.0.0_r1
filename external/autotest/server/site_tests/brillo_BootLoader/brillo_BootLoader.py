# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


def _assert_equal(expected, actual):
    """Compares objects.

    @param expected: the expected value.
    @param actual: the actual value.

    @raises error.TestFail
    """
    if expected != actual:
        raise error.TestFail('Expected: %r, actual: %r' % (expected, actual))


class brillo_BootLoader(test.test):
    """A/B tests for boot loader and boot_control HAL implementation."""
    version = 1


    def get_slots_and_suffix(self):
        """Gets number of slots supported and slot suffixes used.

        Prerequisite: The DUT is in ADB mode.
        """
        self.num_slots = int(self.dut.run_output('bootctl get-number-slots'))
        logging.info('Number of slots: %d', self.num_slots)
        self.suffix_a = self.dut.run_output('bootctl get-suffix 0')
        self.suffix_b = self.dut.run_output('bootctl get-suffix 1')
        logging.info('Slot 0 suffix: "%s"', self.suffix_a)
        logging.info('Slot 1 suffix: "%s"', self.suffix_b)
        _assert_equal(self.num_slots, 2)


    def get_current_slot(self):
        """Gets the current slot the DUT is running from.

        Prerequisite: The DUT is in ADB mode.
        """
        return int(self.dut.run_output('bootctl get-current-slot'))


    def assert_current_slot(self, slot_number):
        """Checks that DUT is running from the given slot.

        Prerequisite: The DUT is in ADB mode.

        @slot_number: Zero-based index of slot to be running from.
        """
        _assert_equal(self.get_current_slot(), slot_number)


    def set_active_slot(self, slot_number):
        """Instructs the DUT to attempt booting from given slot.

        Prerequisite: The DUT is in ADB mode.

        @slot_number: Zero-based index of slot to make active.
        """
        logging.info('Setting slot %d active.', slot_number)
        self.dut.run('bootctl set-active-boot-slot %d' % slot_number)


    def ensure_running_slot(self, slot_number):
        """Ensures that DUT is running from the given slot.

        Prerequisite: The DUT is in ADB mode.

        @slot_number: Zero-based index of slot to be running from.
        """
        logging.info('Ensuring device is running from slot %d.', slot_number)
        if self.get_current_slot() != slot_number:
            logging.info('Rebooting into slot %d', slot_number)
            self.set_active_slot(slot_number)
            self.dut.reboot()
            self.assert_current_slot(slot_number)


    def copy_a_to_b(self):
        """Copies contents of slot A to slot B.

        Prerequisite: The DUT is in ADB mode and booted from slot A.
        """
        self.assert_current_slot(0)
        for i in ['boot', 'system']:
            logging.info('Copying %s%s to %s%s.',
                         i, self.suffix_a, i, self.suffix_b)
            self.dut.run('dd if=/dev/block/by-name/%s%s '
                         'of=/dev/block/by-name/%s%s bs=4096' %
                         (i, self.suffix_a, i, self.suffix_b))


    def check_bootctl_set_active(self):
        """Checks that setActiveBootSlot in the boot_control HAL work.

        Prerequisite: The DUT is in ADB mode with populated A and B slots.
        """
        logging.info('Check setActiveBootSlot() in boot_control HAL.')
        self.set_active_slot(0)
        self.dut.reboot()
        self.assert_current_slot(0)
        self.set_active_slot(1)
        self.dut.reboot()
        self.assert_current_slot(1)


    def check_fastboot_set_active(self):
        """Checks that 'fastboot set_active <SUFFIX>' work.

        Prerequisite: The DUT is in ADB mode with populated A and B slots.
        """
        logging.info('Check set_active command in fastboot-compliant bootloader.')
        self.dut.ensure_bootloader_mode()
        # TODO(zeuthen): The "oem set_active <NUMBER>" is specific to
        # edison. We need to get set_active support in fastboot - see
        # b/25075082 - and then for vendors to implement "set_active
        # <SLOT_SUFFIX>" e.g. use suffix instead of number.
        self.dut.fastboot_run('oem set_active 0')
        self.dut.fastboot_run('continue')
        self.dut.adb_run('wait-for-device')
        self.assert_current_slot(0)
        self.dut.ensure_bootloader_mode()
        self.dut.fastboot_run('oem set_active 1')
        self.dut.fastboot_run('continue')
        self.dut.adb_run('wait-for-device')
        self.assert_current_slot(1)


    def check_bootloader_fallback_on_invalid(self):
        """Checks bootloader fallback if current slot is invalid.

        Prerequisite: The DUT is in ADB mode with populated A and B slots.
        """
        logging.info('Checking bootloader fallback if current slot '
                     'is invalid.')
        # Make sure we're in slot B, then zero out boot_b (so slot B
        # is invalid), reboot and check that the bootloader correctly
        # fell back to A.
        self.ensure_running_slot(1)
        self.dut.run('dd if=/dev/zero of=/dev/block/by-name/boot%s '
                     'count=8192 bs=4096' % self.suffix_b)
        self.dut.reboot()
        self.assert_current_slot(0)
        # Restore boot_b for use in future test cases.
        self.dut.run('dd if=/dev/block/by-name/boot%s '
                     'of=/dev/block/by-name/boot%s bs=4096' %
                     (self.suffix_a, self.suffix_b))


    def check_bootloader_fallback_on_retries(self):
        """Checks bootloader fallback if slot made active runs out of tries.

        Prerequisite: The DUT is in ADB mode with populated A and B slots.

        @raises error.TestFail
        """
        logging.info('Checking bootloader fallback if slot made active '
                     'runs out of tries.')
        self.ensure_running_slot(0)
        self.set_active_slot(1)
        self.dut.reboot()
        num_retries = 1
        while num_retries < 10 and self.get_current_slot() == 1:
            logging.info('Still with B after %d retries' % num_retries)
            num_retries += 1
            self.dut.reboot()
        if self.get_current_slot() != 0:
            raise error.TestFail('Bootloader did not fall back after '
                                 '%d retries without the slot being marked '
                                 'as GOOD' % num_retries)
        logging.info('Falled back to A after %d retries.', num_retries)


    def check_bootloader_mark_successful(self):
        """Checks bootloader stays with slot after it has been marked good.

        Prerequisite: The DUT is in ADB mode with populated A and B slots.
        """
        logging.info('Checking bootloader is staying with a slot after it has '
                     'been marked as GOOD for at least 10 reboots.')
        self.ensure_running_slot(0)
        self.dut.run('bootctl mark-boot-successful')
        num_reboots = 0
        while num_reboots < 10:
            self.dut.reboot()
            self.assert_current_slot(0)
            num_reboots += 1
            logging.info('Still with A after %d reboots' % num_reboots)


    def run_once(self, dut=None):
        """A/B tests for boot loader and boot_control HAL implementation.

        Verifies that boot loader and boot_control HAL implementation
        implements A/B correctly.

        Prerequisite: The DUT is in ADB mode.

        @param dut: host object representing the device under test.
        """
        self.dut = dut
        self.get_slots_and_suffix()
        self.ensure_running_slot(0)
        self.copy_a_to_b()
        self.check_bootctl_set_active()
        self.check_fastboot_set_active()
        self.check_bootloader_fallback_on_invalid()
        self.check_bootloader_fallback_on_retries()
        self.check_bootloader_mark_successful()

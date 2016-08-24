# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re, commands
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_status
from autotest_lib.client.cros import power_utils

class power_ARMSettings(test.test):
    version = 1


    def initialize(self):
        self._usbpower = power_utils.USBPower()


    def run_once(self):
        if not self._check_cpu_type():
            raise error.TestNAError('Unsupported CPU')

        status = power_status.get_status()
        if status.on_ac():
            logging.info('AC Power is online')
            self._on_ac = True
        else:
            logging.info('AC Power is offline')
            self._on_ac = False

        failures = ''

        fail_count = self._verify_wifi_power_settings()
        if fail_count:
            failures += 'wifi_failures(%d) ' % fail_count

        fail_count = self._verify_usb_power_settings()
        if fail_count:
            failures += 'usb_failures(%d) ' % fail_count

        fail_count = self._verify_filesystem_power_settings()
        if fail_count:
            failures += 'filesystem_failures(%d) ' % fail_count


        if failures:
            raise error.TestFail(failures)


    def _check_cpu_type(self):
        cpuinfo = utils.read_file('/proc/cpuinfo')

        # Look for ARM
        match = re.search(r'ARMv[4-7]', cpuinfo)
        if match:
            return True

        logging.info(cpuinfo)
        return False


    def _verify_wifi_power_settings(self):
        if self._on_ac:
            expected_state = 'off'
        else:
            expected_state = 'on'

        iwconfig_out = utils.system_output('iwconfig', retain_output=True)
        match = re.search(r'Power Management:(.*)', iwconfig_out)
        if match and match.group(1) == expected_state:
            return 0

        logging.info(iwconfig_out)
        return 1


    def _verify_usb_power_settings(self):
        errors = 0
        self._usbpower.query_devices()
        for dev in self._usbpower.devices:
            # whitelist MUST autosuspend
            autosuspend = dev.autosuspend()
            logging.debug("USB %s:%s whitelisted:%s autosuspend:%s",
                          dev.vid, dev.pid, dev.whitelisted, autosuspend)
            if dev.whitelisted and not autosuspend:
                logging.error("Whitelisted USB %s:%s "
                              "has autosuspend disabled", dev.vid, dev.pid)
                errors += 1
            elif not dev.whitelisted:
                # TODO(crbug.com/242228): Deprecate warnings once we can
                # definitively identify preferred USB autosuspend settings
                logging.warning("Non-Whitelisted USB %s:%s present.  "
                                "Should it be whitelisted?", dev.vid, dev.pid)

        return errors


    def _verify_filesystem_power_settings(self):
        mount_output = commands.getoutput('mount | fgrep commit=').split('\n')
        if len(mount_output) == 0:
            logging.debug('No file system entries with commit intervals found.')
            return 1

        errors = 0
        # Parse for 'commit' param
        for line in mount_output:
            try:
                commit = int(re.search(r'(commit=)([0-9]*)', line).group(2))
            except:
                logging.debug('Error reading commit value from \'%s\'', line)
                errors += 1
                continue


            # Check for the correct commit interval.
            if commit != 600:
                logging.debug('File System: Incorrect commit interval %d', \
                              commit)
                errors += 1

        return errors

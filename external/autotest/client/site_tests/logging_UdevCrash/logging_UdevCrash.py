# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gzip, logging, os, utils
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import crash_test


class logging_UdevCrash(crash_test.CrashTest):
    """Verify udev triggered crash works as expected."""
    version = 1


    def CheckAtmelCrashes(self):
        """Check proper Atmel trackpad crash reports are created."""
        if not os.path.exists(self._SYSTEM_CRASH_DIR):
              return False

        for filename in os.listdir(self._SYSTEM_CRASH_DIR):
            if not filename.startswith('change__i2c_atmel_mxt_ts'):
                raise error.TestFail('Crash report %s has wrong name' %
                                     filename)
            if filename.endswith('meta'):
                continue

            filepath = os.path.join(self._SYSTEM_CRASH_DIR, filename)
            if filename.endswith('.log.gz'):
                f = gzip.open(filepath, 'r')
            elif filename.endswith('.log'):
                f = open(filepath)
            else:
                raise error.TestFail('Crash report %s has wrong extension' %
                                     filename)

            data = f.read()
            # Check that we have seen the end of the file. Otherwise we could
            # end up racing bwtween writing to the log file and reading/checking
            # the log file.
            if 'END-OF-LOG' not in data:
                continue

            lines = data.splitlines()
            bad_lines = [x for x in lines if 'atmel_mxt_ts' not in x
                                             and 'END-OF-LOG' not in x]
            if bad_lines:
                raise error.TestFail('Crash report contains invalid '
                                     'content %s' % bad_lines)
            return True

        return False

    def _test_udev_report_atmel(self):
        """Test that atmel trackpad failure can trigger udev crash report."""
        DRIVER_DIR = '/sys/bus/i2c/drivers/atmel_mxt_ts'
        has_atmel_device = False
        if os.path.exists(DRIVER_DIR):
            for filename in os.listdir(DRIVER_DIR):
                if os.path.isdir(os.path.join(DRIVER_DIR, filename)):
                    has_atmel_device = True

        if not has_atmel_device:
            logging.info('No atmel device, skip the test')
            return None

        self._set_consent(True)

        # Use udevadm to trigger a fake udev event representing atmel driver
        # failure. The uevent match rule in 99-crash-reporter.rules is
        # ACTION=="change", SUBSYSTEM=="i2c", DRIVER=="atmel_mxt_ts",
        # ENV{ERROR}=="1" RUN+="/sbin/crash_reporter
        # --udev=SUBSYSTEM=i2c-atmel_mxt_ts:ACTION=change"
        utils.system('udevadm control --property=ERROR=1',
                     ignore_status=True)
        utils.system('udevadm trigger '
                     '--action=change '
                     '--subsystem-match=i2c '
                     '--attr-match=driver=atmel_mxt_ts',
                     ignore_status=True)
        utils.system('udevadm control --property=ERROR=0',
                     ignore_status=True)

        utils.poll_for_condition(
            self.CheckAtmelCrashes,
            timeout=60,
            exception=error.TestFail('No valid Atmel crash reports'))

    def run_once(self):
        self._automatic_consent_saving = True
        self.run_crash_tests(['udev_report_atmel'], True)

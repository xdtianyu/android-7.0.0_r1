# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_DiskFirmwareUpgrade(test.test):
    """
    Run the disk firmware upgrade script.
    """
    TEST_SCRIPT = '/usr/sbin/chromeos-disk-firmware-update.sh'
    UPGRADED_RE = r'^Upgraded.*'
    version = 1

    def run_once(self,
                 disk_firmware_package='/opt/google/disk/firmware',
                 expected_result=0,
                 upgrade_required=True):
        """
        Runs the shell script that upgrade disk firmware.

        @param disk_firmware_package: pre-installed firmware package location.
        @param expected_result:       expected results of the upgrade.
        @param upgrade_required:      if True, the firmware must change on the
                                      device.
        """
        status_file = os.path.join(self.resultsdir, 'status')
        cmd = [self.TEST_SCRIPT,
               '--status %s' % (status_file),
               '--fw_package_dir %s' % (disk_firmware_package)]
        fw_upgrade = utils.run(' '.join(cmd), ignore_status=True)

        # Check the result of the upgrade.
        upgrade_happened = False
        try:
            with open(status_file) as sf:
                for l in sf:
                    if re.match(self.UPGRADED_RE, l):
                        upgrade_happened = True
        except IOError:
            pass
        if fw_upgrade.exit_status != expected_result:
            raise error.TestError(
                'Expected %d Result is %d' % (
                    expected_result, fw_upgrade.exit_status))
        if (fw_upgrade.exit_status == 0 and
            upgrade_required and not upgrade_happened):
            raise error.TestError('Expected upgrade did not happened')

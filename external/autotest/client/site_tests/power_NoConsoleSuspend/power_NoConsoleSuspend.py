# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, errno, shutil, os
from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros import rtc, sys_power
from autotest_lib.client.common_lib import error

SYSFS_CONSOLE_SUSPEND = '/sys/module/printk/parameters/console_suspend'

class power_NoConsoleSuspend(test.test):
    """Test suspend/resume with no_console_suspend option set."""

    version = 1

    def initialize(self):
        # Save & disable console_suspend module param
        self.old_console_suspend = utils.read_file(SYSFS_CONSOLE_SUSPEND)
        utils.write_one_line(SYSFS_CONSOLE_SUSPEND, 'N')

    def run_once(self):
        sys_power.kernel_suspend(10)

    def cleanup(self):
        # Restore old console_suspend module param
        logging.info('restoring value for console_suspend: %s',
                     self.old_console_suspend)
        utils.open_write_close(SYSFS_CONSOLE_SUSPEND, self.old_console_suspend)

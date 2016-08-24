# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class firmware_RomSize(test.test):
    version = 3

    TARGET_BIOS = '-p host'
    TARGET_EC = '-p ec'

    def get_size(self, target):
        data = utils.system_output("flashrom --get-size %s" % target)
        return int(data.splitlines()[-1])

    def has_flash_chip(self, target):
        return utils.system("flashrom --flash-name %s" % target,
                            ignore_status=True) == 0

    def run_once(self):
        ec_size = 0
        if self.has_flash_chip(self.TARGET_EC):
            ec_size = self.get_size(self.TARGET_EC) / 1024
        bios_size = self.get_size(self.TARGET_BIOS) / 1024

        self.write_perf_keyval({"kb_system_rom_size": bios_size,
                                "kb_ec_rom_size": ec_size})

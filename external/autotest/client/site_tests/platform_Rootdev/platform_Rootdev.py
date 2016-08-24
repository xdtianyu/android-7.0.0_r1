# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kobic@codeaurora.org (Kobi Cohen-Arazi)'

import os
import datetime
import logging
import re
import utils
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_Rootdev(test.test):
    version = 1

    def test_no_partition(self, inputDev, cpuType):

        # arm should be /dev/mmcblk0,  /dev/mmcblk1 etc 
        # x86 should be /dev/sda, /dev/sdb

        if (cpuType == "arm"):
            m = re.match("/dev/mmcblk[0-9]$", inputDev)
            if not m:
                raise error.TestFail(
                    "Rootdev test_no_partition arm failed != /dev/mmcblk[0-9]")

        else:
            m = re.match("/dev/sd[a-z]$", inputDev)
            if not m:
                raise error.TestFail(
                    "Rootdev test_no_partition x86 failed != /dev/sd[a-z]")


    def run_once(self):

        cpuType = utils.get_cpu_arch()
        logging.debug("cpu type is %s" % cpuType)

        # test return values
        result = utils.system("rootdev -s")
        logging.debug("Rootdev test res: %d", result)
        if (result != 0):
            raise error.TestFail("Rootdev failed")
        result = utils.system("rootdev -s -d")
        logging.debug("Rootdev test -d switch res: %d", result)
        if (result != 0):
            raise error.TestFail("Rootdev failed -s -d")

        # test with -d Results should be without the partition device number
        text = utils.system_output("rootdev -s -d 2>&1")
        text = text.strip()
        logging.debug("Rootdev -s -d txt is *%s*", text)
        self.test_no_partition(text, cpuType)



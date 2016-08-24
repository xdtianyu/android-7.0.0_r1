# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_MultiReader(test.test):
    version = 1

    def run_once(self):
        blockdev_paths = glob.glob("/sys/block/*/removable")
        for path in blockdev_paths:
            removable = utils.read_one_line(path)
            if removable == "1":
                logging.info("Found removable block device %s",
                             os.path.dirname(path))
                return True

        raise error.TestFail("No removable block devices are seen.")

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob, logging, os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_RealtekCardReader(test.test):
    version = 1

    def run_once(self):
        # Look for the Realtek USB card reader.
        # This requires a plugged in SD card.
        lsusb_output = utils.system_output("lsusb -t")
        if not "Driver=ums-realtek" in lsusb_output:
            raise error.TestFail("The Realtek card reader USB device was not "
                                 "detected.  This test requires an SD card to "
                                 "be inserted to detect the USB device.")

        blockdevs = glob.glob("/sys/block/*")
        for dev in blockdevs:
            removable = utils.read_one_line(os.path.join(dev, "removable"))
            if removable == "1":
                logging.info("Found removable block device %s", dev)
                return True

        raise error.TestFail("The card reader USB device was detected, but "
                             "no removable block devices are seen.")

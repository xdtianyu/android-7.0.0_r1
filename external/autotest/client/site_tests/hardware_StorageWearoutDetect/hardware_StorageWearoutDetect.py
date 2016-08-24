# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class hardware_StorageWearoutDetect(test.test):
    """
    Check wear out status for storage device available in SMART for SSD and
    in ext_csd for eMMC version 5.0 or later. For previous version of eMMC,
    it will be treat as data not available.

    The test will be failed if:
    - At least one SMART variable has value under its threshold
      or
    - eMMC wear out status variable is in 90-100% band or higher.
    """

    version = 1
    STORAGE_INFO_PATH = '/var/log/storage_info.txt'
    STORAGE_INFO_UPDATE_PATH = '/usr/share/userfeedback/scripts/storage_info'

    # Example     "   Model Number:    LITEONIT LSS-32L6G-HP"
    SSD_DETECT = r"\s*Model Number:\s*(?P<model>.*)\s*$"

    # Example     "   Extended CSD rev 1.7 (MMC 5.0)"
    MMC_DETECT = r"\s*Extended CSD rev.*MMC (?P<version>\d+.\d+)"

    # Field meaning and example line that have failing attribute
    # ID# ATTRIBUTE_NAME          FLAGS    VALUE WORST THRESH FAIL RAW_VALUE
    # 184 End-to-End_Error        PO--CK   001   001   097    NOW  135
    SSD_FAIL = r"""\s*(?P<param>\S+\s\S+)      # ID and attribute name
                   \s+[P-][O-][S-][R-][C-][K-] # flags
                   (\s+\d{3}){3}               # three 3-digits numbers
                   \s+NOW                      # fail indicator"""

    # Ex "Device life time estimation type A [DEVICE_LIFE_TIME_EST_TYP_A: 0x01]"
    # 0x0a means 90-100% band, 0x0b means over 100% band -> find not digit
    MMC_FAIL = r".*(?P<param>DEVICE_LIFE_TIME_EST_TYP_.): 0x0\D"


    def run_once(self, use_cached_result=True):
        """
        Run the test

        @param use_cached_result: Use the result that generated when machine
                                  booted or generate new one.
        """

        if not use_cached_result:
            if not os.path.exists(self.STORAGE_INFO_UPDATE_PATH):
                msg = str('Test failed with error: %s not exist'
                          % self.STORAGE_INFO_UPDATE_PATH)
                raise error.TestFail(msg)
            utils.system(self.STORAGE_INFO_UPDATE_PATH)

        # Check that storage_info file exist.
        if not os.path.exists(self.STORAGE_INFO_PATH):
            msg = str('Test failed with error: %s not exist'
                      % self.STORAGE_INFO_PATH)
            raise error.TestFail(msg)

        mmc_detect = False
        ssd_detect = False
        legacy_mmc = False
        fail_msg = ''

        with open(self.STORAGE_INFO_PATH) as f:
            for line in f:
                m = re.match(self.SSD_DETECT, line)
                if m:
                    model = m.group('model')
                    ssd_detect = True
                    logging.info('Found SSD model %s', model)

                m = re.match(self.MMC_DETECT, line)
                if m:
                    version = m.group('version')
                    if float(version) < 5.0:
                        legacy_mmc = True
                    mmc_detect = True
                    logging.info('Found eMMC version %s', version)

                m = re.match(self.SSD_FAIL, line, re.X)
                if m:
                    param = m.group('param')
                    fail_msg += 'SSD failure ' + param

                m = re.match(self.MMC_FAIL, line)
                if m:
                    param = m.group('param')
                    fail_msg += 'MMC failure ' + param

        if not ssd_detect and not mmc_detect:
            raise error.TestFail('Can not detect storage device.')

        if fail_msg:
            msg = 'Detected wearout parameter:%s' % fail_msg
            raise error.TestFail(msg)

        if legacy_mmc:
            msg = 'eMMC version %s detected. ' % version
            msg += 'Wearout attributes are supported in eMMC 5.0 and later.'
            logging.info(msg)

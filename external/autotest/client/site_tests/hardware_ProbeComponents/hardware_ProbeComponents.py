# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
hardware_ProbeComponents runs "gooftool probe" command. The results will be
provided back to Google by the OEMs/ODMs who are qualifying new hardware
components and used to generate a new HWID.
"""

import logging
from autotest_lib.client.bin import test, utils

class hardware_ProbeComponents(test.test):
    """Logs "gooftool probe" command output"""
    version = 1


    def run_once(self):
        probe_results = utils.system_output('gooftool probe')
        logging.info(probe_results)
        return

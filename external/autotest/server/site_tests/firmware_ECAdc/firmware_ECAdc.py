# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECAdc(FirmwareTest):
    """
    Servo based EC ADC test.
    """
    version = 1

    # Repeat read count
    READ_COUNT = 200

    def _check_read(self):
        """Read EC internal temperature by EC ADC.

        Raises:
          error.TestFail: Raised when read fails.
        """
        t = int(self.ec.send_command_get_output("temps",
                ["ECInternal\s+: (\d+) K"])[0][1])
        if t < 273 or t > 373:
            raise error.TestFail("Abnormal EC temperature %d K" % t)

    def run_once(self):
        if not self.check_ec_capability(['adc_ectemp']):
            raise error.TestNAError("Nothing needs to be tested on this device")
        logging.info("Reading EC internal temperature for %d times.",
                     self.READ_COUNT)
        for _ in xrange(self.READ_COUNT):
            self._check_read()

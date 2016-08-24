# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECPeci(FirmwareTest):
    """
    Servo based EC PECI test.
    """
    version = 1

    # Repeat read count
    READ_COUNT = 200

    def initialize(self, host, cmdline_args):
        super(firmware_ECPeci, self).initialize(host, cmdline_args)
        self.ec.send_command("chan 0")

    def cleanup(self):
        self.ec.send_command("chan 0xffffffff")
        super(firmware_ECPeci, self).cleanup()

    def _check_read(self):
        """Read CPU temperature through PECI.

        Raises:
          error.TestFail: Raised when read fails.
        """
        t = int(self.ec.send_command_get_output("pecitemp",
                ["CPU temp = (\d+) K"])[0][1])
        if t < 273 or t > 400:
            raise error.TestFail("Abnormal CPU temperature %d K" % t)

    def run_once(self):
        if not self.check_ec_capability(['peci']):
            raise error.TestNAError("Nothing needs to be tested on this device")
        logging.info("Reading PECI CPU temperature for %d times.",
                     self.READ_COUNT)
        for _ in xrange(self.READ_COUNT):
            self._check_read()

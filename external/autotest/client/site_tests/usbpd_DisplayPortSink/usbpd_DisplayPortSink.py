# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import ec as cros_ec


class usbpd_DisplayPortSink(test.test):
    """Integration test for USB-PD DisplayPort sink."""

    version = 1
    DP_SVID = '0xff01'

    def _is_displayport(self, port):
        return port.is_amode_supported(self.DP_SVID)

    def _set_displayport(self, port, opos, enter):
        return port.set_amode(self.DP_SVID, opos, enter)

    def run_once(self, enter_reps=1):
        usbpd = cros_ec.EC_USBPD()
        logging.info("device has %d USB-PD ports", len(usbpd.ports))

        for i,port in enumerate(usbpd.ports):
            if not port.is_dfp():
                continue

            logging.info("Port %d is dfp", i)

            if not self._is_displayport(port):
                continue

            logging.info("Port %d supports dp", i)

            for _ in xrange(enter_reps):
                if not self._set_displayport(port, 1, False):
                    raise error.TestError("Failed to exit DP mode")

                if not self._set_displayport(port, 1, True):
                    raise error.TestError("Failed to enter DP mode")

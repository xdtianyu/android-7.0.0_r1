# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_device_context
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class MbimTestBase(object):
    """
    Base class for all MBIM Compliance Suite tests.
    This class contains boilerplate code and utility functions for MBIM
    Compliance Suite. A brief description of non-trivial facilities follows.
    Test initialization: populates the following members:
        - device_context: An MBIMTestContext. This class finds the relevant MBIM
                          device on the DUT and stashes that in this context.
    Utility functions: None yet.
    """

    def run_test(self, id_vendor=None, id_product=None, **kwargs):
        """
        Run the test.

        To test a specific device based on VID/PID, add id_vendor=0xHHHH,
        id_product=0xHHHH to the control file invocation of tests.

        @param id_vendor: Specific vendor ID for the modem to be tested.
        @param id_product: Specific product ID for the modem to be tested.
        @param kwargs: Optional parameters passed to tests.

        """
        self.device_context = mbim_device_context.MbimDeviceContext(
                id_vendor=id_vendor, id_product=id_product)
        logging.info('Running test on modem with VID: %04X, PID: %04X',
                     self.device_context.id_vendor,
                     self.device_context.id_product)
        self.run_internal(**kwargs)


    def run_internal(self):
        """
        This method actually implements the core test logic.

        Subclasses should override this method to run their own test.

        """
        mbim_errors.log_and_raise(NotImplementedError)

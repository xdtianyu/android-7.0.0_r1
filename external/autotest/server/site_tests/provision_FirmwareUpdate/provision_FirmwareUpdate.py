# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


""" The autotest performing FW update, both EC and AP."""


import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class provision_FirmwareUpdate(test.test):
    """A test that can provision a machine to the correct firmware version."""

    version = 1


    def run_once(self, host, value, rw_only=False):
        """The method called by the control file to start the test.

        @param host:  a CrosHost object of the machine to update.
        @param value: the provisioning value, which is the build version
                      to which we want to provision the machine,
                      e.g. 'link-firmware/R22-2695.1.144'.
        @param rw_only: True to only update the RW firmware.
        """
        try:
            host.confirm_servo()
            host.firmware_install(build=value, rw_only=rw_only)
        except Exception as e:
            logging.error(e)
            raise error.TestFail(str(e))

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
CM_06 Validation of |status_codes| in Modem's Response to MBIM_COMMAND_MSG

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 39
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""
import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_cid_device_caps_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class CM06Test(test.Test):
    """ Implement the CM_06 test. """

    def run_internal(self):
        """ Run CM_06 test. """
        # Precondition
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.test_context).run()

        # Step 1
        command_message, response_message = (
                mbim_cid_device_caps_sequence.MBIMCIDDeviceCapsSequence(
                        self.test_context).run())
        if response_message.status_codes != mbim_constants.MBIM_STATUS_SUCCESS:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.5#1')

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
CM_03 Validation of Function's Behavior for an Unsynchronized MBIM_OPEN_MSG.

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 38
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class CM03Test(test.Test):
    """ Implement the CM_03 test. """

    def run_internal(self):
        """ Run CM_03 test. """
        # Precondition
        _, _ = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.test_context).run()
        # Step 1
        open_message, response_message = (
                mbim_open_generic_sequence.MBIMOpenGenericSequence(
                        self.test_context).run())

        # Validate function's behaviour for an unsynchronized MBIM_OPEN_MSG.
        if response_message.message_type == mbim_constants.MBIM_CLOSE_DONE:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.3.1#1')

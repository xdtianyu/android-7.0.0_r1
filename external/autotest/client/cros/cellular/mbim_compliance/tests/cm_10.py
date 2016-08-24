# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
CM_10 Validation of Modem's Response to MBIM_CLOSE_MSG.

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 41
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""
import common
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_close_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class CM10Test(test.Test):
    """ Implement the test CM_10 test. """

    def run_internal(self):
        """ Run CM_10 test. """
        # Precondition
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.test_context).run()

        # Step 1
        close_message, response_message = mbim_close_sequence.MBIMCloseSequence(
                self.test_context).run()

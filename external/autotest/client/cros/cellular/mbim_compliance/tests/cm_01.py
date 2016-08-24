# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
CM_01 Validation of |transaction_id| and |status_codes| in Modem's Response to
MBIM_OPEN_MSG

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 38
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""

import common
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class CM01Test(test.Test):
    """ Implement the CM_01 test. """

    def run_internal(self):
        """ Run CM_01 test. """
        # Step 1
        open_message, response_message = (
                mbim_open_generic_sequence.MBIMOpenGenericSequence(
                        self.test_context).run())

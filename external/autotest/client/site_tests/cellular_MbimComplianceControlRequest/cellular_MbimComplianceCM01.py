# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM01(mbim_test_base.MbimTestBase):
    """
    CM_01 Validation of |transaction_id| and |status_codes| in modem's
    response to MBIM_OPEN_MSG.

    This test verifies that MBIM_OPEN_DONE message is issued by the function
    in response to MBIM_OPEN_MSG message and checks TransactionId and
    Status fields.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 38
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run the CM_01 test. """
        # Precondition.
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)

        # Step 1
        open_message, response_message = (
                mbim_open_generic_sequence.MBIMOpenGenericSequence(
                        self.device_context).run())

        # TODO(rpius): Complete the rest of the test

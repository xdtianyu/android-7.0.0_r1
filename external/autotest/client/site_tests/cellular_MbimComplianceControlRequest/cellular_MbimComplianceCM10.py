# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_close_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM10(mbim_test_base.MbimTestBase):
    """
    CM_10 Validation of Modem's Response to MBIM_CLOSE_MSG.

    This test verifies that an MBIM_CLOSE_DONE message is issued by the
    function in response to an MBIM_CLOSE_MSG message and checks TransactionId
    and Status fields.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 41
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run CM_10 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()

        # Step 1
        close_message, response_message = mbim_close_sequence.MBIMCloseSequence(
                self.device_context).run()

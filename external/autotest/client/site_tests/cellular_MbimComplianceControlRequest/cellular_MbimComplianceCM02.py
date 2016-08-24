# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM02(mbim_test_base.MbimTestBase):
    """
    CM_02 Validation of Message Length of the response to MBIM_OPEN_MSG.

    This test validates MessageLength field in MBIM_MESSAGE_HEADER.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 38
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run the CM_02 test. """
        # Precondition.
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)

        # Step 1
        _, response_message = (
                mbim_open_generic_sequence.MBIMOpenGenericSequence(
                        self.device_context).run())

        # Validate message length of response to MBIM_OPEN_MESSAGE.
        if response_message.message_length < 0x0C:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.1#2')

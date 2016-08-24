# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import connect_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_close_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM14(mbim_test_base.MbimTestBase):
    """
    CM_14 Validation of not sending data payload in error messages.

    This test verifies that an MBIM_FUNCTION_ERROR_MSG does contain a data
    payload.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 43
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run CM_14 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()
        mbim_close_sequence.MBIMCloseSequence(self.device_context).run()

        # Step 1
        _, response_message, _ = (
                connect_sequence.ConnectSequence(self.device_context).run(
                        raise_exception_on_failure=False))

        # Step 2
        if ((response_message.message_type !=
             mbim_constants.MBIM_FUNCTION_ERROR_MSG) or
            (response_message.message_length != 16)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.3.4#2')

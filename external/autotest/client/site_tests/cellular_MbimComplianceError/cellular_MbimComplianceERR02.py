# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import connect_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceERR02(mbim_test_base.MbimTestBase):
    """
    Validation of issuing the error message.

    This test verifies that an error message with status code
    MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE is issued when fragments received in a
    wrong order.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 45
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run ERR_02 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        open_sequence = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context)
        open_sequence.run(max_control_transfer_size=64)

        # Step 1
        request_message, response_message, _ = (
                connect_sequence.ConnectSequence(self.device_context).run(
                        introduce_error_in_packets_order=[1, 0, 2],
                        raise_exception_on_failure=False))

        # Step 2
        if ((response_message.message_type !=
             mbim_constants.MBIM_FUNCTION_ERROR_MSG) or
            (response_message.error_status_code !=
             mbim_constants.MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.3.4#3')

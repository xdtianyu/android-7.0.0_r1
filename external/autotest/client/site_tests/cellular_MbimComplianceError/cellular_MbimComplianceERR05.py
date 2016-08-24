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


class cellular_MbimComplianceERR05(mbim_test_base.MbimTestBase):
    """
    Validation of issuing a new error message.

    This test verifies that another error message with status code
    MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE is issued when another message with
    out-of-order fragmentation with the same TransactionId is received.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 46
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run ERR_05 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        open_sequence = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context)
        open_sequence.run(max_control_transfer_size=64)

        # Step 1
        request_message, first_response_message, notifications = (
                connect_sequence.ConnectSequence(self.device_context).run(
                        introduce_error_in_packets_order=[1, 1],
                        raise_exception_on_failure=False))

        if len(notifications) > 1:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'Not expecting more than 1 pending response.')
        second_response_message = notifications[0]

        # Step 2
        if (((first_response_message.transaction_id !=
              request_message.transaction_id) or
             (first_response_message.message_type !=
              mbim_constants.MBIM_FUNCTION_ERROR_MSG) or
             (first_response_message.error_status_code !=
              mbim_constants.MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE)) or
            ((second_response_message.transaction_id !=
              request_message.transaction_id) or
             (second_response_message.message_type !=
              mbim_constants.MBIM_FUNCTION_ERROR_MSG) or
             (second_response_message.error_status_code !=
              mbim_constants.MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE))):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.3.4.2#4')

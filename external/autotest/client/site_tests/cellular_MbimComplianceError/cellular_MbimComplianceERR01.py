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


class cellular_MbimComplianceERR01(mbim_test_base.MbimTestBase):
    """
    Validation of function's response to messages with variable-length encoding
    errors.

    This test verifies that incoming messages are rejected when variable-length
    encoding rules are not followed.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 45
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run ERR_01 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        open_sequence = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context)
        open_sequence.run()

        # Step 1
        request_message, response_message, _ = (
                connect_sequence.ConnectSequence(self.device_context).run(
                        introduce_error_in_access_offset=True,
                        raise_exception_on_failure=False))

        # Step 2
        if ((response_message.transaction_id !=
             request_message.transaction_id) or
            (response_message.device_service_id !=
             request_message.device_service_id) or
            (response_message.cid != request_message.cid)):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'Mismatch in request/response message params: '
                    '(transaction_id, service_id, cid). '
                    'Request Message: (%s, %s, %s), '
                    'Response Message: (%s, %s, %s)' % (
                        request_message.transaction_id,
                        request_message.device_service_id,
                        request_message.cid,
                        response_message.transaction_id,
                        response_message.device_service_id,
                        response_message.cid))

        # Step 3
        if ((response_message.message_type !=
             mbim_constants.MBIM_COMMAND_DONE) or
            (response_message.status_codes !=
             mbim_constants.MBIM_STATUS_INVALID_PARAMETERS)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:10.3#2')

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


class cellular_MbimComplianceERR04(mbim_test_base.MbimTestBase):
    """
    Validation of discarding packets in case of an error.

    This test verifies that in case of an error message with status code
    MBIM_ERROR_FRAGMENT_OUT_OF_SEQUENCE all packets of the message caused the
    error are discarded by the function.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 46
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run ERR_04 test. """
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
        if ((response_message.transaction_id !=
             request_message.transaction_id) or
            (response_message.message_type ==
             mbim_constants.MBIM_COMMAND_DONE)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.3.4.2#3')

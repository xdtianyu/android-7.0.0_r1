# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_channel
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_command_message
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_request
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_response
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


class cellular_MbimComplianceCM13(mbim_test_base.MbimTestBase):
    """
    CM_13 Validation of active context termination on function's closing.

    This test verifies that no any active context exists after closing of the
    function.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 42
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run CM_13 test. """
        # Precondition
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()
        self.device_context.update_descriptor_cache(descriptors)
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()
        connect_sequence.ConnectSequence(self.device_context).run()
        mbim_close_sequence.MBIMCloseSequence(self.device_context).run()
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context).run()

        # Step 1
        device_context = self.device_context
        descriptor_cache = device_context.descriptor_cache
        command_message = mbim_command_message.MBIMConnectQuery(session_id=0)
        packets = mbim_message_request.generate_request_packets(
                command_message,
                device_context.max_control_transfer_size)
        channel = mbim_channel.MBIMChannel(
                device_context._device,
                descriptor_cache.mbim_communication_interface.bInterfaceNumber,
                descriptor_cache.interrupt_endpoint.bEndpointAddress,
                device_context.max_control_transfer_size)
        response_packets = channel.bidirectional_transaction(*packets)
        channel.close()

        # Step 2
        response_message = mbim_message_response.parse_response_packets(
                response_packets)

        # Step 3
        if (response_message.status_codes !=
            mbim_constants.MBIM_STATUS_CONTEXT_NOT_ACTIVATED):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.3.2#3')

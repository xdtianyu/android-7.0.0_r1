# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
MBIM_CID_DEVICE_CAPS Sequence

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 22
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""
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
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import sequence


class MBIMCIDDeviceCapsSequence(sequence.Sequence):
    """
    Implement |MBIMCIDDeviceCapsSequence|.
    In this sequence, cid |MBIM_CID_DEVICE_CAPS| and uuid |UUID_BASIC_CONNECT|
    are used to retrieve a MBIM command done response with a
    |MBIM_DEVICE_CPAS_INFO| in its information buffer.
    """

    def run_internal(self):
        """ Run the MBIM_CID_DEVICE_CAPS Sequence. """
        # Step 1
        # Send MBIM_COMMAND_MSG.
        device_context = self.device_context
        descriptor_cache = device_context.descriptor_cache
        command_message = mbim_command_message.MBIMDeviceCapsQuery()
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
        is_message_valid = isinstance(
                response_message,
                mbim_command_message.MBIMDeviceCapsInfo)
        if ((not is_message_valid) or
            (response_message.message_type !=
             mbim_constants.MBIM_COMMAND_DONE) or
            (response_message.status_codes !=
             mbim_constants.MBIM_STATUS_SUCCESS)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.3')

        return command_message, response_message

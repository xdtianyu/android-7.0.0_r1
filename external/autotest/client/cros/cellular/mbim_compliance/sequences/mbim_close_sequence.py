# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
MBIM Close Sequence

Reference:
  [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 20
      http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

"""
import common

from autotest_lib.client.cros.cellular.mbim_compliance import mbim_channel
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_request
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_response
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import sequence


class MBIMCloseSequence(sequence.Sequence):
    """
    Implement the MBIM Close Sequence.
    In this sequence, a |MBIM_CLOSE_MSG| is sent to the modem in order to
    terminate the session between the host and the modem. The modem should send
    a |MBIM_CLOSE_DONE| as the response to |MBIM_CLOSE_MSG|.
    """

    def run_internal(self):
        """ Run the MBIM Close Sequence. """
        # Step 1
        # Send MBIM_CLOSE_MSG to the device.
        close_message = mbim_message_request.MBIMClose()
        device_context = self.device_context
        descriptor_cache = device_context.descriptor_cache
        packets = mbim_message_request.generate_request_packets(
                close_message,
                device_context.max_control_transfer_size)
        channel = mbim_channel.MBIMChannel(
                device_context._device,
                descriptor_cache.mbim_communication_interface.bInterfaceNumber,
                descriptor_cache.interrupt_endpoint.bEndpointAddress,
                device_context.max_control_transfer_size)

        # Step 2
        response_packets = channel.bidirectional_transaction(*packets)
        channel.close()

        response_message = mbim_message_response.parse_response_packets(
                response_packets)

        # Step 3
        if response_message.transaction_id != close_message.transaction_id:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                       'mbim1.0:9.4.2#1')

        if response_message.status_codes != mbim_constants.MBIM_STATUS_SUCCESS:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.2#2')

        return close_message, response_message

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
MBIM Open Generic Sequence

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 19
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_channel
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_device_context
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_request
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_response
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import open_sequence


class MBIMOpenGenericSequence(open_sequence.OpenSequence):
    """
    Implement the MBIM Open Generic Sequence.
    In this sequence, a |MBIM_OPEN_MSG| is sent from the host to the modem in
    order to start the interaction. The modem should send a |MBIM_OPEN_DONE| as
    the response to |MBIM_OPEN_MSG|.
    """

    def run_internal(self,
                     max_control_transfer_size=None,
                     ntb_format=mbim_constants.NTB_FORMAT_32):
        """
        Run the MBIM Open Generic Sequence.

        @param max_control_transfer_size: Sets the max_control_transfer
                parameter in the open message sent to the device and the size
                of control buffers sent to the device.
        @param ntb_format: Sets the NTB type to 16 bit vs 32 bit. This will only
                be set on devices which support both 32 bit NTB and 16 bit NTB.
        @returns tuple of (command_message, response_message):
                command_message: The command message sent to device.
                |command_message| is a MBIMCommandMessage object.
                response_message: The response to the |command_message|.
                |response_message| is a MBIMCommandDoneMessage object.
        """
        # Step 1 and 2
        device_context = self.device_context
        device_type = device_context.device_type
        mbim_communication_interface = (
                device_context.descriptor_cache.mbim_communication_interface)
        ncm_communication_interface = (
                device_context.descriptor_cache.ncm_communication_interface)
        no_data_data_interface = (
                device_context.descriptor_cache.no_data_data_interface)
        ncm_data_interface = (
                device_context.descriptor_cache.ncm_data_interface)
        mbim_data_interface = (
                device_context.descriptor_cache.mbim_data_interface)
        mbim_functional_descriptor = (
                device_context.descriptor_cache.mbim_functional)
        interrupt_endpoint = (
                device_context.descriptor_cache.interrupt_endpoint)
        descriptor_cache = device_context.descriptor_cache

        communication_interface_number = (
                mbim_communication_interface.bInterfaceNumber)
        data_interface_number = mbim_data_interface.bInterfaceNumber

        # Step 3
        # Set alternate setting to be 0 for MBIM only data interface and
        # NCM/MBIM data interface.
        self.detach_kernel_driver_if_active(data_interface_number)
        self.set_alternate_setting(data_interface_number, 0)

        # Step 4
        # Set alternate setting to be 1 for MBIM communication interface of
        # NCM/MBIM function.
        if device_type == mbim_device_context.DEVICE_TYPE_NCM_MBIM:
            self.set_alternate_setting(communication_interface_number, 1)

        # Step 5
        # Send a RESET_FUNCTION(0x05) request to reset communication interface.
        self.reset_function(communication_interface_number)

        # Step 6
        # Send GetNtbParameters() request to communication interface.
        ntb_parameters = self.get_ntb_parameters(
                mbim_communication_interface.bInterfaceNumber)

        # Step 7
        # Send SetNtbFormat() request to communication interface.
        # Bit 1 of |bmNtbForatsSupported| indicates whether the device
        # supports 32-bit NTBs.
        if (ntb_parameters.bmNtbFormatsSupported >> 1) & 1:
            self.set_ntb_format(communication_interface_number, ntb_format)

        # Step 8
        # Send SetNtbInputSize() request to communication interface.
        self.set_ntb_input_size(communication_interface_number,
                                ntb_parameters.dwNtbInMaxSize)

        # Step 9
        # Send SetMaxDatagramSize() request to communication interface.
        # Bit 3 determines whether the device can process SetMaxDatagramSize()
        # and GetMaxDatagramSize() requests.
        if (mbim_functional_descriptor.bmNetworkCapabilities>>3) & 1:
            self.set_max_datagram_size(communication_interface_number)

        # Step 10
        if device_type == mbim_device_context.DEVICE_TYPE_MBIM:
            alternate_setting = 1
        else:
            alternate_setting = 2
        self.set_alternate_setting(data_interface_number, alternate_setting)

        # Step 11 and 12
        # Send MBIM_OPEN_MSG request and receive the response.
        interrupt_endpoint_address = interrupt_endpoint.bEndpointAddress

        # If |max_control_transfer_size| is not explicitly set by the test,
        # we'll revert to using the |wMaxControlMessage| advertized by the
        # device in the MBIM functional descriptor.
        if not max_control_transfer_size:
            max_control_transfer_size = (
                    mbim_functional_descriptor.wMaxControlMessage)
        open_message = mbim_message_request.MBIMOpen(
                max_control_transfer=max_control_transfer_size)
        packets = mbim_message_request.generate_request_packets(
                open_message,
                max_control_transfer_size)
        channel = mbim_channel.MBIMChannel(
                device_context._device,
                communication_interface_number,
                interrupt_endpoint_address,
                max_control_transfer_size)
        response_packets = channel.bidirectional_transaction(*packets)
        channel.close()

        # Step 13
        # Verify if MBIM_OPEN_MSG request succeeds.
        response_message = mbim_message_response.parse_response_packets(
                response_packets)

        if response_message.transaction_id != open_message.transaction_id:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.4.1#1')

        if response_message.status_codes != mbim_constants.MBIM_STATUS_SUCCESS:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceSequenceError,
                                      'mbim1.0:9.4.1#2')

        # Store data/control transfer parameters in the device context so that
        # it can be used in any further control/data transfers.
        device_context.max_control_transfer_size = max_control_transfer_size
        device_context.current_ntb_format = self.get_ntb_format(
                communication_interface_number)
        device_context.max_in_data_transfer_size = (
                ntb_parameters.dwNtbInMaxSize)
        device_context.max_out_data_transfer_size = (
                ntb_parameters.dwNtbOutMaxSize)
        device_context.out_data_transfer_divisor = (
                ntb_parameters.wNdpOutDivisor)
        device_context.out_data_transfer_payload_remainder = (
                ntb_parameters.wNdpOutPayloadRemainder)
        device_context.out_data_transfer_ndp_alignment = (
                ntb_parameters.wNdpOutAlignment)

        return open_message, response_message

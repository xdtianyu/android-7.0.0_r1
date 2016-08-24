# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_channel
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_command_message
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_request
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_message_response
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence


class cellular_MbimComplianceCM16(mbim_test_base.MbimTestBase):
    """
    CM_16 Validation of fragmented message transmission in case of multiple
    fragmented messages.

    This test verifies that fragmented messages sent from the function are not
    intermixed. Note that this test is only applicable for devices that support
    multiple outstanding commands.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 44
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
    """
    version = 1

    def run_internal(self):
        """ Run CM_16 test. """
        # Precondition
        desc_sequence = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context)
        descriptors = desc_sequence.run()
        self.device_context.update_descriptor_cache(descriptors)
        open_sequence = mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.device_context)
        open_sequence.run(max_control_transfer_size=64)

        device_context = self.device_context
        descriptor_cache = device_context.descriptor_cache
        self.channel = mbim_channel.MBIMChannel(
                device_context.device,
                descriptor_cache.mbim_communication_interface.bInterfaceNumber,
                descriptor_cache.interrupt_endpoint.bEndpointAddress,
                device_context.max_control_transfer_size)

        # Step 1
        caps_command_message = mbim_command_message.MBIMDeviceCapsQuery()
        caps_packets = mbim_message_request.generate_request_packets(
                caps_command_message,
                device_context.max_control_transfer_size)
        self.caps_transaction_id = caps_command_message.transaction_id

        # Step 2
        services_command_message = (
                mbim_command_message.MBIMDeviceServicesQuery())
        services_packets = mbim_message_request.generate_request_packets(
                services_command_message,
                device_context.max_control_transfer_size)
        self.services_transaction_id = services_command_message.transaction_id

        # Transmit the messages now
        self.channel.unidirectional_transaction(*caps_packets)
        self.channel.unidirectional_transaction(*services_packets)

        # Step 3
        utils.poll_for_condition(
                self._get_response_packets,
                timeout=5,
                exception=mbim_errors.MBIMComplianceChannelError(
                        'Failed to retrieve the response packets to specific '
                        'control messages.'))
        self.channel.close()

        caps_response_message = self.caps_response
        services_response_message = self.services_response
        is_caps_message_valid = isinstance(
                caps_response_message,
                mbim_command_message.MBIMDeviceCapsInfo)
        is_services_message_valid = isinstance(
                services_response_message,
                mbim_command_message.MBIMDeviceServicesInfo)
        if not ((is_caps_message_valid and is_services_message_valid) and
                (caps_response_message.transaction_id ==
                 caps_command_message.transaction_id) and
                (caps_response_message.device_service_id ==
                 caps_command_message.device_service_id) and
                caps_response_message.cid == caps_command_message.cid and
                (services_command_message.transaction_id ==
                 services_response_message.transaction_id) and
                (services_command_message.device_service_id ==
                 services_response_message.device_service_id) and
                services_command_message.cid == services_response_message.cid):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.5#1')


    def _get_response_packets(self):
        """
        Condition method for |poll_for_condition| to check the retrieval of
        target packets.

        @returns True if both caps response packet and services response packet
                are received, False otherwise.

        """
        try:
            packets = self.channel.get_outstanding_packets()
        except mbim_errors.MBIMComplianceChannelError:
            logging.debug("Error in receiving response fragments from the device")
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:9.5#1')
        self.caps_response = None
        self.services_response = None
        for packet in packets:
            try:
                message_response = mbim_message_response.parse_response_packets(
                        packet)
            except mbim_errors.MBIMComplianceControlMessageError:
                logging.debug("Error in parsing response fragments from the device")
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:9.5#1')
            if message_response.transaction_id == self.caps_transaction_id:
                self.caps_response = message_response
            elif (message_response.transaction_id ==
                  self.services_transaction_id):
                self.services_response = message_response
            if self.caps_response and self.services_response:
                return True
        return False

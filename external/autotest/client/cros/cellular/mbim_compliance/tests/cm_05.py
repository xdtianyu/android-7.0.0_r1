# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
CM_05 Validatation for modem's responses to two consecutive MBIM command
messages are correct with regards to |transaction_id|, |service_id| and |cid|.

Reference:
    [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 39
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf
"""
import common
from autotest_lib.client.bin import utils
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_channel
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_control
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import mbim_open_generic_sequence
from autotest_lib.client.cros.cellular.mbim_compliance.tests import test


class CM05Test(test.Test):
    """ Implement the CM_05 test. """

    def run_internal(self):
        """ Run CM_05 test. """
        # Precondition
        mbim_open_generic_sequence.MBIMOpenGenericSequence(
                self.test_context).run()

        caps_command_message = mbim_control.MBIMCommandMessage(
                device_service_id=mbim_constants.UUID_BASIC_CONNECT.bytes,
                cid=mbim_constants.MBIM_CID_DEVICE_CAPS,
                command_type=mbim_constants.COMMAND_TYPE_QUERY,
                information_buffer_length=0)
        caps_packets = caps_command_message.generate_packets()
        services_command_message = mbim_control.MBIMCommandMessage(
                device_service_id=mbim_constants.UUID_BASIC_CONNECT.bytes,
                cid=mbim_constants.MBIM_CID_DEVICE_SERVICES,
                command_type=mbim_constants.COMMAND_TYPE_QUERY,
                information_buffer_length=0)
        services_packets = services_command_message.generate_packets()
        self.caps_transaction_id = caps_command_message.transaction_id
        self.services_transaction_id = services_command_message.transaction_id
        self.channel = mbim_channel.MBIMChannel(
                {'idVendor': self.test_context.id_vendor,
                 'idProduct': self.test_context.id_product},
                self.test_context.mbim_communication_interface.bInterfaceNumber,
                self.test_context.interrupt_endpoint.bEndpointAddress,
                self.test_context.mbim_functional.wMaxControlMessage)
        # Step 1
        self.channel.unidirectional_transaction(*caps_packets)
        # Step 2
        self.channel.unidirectional_transaction(*services_packets)

        utils.poll_for_condition(
                self._get_response_packets,
                timeout=5,
                exception=mbim_errors.MBIMComplianceChannelError(
                        'Failed to retrieve the response packets to specific '
                        'control messages.'))

        self.channel.close()
        caps_response_message = mbim_control.parse_response_packets(
                self.caps_response_packet)
        services_response_message = mbim_control.parse_response_packets(
                self.services_response_packet)

        # Step 3
        if not ((caps_response_message.transaction_id ==
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
                                      'mbim1.0:8.1.2#2')


    def _get_response_packets(self):
        """
        Condition method for |poll_for_condition| to check the retrieval of
        target packets.

        @returns True if both caps response packet and services response packet
                are received, False otherwise.

        """
        self.caps_response_packet, self.services_response_packet = None, None
        packets = self.channel.get_outstanding_packets()
        header = {}
        for packet in packets:
            for fragment in packet:
                try:
                    header = mbim_control.MBIMHeader.unpack(fragment)
                except mbim_errors.MBIMComplianceControlMessageError:
                    continue
                if header.get('transaction_id') == self.caps_transaction_id:
                    self.caps_response_packet = packet
                elif (header.get('transaction_id') ==
                      self.services_transaction_id):
                    self.services_response_packet = packet
                if self.caps_response_packet and self.services_response_packet:
                    return True
        return False

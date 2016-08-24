# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
All of the MBIM response message type definitions are in this file. These
definitions inherit from MBIMControlMessage.

Reference:
    [1] Universal Serial Bus Communications Class Subclass Specification for
        Mobile Broadband Interface Model
        http://www.usb.org/developers/docs/devclass_docs/
        MBIM10Errata1_073013.zip
"""
import logging

from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_message


class MBIMControlMessageResponse(mbim_message.MBIMControlMessage):
    """ MBIMMessage Response Message base class. """

    MESSAGE_TYPE = mbim_message.MESSAGE_TYPE_RESPONSE
    _FIELDS = (('I', 'message_type', mbim_message.FIELD_TYPE_PAYLOAD_ID),
               ('I', 'message_length', mbim_message.FIELD_TYPE_TOTAL_LEN),
               ('I', 'transaction_id', ''))


class MBIMOpenDone(MBIMControlMessageResponse):
    """ The class for MBIM_OPEN_DONE. """

    _FIELDS = (('I', 'status_codes', ''),)
    _IDENTIFIERS = {'message_type': mbim_constants.MBIM_OPEN_DONE}


class MBIMCloseDone(MBIMControlMessageResponse):
    """ The class for MBIM_CLOSE_DONE. """

    _FIELDS = (('I', 'status_codes', ''),)
    _IDENTIFIERS = {'message_type': mbim_constants.MBIM_CLOSE_DONE}


class MBIMCommandDoneSecondary(MBIMControlMessageResponse):
    """ The class for MBIM_COMMAND_DONE. """

    _FIELDS = (('I', 'total_fragments', mbim_message.FIELD_TYPE_NUM_FRAGMENTS),
               ('I', 'current_fragment', ''))


class MBIMCommandDone(MBIMControlMessageResponse):
    """ The class for MBIM_COMMAND_DONE. """

    _FIELDS = (('I', 'total_fragments', mbim_message.FIELD_TYPE_NUM_FRAGMENTS),
               ('I', 'current_fragment', ''),
               ('16s', 'device_service_id', mbim_message.FIELD_TYPE_PAYLOAD_ID),
               ('I', 'cid', mbim_message.FIELD_TYPE_PAYLOAD_ID),
               ('I', 'status_codes', ''),
               ('I', 'information_buffer_length',
                mbim_message.FIELD_TYPE_PAYLOAD_LEN))
    _IDENTIFIERS = {'message_type': mbim_constants.MBIM_COMMAND_DONE}
    _SECONDARY_FRAGMENT = MBIMCommandDoneSecondary


class MBIMIndicateStatusSecondary(MBIMControlMessageResponse):
    """ The class for MBIM_INDICATE_STATUS_MSG. """

    _FIELDS = (('I', 'total_fragments', mbim_message.FIELD_TYPE_NUM_FRAGMENTS),
               ('I', 'current_fragment', ''))


class MBIMIndicateStatus(MBIMControlMessageResponse):
    """ The class for MBIM_INDICATE_STATUS_MSG. """

    _FIELDS = (('I', 'total_fragments', mbim_message.FIELD_TYPE_NUM_FRAGMENTS),
               ('I', 'current_fragment', ''),
               ('16s', 'device_service_id', mbim_message.FIELD_TYPE_PAYLOAD_ID),
               ('I', 'cid', mbim_message.FIELD_TYPE_PAYLOAD_ID),
               ('I', 'information_buffer_length',
                mbim_message.FIELD_TYPE_PAYLOAD_LEN))
    _IDENTIFIERS = {'message_type': mbim_constants.MBIM_INDICATE_STATUS_MSG}
    _SECONDARY_FRAGMENT = MBIMIndicateStatusSecondary


class MBIMFunctionError(MBIMControlMessageResponse):
    """ The class for MBIM_FUNCTION_ERROR_MSG. """

    _FIELDS = (('I', 'error_status_code', ''),)
    _IDENTIFIERS = {'message_type': mbim_constants.MBIM_FUNCTION_ERROR_MSG}


def reassemble_response_packets(primary_fragment, secondary_packets):
    """
    Reassembles fragmented response messages into a single object.

    It parses all the secondary fragments as |secondary_frag_class| and
    merges all the payload_buffer fields into the primary fragment.

    @param primary_fragment: Primary fragment message object.
    @param secondary_packets: Array of the raw byte array response received
                               from device.
    @returns Reassembled Response Message object.

    """
    secondary_frag_class = primary_fragment.get_secondary_fragment()
    # Check if we can reassemble at this tree level or not. If there is
    # no associated _SECONDARY_FRAG_CLASS, we need to go down the tree further
    # to reassemble.
    if not secondary_frag_class:
        return None

    for packet in secondary_packets:
        secondary_fragment = secondary_frag_class(raw_data=packet)
        primary_fragment.payload_buffer.extend(
            secondary_fragment.payload_buffer)

    payload_len = primary_fragment.get_payload_len()
    num_fragments = primary_fragment.get_num_fragments()
    if ((num_fragments != len(secondary_packets) + 1) or
        (payload_len != len(primary_fragment.payload_buffer))):
        mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                  'mbim1.0:9.2')
    total_length = primary_fragment.calculate_total_len()
    primary_fragment =  primary_fragment.copy(message_length=total_length)
    logging.debug('Reassembled response-> Fragments: %d, Payload length: %d',
                  num_fragments, payload_len)
    return primary_fragment


def parse_response_packets(packets):
    """
    Parses the incoming raw data |packets| into corresponding message response
    object.

    The function starts the at the root of the message hierarchy tree
    and then goes down the root to find the exact leaf node message class. If
    there are multiple frgaments expected at any level, it will reassemble the
    secondary fragments before proceeding.

    @param packets: Array of the raw byte array response received from device.
    @returns Response Message object.

    """
    # Start with the root class for all responses and then go down the tree.
    message_class = MBIMControlMessageResponse
    parse_packets = packets

    while message_class is not None:
        first_packet = parse_packets[0]
        message = message_class(raw_data=first_packet)
        # If there are secondary fragments expected at this level,
        # let's reassemble the payload together before traversing down the
        # message heirarchy.
        if len(parse_packets) > 1:
            reassembled_message = reassemble_response_packets(message,
                                                              parse_packets[1:])
            if reassembled_message is not None:
                message = reassembled_message
                reassembled_packet = message.create_raw_data()
                parse_packets = [reassembled_packet]
        message_class = message.find_payload_class()
    logging.debug("Response Message parsed: %s", message)
    return message

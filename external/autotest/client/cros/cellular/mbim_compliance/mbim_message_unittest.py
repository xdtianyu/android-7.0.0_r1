# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import array
import logging
import unittest

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import \
        mbim_command_message
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_message
from autotest_lib.client.cros.cellular.mbim_compliance import \
        mbim_message_request
from autotest_lib.client.cros.cellular.mbim_compliance import \
        mbim_message_response


class TestMessage(mbim_message.MBIMControlMessage):
    """ MBIMMessage for unit testing. """
    _FIELDS = (('I', 'message_type', mbim_message.FIELD_TYPE_PAYLOAD_ID),
               ('I', 'message_length', ''),
               ('I', 'transaction_id', ''))
    _DEFAULTS = {'message_length': 0, 'transaction_id': 0}


class MBIMMessageTestCase(unittest.TestCase):
    """ Test cases for verifying MBIMMessage classes and MBIMMessageParser. """


    def test_fields_not_defined(self):
        """
        Verifies that an excepion is raised when constructing an MBIMMessage
        subclass that does not define a _FIELDS attribute.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceControlMessageError,
                'message must have some fields defined$'):
            class MBIMMessageFieldsNotDefined(mbim_message.MBIMControlMessage):
                """ MBIMMessage without _FIELDS attribute. """
                pass


    def test_message_missing_field_values(self):
        """
        Verifies that an exception is raised when constructing an MBIMMessage
        subclass object without providing values for all of the fields either
        in _DEFAULTS or in the constructor.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceControlMessageError,
                '^Missing field value'):
                message = TestMessage()


    def test_argument_mismatch(self):
        """
        Verifies that an exception is raised when there is any argument which is
        not defined in the control message class.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceControlMessageError,
                '^Unexpected fields'):
                message = TestMessage(message_type=4, fake=5)


    def test_message_default_value_set(self):
        """
        Verifies that the values for fields not provided in MBIMMessage
        constructor is taken from the _DEFAULTS attribute of the class.
        """
        message = TestMessage(message_type=3)
        self.assertEqual(message.message_length, 0)
        self.assertEqual(message.transaction_id, 0)
        self.assertEqual(message.message_type, 3)


    def test_message_default_value_override(self):
        """
        Verifies that the values for fields provided in MBIMMessage
        constructor overrides the values from the _DEFAULTS attribute of the
        class.
        """
        message = TestMessage(message_type=3, transaction_id=4)
        self.assertEqual(message.message_length, 0)
        self.assertEqual(message.transaction_id, 4)
        self.assertEqual(message.message_type, 3)


    def test_message_data_less_than_total_size_of_fields(self):
        """
        Verifies that an exception is raised when constructing a MBIMMessage
        subclass from raw message data of length less than the total size of
        fields specified by the _FIELDS attribute.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceControlMessageError,
                '^Length of Data'):
            message_data = array.array('B', [0x02, 0xAA])
            message = TestMessage(raw_data=message_data)


    def test_message_data_more_than_total_size_of_fields(self):
        """
        Verifies that it is OK to construct a MBIMMessage subclass from raw
        message data of length more than the total size of fields specified
        by the _FIELDS attribute. The additional data is put into
        |payload_buffer| field.
        """
        message_data = array.array('B', [0x02, 0xAA, 0xAA, 0XCC, 0xED, 0x98,
                                         0x80, 0x80, 0xAA, 0xED, 0x45, 0x45,
                                         0x50, 0x40])
        message = TestMessage(raw_data=message_data)
        self.assertEqual(message.payload_buffer, array.array('B', [0x50, 0x40]))


    def test_parse_mbim_open_done(self):
        """
        Verifies the packets of |MBIM_OPEN_DONE| type are parsed correctly.
        """
        packets = [array.array('B', [0x01, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00])]
        message = mbim_message_response.parse_response_packets(packets)
        self.assertEqual(True, isinstance(message,
                mbim_message_response.MBIMOpenDone))
        self.assertEqual(message.message_type, mbim_constants.MBIM_OPEN_DONE)
        self.assertEqual(message.message_length, 16)
        self.assertEqual(message.transaction_id, 1)
        self.assertEqual(message.status_codes,
                         mbim_constants.MBIM_STATUS_SUCCESS)


    def test_parse_mbim_close_done(self):
        """
        Verifies the packets of |MBIM_OPEN_DONE| type are parsed correctly.
        """
        packets = [array.array('B', [0x02, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00])]
        message = mbim_message_response.parse_response_packets(packets)
        self.assertEqual(True, isinstance(message,
                mbim_message_response.MBIMCloseDone))
        self.assertEqual(message.message_type, mbim_constants.MBIM_CLOSE_DONE)
        self.assertEqual(message.message_length, 16)
        self.assertEqual(message.transaction_id, 1)
        self.assertEqual(message.status_codes,
                         mbim_constants.MBIM_STATUS_SUCCESS)


    def test_parse_mbim_function_error_msg(self):
        """
        Verifies the |MBIM_FUNCTION_ERROR_MSG| packets are parsed correctly.
        """
        packets = [array.array('B', [0x04, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00,
                                     0x00, 0x00])]
        message = mbim_message_response.parse_response_packets(packets)
        self.assertEqual(True, isinstance(message,
                mbim_message_response.MBIMFunctionError))
        self.assertEqual(message.message_type,
                         mbim_constants.MBIM_FUNCTION_ERROR_MSG)
        self.assertEqual(message.message_length, 16)
        self.assertEqual(message.transaction_id, 1)
        self.assertEqual(message.error_status_code,
                         mbim_constants.MBIM_ERROR_UNKNOWN)


    def test_parse_mbim_command_done(self):
        """
        Verifies the packets of |MBIM_COMMAND_DONE| type are parsed correctly.
        This tests both the fragmentation reassembly and message parsing
        functionality.
        """
        packets = [array.array('B', [0x03, 0x00, 0x00, 0x80, 0x34, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                                     0x00, 0x06, 0xEE, 0x00, 0x00, 0x00, 0x00,
                                     0x80, 0x40, 0x20, 0x10, 0x00, 0xAA, 0xBB,
                                     0xCC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01,
                                     0x01, 0x01, 0x01]),
                   array.array('B', [0x03, 0x00, 0x00, 0x80, 0x18, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
                                     0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
                                     0x01, 0x01, 0x01])]
        message = mbim_message_response.parse_response_packets(packets)
        is_instance = isinstance(message,
                                 mbim_message_response.MBIMCommandDone)
        self.assertEqual(is_instance, True)
        self.assertEqual(message.message_type, mbim_constants.MBIM_COMMAND_DONE)
        self.assertEqual(message.message_length, 56)
        self.assertEqual(message.transaction_id, 1)
        self.assertEqual(message.total_fragments, 2)
        self.assertEqual(message.current_fragment, 0)
        self.assertEqual(message.device_service_id,
                         '\x02\x00\x06\xEE\x00\x00\x00\x00\x80\x40\x20\x10'
                         '\x00\xAA\xBB\xCC')
        self.assertEqual(message.cid, 1)
        self.assertEqual(message.status_codes,
                         mbim_constants.MBIM_STATUS_SUCCESS)
        self.assertEqual(message.information_buffer_length, 8)
        self.assertEqual(message.payload_buffer,
                         array.array('B', [0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                           0x01, 0x01]))


    def test_parse_mbim_get_device_caps(self):
        """
        Verifies the packets of |MBIM_COMMAND_DONE| type for a GetDeviceCaps
        CID query are parsed correctly.
        This tests both the fragmentation reassembly and message parsing
        functionality.
        """
        packets = [array.array('B', [0x03, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2,
                                     0x89, 0xCC, 0x33, 0xBC, 0xBB, 0x8B, 0x4F,
                                     0xB6, 0xB0, 0x13, 0x3E, 0xC2, 0xAA, 0xE6,
                                     0xDF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x01,
                                     0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                                     0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
                                     0x0]),
                   array.array('B', [0x03, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00,
                                     0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1F,
                                     0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00,
                                     0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
                                     0x00, 0x40, 0x00, 0x00, 0x00, 0x0A, 0x00,
                                     0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x1E,
                                     0x00, 0x00, 0x00, 0x6C, 0x00, 0x00, 0x00,
                                     0x1E, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00,
                                     0x00]),
                   array.array('B', [0x03, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00,
                                     0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12,
                                     0x00, 0x00, 0x00, 0x48, 0x00, 0x53, 0x00,
                                     0x50, 0x00, 0x41, 0x00, 0x2B, 0x00, 0x00,
                                     0x00, 0x33, 0x00, 0x35, 0x00, 0x31, 0x00,
                                     0x38, 0x00, 0x35, 0x00, 0x31, 0x00, 0x30,
                                     0x00, 0x36, 0x00, 0x30, 0x00, 0x30, 0x00,
                                     0x30, 0x00, 0x30, 0x00, 0x37, 0x00, 0x38,
                                     0x00]),
                   array.array('B', [0x03, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00,
                                     0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x34,
                                     0x00, 0x00, 0x00, 0x31, 0x00, 0x31, 0x00,
                                     0x2E, 0x00, 0x33, 0x00, 0x35, 0x00, 0x30,
                                     0x00, 0x2E, 0x00, 0x31, 0x00, 0x36, 0x00,
                                     0x2E, 0x00, 0x30, 0x00, 0x34, 0x00, 0x2E,
                                     0x00, 0x30, 0x00, 0x30, 0x00, 0x00, 0x00,
                                     0x4D, 0x00, 0x4C, 0x00, 0x31, 0x00, 0x4D,
                                     0x0]),
                   array.array('B', [0x03, 0x00, 0x00, 0x80, 0x20, 0x00, 0x00,
                                     0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00,
                                     0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x45,
                                     0x00, 0x39, 0x00, 0x33, 0x00, 0x36, 0x00,
                                     0x4D, 0x00, 0x00, 0x00])]
        message = mbim_message_response.parse_response_packets(packets)
        is_instance = isinstance(message,
                                 mbim_command_message.MBIMDeviceCapsInfo)
        self.assertEqual(is_instance, True)
        self.assertEqual(message.message_type, mbim_constants.MBIM_COMMAND_DONE)
        self.assertEqual(message.message_length, 208)
        self.assertEqual(message.transaction_id, 1)
        self.assertEqual(message.total_fragments, 5)
        self.assertEqual(message.current_fragment, 0)
        self.assertEqual(message.device_service_id,
                         '\xA2\x89\xCC3\xBC\xBB\x8BO\xB6\xB0\x13>\xC2\xAA\xE6'
                         '\xDF')
        self.assertEqual(message.cid, 1)
        self.assertEqual(message.status_codes,
                         mbim_constants.MBIM_STATUS_SUCCESS)
        self.assertEqual(message.information_buffer_length, 160)
        self.assertEqual(message.device_type, 1)
        self.assertEqual(message.cellular_class, 1)
        self.assertEqual(message.voice_class, 1)
        self.assertEqual(message.sim_class, 2)
        self.assertEqual(message.data_class, 2147483679)
        self.assertEqual(message.sms_caps, 3)
        self.assertEqual(message.control_caps, 3)
        self.assertEqual(message.max_sessions, 8)
        self.assertEqual(message.custom_data_class_offset, 64)
        self.assertEqual(message.custom_data_class_size, 10)
        self.assertEqual(message.device_id_offset, 76)
        self.assertEqual(message.device_id_size, 30)
        self.assertEqual(message.firmware_info_offset, 108)
        self.assertEqual(message.firmware_info_size, 30)
        self.assertEqual(message.hardware_info_offset, 140)
        self.assertEqual(message.hardware_info_size, 18)


    def test_generate_mbim_open(self):
        """
        Verifies the raw packet of |MBIM_OPEN| type is generated correctly.
        """
        message = mbim_message_request.MBIMOpen(max_control_transfer=40)
        packets = mbim_message_request.generate_request_packets(message, 64)
        self.assertEqual(packets, [array.array('B', [0x01, 0x00, 0x00, 0x00,
                                                     0x10, 0x00, 0x00, 0x00,
                                                     0x02, 0x00, 0x00, 0x00,
                                                     0x28, 0x00, 0x00, 0x00])])


    def test_generate_mbim_command_packets(self):
        """
        Verifies the raw packets of |MBIM_COMMAND| type are generated correctly.
        This verifies the fragmentation logic in the generate_request_packets.
        """
        payload_buffer=array.array('B', [0x01, 0x00, 0x00, 0x00, 0x10, 0x00,
                                         0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                                         0x28, 0x00, 0x00, 0x00, 0x04, 0x05,
                                         0x06, 0x10, 0x87, 0xDE, 0xED, 0xAC,
                                         0x45, 0x35, 0x50, 0x60, 0x90, 0xED,
                                         0xAB])
        message = mbim_message_request.MBIMCommand(
                device_service_id=mbim_constants.UUID_BASIC_CONNECT.bytes,
                cid=mbim_constants.MBIM_CID_DEVICE_CAPS,
                command_type=mbim_constants.COMMAND_TYPE_QUERY,
                information_buffer_length=len(payload_buffer),
                payload_buffer=payload_buffer)
        packets = mbim_message_request.generate_request_packets(message, 64)
        self.assertEqual(packets, [array.array('B', [0x03, 0x00, 0x00, 0x00,
                                                     0x40, 0x00, 0x00, 0x00,
                                                     0x01, 0x00, 0x00, 0x00,
                                                     0x02, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00,
                                                     0xA2, 0x89, 0xCC, 0x33,
                                                     0xBC, 0xBB, 0x8B, 0x4F,
                                                     0xB6, 0xB0, 0x13, 0x3E,
                                                     0xC2, 0xAA, 0xE6, 0xDF,
                                                     0x01, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00,
                                                     0x1F, 0x00, 0x00, 0x00,
                                                     0x01, 0x00, 0x00, 0x00,
                                                     0x10, 0x00, 0x00, 0x00,
                                                     0x01, 0x00, 0x00, 0x00,
                                                     0x28, 0x00, 0x00, 0x00]),
                                   array.array('B', [0x03, 0x00, 0x00, 0x00,
                                                     0x23, 0x00, 0x00, 0x00,
                                                     0x01, 0x00, 0x00, 0x00,
                                                     0x02, 0x00, 0x00, 0x00,
                                                     0x01, 0x00, 0x00, 0x00,
                                                     0x04, 0x05, 0x06, 0x10,
                                                     0x87, 0xDE, 0xED, 0xAC,
                                                     0x45, 0x35, 0x50, 0x60,
                                                     0x90, 0xED, 0xAB])])


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()

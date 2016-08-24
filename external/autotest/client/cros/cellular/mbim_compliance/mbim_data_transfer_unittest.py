# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import array
import logging
import unittest

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_data_transfer

class TestMbimDeviceContext(object):
    """ Dummy device context. """
    pass

class TestMbimDescriptorCache(object):
    """ Dummy MBIM descriptor cache. """
    pass

class TestMbimEndpointDescriptor(object):
    """ Dummy MBIM endpoint descriptor. """
    pass

class MBIMMessageTestCase(unittest.TestCase):
    """ Test cases for verifying MBIMDataTransfer class and MBIMNtb class. """

    def test_ntb_generation(self):
        """ Verifies the NTB frame generation from the given payload. """

        ntb = mbim_data_transfer.MBIMNtb(mbim_constants.NTB_FORMAT_32)
        payload = [array.array('B', [0x45, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x01, 0xBC, 0xB4, 0x7F, 0x00,
                                     0x00, 0x01, 0x7F, 0x00, 0x00, 0x02, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                                     0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                                     0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
                                     0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
                                     0x76, 0x77, 0x61, 0x62, 0x63, 0x64, 0x65,
                                     0x66, 0x67, 0x68, 0x69, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00])]
        ntb_frame = ntb.generate_ntb(payload, 1024, 32, 0, 1)
        verify_ntb_frame = array.array('B', [0x6E, 0x63, 0x6D, 0x68, 0x10,
                                             0x00, 0x00, 0x00, 0x90, 0x00,
                                             0x00, 0x00, 0x70, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x45, 0x00, 0x00,
                                             0x46, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x01, 0xBC, 0xB4, 0x7F,
                                             0x00, 0x00, 0x01, 0x7F, 0x00,
                                             0x00, 0x02, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x01,
                                             0x61, 0x62, 0x63, 0x64, 0x65,
                                             0x66, 0x67, 0x68, 0x69, 0x6A,
                                             0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
                                             0x70, 0x71, 0x72, 0x73, 0x74,
                                             0x75, 0x76, 0x77, 0x61, 0x62,
                                             0x63, 0x64, 0x65, 0x66, 0x67,
                                             0x68, 0x69, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x69, 0x70, 0x73,
                                             0x00, 0x20, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x20, 0x00,
                                             0x00, 0x00, 0x50, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00])
        self.assertEqual(ntb_frame, verify_ntb_frame)


    def test_ntb_parsing(self):
        """ Verifies the NTB frame parsing from the given NTB frame. """

        ntb = mbim_data_transfer.MBIMNtb(mbim_constants.NTB_FORMAT_32)
        ntb_frame = array.array('B', [0x6E, 0x63, 0x6D, 0x68, 0x10,
                                      0x00, 0x01, 0x00, 0x90, 0x00,
                                      0x00, 0x00, 0x70, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x45, 0x00, 0x00,
                                      0x46, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x01, 0xBC, 0xB4, 0x7F,
                                      0x00, 0x00, 0x01, 0x7F, 0x00,
                                      0x00, 0x02, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x01,
                                      0x61, 0x62, 0x63, 0x64, 0x65,
                                      0x66, 0x67, 0x68, 0x69, 0x6A,
                                      0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
                                      0x70, 0x71, 0x72, 0x73, 0x74,
                                      0x75, 0x76, 0x77, 0x61, 0x62,
                                      0x63, 0x64, 0x65, 0x66, 0x67,
                                      0x68, 0x69, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x69, 0x70, 0x73,
                                      0x00, 0x20, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x20, 0x00,
                                      0x00, 0x00, 0x50, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00])
        nth, ndp, ndp_entries, payload = ntb.parse_ntb(ntb_frame)
        verify_payload = [array.array('B', [0x45, 0x00, 0x00, 0x46, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x01, 0xBC, 0xB4,
                                            0x7F, 0x00, 0x00, 0x01, 0x7F, 0x00,
                                            0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x01, 0x61, 0x62,
                                            0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                                            0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
                                            0x6F, 0x70, 0x71, 0x72, 0x73, 0x74,
                                            0x75, 0x76, 0x77, 0x61, 0x62, 0x63,
                                            0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00])]
        # Verify the fields of the headers and payload
        self.assertEqual(nth.signature, mbim_data_transfer.NTH_SIGNATURE_32)
        self.assertEqual(nth.header_length, 16)
        self.assertEqual(nth.sequence_number, 1)
        self.assertEqual(nth.block_length, 144)
        self.assertEqual(nth.fp_index, 112)
        self.assertEqual(ndp.signature, mbim_data_transfer.NDP_SIGNATURE_IPS_32)
        self.assertEqual(ndp.length, 32)
        self.assertEqual(ndp.next_ndp_index, 0)
        self.assertEqual(ndp_entries[0].datagram_index, 32)
        self.assertEqual(ndp_entries[0].datagram_length, 80)
        self.assertEqual(ndp_entries[1].datagram_index, 0)
        self.assertEqual(ndp_entries[1].datagram_length, 0)
        self.assertEqual(payload, verify_payload)


    def test_data_transfer_object_creation(self):
        """ Verifies the Data transfer object creation. """
        device_context = TestMbimDeviceContext()
        device_context.device = 1
        device_context.max_out_data_transfer_size = 100
        device_context.max_in_data_transfer_size = 100
        device_context.out_data_transfer_divisor = 32
        device_context.out_data_transfer_payload_remainder = 0
        device_context.descriptor_cache = TestMbimDescriptorCache()
        device_context.descriptor_cache.mbim_data_interface = (
                TestMbimDescriptorCache())
        device_context.descriptor_cache.bulk_in_endpoint = (
                TestMbimDescriptorCache())
        device_context.descriptor_cache.bulk_out_endpoint = (
                TestMbimDescriptorCache())
        device_context.descriptor_cache.mbim_data_interface.bInterfaceNumber = 0
        device_context.descriptor_cache.bulk_in_endpoint.bEndpointAddress = 0
        device_context.descriptor_cache.bulk_out_endpoint.bEndpointAddress = 0
        data_transfer = mbim_data_transfer.MBIMDataTransfer(device_context)


    def test_data_transfer_send(self):
        """ Verifies the send_data_packets API in data transfer. """
        #TODO(rpius): Need to come up with a way to unittest the data transfer


    def test_data_transfer_received(self):
        """ Verifies the receive_data_packets API in data transfer. """
        #TODO(rpius): Need to come up with a way to unittest the data transfer


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()

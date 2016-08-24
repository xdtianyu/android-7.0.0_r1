# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import unittest
from array import array

import common
from autotest_lib.client.cros.cellular.mbim_compliance.usb_descriptors \
    import *


class TestDescriptor(Descriptor):
    """ Descriptor for unit testing. """
    DESCRIPTOR_TYPE = 0xAA
    DESCRIPTOR_SUBTYPE = 0xBB
    _FIELDS = (('B', 'bLength'),
               ('B', 'bDescriptorType'),
               ('B', 'bDescriptorSubtype'))


class DescriptorTestCase(unittest.TestCase):
    """ Test cases for verifying Descriptor classes and DescriptorParser. """


    def test_fields_not_defined(self):
        """
        Verifies that an excepion is raised when constructing a Descriptor
        subclass that does not define a _FIELDS attribute.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceFrameworkError,
                'DescriptorFieldsNotDefined must define a _FIELDS attribute$'):
            class DescriptorFieldsNotDefined(Descriptor):
                """ Descriptor without _FIELDS attribute. """
                pass


    def test_descriptor_type_not_defined(self):
        """
        Verifies that it is OK to construct a Descriptor subclass that does not
        define a DESCRIPTOR_TYPE attribute.
        """
        class DescriptorTypeNotDefined(Descriptor):
            """ Descriptor without DESCRIPTOR_TYPE attribute. """
            _FIELDS = (('B', 'bLength'), ('B', 'bDescriptorType'))

        descriptor_data = array('B', [0x02, 0xAA])
        descriptor = DescriptorTypeNotDefined(descriptor_data)
        self.assertEqual(2, descriptor.bLength)
        self.assertEqual(0xAA, descriptor.bDescriptorType)
        self.assertEqual(descriptor_data, descriptor.data)


    def test_descriptor_type_mismatch(self):
        """
        Verifies that an exception is raised when constructing a Descriptor
        subclass from raw descriptor data with a descriptor type that differs
        from the value specified by the DESCRIPTOR_TYPE attribute of the
        subclass.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceFrameworkError,
                '^Expected descriptor type 0xAA, got 0xBB$'):
            descriptor = TestDescriptor(array('B', [0x03, 0xBB, 0xBB]))


    def test_descriptor_subtype_mismatch(self):
        """
        Verifies that an exception is raised when constructing a Descriptor
        subclass from raw descriptor data with a descriptor subtype that differs
        from the value specified by the DESCRIPTOR_SUBTYPE attribute of the
        subclass.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceFrameworkError,
                '^Expected descriptor subtype 0xBB, got 0xCC$'):
            descriptor = TestDescriptor(array('B', [0x03, 0xAA, 0xCC]))


    def test_descriptor_length_mismatch(self):
        """
        Verifies that an exception is raised when constructing a Descriptor
        subclass from raw descriptor data with a descriptor length that differs
        from the length of the descriptor data.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceFrameworkError,
                '^Expected descriptor length 3, got 1$'):
            descriptor = TestDescriptor(array('B', [0x01, 0xAA, 0xBB]))

        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceFrameworkError,
                '^Expected descriptor length 3, got 4$'):
            descriptor = TestDescriptor(array('B', [0x04, 0xAA, 0xBB]))


    def test_descriptor_data_less_than_total_size_of_fields(self):
        """
        Verifies that an exception is raised when constructing a Descriptor
        subclass from raw descriptor data of length less than the total size of
        fields specified by the _FIELDS attribute.
        """
        with self.assertRaisesRegexp(
                mbim_errors.MBIMComplianceFrameworkError,
                '^Expected 3 or more bytes of descriptor data, got 1$'):
            descriptor = TestDescriptor(array('B', [0x03]))


    def test_descriptor_data_more_than_total_size_of_fields(self):
        """
        Verifies that it is OK to construct a Descriptor subclass from raw
        descriptor data of length more than the total size of fields specified
        by the _FIELDS attribute.
        """
        descriptor_data = array('B', [0x03, 0xAA, 0xBB])
        descriptor = TestDescriptor(descriptor_data)
        self.assertEqual(3, descriptor.bLength)
        self.assertEqual(0xAA, descriptor.bDescriptorType)
        self.assertEqual(descriptor_data, descriptor.data)


    def test_parsing_unknown_descriptor_type(self):
        """
        Verifies that DescriptorParser returns an instance of UnknownDescriptor
        when the descriptor type is not specified by any Descriptor subclass.
        """
        descriptor_data = array('B', [0x02, 0xFF])
        descriptors = list(DescriptorParser(descriptor_data))
        self.assertEqual(1, len(descriptors))
        descriptor = descriptors[0]
        self.assertIsInstance(descriptor, UnknownDescriptor)
        self.assertEqual(2, descriptor.bLength)
        self.assertEqual(0xFF, descriptor.bDescriptorType)
        self.assertEqual(descriptor_data, descriptor.data)


    def test_parsing_unsupported_descriptor_subtype(self):
        """
        Verifies that DescriptorParser returns an instance of
        FunctionalDescriptor when the descriptor type is 0x24 but the descriptor
        subtype is not supported.
        """
        descriptor_data = array('B', [0x03, 0x24, 0xFF])
        descriptors = list(DescriptorParser(descriptor_data))
        self.assertEqual(1, len(descriptors))
        descriptor = descriptors[0]
        self.assertIsInstance(descriptor, FunctionalDescriptor)
        self.assertEqual(3, descriptor.bLength)
        self.assertEqual(0x24, descriptor.bDescriptorType)
        self.assertEqual(0xFF, descriptor.bDescriptorSubtype)
        self.assertEqual(descriptor_data, descriptor.data)


    def test_parsing_descriptors(self):
        """
        Verifies that DescriptorParser returns an instance of an appropriate
        Descriptor subclass for each descriptor found in the given raw
        descriptor data.
        """
        descriptor_data = array('B', [0x09, 0x02, 0x5f, 0x00, 0x02, 0x01, 0x04,
                                      0xa0, 0xfa, 0x08, 0x0b, 0x00, 0x02, 0x02,
                                      0x0e, 0x00, 0x00, 0x09, 0x04, 0x00, 0x00,
                                      0x01, 0x02, 0x0e, 0x00, 0x05, 0x05, 0x24,
                                      0x00, 0x20, 0x01, 0x0c, 0x24, 0x1b, 0x00,
                                      0x01, 0x00, 0x06, 0x20, 0x80, 0x96, 0x05,
                                      0x00, 0x08, 0x24, 0x1c, 0x00, 0x01, 0x0f,
                                      0x96, 0x05, 0x05, 0x24, 0x06, 0x00, 0x01,
                                      0x07, 0x05, 0x81, 0x03, 0x40, 0x00, 0x05,
                                      0x09, 0x04, 0x01, 0x00, 0x00, 0x0a, 0x00,
                                      0x02, 0x06, 0x09, 0x04, 0x01, 0x01, 0x02,
                                      0x0a, 0x00, 0x02, 0x07, 0x07, 0x05, 0x82,
                                      0x02, 0x00, 0x02, 0x00, 0x07, 0x05, 0x01,
                                      0x02, 0x00, 0x02, 0x00])
        parser = DescriptorParser(descriptor_data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, ConfigurationDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(9, descriptor.bLength)
        self.assertEquals(0x02, descriptor.bDescriptorType)
        self.assertEquals(95, descriptor.wTotalLength)
        self.assertEquals(2, descriptor.bNumInterfaces)
        self.assertEquals(1, descriptor.bConfigurationValue)
        self.assertEquals(4, descriptor.iConfiguration)
        self.assertEquals(0xA0, descriptor.bmAttributes)
        self.assertEquals(250, descriptor.bMaxPower)
        self.assertEqual(array('B', [0x09, 0x02, 0x5f, 0x00, 0x02, 0x01, 0x04,
                                     0xa0, 0xfa]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, InterfaceAssociationDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(8, descriptor.bLength)
        self.assertEquals(0x0B, descriptor.bDescriptorType)
        self.assertEquals(0, descriptor.bFirstInterface)
        self.assertEquals(2, descriptor.bInterfaceCount)
        self.assertEquals(0x02, descriptor.bFunctionClass)
        self.assertEquals(0x0E, descriptor.bFunctionSubClass)
        self.assertEquals(0x00, descriptor.bFunctionProtocol)
        self.assertEquals(0, descriptor.iFunction)
        self.assertEqual(array('B', [0x08, 0x0b, 0x00, 0x02, 0x02, 0x0e, 0x00,
                                     0x00]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, InterfaceDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(9, descriptor.bLength)
        self.assertEquals(0x04, descriptor.bDescriptorType)
        self.assertEquals(0, descriptor.bInterfaceNumber)
        self.assertEquals(0, descriptor.bAlternateSetting)
        self.assertEquals(1, descriptor.bNumEndpoints)
        self.assertEquals(0x02, descriptor.bInterfaceClass)
        self.assertEquals(0x0E, descriptor.bInterfaceSubClass)
        self.assertEquals(0x00, descriptor.bInterfaceProtocol)
        self.assertEquals(5, descriptor.iInterface)
        self.assertEqual(array('B', [0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x0e,
                                     0x00, 0x05]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, HeaderFunctionalDescriptor)
        self.assertIsInstance(descriptor, FunctionalDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(5, descriptor.bLength)
        self.assertEquals(0x24, descriptor.bDescriptorType)
        self.assertEquals(0x00, descriptor.bDescriptorSubtype)
        self.assertEquals(0x120, descriptor.bcdCDC)
        self.assertEqual(array('B', [0x05, 0x24, 0x00, 0x20, 0x01]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, MBIMFunctionalDescriptor)
        self.assertIsInstance(descriptor, FunctionalDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(12, descriptor.bLength)
        self.assertEquals(0x24, descriptor.bDescriptorType)
        self.assertEquals(0x1B, descriptor.bDescriptorSubtype)
        self.assertEquals(0x100, descriptor.bcdMBIMVersion)
        self.assertEquals(1536, descriptor.wMaxControlMessage)
        self.assertEquals(32, descriptor.bNumberFilters)
        self.assertEquals(128, descriptor.bMaxFilterSize)
        self.assertEquals(1430, descriptor.wMaxSegmentSize)
        self.assertEquals(0x00, descriptor.bmNetworkCapabilities)
        self.assertEqual(array('B', [0x0c, 0x24, 0x1b, 0x00, 0x01, 0x00, 0x06,
                                     0x20, 0x80, 0x96, 0x05, 0x00]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, MBIMExtendedFunctionalDescriptor)
        self.assertIsInstance(descriptor, FunctionalDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(8, descriptor.bLength)
        self.assertEquals(0x24, descriptor.bDescriptorType)
        self.assertEquals(0x1C, descriptor.bDescriptorSubtype)
        self.assertEquals(0x100, descriptor.bcdMBIMExtendedVersion)
        self.assertEquals(15, descriptor.bMaxOutstandingCommandMessages)
        self.assertEquals(1430, descriptor.wMTU)
        self.assertEqual(array('B', [0x08, 0x24, 0x1c, 0x00, 0x01, 0x0f, 0x96,
                                     0x05]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, UnionFunctionalDescriptor)
        self.assertIsInstance(descriptor, FunctionalDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(5, descriptor.bLength)
        self.assertEquals(0x24, descriptor.bDescriptorType)
        self.assertEquals(0x06, descriptor.bDescriptorSubtype)
        self.assertEquals(0, descriptor.bControlInterface)
        self.assertEquals(1, descriptor.bSubordinateInterface0)
        self.assertEqual(array('B', [0x05, 0x24, 0x06, 0x00, 0x01]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, EndpointDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(7, descriptor.bLength)
        self.assertEquals(0x05, descriptor.bDescriptorType)
        self.assertEquals(0x81, descriptor.bEndpointAddress)
        self.assertEquals(0x03, descriptor.bmAttributes)
        self.assertEquals(64, descriptor.wMaxPacketSize)
        self.assertEquals(5, descriptor.bInterval)
        self.assertEqual(array('B', [0x07, 0x05, 0x81, 0x03, 0x40, 0x00, 0x05]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, InterfaceDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(9, descriptor.bLength)
        self.assertEquals(0x04, descriptor.bDescriptorType)
        self.assertEquals(1, descriptor.bInterfaceNumber)
        self.assertEquals(0, descriptor.bAlternateSetting)
        self.assertEquals(0, descriptor.bNumEndpoints)
        self.assertEquals(0x0A, descriptor.bInterfaceClass)
        self.assertEquals(0x00, descriptor.bInterfaceSubClass)
        self.assertEquals(0x02, descriptor.bInterfaceProtocol)
        self.assertEquals(6, descriptor.iInterface)
        self.assertEqual(array('B', [0x09, 0x04, 0x01, 0x00, 0x00, 0x0a, 0x00,
                                     0x02, 0x06]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, InterfaceDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(9, descriptor.bLength)
        self.assertEquals(0x04, descriptor.bDescriptorType)
        self.assertEquals(1, descriptor.bInterfaceNumber)
        self.assertEquals(1, descriptor.bAlternateSetting)
        self.assertEquals(2, descriptor.bNumEndpoints)
        self.assertEquals(0x0A, descriptor.bInterfaceClass)
        self.assertEquals(0x00, descriptor.bInterfaceSubClass)
        self.assertEquals(0x02, descriptor.bInterfaceProtocol)
        self.assertEquals(7, descriptor.iInterface)
        self.assertEqual(array('B', [0x09, 0x04, 0x01, 0x01, 0x02, 0x0a, 0x00,
                                     0x02, 0x07]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, EndpointDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(7, descriptor.bLength)
        self.assertEquals(0x05, descriptor.bDescriptorType)
        self.assertEquals(0x82, descriptor.bEndpointAddress)
        self.assertEquals(0x02, descriptor.bmAttributes)
        self.assertEquals(512, descriptor.wMaxPacketSize)
        self.assertEquals(0, descriptor.bInterval)
        self.assertEqual(array('B', [0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00]),
                         descriptor.data)

        descriptor = parser.next()
        self.assertIsInstance(descriptor, EndpointDescriptor)
        self.assertIsInstance(descriptor, Descriptor)
        self.assertEquals(7, descriptor.bLength)
        self.assertEquals(0x05, descriptor.bDescriptorType)
        self.assertEquals(0x01, descriptor.bEndpointAddress)
        self.assertEquals(0x02, descriptor.bmAttributes)
        self.assertEquals(512, descriptor.wMaxPacketSize)
        self.assertEquals(0, descriptor.bInterval)
        self.assertEqual(array('B', [0x07, 0x05, 0x01, 0x02, 0x00, 0x02, 0x00]),
                         descriptor.data)

        with self.assertRaises(StopIteration):
            descriptor = parser.next()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()

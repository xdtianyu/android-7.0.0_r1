# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.cros.cellular.mbim_compliance.sequences \
        import get_descriptors_sequence
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_test_base
from autotest_lib.client.cros.cellular.mbim_compliance import usb_descriptors


class cellular_MbimComplianceDES02(mbim_test_base.MbimTestBase):
    """
    DES_02 Descriptors Validation for MBIM Only Functions

    This test validates descriptors for MBIM only functions.

    Reference:
        [1] Universal Serial Bus Communication Class MBIM Compliance Testing: 26
        http://www.usb.org/developers/docs/devclass_docs/MBIM-Compliance-1.0.pdf

    """
    version = 1

    def run_internal(self):
        """ Run the DES_02 test. """
        # Precondition.
        descriptors = get_descriptors_sequence.GetDescriptorsSequence(
                self.device_context).run()

        # Test step 1
        # Get MBIM communication interface.
        interfaces = usb_descriptors.filter_descriptors(
                usb_descriptors.InterfaceDescriptor, descriptors)

        mbim_communication_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.MBIM_ONLY_COMMUNICATION_INTERFACE))

        if not mbim_communication_interfaces:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.3#1')

        if len(mbim_communication_interfaces) > 1:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Expected 1 mbim communication interface, got %d.' % (
                            len(mbim_communication_interfaces)))
        mbim_communication_interface = mbim_communication_interfaces[0]

        # Test step 2
        # Get header functional descriptor, union functional descriptor,
        # MBIM functional descriptor and MBIM extended functional
        # descriptor.
        mbim_communication_interface_bundle = (
                usb_descriptors.get_descriptor_bundle(
                        descriptors, mbim_communication_interface))

        header_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.HeaderFunctionalDescriptor,
                mbim_communication_interface_bundle)
        union_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.UnionFunctionalDescriptor,
                mbim_communication_interface_bundle)
        mbim_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.MBIMFunctionalDescriptor,
                mbim_communication_interface_bundle)
        mbim_extended_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.MBIMExtendedFunctionalDescriptor,
                mbim_communication_interface_bundle)
        if not(header_descriptors and union_descriptors and mbim_descriptors):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.3#2')

        # Test step 3
        # Check header functional descriptor.
        if usb_descriptors.has_distinct_descriptors(header_descriptors):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Expected 1 unique header functional descriptor.')
        header_descriptor = header_descriptors[0]
        if not(header_descriptor.bDescriptorType == 0x24 and
               header_descriptor.bDescriptorSubtype == 0x00 and
               header_descriptor.bLength == 5 and
               header_descriptor.bcdCDC >= 0x0120):
            mbim_errors.log_and_raise(
                mbim_errors.MBIMComplianceGenericAssertionError,
                'Header functional descriptor: wrong value(s)')

        # Test step 4
        # Check union functional descriptor.
        if usb_descriptors.has_distinct_descriptors(union_descriptors):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenerisAssertionError,
                    'Expected 1 unique union functional descriptor.')
        union_descriptor = union_descriptors[0]
        if union_descriptor.index < header_descriptor.index:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.3#3')

        # Get CDC no data data interface.
        no_data_data_interfaces = usb_descriptors.filter_interface_descriptors(
                interfaces, usb_descriptors.MBIM_ONLY_DATA_INTERFACE_NO_DATA)
        if not no_data_data_interfaces:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.6#4')
        if len(no_data_data_interfaces) > 1:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Exactly 1 CDC data interface, got %d.' % (
                            len(no_data_data_interfaces)))
        no_data_data_interface = no_data_data_interfaces[0]
        no_data_data_interface_bundle = usb_descriptors.get_descriptor_bundle(
                descriptors, no_data_data_interface)
        data_endpoint_descriptors = (
                usb_descriptors.filter_descriptors(
                        usb_descriptors.EndpointDescriptor,
                        no_data_data_interface_bundle))
        if data_endpoint_descriptors:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.6#2')

        # Get MBIM data interface.
        mbim_data_interfaces = usb_descriptors.filter_interface_descriptors(
                interfaces, usb_descriptors.MBIM_ONLY_DATA_INTERFACE_MBIM)
        if not mbim_data_interfaces:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.6#4')
        if len(mbim_data_interfaces) > 1:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Expected 1 MBIM data interface, got %d.' % (
                            len(mbim_data_interfaces)))
        mbim_data_interface = mbim_data_interfaces[0]

        # Check if there are two endpoint descriptors.
        if mbim_data_interface.bNumEndpoints != 2:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.6#3.')

        mbim_data_interface_bundle = usb_descriptors.get_descriptor_bundle(
                descriptors, mbim_data_interface)
        data_endpoint_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.EndpointDescriptor,
                mbim_data_interface_bundle)

        # Check the values of fields in endpoint descriptors.
        # There should be one bulk OUT and one bulk IN.
        if not usb_descriptors.has_bulk_in_and_bulk_out(
                data_endpoint_descriptors):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.6#3')

        # MBIM cdc data interface should have both no data data interface and
        # MBIM data interface. Therefore two interface numbers should be
        # the same.
        if (no_data_data_interface.bInterfaceNumber !=
            mbim_data_interface.bInterfaceNumber):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.6#1')

        # Check the fields of union functional descriptor
        if not(union_descriptor.bLength == 5 and
               (union_descriptor.bControlInterface ==
                mbim_communication_interface.bInterfaceNumber) and
               (union_descriptor.bSubordinateInterface0 ==
                mbim_data_interface.bInterfaceNumber)):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.3#4')

        # Test step 5
        # Get MBIM functional descriptor.
        if usb_descriptors.has_distinct_descriptors(mbim_descriptors):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Expected 1 unique MBIM functional descriptor.')
        mbim_descriptor = mbim_descriptors[0]

        if mbim_descriptor.index < header_descriptor.index:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.3#3')

        if mbim_descriptor.bLength != 12:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.4#5')

        if mbim_descriptor.bcdMBIMVersion != 0x0100:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.4#6')

        if mbim_descriptor.wMaxControlMessage < 64:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.4#1')

        if mbim_descriptor.bNumberFilters < 16:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.4#2')

        if mbim_descriptor.bMaxFilterSize > 192:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.4#3')

        # TODO(mcchou): Most of vendors set wMaxSegmentSize to be less than
        # 1500, so this assertion is skipped for now.
        #
        #if not mbim_descriptor.wMaxSegmentSize >= 2048:
        #    mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
        #                              'mbim1.0:6.4#4')

        # Use a byte as the mask to check if D0, D1, D2, D4, D6 and D7 are
        # zeros.
        if (mbim_descriptor.bmNetworkCapabilities & 0b11010111) > 0:
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.4#7')

        # Test step 6
        # Get MBIM extended functional descriptor, which is optional.
        if len(mbim_extended_descriptors) >= 1:
            if usb_descriptors.has_distinct_descriptors(
                    mbim_extended_descriptors):
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceGenerisAssertionError,
                        'Expected 1 unique MBIM extended functional '
                        'descriptor.')
            mbim_extended_descriptor = mbim_extended_descriptors[0]

            if mbim_extended_descriptor.index < mbim_descriptor.index:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:6.5#1')

            if mbim_extended_descriptor.bLength != 8:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:6.5#2')

            if mbim_extended_descriptor.bcdMBIMExtendedVersion != 0x0100:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:6.5#3')

            if mbim_extended_descriptor.bMaxOutstandingCommandMessages == 0:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:6.5#4')

        # Test step 7
        # Get the first endpoint for the communication interface.
        interrupt_endpoint_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.EndpointDescriptor,
                mbim_communication_interface_bundle)

        if len(interrupt_endpoint_descriptors) != 1:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Expected 1 endpoint, got %d.' % (
                            len(interrupt_endpoint_descriptors)))
        interrupt_endpoint_descriptor = interrupt_endpoint_descriptors[0]
        if not (interrupt_endpoint_descriptor.bDescriptorType == 0x05 and
                interrupt_endpoint_descriptor.bLength == 7 and
                interrupt_endpoint_descriptor.bEndpointAddress >= 0x80 and
                interrupt_endpoint_descriptor.bmAttributes == 0x03):
            mbim_errors.log_and_raise(mbim_errors.MBIMComplianceAssertionError,
                                      'mbim1.0:6.3#5')

        appear_before_functional_descriptors = False
        if mbim_extended_descriptors:
            if (mbim_extended_descriptor.index >
                interrupt_endpoint_descriptor.index):
                appear_before_functional_descriptors = True
        else:
            if (mbim_descriptor.index > interrupt_endpoint_descriptor.index or
                union_descriptor.index > interrupt_endpoint_descriptor.index):
                appear_before_functional_descriptors = True
        if appear_before_functional_descriptors:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'All functional descriptors must appear before endpoint'
                    'descriptors.')

        # Test step 8
        # Get interface association descriptor.
        interface_association_descriptors = (
                usb_descriptors.filter_descriptors(
                        usb_descriptors.InterfaceAssociationDescriptor,
                        descriptors))

        if usb_descriptors.has_distinct_descriptors(
                interface_association_descriptors):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceGenericAssertionError,
                    'Expected 1 interface association descriptor, got %d.' % (
                            len(interface_association_descriptors)))

        for association_descriptor in interface_association_descriptors:
            # Check interface association descriptor if one of the following
            # condition is met:
            # 1. bFirstInterface <= bControlInterface < (bFirstInterface +
            #                                            bInterfaceCount)
            # 2. bFirstInterface <= bSubordinateInterface0 < (
            #            bFirstInterface + bInterfaceCount)
            b_first_interface = association_descriptor.bFirstInterface
            b_interface_count = association_descriptor.bInterfaceCount
            b_control_interface = union_descriptor.bControlInterface
            b_subordinate_interface_0 = (
                    union_descriptor.bSubordinateInterface0)
            check_inteface_association_descriptor = False

            if ((b_first_interface <= b_control_interface < (
                         b_first_interface + b_interface_count)) or
                (b_first_interface <= b_subordinate_interface_0 < (
                         b_first_interface + b_interface_count))):
                check_interface_association_descriptor = True

            if not check_interface_association_descriptor:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceAssertionError,
                        'mbim1.0:6.1#1')

            if check_interface_association_descriptor:
                if not((b_first_interface == b_control_interface or
                        b_first_interface == b_subordinate_interface_0) and
                       (b_interface_count == 2) and
                       (b_subordinate_interface_0 == b_control_interface + 1 or
                        b_subordinate_interface_0 ==
                        b_control_interface - 1) and
                       (association_descriptor.bFunctionClass == 0x02) and
                       (association_descriptor.bFunctionSubClass == 0x0E) and
                       (association_descriptor.bFunctionProtocol == 0x00)):
                    mbim_errors.log_and_raise(
                            mbim_errors.MBIMComplianceAssertionError,
                            'mbim1.0:6.1#2')

    # End of run_internal().

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import usb_descriptors


class MbimDescriptorCache(object):
    """
    Class used to store a cache of the most frequently used MBIM descriptors.
    This caching of descriptors avoids frequent access to the device for
    any control information.

    """

    def __init__(self, descriptors):
        """
        Store the the relevant descriptors from the list of descriptors.

        @param descriptors: Raw descriptor set obtained from the device.
                            Type: Array of |usb_descriptors.Descriptor| objects.

        """
        if self._check_ncm_mbim_device(descriptors):
            self._update_ncm_mbim_cache(descriptors)
        else:
            self._update_mbim_cache(descriptors)


    def _store_in_cache(self,
                        descriptors,
                        mbim_communication_interface,
                        ncm_communication_interface,
                        no_data_data_interface,
                        mbim_data_interface,
                        ncm_data_interface):
        """
        Store the MBIM/NCM interface descriptors into the |device_context| and
        also fetch the MBIM funnction descriptor, interrrupt endpoint
        descriptor and bulk endpoint descriptors.

        @param descriptors: Raw descriptor set obtained from the device.
                Type: Array of |usb_descriptors.Descriptor| objects.
        @param mbim_communication_interface: MBIM communication interface
                descriptor object.
        @param ncm_communication_interface: NCM communication interface
                descriptor object if the device supports NCM/MBIM.
        @param no_data_data_interface: MBIM/NCM data interface object. To be set
                when not being used for any active data transfer.
        @param mbim_data_interface: MBIM data interface object. To be set
                when being used for any active MBIM NTB data transfer.
        @param ncm_data_interface: NCM data interface object. To be set
                when being used for any active NCM NTB data transfer.

        """
        # Fetch the MBIM function descriptor
        mbim_communication_interface_bundle = (
                usb_descriptors.get_descriptor_bundle(
                        descriptors, mbim_communication_interface))
        mbim_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.MBIMFunctionalDescriptor,
                mbim_communication_interface_bundle)
        if not mbim_descriptors:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No MBIM functional descriptor found')

        # Fetch the MBIM interrupt enpoint
        interrupt_endpoint_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.EndpointDescriptor,
                mbim_communication_interface_bundle)
        if not interrupt_endpoint_descriptors:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No MBIM Interrupt Endpoint descriptor found')

        # Fetch the MBIM bulk-in/out endpoints
        mbim_data_interface_bundle = (
                usb_descriptors.get_descriptor_bundle(
                        descriptors, mbim_data_interface))
        bulk_endpoint_descriptors = usb_descriptors.filter_descriptors(
                usb_descriptors.EndpointDescriptor,
                mbim_data_interface_bundle)
        if len(bulk_endpoint_descriptors) != 2:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'MBIM Bulk-In/Bulk-Out Endpoint descriptors not found')

        # Update with MBIM function settings.
        self.ncm_communication_interface = ncm_communication_interface
        self.mbim_communication_interface = mbim_communication_interface
        self.no_data_data_interface = no_data_data_interface
        self.ncm_data_interface = ncm_data_interface
        self.mbim_data_interface = mbim_data_interface
        self.mbim_functional = mbim_descriptors[0]
        self.interrupt_endpoint = interrupt_endpoint_descriptors[0]
        for endpoint in bulk_endpoint_descriptors:
            # Check for MSB bit to determine if it is a
            # BULK-OUT vs BULK-IN endpoint
            if endpoint.bEndpointAddress < 0x80:
                self.bulk_out_endpoint = endpoint
            else:
                self.bulk_in_endpoint = endpoint


    def _update_mbim_cache(self, descriptors):
        """
        Parse and cache given raw |descriptors| as MBIM descriptors.

        """
        self.is_mbim_only = True

        # Fetch the MBIM communication interface
        interfaces = usb_descriptors.filter_descriptors(
                usb_descriptors.InterfaceDescriptor, descriptors)
        mbim_communication_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.MBIM_ONLY_COMMUNICATION_INTERFACE))
        if not mbim_communication_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No MBIM communication interface descriptor found')

        # Fetch the MBIM no_data data interface
        no_data_data_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.MBIM_ONLY_DATA_INTERFACE_NO_DATA))
        if not no_data_data_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No No_Data data interface descriptor found')
        # Fetch the MBIM data interface
        mbim_data_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.MBIM_ONLY_DATA_INTERFACE_MBIM))
        if not mbim_data_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No MBIM data interface descriptor found')

        # Store the info in our |device_context| cache
        self._store_in_cache(descriptors,
                             mbim_communication_interfaces[0],
                             None,
                             no_data_data_interfaces[0],
                             mbim_data_interfaces[0],
                             None)


    def _update_ncm_mbim_cache(self, descriptors):
        """
        Parse and cache given raw |descriptors| as NCM + MBIM descriptors.

        """
        self.is_mbim_only = False

        # Fetch the NCM communication interface
        interfaces = usb_descriptors.filter_descriptors(
                usb_descriptors.InterfaceDescriptor, descriptors)
        ncm_communication_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.NCM_MBIM_COMMUNICATION_INTERFACE_NCM))

        # Fetch the MBIM communication interface
        mbim_communication_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.NCM_MBIM_COMMUNICATION_INTERFACE_MBIM))
        if not mbim_communication_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No MBIM communication interface descriptor found')

        # Fetch the NCM + MBIM no_data data interface
        no_data_data_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.NCM_MBIM_DATA_INTERFACE_NO_DATA))
        if not no_data_data_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No No_Data data interface descriptor found')
        # Fetch the NCM data interface
        ncm_data_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.NCM_MBIM_DATA_INTERFACE_NCM))
        if not ncm_data_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No NCM data interface descriptor found')
        # Fetch the MBIM data interface
        mbim_data_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.NCM_MBIM_DATA_INTERFACE_MBIM))
        if not mbim_data_interfaces:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceTestError,
                    'No MBIM data interface descriptor found')

        # Store the info in our |device_context| cache
        self._store_in_cache(descriptors,
                             mbim_communication_interfaces[0],
                             ncm_communication_interfaces[0],
                             no_data_data_interfaces[0],
                             mbim_data_interfaces[0],
                             ncm_data_interfaces[0])


    def _check_ncm_mbim_device(self, descriptors):
        """
        Checks whether the connected device supports NCM + MBIM or MBIM only.

        @returns True if the device supports NCM + MBIM, else False

        """
        # Only a dual NCM/MBIM device has an NCM communication interface
        # in its descriptors
        interfaces = usb_descriptors.filter_descriptors(
                usb_descriptors.InterfaceDescriptor, descriptors)
        ncm_communication_interfaces = (
                usb_descriptors.filter_interface_descriptors(
                        interfaces,
                        usb_descriptors.NCM_MBIM_COMMUNICATION_INTERFACE_NCM))
        return bool(ncm_communication_interfaces)

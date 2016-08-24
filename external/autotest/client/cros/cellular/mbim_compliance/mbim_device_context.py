# Copyright 2015 The chromimn OS Authros. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from usb import core
from usb import util as usb_util
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors
from autotest_lib.client.cros.cellular.mbim_compliance import \
        mbim_descriptor_cache

# Device types.
DEVICE_TYPE_UNKNOWN = 0
DEVICE_TYPE_MBIM = 1
DEVICE_TYPE_NCM_MBIM = 2
# MBIM Communication interface codes
INTERFACE_MBIM_CLASS = 0x02
INTERFACE_MBIM_SUBCLASS = 0x0E
INTERFACE_MBIM_PROTOCOL = 0x00


class MbimDeviceContext:
    """ Context of device under test. """

    def __init__(self, id_vendor=None, id_product=None):
        """
        Initialize the MBIM modem device test context.

        @param id_vendor: Specific vendor ID for the modem to be tested.
        @param id_product: Specific product ID for the modem to be tested.

        """
        # Find the device to be tested
        self._device = self._find_device(id_vendor, id_product)
        # Set the device vendor/product ID in the test context
        self._id_vendor = self._device.idVendor
        self._id_product = self._device.idProduct

        # TODO(mcchou): Generalize the order of running sequence and tests by
        # extracting the information retrieval logic as utility functions.
        # These utility functions will be used by |get_descriptors_sequence| and
        # DES_xx tests. Instead of retrieving information from DES_xx tests,
        # the information should be obtained from |get_descriptors_sequence|.

        # Once a device has been discovered, and its USB descriptors have been
        # parsed, this property determines whether the discovered device is an
        # MBIM only function (DEVICE_TYPE_MBIM) or an NCM/MBIM combined function
        # (DEVICE_TYPE_NCM_MBIM). The other |*_interface| properties are
        # determined accordingly.
        self.device_type = DEVICE_TYPE_UNKNOWN

        # The USB descriptor for the communication interface for the modem. This
        # descirptor corresponds to the alternate setting of the interface over
        # which mbim control command can be transferred.
        self.mbim_communication_interface = None

        # The USB descriptor for the communication interface for the modem. This
        # descriptor corresponds to the alternate setting of the interface over
        # which ncm control command can be transferred.
        self.ncm_communication_interface = None

        # The USB descriptor for the CDC Data interface for the modem. This
        # descriptor corresponds to the alternate setting of the interface over
        # which no data can be transferred.
        self.no_data_data_interface = None

        # The USB descriptor for the CDC Data interface for the modem. This
        # descriptor corresponds to the alternate setting of the interface over
        # which MBIM data must be transferred.
        self.mbim_data_interface = None

        # The USB descriptor for the CDC Data interface for the modem. This
        # descriptor corresponds to the alternate setting of the interface over
        # which NCM data must be transferred.
        self.ncm_data_interface = None

        # The USB descriptor for the MBIM functional settings for the modem.
        # This descriptor corresponds to the MBIM functional descriptor in the
        # MBIM communication interface settings.
        self.mbim_functional = None

        # The USB descriptor for the interrupt endpoint. This descriptor
        # corresponds to the interrupt endpoint in the MBIM communication
        # interface where MBIM control messages are sent and received.
        self.interrupt_endpoint = None


    def _find_device(self, id_vendor, id_product):
        """
        Find and initialize the MBIM modem device under consideration.

        @param id_vendor: Specific vendor ID for the modem to be tested.
        @param id_product: Specific product ID for the modem to be tested.
        @returns The PyUSB handle to the device.

        """
        # If a specific device VID/PID is sent, we'll use that info to find
        # the modem, else we'll try to find any MBIM CDC device attached
        if id_vendor is not None and id_product is not None:
            device = core.find(idVendor=id_vendor, idProduct=id_product)
            if device is None:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceFrameworkError,
                        'Device not found with VID: %04X, PID: %04X. ' % (
                                id_vendor, id_product))
        else:
            # Find device based on the communication class interface descriptor
            devices = core.find(
                    find_all=1,
                    custom_match=(lambda device: self._device_interface_matcher(
                            device,
                            interface_class=INTERFACE_MBIM_CLASS,
                            interface_subclass=INTERFACE_MBIM_SUBCLASS,
                            interface_protocol=INTERFACE_MBIM_PROTOCOL)))
            if not devices:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceFrameworkError,
                        'MBIM device not found. ')
            elif len(devices) > 1:
                mbim_errors.log_and_raise(
                        mbim_errors.MBIMComplianceFrameworkError,
                        'More than one MBIM device found: %d. ' %
                        len(devices))
            else:
                device = devices[0]
        return device


    def _device_interface_matcher(self,
                                  device,
                                  interface_class,
                                  interface_subclass,
                                  interface_protocol):
        """
        Find the USB device with a specific set of interface parameters.

        Go thru all the USB configurations and find an interface
        descriptor that matches the specified class, subclass and
        protocol.

        @param device: USB device under consideration.
        @param interface_class: Class ID to be matched in Interface
                                descriptor.
        @param interface_sub_class: Sub class ID to be matched in
                                    Interface descriptor.
        @param interface_protocol: Protocol ID to be matched in
                                   Interface descriptor.
        @returns True if the device's interface descriptor matches,
                 False otherwise.
        """
        for cfg in device:
            interface = usb_util.find_descriptor(
                    cfg,
                    bInterfaceClass=interface_class,
                    bInterfaceSubClass=interface_subclass,
                    bInterfaceProtocol=interface_protocol)
            if interface is not None:
                return True
        return False


    def update_descriptor_cache(self, descriptors):
        """
        Fetch and store the MBIM descriptor cache into the test context.

        @param descriptors: Raw descriptor set obtained from the device.
                            Type: Array of |usb_descriptors.Descriptor| objects.

        """
        self.descriptor_cache = (
                mbim_descriptor_cache.MbimDescriptorCache(descriptors))
        if self.descriptor_cache.is_mbim_only:
            self.device_type = DEVICE_TYPE_MBIM
        else:
            self.device_type = DEVICE_TYPE_NCM_MBIM


    @property
    def id_vendor(self):
        """
        Refer to the idVendor for the device under test.

        @returns The value of idVendor.

        """
        return self._id_vendor


    @property
    def id_product(self):
        """
        Refer to the idProduct for the device under test.

        @returns The value of idProduct.

        """
        return self._id_product


    @property
    def device(self):
        """
        Refer to the device under test.

        @returns The usb.core.Device object.

        """
        return self._device

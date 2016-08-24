# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An interface to access the local USB facade."""

import glob
import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import base_utils
from autotest_lib.client.cros.audio import cras_dbus_utils
from autotest_lib.client.cros.audio import cras_utils


class USBFacadeNativeError(Exception):
    """Error in USBFacadeNative."""
    pass


class USBFacadeNative(object):
    """Facade to access the USB-related functionality.

    Property:
      _drivers_manager: A USBDeviceDriversManager object used to manage the
                        status of drivers associated with the USB audio gadget
                        on the host side.

    """
    _DEFAULT_DEVICE_PRODUCT_NAME = 'Linux USB Audio Gadget'
    _TIMEOUT_FINDING_USB_DEVICE_SECS = 10
    _TIMEOUT_CRAS_NODES_CHANGE_SECS = 30

    def __init__(self):
        """Initializes the USB facade.

        The _drivers_manager is set with a USBDeviceDriversManager, which is
        used to control the visibility and availability of a USB device on a
        host Cros device.

        """
        self._drivers_manager = USBDeviceDriversManager()


    def _reenumerate_usb_devices(self):
        """Resets host controller to re-enumerate usb devices."""
        self._drivers_manager.reset_host_controller()


    def plug(self):
        """Sets and plugs the USB device into the host.

        The USB device is initially set to one with the default product name,
        which is assumed to be the name of the USB audio gadget on Chameleon.
        This method blocks until Cras enumerate USB nodes within a timeout
        specified in _wait_for_nodes_changed.

        """
        # Only supports controlling one USB device of default name.
        device_name = self._DEFAULT_DEVICE_PRODUCT_NAME

        def find_usb_device():
            """Find USB device with name device_name.

            @returns: True if succeed to find the device, False otherwise.

            """
            try:
                self._drivers_manager.find_usb_device(device_name)
                return True
            except USBDeviceDriversManagerError:
                logging.debug('Can not find %s yet' % device_name)
                return False

        if self._drivers_manager.has_found_device(device_name):
            if self._drivers_manager.drivers_are_bound():
                return
            self._drivers_manager.bind_usb_drivers()
            self._wait_for_nodes_changed()
        else:
            # If driver manager has not found device yet, re-enumerate USB
            # devices. The correct USB driver will be binded automatically.
            self._reenumerate_usb_devices()
            self._wait_for_nodes_changed()
            # Wait some time for paths and fields in sysfs to be created.
            utils.poll_for_condition(
                    condition=find_usb_device,
                    desc='Find USB device',
                    timeout=self._TIMEOUT_FINDING_USB_DEVICE_SECS)


    def unplug(self):
        """Unplugs the USB device from the host."""
        self._drivers_manager.unbind_usb_drivers()


    def _wait_for_nodes_changed(self):
        """Waits for Cras to enumerate USB nodes.

        USB nodes will be plugged, but not necessarily selected.

        """
        def find_usb_node():
            """Checks if USB input and output nodes are plugged.

            @returns: True if USB input and output nodes are plugged. False
                      otherwise.
            """
            out_nodes, in_nodes = cras_utils.get_plugged_node_types()
            logging.info('Cras nodes: output: %s, input: %s',
                         out_nodes, in_nodes)
            return 'USB' in out_nodes and 'USB' in in_nodes

        utils.poll_for_condition(
                condition=find_usb_node,
                desc='Find USB node',
                timeout=self._TIMEOUT_CRAS_NODES_CHANGE_SECS)


class USBDeviceDriversManagerError(Exception):
    """Error in USBDeviceDriversManager."""
    pass


class HostControllerDriver(object):
    """Abstract a host controller driver.

    This class stores id and path like:
    path: /sys/bus/pci/drivers/echi_hcd
    id: 0000:00:1a.0
    Then, it can bind/unbind driver by writing
    0000:00:1a.0 to /sys/bus/pci/drivers/echi_hcd/bind
    and /sys/bus/pci/drivers/echi_hcd/unbind.

    """
    def __init__(self, hcd_id, hcd_path):
        """Inits an HostControllerDriver object.

        @param hcd_id: The HCD id, e.g. 0000:00:1a.0
        @param hcd_path: The path to HCD, e.g. /sys/bus/pci/drivers/echi_hcd.

        """
        logging.debug('hcd id: %s, hcd path: %s', hcd_id, hcd_path)
        self._hcd_id = hcd_id
        self._hcd_path = hcd_path


    def reset(self):
        """Resets HCD by unbinding and binding driver."""
        base_utils.open_write_close(
            os.path.join(self._hcd_path, 'unbind'), self._hcd_id)
        base_utils.open_write_close(
            os.path.join(self._hcd_path, 'bind'), self._hcd_id)


class USBDeviceDriversManager(object):
    """The class to control the USB drivers associated with a USB device.

    By binding/unbinding certain USB driver, we can emulate the plug/unplug
    action on that bus. However, this method only applies when the USB driver
    has already been binded once.
    To solve above problem, we can unbind then bind USB host controller driver
    (HCD), then, HCD will re-enumerate all the USB devices. This method has
    a side effect that all the USB devices will be disconnected for several
    seconds, so we should only do it if needed.
    Note that there might be multiple HCDs, e.g. 0000:00:1a.0 for bus1 and
    0000:00:1b.0 for bus2.

    Properties:
        _device_product_name: The product name given to the USB device.
        _device_bus_id: The bus ID of the USB device in the host.
        _hcd_ids: The host controller driver IDs.
        _hcds: A list of HostControllerDrivers.

    """
    # The file to write to bind USB drivers of specified device
    _USB_BIND_FILE_PATH = '/sys/bus/usb/drivers/usb/bind'
    # The file to write to unbind USB drivers of specified device
    _USB_UNBIND_FILE_PATH = '/sys/bus/usb/drivers/usb/unbind'
    # The file path that exists when drivers are bound for current device
    _USB_BOUND_DRIVERS_FILE_PATH = '/sys/bus/usb/drivers/usb/%s/driver'
    # The pattern to glob usb drivers
    _USB_DRIVER_GLOB_PATTERN = '/sys/bus/usb/drivers/usb/usb?/'
    # The path to search for HCD on PCI or platform bus.
    # The HCD id should be filled in the end.
    _HCD_GLOB_PATTERNS = [
            '/sys/bus/pci/drivers/*/%s',
            '/sys/bus/platform/drivers/*/%s']

    # Skips auto HCD for issue crbug.com/537513.
    # Skips s5p-echi for issue crbug.com/546651.
    # This essentially means we can not control HCD on these boards.
    _SKIP_HCD_BLACKLIST = ['daisy', 'peach_pit', 'peach_pi']

    def __init__(self):
        """Initializes the manager.

        _device_product_name and _device_bus_id are initially set to None.

        """
        self._device_product_name = None
        self._device_bus_id = None
        self._hcd_ids = None
        self._hcds = None
        self._find_hcd_ids()
        self._create_hcds()


    def _skip_hcd(self):
        """Skips HCD controlling on some boards."""
        board = utils.get_board()
        if board in self._SKIP_HCD_BLACKLIST:
            logging.info('Skip HCD controlling on board %s', board)
            return True
        return False


    def _find_hcd_ids(self):
        """Finds host controller driver ids for USB.

        We can find the HCD id for USB from driver's realpath.
        E.g. On ARM device:
        /sys/bus/usb/drivers/usb/usb1 links to
        /sys/devices/soc0/70090000.usb/xhci-hcd.0.auto/usb1
        => HCD id is xhci-hcd.0.auto

        E.g. On X86 device:
        /sys/bus/usb/drivers/usb/usb1 links to
        /sys/devices/pci0000:00/0000:00:14.0/usb1
        => HCD id is 0000:00:14.0

        There might be multiple HCD ids like 0000:00:1a.0 for usb1,
        and 0000:00:1d.0 for usb2.

        @raises: USBDeviceDriversManagerError if HCD id can not be found.

        """
        def _get_dir_name(path):
            return os.path.basename(os.path.dirname(path))

        if self._skip_hcd():
            self._hcd_ids = set()
            return

        hcd_ids = set()

        for search_root_path in glob.glob(self._USB_DRIVER_GLOB_PATTERN):
            hcd_id = _get_dir_name(os.path.realpath(search_root_path))
            hcd_ids.add(hcd_id)

        if not hcd_ids:
            raise USBDeviceDriversManagerError('Can not find HCD id')

        self._hcd_ids = hcd_ids
        logging.debug('Found HCD ids: %s', self._hcd_ids)


    def _create_hcds(self):
        """Finds HCD paths from HCD id and create HostControllerDrivers.

        HCD is under /sys/bus/pci/drivers/ for x86 boards, and under
        /sys/bus/platform/drivers/ for ARM boards.

        For each HCD id, finds HCD by checking HCD id under it, e.g.
        /sys/bus/pci/drivers/ehci_hcd has 0000:00:1a.0 under it.
        Then, create a HostControllerDriver and store it in self._hcds.

        @raises: USBDeviceDriversManagerError if there are multiple
                 HCD path found for a given HCD id.

        @raises: USBDeviceDriversManagerError if no HostControllerDriver is found.

        """
        self._hcds = []

        for hcd_id in self._hcd_ids:
            for glob_pattern in self._HCD_GLOB_PATTERNS:
                glob_pattern = glob_pattern % hcd_id
                hcd_id_paths = glob.glob(glob_pattern)
                if not hcd_id_paths:
                    continue
                if len(hcd_id_paths) > 1:
                    raise USBDeviceDriversManagerError(
                            'More than 1 HCD id path found: %s' % hcd_id_paths)
                hcd_id_path = hcd_id_paths[0]

                # Gets /sys/bus/pci/drivers/echi_hcd from
                # /sys/bus/pci/drivers/echi_hcd/0000:00:1a.0
                hcd_path = os.path.dirname(hcd_id_path)
                self._hcds.append(
                        HostControllerDriver(hcd_id=hcd_id, hcd_path=hcd_path))


    def reset_host_controller(self):
        """Resets host controller by unbinding then binding HCD.

        @raises: USBDeviceDriversManagerError if there is no HCD to control.

        """
        if not self._hcds and not self._skip_hcd():
            raise USBDeviceDriversManagerError('HCD is not found yet')
        for hcd in self._hcds:
            hcd.reset()


    def _find_usb_device_bus_id(self, product_name):
        """Finds the bus ID of the USB device with the given product name.

        @param product_name: The product name of the USB device as it appears
                             to the host.

        @returns: The bus ID of the USB device if it is detected by the host
                  successfully; or None if there is no such device with the
                  given product name.

        """
        def product_matched(path):
            """Checks if the product field matches expected product name.

            @returns: True if the product name matches, False otherwise.

            """
            read_product_name = base_utils.read_one_line(path)
            logging.debug('Read product at %s = %s', path, read_product_name)
            return read_product_name == product_name

        # Find product field at these possible paths:
        # '/sys/bus/usb/drivers/usb/usbX/X-Y/product' => bus id is X-Y.
        # '/sys/bus/usb/drivers/usb/usbX/X-Y/X-Y.Z/product' => bus id is X-Y.Z.

        for search_root_path in glob.glob(self._USB_DRIVER_GLOB_PATTERN):
            logging.debug('search_root_path: %s', search_root_path)
            for root, dirs, _ in os.walk(search_root_path):
                logging.debug('root: %s', root)
                for bus_id in dirs:
                    logging.debug('bus_id: %s', bus_id)
                    product_path = os.path.join(root, bus_id, 'product')
                    logging.debug('product_path: %s', product_path)
                    if not os.path.exists(product_path):
                        continue
                    if not product_matched(product_path):
                        continue
                    logging.debug(
                            'Bus ID of %s found: %s', product_name, bus_id)
                    return bus_id

        logging.error('Bus ID of %s not found', product_name)
        return None


    def has_found_device(self, product_name):
        """Checks if the device has been found.

        @param product_name: The product name of the USB device as it appears
                             to the host.

        @returns: True if device has been found, False otherwise.

        """
        return self._device_product_name == product_name


    def find_usb_device(self, product_name):
        """Sets _device_product_name and _device_bus_id if it can be found.

        @param product_name: The product name of the USB device as it appears
                             to the host.

        @raises: USBDeviceDriversManagerError if device bus ID cannot be found
                 for the device with the given product name.

        """
        device_bus_id = self._find_usb_device_bus_id(product_name)
        if device_bus_id is None:
            error_message = 'Cannot find device with product name: %s'
            raise USBDeviceDriversManagerError(error_message % product_name)
        else:
            self._device_product_name = product_name
            self._device_bus_id = device_bus_id


    def drivers_are_bound(self):
        """Checks whether the drivers with the of current device are bound.

        If the drivers are already bound, calling bind_usb_drivers will be
        redundant and also result in an error.

        @return: True if the path to the drivers exist, meaning the drivers
                 are already bound. False otherwise.

        """
        if self._device_bus_id is None:
            raise USBDeviceDriversManagerError('USB Bus ID is not set yet.')
        driver_path = self._USB_BOUND_DRIVERS_FILE_PATH % self._device_bus_id
        return os.path.exists(driver_path)


    def bind_usb_drivers(self):
        """Binds the USB driver(s) of the current device to the host.

        This is applied to all the drivers associated with and listed under
        the USB device with the current _device_product_name and _device_bus_id.

        @raises: USBDeviceDriversManagerError if device bus ID for this instance
                 has not been set yet.

        """
        if self._device_bus_id is None:
            raise USBDeviceDriversManagerError('USB Bus ID is not set yet.')
        if self.drivers_are_bound():
            return
        base_utils.open_write_close(self._USB_BIND_FILE_PATH,
                self._device_bus_id)


    def unbind_usb_drivers(self):
        """Unbinds the USB driver(s) of the current device from the host.

        This is applied to all the drivers associated with and listed under
        the USB device with the current _device_product_name and _device_bus_id.

        @raises: USBDeviceDriversManagerError if device bus ID for this instance
                 has not been set yet.

        """
        if self._device_bus_id is None:
            raise USBDeviceDriversManagerError('USB Bus ID is not set yet.')
        if not self.drivers_are_bound():
            return
        base_utils.open_write_close(self._USB_UNBIND_FILE_PATH,
                                    self._device_bus_id)

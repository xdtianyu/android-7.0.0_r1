# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from autotest_lib.client.cros import constants
from autotest_lib.server import autotest


class BluetoothDevice(object):
    """BluetoothDevice is a thin layer of logic over a remote DUT.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its host property.

    """

    XMLRPC_BRINGUP_TIMEOUT_SECONDS = 60
    XMLRPC_LOG_PATH = '/var/log/bluetooth_xmlrpc_device.log'

    def __init__(self, device_host):
        """Construct a BluetoothDevice.

        @param device_host: host object representing a remote host.

        """
        self.host = device_host
        # Make sure the client library is on the device so that the proxy code
        # is there when we try to call it.
        client_at = autotest.Autotest(self.host)
        client_at.install()
        # Start up the XML-RPC proxy on the client.
        self._proxy = self.host.rpc_server_tracker.xmlrpc_connect(
                constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_COMMAND,
                constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_PORT,
                command_name=
                  constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_CLEANUP_PATTERN,
                ready_test_name=
                  constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_READY_METHOD,
                timeout_seconds=self.XMLRPC_BRINGUP_TIMEOUT_SECONDS,
                logfile=self.XMLRPC_LOG_PATH)


    def reset_on(self):
        """Reset the adapter and settings and power up the adapter.

        @return True on success, False otherwise.

        """
        return self._proxy.reset_on()


    def reset_off(self):
        """Reset the adapter and settings, leave the adapter powered off.

        @return True on success, False otherwise.

        """
        return self._proxy.reset_off()


    def has_adapter(self):
        """@return True if an adapter is present, False if not."""
        return self._proxy.has_adapter()


    def set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        return self._proxy.set_powered(powered)


    def set_discoverable(self, discoverable):
        """Set the adapter discoverable state.

        @param discoverable: adapter discoverable state to set (True or False).

        @return True on success, False otherwise.

        """
        return self._proxy.set_discoverable(discoverable)


    def set_pairable(self, pairable):
        """Set the adapter pairable state.

        @param pairable: adapter pairable state to set (True or False).

        @return True on success, False otherwise.

        """
        return self._proxy.set_pairable(pairable)


    def get_adapter_properties(self):
        """Read the adapter properties from the Bluetooth Daemon.

        @return the properties as a dictionary on success,
            the value False otherwise.

        """
        return json.loads(self._proxy.get_adapter_properties())


    def read_version(self):
        """Read the version of the management interface from the Kernel.

        @return the version as a tuple of:
          ( version, revision )

        """
        return json.loads(self._proxy.read_version())


    def read_supported_commands(self):
        """Read the set of supported commands from the Kernel.

        @return set of supported commands as arrays in a tuple of:
          ( commands, events )

        """
        return json.loads(self._proxy.read_supported_commands())


    def read_index_list(self):
        """Read the list of currently known controllers from the Kernel.

        @return array of controller indexes.

        """
        return json.loads(self._proxy.read_index_list())


    def read_info(self):
        """Read the adapter information from the Kernel.

        @return the information as a tuple of:
          ( address, bluetooth_version, manufacturer_id,
            supported_settings, current_settings, class_of_device,
            name, short_name )

        """
        return json.loads(self._proxy.read_info())


    def add_device(self, address, address_type, action):
        """Add a device to the Kernel action list.

        @param address: Address of the device to add.
        @param address_type: Type of device in @address.
        @param action: Action to take.

        @return tuple of ( address, address_type ) on success,
          None on failure.

        """
        return json.loads(self._proxy.add_device(address, address_type, action))


    def remove_device(self, address, address_type):
        """Remove a device from the Kernel action list.

        @param address: Address of the device to remove.
        @param address_type: Type of device in @address.

        @return tuple of ( address, address_type ) on success,
          None on failure.

        """
        return json.loads(self._proxy.remove_device(address, address_type))


    def get_devices(self):
        """Read information about remote devices known to the adapter.

        @return the properties of each device as an array of
            dictionaries on success, the value False otherwise.

        """
        return json.loads(self._proxy.get_devices())


    def start_discovery(self):
        """Start discovery of remote devices.

        Obtain the discovered device information using get_devices(), called
        stop_discovery() when done.

        @return True on success, False otherwise.

        """
        return self._proxy.start_discovery()


    def stop_discovery(self):
        """Stop discovery of remote devices.

        @return True on success, False otherwise.

        """
        return self._proxy.stop_discovery()


    def get_dev_info(self):
        """Read raw HCI device information.

        @return tuple of (index, name, address, flags, device_type, bus_type,
                       features, pkt_type, link_policy, link_mode,
                       acl_mtu, acl_pkts, sco_mtu, sco_pkts,
                       err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx,
                       sco_tx, sco_rx, byte_rx, byte_tx) on success,
                None on failure.

        """
        return json.loads(self._proxy.get_dev_info())


    def register_profile(self, path, uuid, options):
        """Register new profile (service).

        @param path: Path to the profile object.
        @param uuid: Service Class ID of the service as string.
        @param options: Dictionary of options for the new service, compliant
                        with BlueZ D-Bus Profile API standard.

        @return True on success, False otherwise.

        """
        return self._proxy.register_profile(path, uuid, options)


    def has_device(self, address):
        """Checks if the device with a given address exists.

        @param address: Address of the device.

        @returns: True if there is a device with that address.
                  False otherwise.

        """
        return self._proxy.has_device(address)


    def pair_legacy_device(self, address, pin, timeout):
        """Pairs a device with a given pin code.

        Registers an agent who handles pin code request and
        pairs a device with known pin code.

        @param address: Address of the device to pair.
        @param pin: The pin code of the device to pair.
        @param timeout: The timeout in seconds for pairing.

        @returns: True on success. False otherwise.

        """
        return self._proxy.pair_legacy_device(address, pin, timeout)


    def connect_device(self, address):
        """Connects a device.

        Connects a device if it is not connected.

        @param address: Address of the device to connect.

        @returns: True on success. False otherwise.

        """
        return self._proxy.connect_device(address)


    def device_is_connected(self, address):
        """Checks if a device is connected.

        @param address: Address of the device to check if it is connected.

        @returns: True if device is connected. False otherwise.

        """
        return self._proxy.device_is_connected(address)


    def disconnect_device(self, address):
        """Disconnects a device.

        Disconnects a device if it is connected.

        @param address: Address of the device to disconnect.

        @returns: True on success. False otherwise.

        """
        return self._proxy.disconnect_device(address)


    def copy_logs(self, destination):
        """Copy the logs generated by this device to a given location.

        @param destination: destination directory for the logs.

        """
        self.host.collect_logs(self.XMLRPC_LOG_PATH, destination)


    def close(self):
        """Tear down state associated with the client."""
        # Turn off the discoverable flag since it may affect future tests.
        self._proxy.set_discoverable(False)
        # Leave the adapter powered off, but don't do a full reset.
        self._proxy.set_powered(False)
        # This kills the RPC server.
        self.host.close()

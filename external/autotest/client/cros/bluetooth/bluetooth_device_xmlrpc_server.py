#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import dbus.mainloop.glib
import dbus.service
import gobject
import json
import logging
import logging.handlers
import os
import shutil

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_socket
from autotest_lib.client.cros import constants
from autotest_lib.client.cros import xmlrpc_server


class _PinAgent(dbus.service.Object):
    """The agent handling bluetooth device with a known pin code.

    _PinAgent overrides RequestPinCode method to return a given pin code.
    User can use this agent to pair bluetooth device which has a known pin code.

    """
    def __init__(self, pin, *args, **kwargs):
        super(_PinAgent, self).__init__(*args, **kwargs)
        self._pin = pin


    @dbus.service.method('org.bluez.Agent1', in_signature="o", out_signature="s")
    def RequestPinCode(self, device_path):
        """Requests pin code for a device.

        Returns the known pin code for the request.

        @param device_path: The object path of the device.

        @returns: The known pin code.

        """
        logging.info('RequestPinCode for %s, return %s', device_path, self._pin)
        return self._pin


class BluetoothDeviceXmlRpcDelegate(xmlrpc_server.XmlRpcDelegate):
    """Exposes DUT methods called remotely during Bluetooth autotests.

    All instance methods of this object without a preceding '_' are exposed via
    an XML-RPC server. This is not a stateless handler object, which means that
    if you store state inside the delegate, that state will remain around for
    future calls.
    """

    UPSTART_PATH = 'unix:abstract=/com/ubuntu/upstart'
    UPSTART_MANAGER_PATH = '/com/ubuntu/Upstart'
    UPSTART_MANAGER_IFACE = 'com.ubuntu.Upstart0_6'
    UPSTART_JOB_IFACE = 'com.ubuntu.Upstart0_6.Job'

    UPSTART_ERROR_UNKNOWNINSTANCE = \
            'com.ubuntu.Upstart0_6.Error.UnknownInstance'

    BLUETOOTHD_JOB = 'bluetoothd'

    DBUS_ERROR_SERVICEUNKNOWN = 'org.freedesktop.DBus.Error.ServiceUnknown'

    BLUEZ_SERVICE_NAME = 'org.bluez'
    BLUEZ_MANAGER_PATH = '/'
    BLUEZ_MANAGER_IFACE = 'org.freedesktop.DBus.ObjectManager'
    BLUEZ_ADAPTER_IFACE = 'org.bluez.Adapter1'
    BLUEZ_DEVICE_IFACE = 'org.bluez.Device1'
    BLUEZ_AGENT_MANAGER_PATH = '/org/bluez'
    BLUEZ_AGENT_MANAGER_IFACE = 'org.bluez.AgentManager1'
    BLUEZ_PROFILE_MANAGER_PATH = '/org/bluez'
    BLUEZ_PROFILE_MANAGER_IFACE = 'org.bluez.ProfileManager1'
    BLUEZ_ERROR_ALREADY_EXISTS = 'org.bluez.Error.AlreadyExists'

    BLUETOOTH_LIBDIR = '/var/lib/bluetooth'

    # Timeout for how long we'll wait for BlueZ and the Adapter to show up
    # after reset.
    ADAPTER_TIMEOUT = 30

    def __init__(self):
        super(BluetoothDeviceXmlRpcDelegate, self).__init__()

        # Open the Bluetooth Raw socket to the kernel which provides us direct,
        # raw, access to the HCI controller.
        self._raw = bluetooth_socket.BluetoothRawSocket()

        # Open the Bluetooth Control socket to the kernel which provides us
        # raw management access to the Bluetooth Host Subsystem. Read the list
        # of adapter indexes to determine whether or not this device has a
        # Bluetooth Adapter or not.
        self._control = bluetooth_socket.BluetoothControlSocket()
        self._has_adapter = len(self._control.read_index_list()) > 0

        # Set up the connection to Upstart so we can start and stop services
        # and fetch the bluetoothd job.
        self._upstart_conn = dbus.connection.Connection(self.UPSTART_PATH)
        self._upstart = self._upstart_conn.get_object(
                None,
                self.UPSTART_MANAGER_PATH)

        bluetoothd_path = self._upstart.GetJobByName(
                self.BLUETOOTHD_JOB,
                dbus_interface=self.UPSTART_MANAGER_IFACE)
        self._bluetoothd = self._upstart_conn.get_object(
                None,
                bluetoothd_path)

        # Arrange for the GLib main loop to be the default.
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

        # Set up the connection to the D-Bus System Bus, get the object for
        # the Bluetooth Userspace Daemon (BlueZ) and that daemon's object for
        # the Bluetooth Adapter.
        self._system_bus = dbus.SystemBus()
        self._update_bluez()
        self._update_adapter()

        # The agent to handle pin code request, which will be
        # created when user calls pair_legacy_device method.
        self._pin_agent = None


    def _update_bluez(self):
        """Store a D-Bus proxy for the Bluetooth daemon in self._bluez.

        This may be called in a loop until it returns True to wait for the
        daemon to be ready after it has been started.

        @return True on success, False otherwise.

        """
        self._bluez = None
        try:
            self._bluez = self._system_bus.get_object(
                    self.BLUEZ_SERVICE_NAME,
                    self.BLUEZ_MANAGER_PATH)
            logging.debug('bluetoothd is running')
            return True
        except dbus.exceptions.DBusException, e:
            if e.get_dbus_name() == self.DBUS_ERROR_SERVICEUNKNOWN:
                logging.debug('bluetoothd is not running')
                self._bluez = None
                return False
            else:
                logging.error('Error updating Bluez!')
                raise


    def _update_adapter(self):
        """Store a D-Bus proxy for the local adapter in self._adapter.

        This may be called in a loop until it returns True to wait for the
        daemon to be ready, and have obtained the adapter information itself,
        after it has been started.

        Since not all devices will have adapters, this will also return True
        in the case where we have obtained an empty adapter index list from the
        kernel.

        @return True on success, including if there is no local adapter,
            False otherwise.

        """
        self._adapter = None
        if self._bluez is None:
            logging.warning('Bluez not found!')
            return False
        if not self._has_adapter:
            logging.debug('Device has no adapter; returning')
            return True

        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        for path, ifaces in objects.iteritems():
            logging.debug('%s -> %r', path, ifaces.keys())
            if self.BLUEZ_ADAPTER_IFACE in ifaces:
                logging.debug('using adapter %s', path)
                self._adapter = self._system_bus.get_object(
                        self.BLUEZ_SERVICE_NAME,
                        path)
                return True
        else:
            logging.warning('No adapter found in interface!')
            return False


    @xmlrpc_server.dbus_safe(False)
    def reset_on(self):
        """Reset the adapter and settings and power up the adapter.

        @return True on success, False otherwise.

        """
        self._reset()
        if not self._adapter:
            return False
        self._set_powered(True)
        return True


    @xmlrpc_server.dbus_safe(False)
    def reset_off(self):
        """Reset the adapter and settings, leave the adapter powered off.

        @return True on success, False otherwise.

        """
        self._reset()
        return True


    def has_adapter(self):
        """Return if an adapter is present.

        This will only return True if we have determined both that there is
        a Bluetooth adapter on this device (kernel adapter index list is not
        empty) and that the Bluetooth daemon has exported an object for it.

        @return True if an adapter is present, False if not.

        """
        return self._has_adapter and self._adapter is not None


    def _reset(self):
        """Reset the Bluetooth adapter and settings."""
        logging.debug('_reset')
        if self._adapter:
            self._set_powered(False)

        try:
            self._bluetoothd.Stop(dbus.Array(signature='s'), True,
                                  dbus_interface=self.UPSTART_JOB_IFACE)
        except dbus.exceptions.DBusException, e:
            if e.get_dbus_name() != self.UPSTART_ERROR_UNKNOWNINSTANCE:
                logging.error('Error resetting adapter!')
                raise

        def bluez_stopped():
            """Checks the bluetooth daemon status.

            @returns: True if bluez is stopped. False otherwise.

            """
            return not self._update_bluez()

        logging.debug('waiting for bluez stop')
        utils.poll_for_condition(
                condition=bluez_stopped,
                desc='Bluetooth Daemon has stopped.',
                timeout=self.ADAPTER_TIMEOUT)

        for subdir in os.listdir(self.BLUETOOTH_LIBDIR):
            shutil.rmtree(os.path.join(self.BLUETOOTH_LIBDIR, subdir))

        self._bluetoothd.Start(dbus.Array(signature='s'), True,
                               dbus_interface=self.UPSTART_JOB_IFACE)

        logging.debug('waiting for bluez start')
        utils.poll_for_condition(
                condition=self._update_bluez,
                desc='Bluetooth Daemon has started.',
                timeout=self.ADAPTER_TIMEOUT)

        logging.debug('waiting for bluez to obtain adapter information')
        utils.poll_for_condition(
                condition=self._update_adapter,
                desc='Bluetooth Daemon has adapter information.',
                timeout=self.ADAPTER_TIMEOUT)


    @xmlrpc_server.dbus_safe(False)
    def set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        if not self._adapter:
            if not powered:
                # Return success if we are trying to power off an adapter that's
                # missing or gone away, since the expected result has happened.
                return True
            else:
                logging.warning('Adapter not found!')
                return False
        self._set_powered(powered)
        return True


    def _set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        """
        logging.debug('_set_powered %r', powered)
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE, 'Powered', powered,
                          dbus_interface=dbus.PROPERTIES_IFACE)


    @xmlrpc_server.dbus_safe(False)
    def set_discoverable(self, discoverable):
        """Set the adapter discoverable state.

        @param discoverable: adapter discoverable state to set (True or False).

        @return True on success, False otherwise.

        """
        if not discoverable and not self._adapter:
            # Return success if we are trying to make an adapter that's
            # missing or gone away, undiscoverable, since the expected result
            # has happened.
            return True
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'Discoverable', discoverable,
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def set_pairable(self, pairable):
        """Set the adapter pairable state.

        @param pairable: adapter pairable state to set (True or False).

        @return True on success, False otherwise.

        """
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE, 'Pairable', pairable,
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def get_adapter_properties(self):
        """Read the adapter properties from the Bluetooth Daemon.

        @return the properties as a JSON-encoded dictionary on success,
            the value False otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        adapter = objects[self._adapter.object_path][self.BLUEZ_ADAPTER_IFACE]
        return json.dumps(adapter)


    def read_version(self):
        """Read the version of the management interface from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( version, revision )

        """
        return json.dumps(self._control.read_version())


    def read_supported_commands(self):
        """Read the set of supported commands from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( commands, events )

        """
        return json.dumps(self._control.read_supported_commands())


    def read_index_list(self):
        """Read the list of currently known controllers from the Kernel.

        @return the information as a JSON-encoded array of controller indexes.

        """
        return json.dumps(self._control.read_index_list())


    def read_info(self):
        """Read the adapter information from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( address, bluetooth_version, manufacturer_id,
            supported_settings, current_settings, class_of_device,
            name, short_name )

        """
        return json.dumps(self._control.read_info(0))


    def add_device(self, address, address_type, action):
        """Add a device to the Kernel action list.

        @param address: Address of the device to add.
        @param address_type: Type of device in @address.
        @param action: Action to take.

        @return on success, a JSON-encoded typle of:
          ( address, address_type ), None on failure.

        """
        return json.dumps(self._control.add_device(
                0, address, address_type, action))


    def remove_device(self, address, address_type):
        """Remove a device from the Kernel action list.

        @param address: Address of the device to remove.
        @param address_type: Type of device in @address.

        @return on success, a JSON-encoded typle of:
          ( address, address_type ), None on failure.

        """
        return json.dumps(self._control.remove_device(
                0, address, address_type))


    @xmlrpc_server.dbus_safe(False)
    def get_devices(self):
        """Read information about remote devices known to the adapter.

        @return the properties of each device as a JSON-encoded array of
            dictionaries on success, the value False otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=True)
        devices = []
        for path, ifaces in objects.iteritems():
            if self.BLUEZ_DEVICE_IFACE in ifaces:
                devices.append(objects[path][self.BLUEZ_DEVICE_IFACE])
        return json.dumps(devices)


    @xmlrpc_server.dbus_safe(False)
    def start_discovery(self):
        """Start discovery of remote devices.

        Obtain the discovered device information using get_devices(), called
        stop_discovery() when done.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return False
        self._adapter.StartDiscovery(dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def stop_discovery(self):
        """Stop discovery of remote devices.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return False
        self._adapter.StopDiscovery(dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True


    def get_dev_info(self):
        """Read raw HCI device information.

        @return JSON-encoded tuple of:
                (index, name, address, flags, device_type, bus_type,
                       features, pkt_type, link_policy, link_mode,
                       acl_mtu, acl_pkts, sco_mtu, sco_pkts,
                       err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx,
                       sco_tx, sco_rx, byte_rx, byte_tx) on success,
                None on failure.

        """
        return json.dumps(self._raw.get_dev_info(0))


    @xmlrpc_server.dbus_safe(False)
    def register_profile(self, path, uuid, options):
        """Register new profile (service).

        @param path: Path to the profile object.
        @param uuid: Service Class ID of the service as string.
        @param options: Dictionary of options for the new service, compliant
                        with BlueZ D-Bus Profile API standard.

        @return True on success, False otherwise.

        """
        profile_manager = dbus.Interface(
                              self._system_bus.get_object(
                                  self.BLUEZ_SERVICE_NAME,
                                  self.BLUEZ_PROFILE_MANAGER_PATH),
                              self.BLUEZ_PROFILE_MANAGER_IFACE)
        profile_manager.RegisterProfile(path, uuid, options)
        return True


    @xmlrpc_server.dbus_safe(False)
    def has_device(self, address):
        """Checks if the device with a given address exists.

        @param address: Address of the device.

        @returns: True if there is a device with that address.
                  False otherwise.

        """
        return self._find_device(address) != None


    def _find_device(self, address):
        """Finds the device with a given address.

        Find the device with a given address and returns the
        device interface.

        @param address: Address of the device.

        @returns: An 'org.bluez.Device1' interface to the device.
                  None if device can not be found.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        for path, ifaces in objects.iteritems():
            device = ifaces.get(self.BLUEZ_DEVICE_IFACE)
            if device is None:
                continue
            if (device['Address'] == address and
                path.startswith(self._adapter.object_path)):
                obj = self._system_bus.get_object(
                        self.BLUEZ_SERVICE_NAME, path)
                return dbus.Interface(obj, self.BLUEZ_DEVICE_IFACE)
        logging.error('Device not found')
        return None


    def _setup_pin_agent(self, pin):
        """Initializes a _PinAgent and registers it to handle pin code request.

        @param pin: The pin code this agent will answer.

        """
        agent_path = '/test/agent'
        if self._pin_agent:
            logging.info('Removing the old agent before initializing a new one')
            self._pin_agent.remove_from_connection()
            self._pin_agent = None
        self._pin_agent = _PinAgent(pin, self._system_bus, agent_path)
        agent_manager = dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                            self.BLUEZ_AGENT_MANAGER_PATH),
                self.BLUEZ_AGENT_MANAGER_IFACE)
        try:
            agent_manager.RegisterAgent(agent_path, 'NoInputNoOutput')
        except dbus.exceptions.DBusException, e:
            if e.get_dbus_name() == self.BLUEZ_ERROR_ALREADY_EXISTS:
                logging.info('Unregistering old agent and registering the new')
                agent_manager.UnregisterAgent(agent_path)
                agent_manager.RegisterAgent(agent_path, 'NoInputNoOutput')
            else:
                logging.error('Error setting up pin agent: %s', e)
                raise
        logging.info('Agent registered')


    def _is_paired(self,  device):
        """Checks if a device is paired.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is paired. False otherwise.

        """
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        paired = props.Get(self.BLUEZ_DEVICE_IFACE, 'Paired')
        return bool(paired)


    def _is_connected(self,  device):
        """Checks if a device is connected.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is connected. False otherwise.

        """
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        connected = props.Get(self.BLUEZ_DEVICE_IFACE, 'Connected')
        logging.info('Got connected = %r', connected)
        return bool(connected)


    @xmlrpc_server.dbus_safe(False)
    def pair_legacy_device(self, address, pin, timeout):
        """Pairs a device with a given pin code.

        Registers a agent who handles pin code request and
        pairs a device with known pin code.

        @param address: Address of the device to pair.
        @param pin: The pin code of the device to pair.
        @param timeout: The timeout in seconds for pairing.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if self._is_paired(device):
            logging.info('Device is already paired')
            return True

        self._setup_pin_agent(pin)
        mainloop = gobject.MainLoop()


        def pair_reply():
            """Handler when pairing succeeded."""
            logging.info('Device paired')
            mainloop.quit()


        def pair_error(error):
            """Handler when pairing failed.

            @param error: one of errors defined in org.bluez.Error representing
                          the error in pairing.

            """
            try:
                error_name = error.get_dbus_name()
                if error_name == 'org.freedesktop.DBus.Error.NoReply':
                    logging.error('Timed out. Cancelling pairing')
                    device.CancelPairing()
                else:
                    logging.error('Pairing device failed: %s', error)
            finally:
                mainloop.quit()


        device.Pair(reply_handler=pair_reply, error_handler=pair_error,
                    timeout=timeout * 1000)
        mainloop.run()
        return self._is_paired(device)


    @xmlrpc_server.dbus_safe(False)
    def remove_device_object(self, address):
        """Removes a device object and the pairing information.

        Calls RemoveDevice method to remove remote device
        object and the pairing information.

        @param address: Address of the device to unpair.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        self._adapter.RemoveDevice(
                device.object_path, dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def connect_device(self, address):
        """Connects a device.

        Connects a device if it is not connected.

        @param address: Address of the device to connect.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if self._is_connected(device):
          logging.info('Device is already connected')
          return True
        device.Connect()
        return self._is_connected(device)


    @xmlrpc_server.dbus_safe(False)
    def device_is_connected(self, address):
        """Checks if a device is connected.

        @param address: Address of the device to connect.

        @returns: True if device is connected. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        return self._is_connected(device)


    @xmlrpc_server.dbus_safe(False)
    def disconnect_device(self, address):
        """Disconnects a device.

        Disconnects a device if it is connected.

        @param address: Address of the device to disconnect.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if not self._is_connected(device):
          logging.info('Device is not connected')
          return True
        device.Disconnect()
        return not self._is_connected(device)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    handler = logging.handlers.SysLogHandler(address='/dev/log')
    formatter = logging.Formatter(
            'bluetooth_device_xmlrpc_server: [%(levelname)s] %(message)s')
    handler.setFormatter(formatter)
    logging.getLogger().addHandler(handler)
    logging.debug('bluetooth_device_xmlrpc_server main...')
    server = xmlrpc_server.XmlRpcServer(
            'localhost',
            constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_PORT)
    server.register_delegate(BluetoothDeviceXmlRpcDelegate())
    server.run()

#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import json
import logging
import logging.handlers

import common
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_sdp_socket
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_socket
from autotest_lib.client.cros import constants
from autotest_lib.client.cros import xmlrpc_server


class BluetoothTesterXmlRpcDelegate(xmlrpc_server.XmlRpcDelegate):
    """Exposes Tester methods called remotely during Bluetooth autotests.

    All instance methods of this object without a preceding '_' are exposed via
    an XML-RPC server. This is not a stateless handler object, which means that
    if you store state inside the delegate, that state will remain around for
    future calls.
    """

    BR_EDR_LE_PROFILE = (
            bluetooth_socket.MGMT_SETTING_POWERED |
            bluetooth_socket.MGMT_SETTING_CONNECTABLE |
            bluetooth_socket.MGMT_SETTING_PAIRABLE |
            bluetooth_socket.MGMT_SETTING_SSP |
            bluetooth_socket.MGMT_SETTING_BREDR |
            bluetooth_socket.MGMT_SETTING_LE)

    LE_PROFILE = (
            bluetooth_socket.MGMT_SETTING_POWERED |
            bluetooth_socket.MGMT_SETTING_CONNECTABLE |
            bluetooth_socket.MGMT_SETTING_PAIRABLE |
            bluetooth_socket.MGMT_SETTING_LE)

    PROFILE_SETTINGS = {
        'computer': BR_EDR_LE_PROFILE,
        'peripheral': LE_PROFILE
    }

    PROFILE_CLASS = {
        'computer': 0x000104,
        'peripheral': None
    }

    PROFILE_NAMES = {
        'computer': ('ChromeOS Bluetooth Tester', 'Tester'),
        'peripheral': ('ChromeOS Bluetooth Tester', 'Tester')
    }


    def __init__(self):
        super(BluetoothTesterXmlRpcDelegate, self).__init__()

        # Open the Bluetooth Control socket to the kernel which provides us
        # the needed raw management access to the Bluetooth Host Subsystem.
        self._control = bluetooth_socket.BluetoothControlSocket()
        # Open the Bluetooth SDP socket to the kernel which provides us the
        # needed interface to use SDP commands.
        self._sdp = bluetooth_sdp_socket.BluetoothSDPSocket()
        # This is almost a constant, but it might not be forever.
        self.index = 0


    def setup(self, profile):
        """Set up the tester with the given profile.

        @param profile: Profile to use for this test, valid values are:
                computer - a standard computer profile

        @return True on success, False otherwise.

        """
        profile_settings = self.PROFILE_SETTINGS[profile]
        profile_class = self.PROFILE_CLASS[profile]
        (profile_name, profile_short_name) = self.PROFILE_NAMES[profile]

        # Make sure the controller actually exists.
        if self.index not in self._control.read_index_list():
            logging.warning('Bluetooth Controller missing on tester')
            return False

        # Make sure all of the settings are supported by the controller.
        ( address, bluetooth_version, manufacturer_id,
          supported_settings, current_settings, class_of_device,
          name, short_name ) = self._control.read_info(self.index)
        if profile_settings & supported_settings != profile_settings:
            logging.warning('Controller does not support requested settings')
            logging.debug('Supported: %b; Requested: %b', supported_settings,
                          profile_settings)
            return False

        # Before beginning, force the adapter power off, even if it's already
        # off; this is enough to persuade an AP-mode Intel chip to accept
        # settings.
        if not self._control.set_powered(self.index, False):
            logging.warning('Failed to power off adapter to accept settings')
            return False

        # Set the controller up as either BR/EDR only, LE only or Dual Mode.
        # This is a bit tricky because it rejects commands outright unless
        # it's in dual mode, so we actually have to figure out what changes
        # we have to make, and we have to turn things on before we turn them
        # off.
        turn_on = (current_settings ^ profile_settings) & profile_settings
        if turn_on & bluetooth_socket.MGMT_SETTING_BREDR:
            if self._control.set_bredr(self.index, True) is None:
                logging.warning('Failed to enable BR/EDR')
                return False
        if turn_on & bluetooth_socket.MGMT_SETTING_LE:
            if self._control.set_le(self.index, True) is None:
                logging.warning('Failed to enable LE')
                return False

        turn_off = (current_settings ^ profile_settings) & current_settings
        if turn_off & bluetooth_socket.MGMT_SETTING_BREDR:
            if self._control.set_bredr(self.index, False) is None:
                logging.warning('Failed to disable BR/EDR')
                return False
        if turn_off & bluetooth_socket.MGMT_SETTING_LE:
            if self._control.set_le(self.index, False) is None:
                logging.warning('Failed to disable LE')
                return False

        # Adjust settings that are BR/EDR specific that we need to set before
        # powering on the adapter, and would be rejected otherwise.
        if profile_settings & bluetooth_socket.MGMT_SETTING_BREDR:
            if (self._control.set_link_security(
                    self.index,
                    (profile_settings &
                            bluetooth_socket.MGMT_SETTING_LINK_SECURITY))
                        is None):
                logging.warning('Failed to set link security setting')
                return False
            if (self._control.set_ssp(
                    self.index,
                    profile_settings & bluetooth_socket.MGMT_SETTING_SSP)
                        is None):
                logging.warning('Failed to set SSP setting')
                return False
            if (self._control.set_hs(
                    self.index,
                    profile_settings & bluetooth_socket.MGMT_SETTING_HS)
                        is None):
                logging.warning('Failed to set High Speed setting')
                return False

            # Split our the major and minor class; it's listed as a kernel bug
            # that we supply these to the kernel without shifting the bits over
            # to take out the CoD format field, so this might have to change
            # one day.
            major_class = (profile_class & 0x00ff00) >> 8
            minor_class = profile_class & 0x0000ff
            if (self._control.set_device_class(
                    self.index, major_class, minor_class)
                        is None):
                logging.warning('Failed to set device class')
                return False

        # Setup generic settings that apply to either BR/EDR, LE or dual-mode
        # that still require the power to be off.
        if (self._control.set_connectable(
                self.index,
                profile_settings & bluetooth_socket.MGMT_SETTING_CONNECTABLE)
                    is None):
            logging.warning('Failed to set connectable setting')
            return False
        if (self._control.set_pairable(
                self.index,
                profile_settings & bluetooth_socket.MGMT_SETTING_PAIRABLE)
                    is None):
            logging.warning('Failed to set pairable setting')
            return False

        if (self._control.set_local_name(
                    self.index, profile_name, profile_short_name)
                    is None):
            logging.warning('Failed to set local name')
            return False

        # Now the settings have been set, power up the adapter.
        if not self._control.set_powered(
                self.index,
                profile_settings & bluetooth_socket.MGMT_SETTING_POWERED):
            logging.warning('Failed to set powered setting')
            return False

        # Fast connectable can only be set once the controller is powered,
        # and only when BR/EDR is enabled.
        if profile_settings & bluetooth_socket.MGMT_SETTING_BREDR:
            # Wait for the device class set event, this happens after the
            # power up "command complete" event when we've pre-set the class
            # even though it's a side-effect of doing that.
            self._control.wait_for_events(
                    self.index,
                    ( bluetooth_socket.MGMT_EV_CLASS_OF_DEV_CHANGED, ))

            if (self._control.set_fast_connectable(
                    self.index,
                    profile_settings &
                    bluetooth_socket.MGMT_SETTING_FAST_CONNECTABLE)
                        is None):
                logging.warning('Failed to set fast connectable setting')
                return False

        # Fetch the settings again and make sure they're all set correctly,
        # including the BR/EDR flag.
        ( address, bluetooth_version, manufacturer_id,
          supported_settings, current_settings, class_of_device,
          name, short_name ) = self._control.read_info(self.index)

        # Check generic settings.
        if profile_settings != current_settings:
            logging.warning('Controller settings did not match those set: '
                            '%x != %x', current_settings, profile_settings)
            return False
        if name != profile_name:
            logging.warning('Local name did not match that set: "%s" != "%s"',
                            name, profile_name)
            return False
        elif short_name != profile_short_name:
            logging.warning('Short name did not match that set: "%s" != "%s"',
                            short_name, profile_short_name)
            return False

        # Check BR/EDR specific settings.
        if profile_settings & bluetooth_socket.MGMT_SETTING_BREDR:
            if class_of_device != profile_class:
                if class_of_device & 0x00ffff == profile_class & 0x00ffff:
                    logging.warning('Class of device matched that set, but '
                                    'Service Class field did not: %x != %x '
                                    'Reboot Tester? ',
                                    class_of_device, profile_class)
                else:
                    logging.warning('Class of device did not match that set: '
                                    '%x != %x', class_of_device, profile_class)
                return False

        return True


    def set_discoverable(self, discoverable, timeout=0):
        """Set the discoverable state of the controller.

        @param discoverable: Whether controller should be discoverable.
        @param timeout: Timeout in seconds before disabling discovery again,
                ignored when discoverable is False, must not be zero when
                discoverable is True.

        @return True on success, False otherwise.

        """
        settings = self._control.set_discoverable(self.index,
                                                  discoverable, timeout)
        return settings & bluetooth_socket.MGMT_SETTING_DISCOVERABLE


    def read_info(self):
        """Read the adapter information from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( address, bluetooth_version, manufacturer_id,
            supported_settings, current_settings, class_of_device,
            name, short_name )

        """
        return json.dumps(self._control.read_info(self.index))


    def set_advertising(self, advertising):
        """Set the whether the controller is advertising via LE.

        @param advertising: Whether controller should advertise via LE.

        @return True on success, False otherwise.

        """
        settings = self._control.set_advertising(self.index, advertising)
        return settings & bluetooth_socket.MGMT_SETTING_ADVERTISING


    def discover_devices(self, br_edr=True, le_public=True, le_random=True):
        """Discover remote devices.

        Activates device discovery and collects the set of devices found,
        returning them as a list.

        @param br_edr: Whether to detect BR/EDR devices.
        @param le_public: Whether to detect LE Public Address devices.
        @param le_random: Whether to detect LE Random Address devices.

        @return List of devices found as JSON-encoded tuples with the format
                (address, address_type, rssi, flags, base64-encoded eirdata),
                or False if discovery could not be started.

        """
        address_type = 0
        if br_edr:
            address_type |= 0x1
        if le_public:
            address_type |= 0x2
        if le_random:
            address_type |= 0x4

        set_type = self._control.start_discovery(self.index, address_type)
        if set_type != address_type:
            logging.warning('Discovery address type did not match that set: '
                            '%x != %x', set_type, address_type)
            return False

        devices = self._control.get_discovered_devices(self.index)
        return json.dumps([
                (address, address_type, rssi, flags,
                 base64.encodestring(eirdata))
                for address, address_type, rssi, flags, eirdata in devices
        ])


    def connect(self, address):
        """Connect to device with the given address

        @param address: Bluetooth address.

        """
        self._sdp.connect(address)
        return True


    def service_search_request(self, uuids, max_rec_cnt, preferred_size=32,
                               forced_pdu_size=None, invalid_request=False):
        """Send a Service Search Request

        @param uuids: List of UUIDs (as integers) to look for.
        @param max_rec_cnt: Maximum count of returned service records.
        @param preferred_size: Preffered size of UUIDs in bits (16, 32, or 128).
        @param forced_pdu_size: Use certain PDU size parameter instead of
               calculating actual length of sequence.
        @param invalid_request: Whether to send request with intentionally
               invalid syntax for testing purposes (bool flag).

        @return list of found services' service record handles or Error Code

        """
        return json.dumps(
                self._sdp.service_search_request(
                 uuids, max_rec_cnt, preferred_size, forced_pdu_size,
                 invalid_request)
        )


    def service_attribute_request(self, handle, max_attr_byte_count, attr_ids,
                                  forced_pdu_size=None, invalid_request=None):
        """Send a Service Attribute Request

        @param handle: service record from which attribute values are to be
               retrieved.
        @param max_attr_byte_count: maximum number of bytes of attribute data to
               be returned in the response to this request.
        @param attr_ids: a list, where each element is either an attribute ID
               or a range of attribute IDs.
        @param forced_pdu_size: Use certain PDU size parameter instead of
               calculating actual length of sequence.
        @param invalid_request: Whether to send request with intentionally
               invalid syntax for testing purposes (string with raw request).

        @return list of found attributes IDs and their values or Error Code

        """
        return json.dumps(
                self._sdp.service_attribute_request(
                 handle, max_attr_byte_count, attr_ids, forced_pdu_size,
                 invalid_request)
        )


    def service_search_attribute_request(self, uuids, max_attr_byte_count,
                                         attr_ids, preferred_size=32,
                                         forced_pdu_size=None,
                                         invalid_request=None):
        """Send a Service Search Attribute Request

        @param uuids: list of UUIDs (as integers) to look for.
        @param max_attr_byte_count: maximum number of bytes of attribute data to
               be returned in the response to this request.
        @param attr_ids: a list, where each element is either an attribute ID
               or a range of attribute IDs.
        @param preferred_size: Preffered size of UUIDs in bits (16, 32, or 128).
        @param forced_pdu_size: Use certain PDU size parameter instead of
               calculating actual length of sequence.
        @param invalid_request: Whether to send request with intentionally
               invalid syntax for testing purposes (string to be prepended
               to correct request).

        @return list of found attributes IDs and their values or Error Code

        """
        return json.dumps(
                self._sdp.service_search_attribute_request(
                 uuids, max_attr_byte_count, attr_ids, preferred_size,
                 forced_pdu_size, invalid_request)
        )


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    handler = logging.handlers.SysLogHandler(address = '/dev/log')
    formatter = logging.Formatter(
            'bluetooth_tester_xmlrpc_server: [%(levelname)s] %(message)s')
    handler.setFormatter(formatter)
    logging.getLogger().addHandler(handler)
    logging.debug('bluetooth_tester_xmlrpc_server main...')
    server = xmlrpc_server.XmlRpcServer(
            'localhost',
            constants.BLUETOOTH_TESTER_XMLRPC_SERVER_PORT)
    server.register_delegate(BluetoothTesterXmlRpcDelegate())
    server.run()

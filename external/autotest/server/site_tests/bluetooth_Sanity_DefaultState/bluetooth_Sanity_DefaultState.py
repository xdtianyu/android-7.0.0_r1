# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_socket
from autotest_lib.server.cros.bluetooth import bluetooth_test


class bluetooth_Sanity_DefaultState(bluetooth_test.BluetoothTest):
    """
    Verify that the Bluetooth adapter has correct state.
    """
    version = 1

    def _log_settings(self, msg, settings):
        strs = []
        if settings & bluetooth_socket.MGMT_SETTING_POWERED:
            strs.append("POWERED")
        if settings & bluetooth_socket.MGMT_SETTING_CONNECTABLE:
            strs.append("CONNECTABLE")
        if settings & bluetooth_socket.MGMT_SETTING_FAST_CONNECTABLE:
            strs.append("FAST-CONNECTABLE")
        if settings & bluetooth_socket.MGMT_SETTING_DISCOVERABLE:
            strs.append("DISCOVERABLE")
        if settings & bluetooth_socket.MGMT_SETTING_PAIRABLE:
            strs.append("PAIRABLE")
        if settings & bluetooth_socket.MGMT_SETTING_LINK_SECURITY:
            strs.append("LINK-SECURITY")
        if settings & bluetooth_socket.MGMT_SETTING_SSP:
            strs.append("SSP")
        if settings & bluetooth_socket.MGMT_SETTING_BREDR:
            strs.append("BR/EDR")
        if settings & bluetooth_socket.MGMT_SETTING_HS:
            strs.append("HS")
        if settings & bluetooth_socket.MGMT_SETTING_LE:
            strs.append("LE")
        logging.debug(msg + ': %s', " ".join(strs))

    def _log_flags(self, msg, flags):
        strs = []
        if flags & bluetooth_socket.HCI_UP:
            strs.append("UP")
        else:
            strs.append("DOWN")
        if flags & bluetooth_socket.HCI_INIT:
            strs.append("INIT")
        if flags & bluetooth_socket.HCI_RUNNING:
            strs.append("RUNNING")
        if flags & bluetooth_socket.HCI_PSCAN:
            strs.append("PSCAN")
        if flags & bluetooth_socket.HCI_ISCAN:
            strs.append("ISCAN")
        if flags & bluetooth_socket.HCI_AUTH:
            strs.append("AUTH")
        if flags & bluetooth_socket.HCI_ENCRYPT:
            strs.append("ENCRYPT")
        if flags & bluetooth_socket.HCI_INQUIRY:
            strs.append("INQUIRY")
        if flags & bluetooth_socket.HCI_RAW:
            strs.append("RAW")
        logging.debug(msg + ' [HCI]: %s', " ".join(strs))

    def run_once(self):
        # Reset the adapter to the powered off state.
        if not self.device.reset_off():
            raise error.TestFail('DUT could not be reset to initial state')

        # Kernel default state depends on whether the kernel supports the
        # BR/EDR Whitelist. When this is supported the 'connectable' setting
        # remains unset and instead page scan is managed by the kernel based
        # on whether or not a BR/EDR device is in the whitelist.
        ( commands, events ) = self.device.read_supported_commands()
        supports_add_device = bluetooth_socket.MGMT_OP_ADD_DEVICE in commands

        # Read the initial state of the adapter. Verify that it is powered down.
        ( address, bluetooth_version, manufacturer_id,
                supported_settings, current_settings, class_of_device,
                name, short_name ) = self.device.read_info()
        self._log_settings('Initial state', current_settings)

        if current_settings & bluetooth_socket.MGMT_SETTING_POWERED:
            raise error.TestFail('Bluetooth adapter is powered')

        # The other kernel settings (connectable, pairable, etc.) reflect the
        # initial state before the bluetooth daemon adjusts them - we're ok
        # with them being on or off during that brief period.
        #
        # Except for discoverable - that one should be off.
        if current_settings & bluetooth_socket.MGMT_SETTING_DISCOVERABLE:
            raise error.TestFail('Bluetooth adapter would be discoverable '
                                 'during power on')

        # Verify that the Bluetooth Daemon sees that it is also powered down,
        # non-discoverable and not discovering devices.
        bluez_properties = self.device.get_adapter_properties()

        if bluez_properties['Powered']:
            raise error.TestFail('Bluetooth daemon Powered property does not '
                                 'match kernel while powered off')
        if bluez_properties['Discoverable']:
            raise error.TestFail('Bluetooth daemon Discoverable property '
                                 'does not match kernel while powered off')
        if bluez_properties['Discovering']:
            raise error.TestFail('Bluetooth daemon believes adapter is '
                                 'discovering while powered off')

        # Compare with the raw HCI state of the adapter as well, this should
        # be just not "UP", otherwise something deeply screwy is happening.
        flags = self.device.get_dev_info()[3]
        self._log_flags('Initial state', flags)

        if flags & bluetooth_socket.HCI_UP:
            raise error.TestFail('HCI UP flag does not match kernel while '
                                 'powered off')

        # Power on the adapter, then read the state again. Verify that it is
        # powered up, pairable, but not discoverable.
        self.device.set_powered(True)
        current_settings = self.device.read_info()[4]
        self._log_settings("Powered up", current_settings)

        if not current_settings & bluetooth_socket.MGMT_SETTING_POWERED:
            raise error.TestFail('Bluetooth adapter is not powered')
        if not current_settings & bluetooth_socket.MGMT_SETTING_PAIRABLE:
            raise error.TestFail('Bluetooth adapter is not pairable')

        if current_settings & bluetooth_socket.MGMT_SETTING_DISCOVERABLE:
            raise error.TestFail('Bluetooth adapter is discoverable')

        # If the kernel supports the BR/EDR whitelist, the adapter should _not_
        # be generically connectable; if it doesn't, it should be.
        if supports_add_device:
            if current_settings & bluetooth_socket.MGMT_SETTING_CONNECTABLE:
                raise error.TestFail('Bluetooth adapter is connectable')
        elif not current_settings & bluetooth_socket.MGMT_SETTING_CONNECTABLE:
            raise error.TestFail('Bluetooth adapter is not connectable')

        # Verify that the Bluetooth Daemon sees the same state as the kernel
        # and that it's not discovering.
        bluez_properties = self.device.get_adapter_properties()

        if not bluez_properties['Powered']:
            raise error.TestFail('Bluetooth daemon Powered property does not '
                                 'match kernel while powered on')
        if not bluez_properties['Pairable']:
            raise error.TestFail('Bluetooth daemon Pairable property does not '
                                 'match kernel while powered on')

        if bluez_properties['Discoverable']:
            raise error.TestFail('Bluetooth daemon Discoverable property '
                                 'does not match kernel while powered on')
        if bluez_properties['Discovering']:
            raise error.TestFail('Bluetooth daemon believes adapter is '
                                 'discovering while powered on')

        # Compare with the raw HCI state of the adapter while powered up as
        # well.
        flags = self.device.get_dev_info()[3]
        self._log_flags('Powered up', flags)

        if not flags & bluetooth_socket.HCI_UP:
            raise error.TestFail('HCI UP flag does not match kernel while '
                                 'powered on')
        if not flags & bluetooth_socket.HCI_RUNNING:
            raise error.TestFail('HCI RUNNING flag does not match kernel while '
                                 'powered on')
        if flags & bluetooth_socket.HCI_ISCAN:
            raise error.TestFail('HCI ISCAN flag does not match kernel while '
                                 'powered on')
        if flags & bluetooth_socket.HCI_INQUIRY:
            raise error.TestFail('HCI INQUIRY flag does not match kernel while '
                                 'powered on')

        # If the kernel supports the BR/EDR whitelist, the adapter isn't
        # supposed to be generically connectable, so should _not_ be in PSCAN
        # mode yet. If it doesn't, it should be. This matches the management
        # API "connectable" setting so far.
        if supports_add_device:
            if flags & bluetooth_socket.HCI_PSCAN:
                raise error.TestFail('HCI PSCAN flag does not match kernel '
                                     'while powered on')
        elif not flags & bluetooth_socket.HCI_PSCAN:
                raise error.TestFail('HCI PSCAN flag does not match kernel '
                                     'while powered on')

        # Now we can examine the differences. Try adding and removing a device
        # from the kernel BR/EDR whitelist. The management API "connectable"
        # setting should remain off, but we should be able to see the PSCAN
        # flag come and go.
        if supports_add_device:
            previous_settings = current_settings
            previous_flags = flags

            self.device.add_device('01:02:03:04:05:06', 0, 1)

            current_settings = self.device.read_info()[4]
            self._log_settings("After add device", current_settings)

            flags = self.device.get_dev_info()[3]
            self._log_flags('After add device', flags)

            if current_settings != previous_settings:
                raise error.TestFail(
                    'Bluetooth adapter settings changed after add device')
            if not flags & bluetooth_socket.HCI_PSCAN:
                raise error.TestFail('HCI PSCAN flag not set after add device')

            # Remove the device again, and make sure the PSCAN flag goes away.
            self.device.remove_device('01:02:03:04:05:06', 0)

            current_settings = self.device.read_info()[4]
            self._log_settings("After remove device", current_settings)

            flags = self.device.get_dev_info()[3]
            self._log_flags('After remove device', flags)

            if current_settings != previous_settings:
                raise error.TestFail(
                    'Bluetooth adapter settings changed after remove device')
            if flags & bluetooth_socket.HCI_PSCAN:
                raise error.TestFail('HCI PSCAN flag set after add device')

        # Finally power off the adapter again, and verify that the adapter has
        # returned to powered down.
        self.device.set_powered(False)
        current_settings = self.device.read_info()[4]
        self._log_settings("After power down", current_settings)

        if current_settings & bluetooth_socket.MGMT_SETTING_POWERED:
            raise error.TestFail('Bluetooth adapter is powered after power off')

        if current_settings & bluetooth_socket.MGMT_SETTING_DISCOVERABLE:
            raise error.TestFail('Bluetooth adapter would be discoverable '
                                 'during next power on')

        # Verify that the Bluetooth Daemon sees the same state as the kernel.
        bluez_properties = self.device.get_adapter_properties()

        if bluez_properties['Powered']:
            raise error.TestFail('Bluetooth daemon Powered property does not '
                                 'match kernel after power off')
        if bluez_properties['Discoverable']:
            raise error.TestFail('Bluetooth daemon Discoverable property '
                                 'does not match kernel after off')
        if bluez_properties['Discovering']:
            raise error.TestFail('Bluetooth daemon believes adapter is '
                                 'discovering after power off')

        # And one last comparison with the raw HCI state of the adapter.
        flags = self.device.get_dev_info()[3]
        self._log_flags('After power down', flags)

        if flags & bluetooth_socket.HCI_UP:
            raise error.TestFail('HCI UP flag does not match kernel after '
                                 'power off')

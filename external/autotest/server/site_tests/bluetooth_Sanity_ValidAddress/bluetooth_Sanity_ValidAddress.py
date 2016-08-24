# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_test


class bluetooth_Sanity_ValidAddress(bluetooth_test.BluetoothTest):
    """
    Verify that the client Bluetooth adapter has a valid address.
    """
    version = 1

    def run_once(self):
        # Reset the adapter to the powered off state.
        if not self.device.reset_off():
            raise error.TestFail('DUT could not be reset to initial state')

        # Read the address both via BlueZ and via the kernel mgmt_ops interface.
        # Compare the two, they should not differ.
        bluez_properties = self.device.get_adapter_properties()
        controller_info = self.device.read_info()

        if bluez_properties['Address'] != controller_info[0]:
            raise error.TestFail(
                    'BlueZ and Kernel adapter address differ: %s != %s' %
                    (bluez_properties['Address'], controller_info[0]))

        address = controller_info[0]
        logging.debug('Bluetooth address of adapter is %s', address)

        # Sanity check the address
        if address == '00:00:00:00:00:00':
            raise error.TestFail('Adapter address is all zeros')
        if address.startswith('00:00:00:'):
            raise error.TestFail('Vendor portion of address is all zeros')
        if address.endswith(':00:00:00'):
            raise error.TestFail('Device portion of address is all zeros')

        if address == 'FF:FF:FF:FF:FF:FF':
            raise error.TestFail('Adapter address is all ones')
        if address.startswith('FF:FF:FF:'):
            raise error.TestFail('Vendor portion of address is all ones')
        if address.endswith(':FF:FF:FF'):
            raise error.TestFail('Device portion of address is all ones')

        # Verify that the address is still the same after powering on the radio.
        self.device.set_powered(True)
        bluez_properties = self.device.get_adapter_properties()
        controller_info = self.device.read_info()

        if bluez_properties['Address'] != address:
            raise error.TestFail(
                    'BlueZ adapter address changed after power on: %s != %s' %
                    (bluez_properties['Address'], address))
        if controller_info[0] != address:
            raise error.TestFail(
                    'Kernel adapter address changed after power on: %s != %s' %
                    (controller_info[0], address))

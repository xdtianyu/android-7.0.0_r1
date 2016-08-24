# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import virtual_ethernet_pair
from autotest_lib.client.cros import hostapd_server
from autotest_lib.client.cros.networking import shill_proxy

class network_8021xEapDetection(test.test):
    """The 802.1x EAP detection class.

    Runs hostapd on one side of an ethernet pair, and shill on the other.
    Ensures that shill detects an EAP request frame sent by hostapd.

    """
    INTERFACE_NAME = 'pseudoethernet0'
    DETECTION_FLAG = 'EapAuthenticatorDetected'
    version = 1

    def get_detection_flag(self, interface_name):
        """Checks the interface for the EAP detection flag.

        @param interface_name: The name of the interface to check

        @return true if the "EAP detected" flag is set, false otherwise.

        """
        device = self._shill_proxy.find_object('Device',
                                               {'Name' : interface_name})
        if device is None:
            raise error.TestFail('Device was not found.')
        device_properties = device.GetProperties(utf8_strings=True)
        logging.info('Device properties are %r', device_properties)
        return self._shill_proxy.dbus2primitive(
                device_properties[self.DETECTION_FLAG])


    def run_once(self):
        """Test main loop."""
        self._shill_proxy = shill_proxy.ShillProxy()
        with virtual_ethernet_pair.VirtualEthernetPair(
                peer_interface_name=self.INTERFACE_NAME,
                peer_interface_ip=None) as ethernet_pair:
            if not ethernet_pair.is_healthy:
                raise error.TestFail('Could not create virtual ethernet pair.')

            if self.get_detection_flag(self.INTERFACE_NAME):
                raise error.TestFail('EAP detection flag is already set.')

            with hostapd_server.HostapdServer(
                    interface=ethernet_pair.interface_name) as hostapd:
                # Wait for hostapd to initialize.
                time.sleep(1)
                if not hostapd.running():
                    raise error.TestFail('hostapd process exited.')

                hostapd.send_eap_packets()
                if not self.get_detection_flag(self.INTERFACE_NAME):
                    raise error.TestFail('EAP detection flag is not set.')

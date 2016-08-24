# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a USB type C USB3 probing test using Plankton board."""

import logging
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_TypeCProbeUSB3(FirmwareTest):
    """USB type C USB3 probing test."""
    version = 1

    RE_SUPERSPEED_DEVICE = r' Port \d+: .+, 5000M$'
    CHARGING_VOLTAGE = 12
    SINK = 0
    POLL_USB3_SECS = 10


    def get_usb_devices_and_buses(self):
        """Gets set from the list of USB devices and buses."""
        result = self.plankton_host.run_short('lsusb -t')
        return set(result.stdout.splitlines())


    def is_superspeed(self, lsusb_line):
        """Checks if the lsusb output contains 5000M."""
        return re.search(self.RE_SUPERSPEED_DEVICE, lsusb_line)


    def run_once(self):
        """Checks DUT USB3 device.

        @raise TestFail: If USB3 can't be found when switch to USB mode.
        """
        # Enumerate USB devices when charging.
        self.plankton.charge(self.CHARGING_VOLTAGE)
        devices_charging = self.get_usb_devices_and_buses()
        logging.info('number of devices while charging: %d',
                     len(devices_charging))
        # Enumerate USB devices when switching to DP.
        self.plankton.set_usbc_mux('dp')
        self.plankton.charge(self.SINK)
        self.plankton.poll_pd_state('sink')
        devices_dp = self.get_usb_devices_and_buses()
        self.plankton.charge(self.CHARGING_VOLTAGE)
        logging.info('number of devices when switch to dp: %d',
                     len(devices_dp))
        # Enumerate USB devices when switching to USB3
        self.plankton.set_usbc_mux('usb')
        self.plankton.charge(self.SINK)
        self.plankton.poll_pd_state('sink')
        # It takes a short period for DUT to get USB3.0 device.
        utils.poll_for_condition(
            lambda: len(self.get_usb_devices_and_buses()) > len(devices_dp),
            exception=error.TestFail(
                    'Can\'t find new device when switching to USB '
                    'after %d seconds' % self.POLL_USB3_SECS),
            timeout=self.POLL_USB3_SECS)
        devices_usb = self.get_usb_devices_and_buses()
        self.plankton.charge(self.CHARGING_VOLTAGE)
        logging.info('number of devices when switch to usb: %d',
                     len(devices_usb))

        # For Samus USB2.0 signal power swap while usbc_role changes due to
        # lack of UFP. Port number of lsusb may change since the device is
        # disconnected for a short period.
        if len(devices_dp) != len(devices_charging):
            raise error.TestFail(
                    'Number of USB devices changed on switching to DP')

        if not any(map(self.is_superspeed, devices_usb - devices_dp)):
            raise error.TestFail('Can\'t find new USB3 device')

#!/usr/bin/env python3.4
#
# Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_atten_utils import set_rssi
from acts.test_utils.tel.tel_defines import MAX_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import MIN_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection

# Attenuator name
ATTEN_NAME_FOR_WIFI = 'wifi0'
ATTEN_NAME_FOR_CELL = 'cell0'


class TelWifiDataTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = ("test_wifi_cell_switching_stress", )
        self.stress_test_number = int(self.user_params["stress_test_number"])
        self.live_network_ssid = self.user_params["wifi_network_ssid"]
        try:
            self.live_network_pwd = self.user_params["wifi_network_pass"]
        except KeyError:
            self.live_network_pwd = None

        self.attens = {}
        for atten in self.attenuators:
            self.attens[atten.path] = atten

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_cell_switching_stress(self):
        """Test for data switch between WiFi and Cell. DUT go in and out WiFi
        coverage for multiple times.

        Steps:
        1. Set WiFi and Cellular signal to good (attenuation value to MIN).
        2. Make sure DUT get Cell data coverage (LTE) and WiFi connected.
        3. Set WiFi RSSI to MAX (WiFi attenuator value to MIN).
        4. Verify DUT report WiFi connected and Internet access OK.
        5. Set WiFi RSSI to MIN (WiFi attenuator value to MAX).
        6. Verify DUT report Cellular Data connected and Internet access OK.
        7. Repeat Step 3~6 for stress number.

        Expected Results:
        4. DUT report WiFi connected and Internet access OK.
        6. DUT report Cellular Data connected and Internet access OK.
        7. Stress test should pass.

        Returns:
        True if Pass. False if fail.
        """
        WIFI_RSSI_CHANGE_STEP_SIZE = 2
        WIFI_RSSI_CHANGE_DELAY_PER_STEP = 1
        # Set Attenuator Value for WiFi and Cell to 0.
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL], 0,
                 MAX_RSSI_RESERVED_VALUE)

        # Make sure DUT get Cell Data coverage (LTE).
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         GEN_4G, NETWORK_SERVICE_DATA):
            return False

        # Make sure DUT WiFi is connected.
        if not ensure_wifi_connected(self.log, self.android_devices[0],
                                     self.live_network_ssid,
                                     self.live_network_pwd):
            self.log.error("{} connect WiFI failed".format(
                self.android_devices[0].serial))
            return False

        total_iteration = self.stress_test_number
        self.log.info("Stress test. Total iteration = {}.".format(
            total_iteration))
        current_iteration = 1
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = {}/{}----<".format(
                current_iteration, total_iteration))

            # Set WiFi RSSI to MAX.
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI], 0,
                     MAX_RSSI_RESERVED_VALUE, WIFI_RSSI_CHANGE_STEP_SIZE,
                     WIFI_RSSI_CHANGE_DELAY_PER_STEP)
            # Wait for DUT report WiFi connected and Internet access OK.
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("Data not on WiFi")
                break

            # Set WiFi RSSI to MIN (DUT lose WiFi coverage).
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI], 0,
                     MIN_RSSI_RESERVED_VALUE, WIFI_RSSI_CHANGE_STEP_SIZE,
                     WIFI_RSSI_CHANGE_DELAY_PER_STEP)
            # Wait for DUT report Cellular Data connected and Internet access OK.
            if (not wait_for_cell_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("Data not on Cell")
                break

            self.log.info(">----Iteration : {}/{} succeed.----<".format(
                current_iteration, total_iteration))
            current_iteration += 1
        if current_iteration <= total_iteration:
            self.log.info(">----Iteration : {}/{} failed.----<".format(
                current_iteration, total_iteration))
            return False
        else:
            return True


if __name__ == "__main__":
    raise Exception("Cannot run this class directly")

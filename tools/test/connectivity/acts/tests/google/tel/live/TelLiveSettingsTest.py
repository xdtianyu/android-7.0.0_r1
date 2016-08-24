#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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
"""
    Test Script for Telephony Settings
"""

import time
from acts.utils import load_config
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WIFI_CONNECTION
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_IMS_REGISTRATION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOLTE_ENABLED
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_ENABLED
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WLAN
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import is_droid_in_rat_family
from acts.test_utils.tel.tel_test_utils import is_wfc_enabled
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_ims_registered
from acts.test_utils.tel.tel_test_utils import wait_for_network_rat
from acts.test_utils.tel.tel_test_utils import wait_for_not_network_rat
from acts.test_utils.tel.tel_test_utils import wait_for_volte_enabled
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_disabled
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_enabled
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import WifiUtils
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan

class TelLiveSettingsTest(TelephonyBaseTest):

    _TEAR_DOWN_OPERATION_DISCONNECT_WIFI = "disconnect_wifi"
    _TEAR_DOWN_OPERATION_RESET_WIFI = "reset_wifi"
    _TEAR_DOWN_OPERATION_DISABLE_WFC = "disable_wfc"
    _DEFAULT_STRESS_NUMBER = 5

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            "test_lte_volte_wifi_connected_toggle_wfc",
            "test_lte_wifi_connected_toggle_wfc",
            "test_3g_wifi_connected_toggle_wfc",
            "test_apm_wifi_connected_toggle_wfc",

            "test_lte_volte_wfc_enabled_toggle_wifi",
            "test_lte_wfc_enabled_toggle_wifi",
            "test_3g_wfc_enabled_toggle_wifi",
            "test_apm_wfc_enabled_toggle_wifi",

            "test_lte_wfc_enabled_wifi_connected_toggle_volte",

            "test_lte_volte_wfc_wifi_preferred_to_cellular_preferred",
            "test_lte_wfc_wifi_preferred_to_cellular_preferred",
            "test_3g_wfc_wifi_preferred_to_cellular_preferred",
            "test_apm_wfc_wifi_preferred_to_cellular_preferred",
            "test_lte_volte_wfc_cellular_preferred_to_wifi_preferred",
            "test_lte_wfc_cellular_preferred_to_wifi_preferred",
            "test_3g_wfc_cellular_preferred_to_wifi_preferred",
            "test_apm_wfc_cellular_preferred_to_wifi_preferred",

            "test_apm_wfc_wifi_preferred_turn_off_apm",
            "test_apm_wfc_cellular_preferred_turn_off_apm",

            "test_wfc_setup_timing",
            "test_lte_volte_wfc_enabled_toggle_wifi_stress",
            "test_lte_volte_wfc_enabled_reset_wifi_stress",
            "test_lte_volte_wfc_wifi_preferred_to_cellular_preferred_stress"

        )
        self.ad = self.android_devices[0]
        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]
        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None
        try:
            self.stress_test_number = int(self.user_params["stress_test_number"])
        except KeyError:
            self.stress_test_number = self._DEFAULT_STRESS_NUMBER

    def _wifi_connected_enable_wfc_teardown_wfc(self,
        tear_down_operation, initial_setup_wifi=True,
        initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED,
        check_volte_after_wfc_disabled=False):
        if initial_setup_wifi and not ensure_wifi_connected(
            self.log, self.ad, self.wifi_network_ssid, self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False
        if initial_setup_wfc_mode and not set_wfc_mode(
            self.log, self.ad, initial_setup_wfc_mode):
            self.log.error("Failed to set WFC mode.")
            return False
        if not phone_idle_iwlan(self.log, self.ad):
            self.log.error("WFC is not available.")
            return False

        # Tear Down WFC based on tear_down_operation
        if tear_down_operation == self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI:
            if not WifiUtils.wifi_toggle_state(self.log, self.ad, False):
                self.log.error("Failed to turn off WiFi.")
                return False
        elif tear_down_operation == self._TEAR_DOWN_OPERATION_RESET_WIFI:
            if not WifiUtils.wifi_reset(self.log, self.ad, False):
                self.log.error("Failed to reset WiFi")
                return False
        elif tear_down_operation == self._TEAR_DOWN_OPERATION_DISABLE_WFC:
            if not set_wfc_mode(self.log, self.ad, WFC_MODE_DISABLED):
                self.log.error("Failed to turn off WFC.")
                return False
        else:
            self.log.error("No tear down operation")
            return False

        if not wait_for_not_network_rat(self.log, self.ad, RAT_FAMILY_WLAN,
            voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("Data Rat is still iwlan.")
            return False
        if not wait_for_wfc_disabled(self.log, self.ad):
            self.log.error("WFC is still available after turn off WFC.")
            return False

        # If VoLTE was previous available, after tear down WFC, DUT should have
        # VoLTE service.
        if check_volte_after_wfc_disabled and not wait_for_volte_enabled(
            self.log, self.ad, MAX_WAIT_TIME_VOLTE_ENABLED):
            self.log.error("Device failed to acquire VoLTE service")
            return False
        return True

    def _wifi_connected_set_wfc_mode_change_wfc_mode(self,
        initial_wfc_mode,
        new_wfc_mode,
        is_wfc_available_in_initial_wfc_mode,
        is_wfc_available_in_new_wfc_mode,
        initial_setup_wifi=True,
        check_volte_after_wfc_disabled=False):
        if initial_setup_wifi and not ensure_wifi_connected(
            self.log, self.ad, self.wifi_network_ssid, self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False
        # Set to initial_wfc_mode first, then change to new_wfc_mode
        for (wfc_mode, is_wfc_available) in \
            [(initial_wfc_mode, is_wfc_available_in_initial_wfc_mode),
             (new_wfc_mode, is_wfc_available_in_new_wfc_mode)]:
            current_wfc_status = is_wfc_enabled(self.log, self.ad)
            self.log.info("Current WFC: {}, Set WFC to {}".
                format(current_wfc_status, wfc_mode))
            if not set_wfc_mode(self.log, self.ad, wfc_mode):
                self.log.error("Failed to set WFC mode.")
                return False
            if is_wfc_available:
                if current_wfc_status:
                    # Previous is True, after set it still need to be true
                    # wait and check if DUT WFC got disabled.
                    if wait_for_wfc_disabled(self.log, self.ad):
                        self.log.error("WFC is not available.")
                        return False
                else:
                    # Previous is False, after set it will be true,
                    # wait and check if DUT WFC got enabled.
                    if not wait_for_wfc_enabled(self.log, self.ad):
                        self.log.error("WFC is not available.")
                        return False
            else:
                if current_wfc_status:
                    # Previous is True, after set it will be false,
                    # wait and check if DUT WFC got disabled.
                    if not wait_for_wfc_disabled(self.log, self.ad):
                        self.log.error("WFC is available.")
                        return False
                else:
                    # Previous is False, after set it still need to be false
                    # Wait and check if DUT WFC got enabled.
                    if wait_for_wfc_enabled(self.log, self.ad):
                        self.log.error("WFC is available.")
                        return False
                if check_volte_after_wfc_disabled and not wait_for_volte_enabled(
                    self.log, self.ad, MAX_WAIT_TIME_VOLTE_ENABLED):
                    self.log.error("Device failed to acquire VoLTE service")
                    return False
        return True

    def _wifi_connected_set_wfc_mode_turn_off_apm(self, wfc_mode,
        is_wfc_available_after_turn_off_apm):
        if not ensure_wifi_connected(self.log, self.ad, self.wifi_network_ssid,
                                     self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False
        if not set_wfc_mode(self.log, self.ad, wfc_mode):
            self.log.error("Failed to set WFC mode.")
            return False
        if not phone_idle_iwlan(self.log, self.ad):
            self.log.error("WFC is not available.")
            return False
        if not toggle_airplane_mode(self.log, self.ad, False):
            self.log.error("Failed to turn off airplane mode")
            return False
        is_wfc_not_available = wait_for_wfc_disabled(self.log, self.ad)
        if is_wfc_available_after_turn_off_apm and is_wfc_not_available:
                self.log.error("WFC is not available.")
                return False
        elif (not is_wfc_available_after_turn_off_apm and
              not is_wfc_not_available):
                self.log.error("WFC is available.")
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Disabled + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_apm_wifi_connected_toggle_wfc(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected, Toggling WFC

        Steps:
        1. Setup DUT Idle, Airplane mode.
        2. Make sure DUT WiFi connected, WFC disabled.
        3. Set DUT WFC enabled (WiFi Preferred), verify DUT WFC available,
            report iwlan rat.
        4. Set DUT WFC disabled, verify DUT WFC unavailable,
            not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISABLE_WFC,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Disabled + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        3G + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_apm_wfc_enabled_toggle_wifi(self):
        """Test for WiFi Calling settings:
        APM + WFC enabled, Toggling WiFi

        Steps:
        1. Setup DUT Idle, Airplane mode.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi,verify DUT WFC unavailable, not report iwlan rat.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        """

        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_enable_wfc_teardown_wfc(
            tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
            initial_setup_wifi=True,
            initial_setup_wfc_mode=WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_wfc_enabled_wifi_connected_toggle_volte(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + WFC enabled, toggle VoLTE setting

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC enabled (WiFi Preferred).
            Verify DUT WFC available, report iwlan rat.
        3. Disable VoLTE on DUT, verify in 2 minutes period,
            DUT does not lost WiFi Calling, DUT still report WFC available,
            rat iwlan.
        4. Enable VoLTE on DUT, verify in 2 minutes period,
            DUT does not lost WiFi Calling, DUT still report WFC available,
            rat iwlan.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        if not phone_setup_iwlan(self.log, self.ad, False,
                                 WFC_MODE_WIFI_PREFERRED,
                                 self.wifi_network_ssid,
                                 self.wifi_network_pass):
            self.log.error("Failed to setup WFC.")
            return False
        # Turn Off VoLTE, then Turn On VoLTE
        for i in range(2):
            if not toggle_volte(self.log, self.ad):
                self.log.error("Failed to toggle VoLTE.")
                return False
            if wait_for_wfc_disabled(self.log, self.ad):
                self.log.error("WFC is not available.")
                return False
            if not is_droid_in_rat_family(self.log, self.ad, RAT_FAMILY_WLAN,
                                          NETWORK_SERVICE_DATA):
                self.log.error("Data Rat is not iwlan.")
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + WiFi Preferred,
        change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED,
            True, False,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        LTE + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED,
            True, False,
            check_volte_after_wfc_disabled=False)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED,
            True, False)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED,
            True, False)

    @TelephonyBaseTest.tel_test_wrap
    def test_apm_wfc_wifi_preferred_to_cellular_preferred(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + WiFi Preferred, change WFC to Cellular Preferred

        Steps:
        1. Setup DUT Idle, airplane mode.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED,
            True, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + Cellular Preferred,
        change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED, WFC_MODE_WIFI_PREFERRED,
            False, True,
            check_volte_after_wfc_disabled=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        LTE + WiFi Connected + Cellular Preferred, change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE disabled.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_csfb(self.log, self.ad):
            self.log.error("Failed to setup LTE.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED, WFC_MODE_WIFI_PREFERRED,
            False, True,
            check_volte_after_wfc_disabled=False)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        3G + WiFi Connected + Cellular Preferred, change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, 3G network type.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not phone_setup_voice_3g(self.log, self.ad):
            self.log.error("Failed to setup 3G.")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED, WFC_MODE_WIFI_PREFERRED,
            False, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_apm_wfc_cellular_preferred_to_wifi_preferred(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + Cellular Preferred, change WFC to WiFi Preferred

        Steps:
        1. Setup DUT Idle, airplane mode.
        2. Make sure DUT WiFi connected, WFC is set to Cellular Preferred.
            Verify DUT WFC not available.
        3. Change WFC setting to WiFi Preferred.
        4. Verify DUT report WFC available.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_change_wfc_mode(
            WFC_MODE_CELLULAR_PREFERRED, WFC_MODE_WIFI_PREFERRED,
            True, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_apm_wfc_wifi_preferred_turn_off_apm(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + WiFi Preferred + turn off APM

        Steps:
        1. Setup DUT Idle in Airplane mode.
        2. Make sure DUT WiFi connected, set WFC mode to WiFi preferred.
        3. verify DUT WFC available, report iwlan rat.
        4. Turn off airplane mode.
        5. Verify DUT WFC still available, report iwlan rat

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        5. DUT WiFI Calling feature bit return True, network rat is iwlan.
        """
        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_turn_off_apm(
            WFC_MODE_WIFI_PREFERRED, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_apm_wfc_cellular_preferred_turn_off_apm(self):
        """Test for WiFi Calling settings:
        APM + WiFi Connected + Cellular Preferred + turn off APM

        Steps:
        1. Setup DUT Idle in Airplane mode.
        2. Make sure DUT WiFi connected, set WFC mode to Cellular preferred.
        3. verify DUT WFC available, report iwlan rat.
        4. Turn off airplane mode.
        5. Verify DUT WFC not available, not report iwlan rat

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        5. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        """
        if not toggle_airplane_mode(self.log, self.ad, True):
            self.log.error("Failed to turn on airplane mode")
            return False
        return self._wifi_connected_set_wfc_mode_turn_off_apm(
            WFC_MODE_CELLULAR_PREFERRED, False)

    @TelephonyBaseTest.tel_test_wrap
    def test_wfc_setup_timing(self):
        """ Measures the time delay in enabling WiFi calling

        Steps:
        1. Make sure DUT idle.
        2. Turn on Airplane Mode, Set WiFi Calling to WiFi_Preferred.
        3. Turn on WiFi, connect to WiFi AP and measure time delay.
        4. Wait for WiFi connected, verify Internet and measure time delay.
        5. Wait for rat to be reported as iwlan and measure time delay.
        6. Wait for ims registered and measure time delay.
        7. Wait for WiFi Calling feature bit to be True and measure time delay.

        Expected results:
        Time Delay in each step should be within pre-defined limit.

        Returns:
            Currently always return True.
        """
        # TODO: b/26338119 Set pass/fail criteria
        ad = self.android_devices[0]

        time_values = {
            'start': 0,
            'wifi_enabled': 0,
            'wifi_connected': 0,
            'wifi_data': 0,
            'iwlan_rat': 0,
            'ims_registered': 0,
            'wfc_enabled': 0,
            'mo_call_success': 0
        }

        WifiUtils.wifi_reset(self.log, ad)
        toggle_airplane_mode(self.log, ad, True)

        set_wfc_mode(self.log, ad, WFC_MODE_WIFI_PREFERRED)

        time_values['start'] = time.time()

        self.log.info("Start Time {}s".format(time_values['start']))

        WifiUtils.wifi_toggle_state(self.log, ad, True)
        time_values['wifi_enabled'] = time.time()
        self.log.info("WiFi Enabled After {}s".format(time_values[
            'wifi_enabled'] - time_values['start']))

        WifiUtils.wifi_connect(self.log, ad, self.wifi_network_ssid,
                               self.wifi_network_pass)

        ad.droid.wakeUpNow()

        if not wait_for_wifi_data_connection(self.log, ad, True,
                                             MAX_WAIT_TIME_WIFI_CONNECTION):
            self.log.error("Failed WiFi connection, aborting!")
            return False
        time_values['wifi_connected'] = time.time()

        self.log.info("WiFi Connected After {}s".format(time_values[
            'wifi_connected'] - time_values['wifi_enabled']))

        if not verify_http_connection(self.log, ad, 'http://www.google.com',
                                      100, .1):
            self.log.error("Failed to get user-plane traffic, aborting!")
            return False

        time_values['wifi_data'] = time.time()
        self.log.info("WifiData After {}s".format(time_values[
            'wifi_data'] - time_values['wifi_connected']))

        if not wait_for_network_rat(self.log,
                                    ad,
                                    RAT_FAMILY_WLAN,
                                    voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("Failed to set-up iwlan, aborting!")
            if is_droid_in_rat_family(self.log, ad, RAT_FAMILY_WLAN,
                                      NETWORK_SERVICE_DATA):
                self.log.error("Never received the event, but droid in iwlan")
            else:
                return False
        time_values['iwlan_rat'] = time.time()
        self.log.info("iWLAN Reported After {}s".format(time_values[
            'iwlan_rat'] - time_values['wifi_data']))

        if not wait_for_ims_registered(self.log, ad,
                                       MAX_WAIT_TIME_IMS_REGISTRATION):
            self.log.error("Never received IMS registered, aborting")
            return False
        time_values['ims_registered'] = time.time()
        self.log.info("Ims Registered After {}s".format(time_values[
            'ims_registered'] - time_values['iwlan_rat']))

        if not wait_for_wfc_enabled(self.log, ad, MAX_WAIT_TIME_WFC_ENABLED):
            self.log.error("Never received WFC feature, aborting")
            return False

        time_values['wfc_enabled'] = time.time()
        self.log.info("Wifi Calling Feature Enabled After {}s".format(
            time_values['wfc_enabled'] - time_values['ims_registered']))

        set_wfc_mode(self.log, ad, WFC_MODE_DISABLED)

        wait_for_not_network_rat(self.log,
                                 ad,
                                 RAT_FAMILY_WLAN,
                                 voice_or_data=NETWORK_SERVICE_DATA)

        self.log.info("\n\n------------------summary-----------------")
        self.log.info("WiFi Enabled After {0:.2f} s".format(time_values[
            'wifi_enabled'] - time_values['start']))
        self.log.info("WiFi Connected After {0:.2f} s".format(time_values[
            'wifi_connected'] - time_values['wifi_enabled']))
        self.log.info("WifiData After {0:.2f} s".format(time_values[
            'wifi_data'] - time_values['wifi_connected']))
        self.log.info("iWLAN Reported After {0:.2f} s".format(time_values[
            'iwlan_rat'] - time_values['wifi_data']))
        self.log.info("Ims Registered After {0:.2f} s".format(time_values[
            'ims_registered'] - time_values['iwlan_rat']))
        self.log.info("Wifi Calling Feature Enabled After {0:.2f} s".format(
            time_values['wfc_enabled'] - time_values['ims_registered']))
        self.log.info("\n\n")
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wfc_enabled_toggle_wifi_stress(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WFC enabled, Toggling WiFi Stress test

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT disconnect WiFi, verify DUT WFC unavailable, not report iwlan rat.
        5. Verify DUT report VoLTE available.
        6. Repeat steps 3~5 for N times.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        5. DUT report VoLTE available.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED)

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Start Iteration {}.".format(i))
            result = self._wifi_connected_enable_wfc_teardown_wfc(
                tear_down_operation=self._TEAR_DOWN_OPERATION_DISCONNECT_WIFI,
                initial_setup_wifi=True,
                initial_setup_wfc_mode=None,
                check_volte_after_wfc_disabled=True)
            if not result:
                self.log.error("Test Failed in iteration: {}.".format(i))
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wfc_enabled_reset_wifi_stress(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WFC enabled, Reset WiFi Stress test

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi disconnected, WFC enabled (WiFi Preferred).
        3. DUT connect WiFi, verify DUT WFC available, report iwlan rat.
        4. DUT Reset WiFi, verify DUT WFC unavailable, not report iwlan rat.
        5. Verify DUT report VoLTE available.
        6. Repeat steps 3~5 for N times.

        Expected Results:
        3. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFi Calling feature bit return False, network rat is not iwlan.
        5. DUT report VoLTE available.
        """

        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE")
            return False
        set_wfc_mode(self.log, self.ad, WFC_MODE_WIFI_PREFERRED)

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Start Iteration {}.".format(i))
            result = self._wifi_connected_enable_wfc_teardown_wfc(
                tear_down_operation=self._TEAR_DOWN_OPERATION_RESET_WIFI,
                initial_setup_wifi=True,
                initial_setup_wfc_mode=None,
                check_volte_after_wfc_disabled=True)
            if not result:
                self.log.error("Test Failed in iteration: {}.".format(i))
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_volte_wfc_wifi_preferred_to_cellular_preferred_stress(self):
        """Test for WiFi Calling settings:
        LTE + VoLTE Enabled + WiFi Connected + WiFi Preferred,
        change WFC to Cellular Preferred stress

        Steps:
        1. Setup DUT Idle, LTE network type, VoLTE enabled.
        2. Make sure DUT WiFi connected, WFC is set to WiFi Preferred.
            Verify DUT WFC available, report iwlan rat.
        3. Change WFC setting to Cellular Preferred.
        4. Verify DUT report WFC not available.
        5. Verify DUT report VoLTE available.
        6. Repeat steps 3~5 for N times.

        Expected Results:
        2. DUT WiFi Calling feature bit return True, network rat is iwlan.
        4. DUT WiFI Calling feature bit return False, network rat is not iwlan.
        5. DUT report VoLTE available.
        """
        if not phone_setup_volte(self.log, self.ad):
            self.log.error("Failed to setup VoLTE.")
            return False
        if not ensure_wifi_connected(
            self.log, self.ad, self.wifi_network_ssid, self.wifi_network_pass):
            self.log.error("Failed to connect WiFi")
            return False

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Start Iteration {}.".format(i))
            result = self._wifi_connected_set_wfc_mode_change_wfc_mode(
                WFC_MODE_WIFI_PREFERRED, WFC_MODE_CELLULAR_PREFERRED,
                True, False,
                initial_setup_wifi=False,
                check_volte_after_wfc_disabled=True)
            if not result:
                self.log.error("Test Failed in iteration: {}.".format(i))
                return False
        return True

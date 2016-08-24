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
    Test Script for Telephony Pre Check In Sanity
"""

import time
from acts.base_test import BaseTestClass
from queue import Empty
from acts.test_utils.tel.tel_subscription_utils import \
    get_subid_from_slot_index
from acts.test_utils.tel.tel_subscription_utils import set_subid_for_data
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_ORIGINATED
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_TERMINATED
from acts.test_utils.tel.tel_defines import DATA_STATE_CONNECTED
from acts.test_utils.tel.tel_defines import GEN_2G
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import RAT_2G
from acts.test_utils.tel.tel_defines import RAT_3G
from acts.test_utils.tel.tel_defines import RAT_4G
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import SIM1_SLOT_INDEX
from acts.test_utils.tel.tel_defines import SIM2_SLOT_INDEX
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WIFI_CONNECTION
from acts.test_utils.tel.tel_defines import TETHERING_MODE_WIFI
from acts.test_utils.tel.tel_defines import WAIT_TIME_AFTER_REBOOT
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_BETWEEN_REG_AND_CALL
from acts.test_utils.tel.tel_defines import \
    WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING
from acts.test_utils.tel.tel_defines import WAIT_TIME_TETHERING_AFTER_REBOOT
from acts.test_utils.tel.tel_data_utils import airplane_mode_test
from acts.test_utils.tel.tel_data_utils import change_data_sim_and_verify_data
from acts.test_utils.tel.tel_data_utils import data_connectivity_single_bearer
from acts.test_utils.tel.tel_data_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_data_utils import tethering_check_internet_connection
from acts.test_utils.tel.tel_data_utils import wifi_cell_switching
from acts.test_utils.tel.tel_data_utils import wifi_tethering_cleanup
from acts.test_utils.tel.tel_data_utils import wifi_tethering_setup_teardown
from acts.test_utils.tel.tel_test_utils import WifiUtils
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import get_slot_index_from_subid
from acts.test_utils.tel.tel_test_utils import get_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import set_call_state_listen_level
from acts.test_utils.tel.tel_test_utils import setup_sim
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_network_rat
from acts.test_utils.tel.tel_test_utils import \
    wait_for_voice_attach_for_subscription
from acts.test_utils.tel.tel_test_utils import \
    wait_for_data_attach_for_subscription
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.wifi.wifi_test_utils import WifiEnums
from acts.utils import disable_doze
from acts.utils import enable_doze
from acts.utils import load_config
from acts.utils import rand_ascii_str


class TelLiveDataTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = ("test_airplane_mode",
                      "test_4g",
                      "test_3g",
                      "test_2g",
                      "test_lte_wifi_switching",
                      "test_wcdma_wifi_switching",
                      "test_gsm_wifi_switching",
                      "test_wifi_connect_disconnect",
                      "test_lte_multi_bearer",
                      "test_wcdma_multi_bearer",
                      "test_2g_wifi_not_associated",
                      "test_3g_wifi_not_associated",
                      "test_4g_wifi_not_associated",

                      # WiFi Tethering tests
                      "test_tethering_entitlement_check",
                      "test_tethering_2g_to_2gwifi",
                      "test_tethering_2g_to_5gwifi",
                      "test_tethering_3g_to_5gwifi",
                      "test_tethering_3g_to_2gwifi",
                      "test_tethering_4g_to_5gwifi",
                      "test_tethering_4g_to_2gwifi",
                      "test_tethering_4g_to_2gwifi_2clients",
                      "test_toggle_apm_during_active_wifi_tethering",
                      "test_toggle_data_during_active_wifi_tethering",
                      "test_disable_wifi_tethering_resume_connected_wifi",
                      "test_tethering_wifi_ssid_quotes",
                      "test_tethering_wifi_no_password",
                      "test_tethering_wifi_password_escaping_characters",
                      "test_tethering_wifi_ssid",
                      "test_tethering_wifi_password",
                      "test_tethering_wifi_volte_call",
                      "test_tethering_wifi_csfb_call",
                      "test_tethering_wifi_3g_call",
                      "test_tethering_wifi_reboot",
                      "test_connect_wifi_start_tethering_wifi_reboot",
                      "test_connect_wifi_reboot_start_tethering_wifi",
                      "test_tethering_wifi_screen_off_enable_doze_mode",

                      # stress tests
                      "test_4g_stress",
                      "test_3g_stress",
                      "test_lte_multi_bearer_stress",
                      "test_wcdma_multi_bearer_stress",
                      "test_tethering_4g_to_2gwifi_stress",)
        self.stress_test_number = int(self.user_params["stress_test_number"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

    @TelephonyBaseTest.tel_test_wrap
    def test_airplane_mode(self):
        """ Test airplane mode basic on Phone and Live SIM.

        Ensure phone attach, data on, WiFi off and verify Internet.
        Turn on airplane mode to make sure detach.
        Turn off airplane mode to make sure attach.
        Verify Internet connection.

        Returns:
            True if pass; False if fail.
        """
        return airplane_mode_test(self.log, self.android_devices[0])

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_wifi_switching(self):
        """Test data connection network switching when phone camped on LTE.

        Ensure phone is camped on LTE
        Ensure WiFi can connect to live network,
        Airplane mode is off, data connection is on, WiFi is on.
        Turn off WiFi, verify data is on cell and browse to google.com is OK.
        Turn on WiFi, verify data is on WiFi and browse to google.com is OK.
        Turn off WiFi, verify data is on cell and browse to google.com is OK.

        Returns:
            True if pass.
        """
        return wifi_cell_switching(self.log, self.android_devices[0],
                                   self.wifi_network_ssid,
                                   self.wifi_network_pass, GEN_4G)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_wifi_switching(self):
        """Test data connection network switching when phone camped on WCDMA.

        Ensure phone is camped on WCDMA
        Ensure WiFi can connect to live network,
        Airplane mode is off, data connection is on, WiFi is on.
        Turn off WiFi, verify data is on cell and browse to google.com is OK.
        Turn on WiFi, verify data is on WiFi and browse to google.com is OK.
        Turn off WiFi, verify data is on cell and browse to google.com is OK.

        Returns:
            True if pass.
        """
        return wifi_cell_switching(self.log, self.android_devices[0],
                                   self.wifi_network_ssid,
                                   self.wifi_network_pass, GEN_3G)

    @TelephonyBaseTest.tel_test_wrap
    def test_gsm_wifi_switching(self):
        """Test data connection network switching when phone camped on GSM.

        Ensure phone is camped on GSM
        Ensure WiFi can connect to live network,,
        Airplane mode is off, data connection is on, WiFi is on.
        Turn off WiFi, verify data is on cell and browse to google.com is OK.
        Turn on WiFi, verify data is on WiFi and browse to google.com is OK.
        Turn off WiFi, verify data is on cell and browse to google.com is OK.

        Returns:
            True if pass.
        """
        return wifi_cell_switching(self.log, self.android_devices[0],
                                   self.wifi_network_ssid,
                                   self.wifi_network_pass, GEN_2G)

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_multi_bearer(self):
        """Test LTE data connection before call and in call. (VoLTE call)


        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in LTE, verify Internet.
        Initiate a voice call. verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.
        Hangup Voice Call, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        if not phone_setup_volte(self.log, self.android_devices[0]):
            self.log.error("Failed to setup VoLTE")
            return False
        return self._test_data_connectivity_multi_bearer(GEN_4G)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_multi_bearer(self):
        """Test WCDMA data connection before call and in call.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in WCDMA, verify Internet.
        Initiate a voice call. verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.
        Hangup Voice Call, verify Internet.

        Returns:
            True if success.
            False if failed.
        """

        return self._test_data_connectivity_multi_bearer(GEN_3G)

    @TelephonyBaseTest.tel_test_wrap
    def test_gsm_multi_bearer_mo(self):
        """Test gsm data connection before call and in call.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in GSM, verify Internet.
        Initiate a MO voice call. Verify there is no Internet during call.
        Hangup Voice Call, verify Internet.

        Returns:
            True if success.
            False if failed.
        """

        return self._test_data_connectivity_multi_bearer(GEN_2G,
            False, DIRECTION_MOBILE_ORIGINATED)

    @TelephonyBaseTest.tel_test_wrap
    def test_gsm_multi_bearer_mt(self):
        """Test gsm data connection before call and in call.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in GSM, verify Internet.
        Initiate a MT voice call. Verify there is no Internet during call.
        Hangup Voice Call, verify Internet.

        Returns:
            True if success.
            False if failed.
        """

        return self._test_data_connectivity_multi_bearer(GEN_2G,
            False, DIRECTION_MOBILE_TERMINATED)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_multi_bearer_stress(self):
        """Stress Test WCDMA data connection before call and in call.

        This is a stress test for "test_wcdma_multi_bearer".
        Default MINIMUM_SUCCESS_RATE is set to 95%.

        Returns:
            True stress pass rate is higher than MINIMUM_SUCCESS_RATE.
            False otherwise.
        """
        ads = self.android_devices
        MINIMUM_SUCCESS_RATE = .95
        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            ensure_phones_default_state(
                self.log, [self.android_devices[0], self.android_devices[1]])

            if self.test_wcdma_multi_bearer():
                success_count += 1
                result_str = "Succeeded"
            else:
                fail_count += 1
                result_str = "Failed"
            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count, str(100 * success_count / (
                success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @TelephonyBaseTest.tel_test_wrap
    def test_lte_multi_bearer_stress(self):
        """Stress Test LTE data connection before call and in call. (VoLTE call)

        This is a stress test for "test_lte_multi_bearer".
        Default MINIMUM_SUCCESS_RATE is set to 95%.

        Returns:
            True stress pass rate is higher than MINIMUM_SUCCESS_RATE.
            False otherwise.
        """
        ads = self.android_devices
        MINIMUM_SUCCESS_RATE = .95
        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            ensure_phones_default_state(
                self.log, [self.android_devices[0], self.android_devices[1]])

            if self.test_lte_multi_bearer():
                success_count += 1
                result_str = "Succeeded"
            else:
                fail_count += 1
                result_str = "Failed"
            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count, str(100 * success_count / (
                success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    def _test_data_connectivity_multi_bearer(self, nw_gen,
        simultaneous_voice_data=True,
        call_direction=DIRECTION_MOBILE_ORIGINATED):
        """Test data connection before call and in call.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Make sure phone in <nw_gen>, verify Internet.
        Initiate a voice call.
        if simultaneous_voice_data is True, then:
            Verify Internet.
            Disable Cellular Data, verify Internet is inaccessible.
            Enable Cellular Data, verify Internet.
        if simultaneous_voice_data is False, then:
            Verify Internet is not available during voice call.
        Hangup Voice Call, verify Internet.

        Returns:
            True if success.
            False if failed.
        """

        class _LocalException(Exception):
            pass

        ad_list = [self.android_devices[0], self.android_devices[1]]
        ensure_phones_idle(self.log, ad_list)

        if not ensure_network_generation_for_subscription(self.log,
            self.android_devices[0],
            self.android_devices[0].droid.subscriptionGetDefaultDataSubId(),
            nw_gen, MAX_WAIT_TIME_NW_SELECTION,
            NETWORK_SERVICE_DATA):
            self.log.error("Device failed to reselect in {}s.".format(
                MAX_WAIT_TIME_NW_SELECTION))
            return False

        if not wait_for_voice_attach_for_subscription(
                self.log, self.android_devices[0], self.android_devices[
                    0].droid.subscriptionGetDefaultVoiceSubId(),
                MAX_WAIT_TIME_NW_SELECTION):
            return False

        self.log.info("Step1 WiFi is Off, Data is on Cell.")
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        self.android_devices[0].droid.telephonyToggleDataConnection(True)
        if (not wait_for_cell_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("Data not available on cell")
            return False

        try:
            self.log.info("Step2 Initiate call and accept.")
            if call_direction == DIRECTION_MOBILE_ORIGINATED:
                ad_caller = self.android_devices[0]
                ad_callee = self.android_devices[1]
            else:
                ad_caller = self.android_devices[1]
                ad_callee = self.android_devices[0]
            if not call_setup_teardown(self.log, ad_caller, ad_callee, None,
                                       None, None):
                self.log.error("Failed to Establish {} Voice Call".format(
                    call_direction))
                return False
            if simultaneous_voice_data:
                self.log.info("Step3 Verify internet.")
                time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
                if not verify_http_connection(self.log, self.android_devices[0]):
                    raise _LocalException("Internet Inaccessible when Enabled")

                self.log.info("Step4 Turn off data and verify not connected.")
                self.android_devices[0].droid.telephonyToggleDataConnection(False)
                if not wait_for_cell_data_connection(
                        self.log, self.android_devices[0], False):
                    raise _LocalException("Failed to Disable Cellular Data")

                if verify_http_connection(self.log, self.android_devices[0]):
                    raise _LocalException("Internet Accessible when Disabled")

                self.log.info("Step5 Re-enable data.")
                self.android_devices[0].droid.telephonyToggleDataConnection(True)
                if not wait_for_cell_data_connection(
                        self.log, self.android_devices[0], True):
                    raise _LocalException("Failed to Re-Enable Cellular Data")
                if not verify_http_connection(self.log, self.android_devices[0]):
                    raise _LocalException("Internet Inaccessible when Enabled")
            else:
                self.log.info("Step3 Verify no Internet and skip step 4-5.")
                if verify_http_connection(self.log, self.android_devices[0],
                    retry=0):
                    raise _LocalException("Internet Accessible.")

            self.log.info("Step6 Verify phones still in call and Hang up.")
            if not verify_incall_state(
                    self.log,
                [self.android_devices[0], self.android_devices[1]], True):
                return False
            if not hangup_call(self.log, self.android_devices[0]):
                self.log.error("Failed to hang up call")
                return False
            if not verify_http_connection(self.log, self.android_devices[0]):
                raise _LocalException("Internet Inaccessible when Enabled")

        except _LocalException as e:
            self.log.error(str(e))
            try:
                hangup_call(self.log, self.android_devices[0])
                self.android_devices[0].droid.telephonyToggleDataConnection(
                    True)
            except Exception:
                pass
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_2g(self):
        """Test data connection in 2G.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Ensure phone data generation is 2G.
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        WifiUtils.wifi_reset(self.log, self.android_devices[0])
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        return data_connectivity_single_bearer(self.log,
                                               self.android_devices[0], RAT_2G)

    @TelephonyBaseTest.tel_test_wrap
    def test_2g_wifi_not_associated(self):
        """Test data connection in 2G.

        Turn off airplane mode, enable WiFi (but not connected), enable Cellular Data.
        Ensure phone data generation is 2G.
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        WifiUtils.wifi_reset(self.log, self.android_devices[0])
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], True)
        return data_connectivity_single_bearer(self.log,
                                               self.android_devices[0], RAT_2G)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g(self):
        """Test data connection in 3G.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Ensure phone data generation is 3G.
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        WifiUtils.wifi_reset(self.log, self.android_devices[0])
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        return data_connectivity_single_bearer(self.log,
                                               self.android_devices[0], RAT_3G)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_wifi_not_associated(self):
        """Test data connection in 3G.

        Turn off airplane mode, enable WiFi (but not connected), enable Cellular Data.
        Ensure phone data generation is 3G.
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        WifiUtils.wifi_reset(self.log, self.android_devices[0])
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], True)
        return data_connectivity_single_bearer(self.log,
                                               self.android_devices[0], RAT_3G)

    @TelephonyBaseTest.tel_test_wrap
    def test_4g(self):
        """Test data connection in 4g.

        Turn off airplane mode, disable WiFi, enable Cellular Data.
        Ensure phone data generation is 4g.
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        WifiUtils.wifi_reset(self.log, self.android_devices[0])
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        return data_connectivity_single_bearer(self.log,
                                               self.android_devices[0], RAT_4G)

    @TelephonyBaseTest.tel_test_wrap
    def test_4g_wifi_not_associated(self):
        """Test data connection in 4g.

        Turn off airplane mode, enable WiFi (but not connected), enable Cellular Data.
        Ensure phone data generation is 4g.
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.

        Returns:
            True if success.
            False if failed.
        """
        WifiUtils.wifi_reset(self.log, self.android_devices[0])
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], True)
        return data_connectivity_single_bearer(self.log,
                                               self.android_devices[0], RAT_4G)

    @TelephonyBaseTest.tel_test_wrap
    def test_3g_stress(self):
        """Stress Test data connection in 3G.

        This is a stress test for "test_3g".
        Default MINIMUM_SUCCESS_RATE is set to 95%.

        Returns:
            True stress pass rate is higher than MINIMUM_SUCCESS_RATE.
            False otherwise.
        """
        ads = self.android_devices
        MINIMUM_SUCCESS_RATE = .95
        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            ensure_phones_default_state(
                self.log, [self.android_devices[0], self.android_devices[1]])
            WifiUtils.wifi_reset(self.log, self.android_devices[0])
            WifiUtils.wifi_toggle_state(self.log, self.android_devices[0],
                                        False)

            if data_connectivity_single_bearer(
                    self.log, self.android_devices[0], RAT_3G):
                success_count += 1
                result_str = "Succeeded"
            else:
                fail_count += 1
                result_str = "Failed"
            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count, str(100 * success_count / (
                success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @TelephonyBaseTest.tel_test_wrap
    def test_4g_stress(self):
        """Stress Test data connection in 4g.

        This is a stress test for "test_4g".
        Default MINIMUM_SUCCESS_RATE is set to 95%.

        Returns:
            True stress pass rate is higher than MINIMUM_SUCCESS_RATE.
            False otherwise.
        """
        ads = self.android_devices
        MINIMUM_SUCCESS_RATE = .95
        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            ensure_phones_default_state(
                self.log, [self.android_devices[0], self.android_devices[1]])
            WifiUtils.wifi_reset(self.log, self.android_devices[0])
            WifiUtils.wifi_toggle_state(self.log, self.android_devices[0],
                                        False)

            if data_connectivity_single_bearer(
                    self.log, self.android_devices[0], RAT_4G):
                success_count += 1
                result_str = "Succeeded"
            else:
                fail_count += 1
                result_str = "Failed"
            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count, str(100 * success_count / (
                success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    def _test_setup_tethering(self, ads, network_generation=None):
        """Pre setup steps for WiFi tethering test.

        Ensure all ads are idle.
        Ensure tethering provider:
            turn off APM, turn off WiFI, turn on Data.
            have Internet connection, no active ongoing WiFi tethering.

        Returns:
            True if success.
            False if failed.
        """
        ensure_phones_idle(self.log, ads)

        if network_generation is not None:
            if not ensure_network_generation_for_subscription(self.log,
                self.android_devices[0],
                self.android_devices[0].droid.subscriptionGetDefaultDataSubId(),
                network_generation, MAX_WAIT_TIME_NW_SELECTION,
                NETWORK_SERVICE_DATA):
                self.log.error("Device failed to reselect in {}s.".format(
                    MAX_WAIT_TIME_NW_SELECTION))
                return False

        self.log.info("Airplane Off, Wifi Off, Data On.")
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        self.android_devices[0].droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(self.log, self.android_devices[0],
                                             True):
            self.log.error("Failed to enable data connection.")
            return False

        self.log.info("Verify internet")
        if not verify_http_connection(self.log, self.android_devices[0]):
            self.log.error("Data not available on cell.")
            return False

        # Turn off active SoftAP if any.
        if ads[0].droid.wifiIsApEnabled():
            WifiUtils.stop_wifi_tethering(self.log, ads[0])

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_4g_to_2gwifi(self):
        """WiFi Tethering test: LTE to WiFI 2.4G Tethering

        1. DUT in LTE mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_4g_to_5gwifi(self):
        """WiFi Tethering test: LTE to WiFI 5G Tethering

        1. DUT in LTE mode, idle.
        2. DUT start 5G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_5G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_3g_to_2gwifi(self):
        """WiFi Tethering test: 3G to WiFI 2.4G Tethering

        1. DUT in 3G mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_3G):
            self.log.error("Verify 3G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_3g_to_5gwifi(self):
        """WiFi Tethering test: 3G to WiFI 5G Tethering

        1. DUT in 3G mode, idle.
        2. DUT start 5G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_3G):
            self.log.error("Verify 3G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_5G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_4g_to_2gwifi_2clients(self):
        """WiFi Tethering test: LTE to WiFI 2.4G Tethering, with multiple clients

        1. DUT in 3G mode, idle.
        2. DUT start 5G WiFi Tethering
        3. PhoneB and PhoneC disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB PhoneC

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1], ads[2]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_2g_to_2gwifi(self):
        """WiFi Tethering test: 2G to WiFI 2.4G Tethering

        1. DUT in 2G mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_2G):
            self.log.error("Verify 2G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_2g_to_5gwifi(self):
        """WiFi Tethering test: 2G to WiFI 5G Tethering

        1. DUT in 2G mode, idle.
        2. DUT start 5G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_2G):
            self.log.error("Verify 2G Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_5G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_disable_wifi_tethering_resume_connected_wifi(self):
        """WiFi Tethering test: WiFI connected to 2.4G network,
        start (LTE) 2.4G WiFi tethering, then stop tethering

        1. DUT in LTE mode, idle. WiFi connected to 2.4G Network
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB
        5. Disable WiFi Tethering on DUT.
        6. Verify DUT automatically connect to previous WiFI network

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False
        self.log.info("Connect WiFi.")
        if not ensure_wifi_connected(self.log, ads[0], self.wifi_network_ssid,
                                     self.wifi_network_pass):
            self.log.error("WiFi connect fail.")
            return False
        self.log.info("Start WiFi Tethering.")
        if not wifi_tethering_setup_teardown(self.log,
                                             ads[0],
                                             [ads[1]],
                                             check_interval=10,
                                             check_iteration=2):
            self.log.error("WiFi Tethering failed.")
            return False

        if (not wait_for_wifi_data_connection(self.log, ads[0], True) or
                not verify_http_connection(self.log, ads[0])):
            self.log.error("Provider data did not return to Wifi")
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_toggle_data_during_active_wifi_tethering(self):
        """WiFi Tethering test: Toggle Data during active WiFi Tethering

        1. DUT in LTE mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB
        5. Disable Data on DUT, verify PhoneB still connected to WiFi, but no Internet access.
        6. Enable Data on DUT, verify PhoneB still connected to WiFi and have Internet access.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False
        try:
            ssid = rand_ascii_str(10)
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False,
                    ssid=ssid):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info(
                "Disable Data on Provider, verify no data on Client.")
            ads[0].droid.telephonyToggleDataConnection(False)
            time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
            if verify_http_connection(self.log, ads[0]):
                self.log.error("Disable data on provider failed.")
                return False
            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False
            wifi_info = ads[1].droid.wifiGetConnectionInfo()

            if wifi_info[WifiEnums.SSID_KEY] != ssid:
                self.log.error("WiFi error. Info: {}".format(wifi_info))
                return False
            if verify_http_connection(self.log, ads[1]):
                self.log.error("Client should not have Internet connection.")
                return False

            self.log.info(
                "Enable Data on Provider, verify data available on Client.")
            ads[0].droid.telephonyToggleDataConnection(True)
            time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
            if not verify_http_connection(self.log, ads[0]):
                self.log.error("Enable data on provider failed.")
                return False
            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False
            wifi_info = ads[1].droid.wifiGetConnectionInfo()

            if wifi_info[WifiEnums.SSID_KEY] != ssid:
                self.log.error("WiFi error. Info: {}".format(wifi_info))
                return False
            if not verify_http_connection(self.log, ads[1]):
                self.log.error("Client have no Internet connection!")
                return False
        finally:
            if not wifi_tethering_cleanup(self.log, ads[0], [ads[1]]):
                return False
        return True

    # Invalid Live Test. Can't rely on the result of this test with live network.
    # Network may decide not to change the RAT when data conenction is active.
    @TelephonyBaseTest.tel_test_wrap
    def test_change_rat_during_active_wifi_tethering_lte_to_3g(self):
        """WiFi Tethering test: Change Cellular Data RAT generation from LTE to 3G,
            during active WiFi Tethering.

        1. DUT in LTE mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verily Internet access on DUT and PhoneB
        5. Change DUT Cellular Data RAT generation from LTE to 3G.
        6. Verify both DUT and PhoneB have Internet access.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False
        try:
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info("Provider change RAT from LTE to 3G.")
            if not ensure_network_generation(
                    self.log,
                    ads[0],
                    RAT_3G,
                    voice_or_data=NETWORK_SERVICE_DATA,
                    toggle_apm_after_setting=False):
                self.log.error("Provider failed to reselect to 3G.")
                return False
            time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
            if not verify_http_connection(self.log, ads[0]):
                self.log.error("Data not available on Provider.")
                return False
            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False
            if not tethering_check_internet_connection(self.log, ads[0],
                                                       [ads[1]], 10, 5):
                return False
        finally:
            if not wifi_tethering_cleanup(self.log, ads[0], [ads[1]]):
                return False
        return True

    # Invalid Live Test. Can't rely on the result of this test with live network.
    # Network may decide not to change the RAT when data conenction is active.
    @TelephonyBaseTest.tel_test_wrap
    def test_change_rat_during_active_wifi_tethering_3g_to_lte(self):
        """WiFi Tethering test: Change Cellular Data RAT generation from 3G to LTE,
            during active WiFi Tethering.

        1. DUT in 3G mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verily Internet access on DUT and PhoneB
        5. Change DUT Cellular Data RAT generation from 3G to LTE.
        6. Verify both DUT and PhoneB have Internet access.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_3G):
            self.log.error("Verify 3G Internet access failed.")
            return False
        try:
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info("Provider change RAT from 3G to 4G.")
            if not ensure_network_generation(
                    self.log,
                    ads[0],
                    RAT_4G,
                    voice_or_data=NETWORK_SERVICE_DATA,
                    toggle_apm_after_setting=False):
                self.log.error("Provider failed to reselect to 4G.")
                return False
            time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
            if not verify_http_connection(self.log, ads[0]):
                self.log.error("Data not available on Provider.")
                return False
            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False
            if not tethering_check_internet_connection(self.log, ads[0],
                                                       [ads[1]], 10, 5):
                return False
        finally:
            if not wifi_tethering_cleanup(self.log, ads[0], [ads[1]]):
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_toggle_apm_during_active_wifi_tethering(self):
        """WiFi Tethering test: Toggle APM during active WiFi Tethering

        1. DUT in LTE mode, idle.
        2. DUT start 2.4G WiFi Tethering
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB
        5. DUT toggle APM on, verify WiFi tethering stopped, PhoneB lost WiFi connection.
        6. DUT toggle APM off, verify PhoneA have cellular data and Internet connection.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Verify 4G Internet access failed.")
            return False
        try:
            ssid = rand_ascii_str(10)
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False,
                    ssid=ssid):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info(
                "Provider turn on APM, verify no wifi/data on Client.")
            if not toggle_airplane_mode(self.log, ads[0], True):
                self.log.error("Provider turn on APM failed.")
                return False
            time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
            if ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering not stopped.")
                return False
            if verify_http_connection(self.log, ads[1]):
                self.log.error("Client should not have Internet connection.")
                return False
            wifi_info = ads[1].droid.wifiGetConnectionInfo()
            self.log.info("WiFi Info: {}".format(wifi_info))

            if wifi_info[WifiEnums.SSID_KEY] == ssid:
                self.log.error(
                    "WiFi error. WiFi should not be connected.".format(
                        wifi_info))
                return False

            self.log.info("Provider turn off APM.")
            if not toggle_airplane_mode(self.log, ads[0], False):
                self.log.error("Provider turn on APM failed.")
                return False
            time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
            if ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering should not on.")
                return False
            if not verify_http_connection(self.log, ads[0]):
                self.log.error("Provider should have Internet connection.")
                return False
        finally:
            ads[1].droid.telephonyToggleDataConnection(True)
            WifiUtils.wifi_reset(self.log, ads[1])
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_entitlement_check(self):
        """Tethering Entitlement Check Test

        Get tethering entitlement check result.

        Returns:
            True if entitlement check returns True.
        """
        ad = self.android_devices[0]

        result = ad.droid.carrierConfigIsTetheringModeAllowed(
            TETHERING_MODE_WIFI, MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK)
        self.log.info("{} tethering entitlement check result: {}.".format(
            ad.serial, result))
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_4g_to_2gwifi_stress(self):
        """Stress Test LTE to WiFI 2.4G Tethering

        This is a stress test for "test_tethering_4g_to_2gwifi".
        Default MINIMUM_SUCCESS_RATE is set to 95%.

        Returns:
            True stress pass rate is higher than MINIMUM_SUCCESS_RATE.
            False otherwise.
        """
        MINIMUM_SUCCESS_RATE = .95
        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            ensure_phones_default_state(
                self.log, [self.android_devices[0], self.android_devices[1]])

            if self.test_tethering_4g_to_2gwifi():
                success_count += 1
                result_str = "Succeeded"
            else:
                fail_count += 1
                result_str = "Failed"
            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {} - {}%".format(
            success_count, fail_count, str(100 * success_count / (
                success_count + fail_count))))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_ssid_quotes(self):
        """WiFi Tethering test: SSID name have quotes.
        1. Set SSID name have double quotes.
        2. Start LTE to WiFi (2.4G) tethering.
        3. Verify tethering.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False
        ssid = "\"" + rand_ascii_str(10) + "\""
        self.log.info("Starting WiFi Tethering test with ssid: {}".format(
            ssid))

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10,
            ssid=ssid)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_password_escaping_characters(self):
        """WiFi Tethering test: password have escaping characters.
        1. Set password have escaping characters.
            e.g.: '"DQ=/{Yqq;M=(^_3HzRvhOiL8S%`]w&l<Qp8qH)bs<4E9v_q=HLr^)}w$blA0Kg'
        2. Start LTE to WiFi (2.4G) tethering.
        3. Verify tethering.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False

        password = '"DQ=/{Yqq;M=(^_3HzRvhOiL8S%`]w&l<Qp8qH)bs<4E9v_q=HLr^)}w$blA0Kg'
        self.log.info("Starting WiFi Tethering test with password: {}".format(
            password))

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10,
            password=password)

    def _test_start_wifi_tethering_connect_teardown(self, ad_host, ad_client,
                                                    ssid, password):
        """Private test util for WiFi Tethering.

        1. Host start WiFi tethering.
        2. Client connect to tethered WiFi.
        3. Host tear down WiFi tethering.

        Args:
            ad_host: android device object for host
            ad_client: android device object for client
            ssid: WiFi tethering ssid
            password: WiFi tethering password

        Returns:
            True if no error happen, otherwise False.
        """
        result = True
        # Turn off active SoftAP if any.
        if ad_host.droid.wifiIsApEnabled():
            WifiUtils.stop_wifi_tethering(self.log, ad_host)

        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        if not WifiUtils.start_wifi_tethering(self.log, ad_host, ssid,
                                              password,
                                              WifiUtils.WIFI_CONFIG_APBAND_2G):
            self.log.error("Provider start WiFi tethering failed.")
            result = False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        if not ensure_wifi_connected(self.log, ad_client, ssid, password):
            self.log.error("Client connect to WiFi failed.")
            result = False
        if not WifiUtils.wifi_reset(self.log, ad_client):
            self.log.error("Reset client WiFi failed. {}".format(
                ad_client.serial))
            result = False
        if not WifiUtils.stop_wifi_tethering(self.log, ad_host):
            self.log.error("Provider strop WiFi tethering failed.")
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_ssid(self):
        """WiFi Tethering test: start WiFi tethering with all kinds of SSIDs.

        For each listed SSID, start WiFi tethering on DUT, client connect WiFi,
        then tear down WiFi tethering.

        Returns:
            True if WiFi tethering succeed on all SSIDs.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Setup Failed.")
            return False
        ssid_list = [" !\"#$%&'()*+,-./0123456789:;<=>?",
                     "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_",
                     "`abcdefghijklmnopqrstuvwxyz{|}~", " a ", "!b!", "#c#",
                     "$d$", "%e%", "&f&", "'g'", "(h(", ")i)", "*j*", "+k+",
                     "-l-", ".m.", "/n/", "_",
                     " !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}",
                     "\u0644\u062c\u0648\u062c", "\u8c37\u6b4c", "\uad6c\uae00"
                     "\u30b0\u30fc\u30eb",
                     "\u0417\u0434\u0440\u0430\u0432\u0441\u0442\u0443\u0439"]
        fail_list = {}

        for ssid in ssid_list:
            password = rand_ascii_str(8)
            self.log.info("SSID: <{}>, Password: <{}>".format(ssid, password))
            if not self._test_start_wifi_tethering_connect_teardown(
                    ads[0], ads[1], ssid, password):
                fail_list[ssid] = password

        if (len(fail_list) > 0):
            self.log.error("Failed cases: {}".format(fail_list))
            return False
        else:
            return True

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_password(self):
        """WiFi Tethering test: start WiFi tethering with all kinds of passwords.

        For each listed password, start WiFi tethering on DUT, client connect WiFi,
        then tear down WiFi tethering.

        Returns:
            True if WiFi tethering succeed on all passwords.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, RAT_4G):
            self.log.error("Setup Failed.")
            return False
        password_list = [
            " !\"#$%&'()*+,-./0123456789:;<=>?",
            "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_",
            "`abcdefghijklmnopqrstuvwxyz{|}~",
            " !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}", "abcdefgh",
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!",
            " a12345 ", "!b12345!", "#c12345#", "$d12345$", "%e12345%",
            "&f12345&", "'g12345'", "(h12345(", ")i12345)", "*j12345*",
            "+k12345+", "-l12345-", ".m12345.", "/n12345/"
        ]
        fail_list = {}

        for password in password_list:
            result = True
            ssid = rand_ascii_str(8)
            self.log.info("SSID: <{}>, Password: <{}>".format(ssid, password))
            if not self._test_start_wifi_tethering_connect_teardown(
                    ads[0], ads[1], ssid, password):
                fail_list[ssid] = password

        if (len(fail_list) > 0):
            self.log.error("Failed cases: {}".format(fail_list))
            return False
        else:
            return True

    def _test_tethering_wifi_and_voice_call(
            self, provider, client, provider_data_rat, provider_setup_func,
            provider_in_call_check_func):
        if not self._test_setup_tethering(
            [provider, client], provider_data_rat):
            self.log.error("Verify 4G Internet access failed.")
            return False

        tasks = [(provider_setup_func, (self.log, provider)),
                 (phone_setup_voice_general, (self.log, client))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up VoLTE.")
            return False

        try:
            self.log.info("1. Setup WiFi Tethering.")
            if not wifi_tethering_setup_teardown(
                    self.log,
                    provider,
                [client],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False
            self.log.info("2. Make outgoing call.")
            if not call_setup_teardown(
                    self.log,
                    provider,
                    client,
                    ad_hangup=None,
                    verify_caller_func=provider_in_call_check_func):
                self.log.error("Setup Call Failed.")
                return False
            self.log.info("3. Verify data.")
            if not verify_http_connection(self.log, provider):
                self.log.error("Provider have no Internet access.")
            if not verify_http_connection(self.log, client):
                self.log.error("Client have no Internet access.")
            hangup_call(self.log, provider)

            time.sleep(WAIT_TIME_BETWEEN_REG_AND_CALL)

            self.log.info("4. Make incoming call.")
            if not call_setup_teardown(
                    self.log,
                    client,
                    provider,
                    ad_hangup=None,
                    verify_callee_func=provider_in_call_check_func):
                self.log.error("Setup Call Failed.")
                return False
            self.log.info("5. Verify data.")
            if not verify_http_connection(self.log, provider):
                self.log.error("Provider have no Internet access.")
            if not verify_http_connection(self.log, client):
                self.log.error("Client have no Internet access.")
            hangup_call(self.log, provider)

        finally:
            if not wifi_tethering_cleanup(self.log, provider, [client]):
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_volte_call(self):
        """WiFi Tethering test: VoLTE call during WiFi tethering
        1. Start LTE to WiFi (2.4G) tethering.
        2. Verify tethering.
        3. Make outgoing VoLTE call on tethering provider.
        4. Verify tethering still works.
        5. Make incoming VoLTE call on tethering provider.
        6. Verify tethering still works.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_tethering_wifi_and_voice_call(
            self.android_devices[0], self.android_devices[1], RAT_4G,
            phone_setup_volte, is_phone_in_call_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_csfb_call(self):
        """WiFi Tethering test: CSFB call during WiFi tethering
        1. Start LTE to WiFi (2.4G) tethering.
        2. Verify tethering.
        3. Make outgoing CSFB call on tethering provider.
        4. Verify tethering still works.
        5. Make incoming CSFB call on tethering provider.
        6. Verify tethering still works.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_tethering_wifi_and_voice_call(
            self.android_devices[0], self.android_devices[1], RAT_4G,
            phone_setup_csfb, is_phone_in_call_csfb)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_3g_call(self):
        """WiFi Tethering test: 3G call during WiFi tethering
        1. Start 3G to WiFi (2.4G) tethering.
        2. Verify tethering.
        3. Make outgoing CS call on tethering provider.
        4. Verify tethering still works.
        5. Make incoming CS call on tethering provider.
        6. Verify tethering still works.

        Returns:
            True if success.
            False if failed.
        """
        return self._test_tethering_wifi_and_voice_call(
            self.android_devices[0], self.android_devices[1], RAT_3G,
            phone_setup_voice_3g, is_phone_in_call_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_no_password(self):
        """WiFi Tethering test: Start WiFi tethering with no password

        1. DUT is idle.
        2. DUT start 2.4G WiFi Tethering, with no WiFi password.
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10,
            password="")

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_reboot(self):
        """WiFi Tethering test: Start WiFi tethering then Reboot device

        1. DUT is idle.
        2. DUT start 2.4G WiFi Tethering.
        3. PhoneB disable data, connect to DUT's softAP
        4. Verify Internet access on DUT and PhoneB
        5. Reboot DUT
        6. After DUT reboot, verify tethering is stopped.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False
        try:
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info("Reboot DUT:{}".format(ads[0].serial))
            ads[0].reboot()
            time.sleep(WAIT_TIME_AFTER_REBOOT +
                       WAIT_TIME_TETHERING_AFTER_REBOOT)

            self.log.info("After reboot check if tethering stopped.")
            if ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering did NOT stopped.")
                return False
        finally:
            ads[1].droid.telephonyToggleDataConnection(True)
            WifiUtils.wifi_reset(self.log, ads[1])
            if ads[0].droid.wifiIsApEnabled():
                WifiUtils.stop_wifi_tethering(self.log, ads[0])
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_connect_wifi_start_tethering_wifi_reboot(self):
        """WiFi Tethering test: WiFI connected, then start WiFi tethering,
            then reboot device.

        Initial Condition: DUT in 4G mode, idle, DUT connect to WiFi.
        1. DUT start 2.4G WiFi Tethering.
        2. PhoneB disable data, connect to DUT's softAP
        3. Verify Internet access on DUT and PhoneB
        4. Reboot DUT
        5. After DUT reboot, verify tethering is stopped. DUT is able to connect
            to previous WiFi AP.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False

        self.log.info("Make sure DUT can connect to live network by WIFI")
        if ((not ensure_wifi_connected(self.log, ads[0],
                                       self.wifi_network_ssid,
                                       self.wifi_network_pass)) or
            (not verify_http_connection(self.log, ads[0]))):
            self.log.error("WiFi connect fail.")
            return False

        try:
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info("Reboot DUT:{}".format(ads[0].serial))
            ads[0].reboot()
            time.sleep(WAIT_TIME_AFTER_REBOOT)
            time.sleep(WAIT_TIME_TETHERING_AFTER_REBOOT)

            self.log.info("After reboot check if tethering stopped.")
            if ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering did NOT stopped.")
                return False

            self.log.info("Make sure WiFi can connect automatically.")
            if (not wait_for_wifi_data_connection(self.log, ads[0], True) or
                    not verify_http_connection(self.log, ads[0])):
                self.log.error("Data did not return to WiFi")
                return False

        finally:
            ads[1].droid.telephonyToggleDataConnection(True)
            WifiUtils.wifi_reset(self.log, ads[1])
            if ads[0].droid.wifiIsApEnabled():
                WifiUtils.stop_wifi_tethering(self.log, ads[0])
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_connect_wifi_reboot_start_tethering_wifi(self):
        """WiFi Tethering test: DUT connected to WiFi, then reboot,
        After reboot, start WiFi tethering, verify tethering actually works.

        Initial Condition: Device set to 4G mode, idle, DUT connect to WiFi.
        1. Verify Internet is working on DUT (by WiFi).
        2. Reboot DUT.
        3. DUT start 2.4G WiFi Tethering.
        4. PhoneB disable data, connect to DUT's softAP
        5. Verify Internet access on DUT and PhoneB

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices

        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False

        self.log.info("Make sure DUT can connect to live network by WIFI")
        if ((not ensure_wifi_connected(self.log, ads[0],
                                       self.wifi_network_ssid,
                                       self.wifi_network_pass)) or
            (not verify_http_connection(self.log, ads[0]))):
            self.log.error("WiFi connect fail.")
            return False

        self.log.info("Reboot DUT:{}".format(ads[0].serial))
        ads[0].reboot()
        time.sleep(WAIT_TIME_AFTER_REBOOT)
        time.sleep(WAIT_TIME_TETHERING_AFTER_REBOOT)

        return wifi_tethering_setup_teardown(
            self.log,
            ads[0],
            [ads[1]],
            ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
            check_interval=10,
            check_iteration=10)

    @TelephonyBaseTest.tel_test_wrap
    def test_tethering_wifi_screen_off_enable_doze_mode(self):
        """WiFi Tethering test: Start WiFi tethering, then turn off DUT's screen,
            then enable doze mode.

        1. Start WiFi tethering on DUT.
        2. PhoneB disable data, and connect to DUT's softAP
        3. Verify Internet access on DUT and PhoneB
        4. Turn off DUT's screen. Wait for 1 minute and
            verify Internet access on Client PhoneB.
        5. Enable doze mode on DUT. Wait for 1 minute and
            verify Internet access on Client PhoneB.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False
        try:
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            self.log.info("Turn off screen on provider: <{}>.".format(ads[
                0].serial))
            ads[0].droid.goToSleepNow()
            time.sleep(60)
            if not verify_http_connection(self.log, ads[1]):
                self.log.error("Client have no Internet access.")
                return False

            self.log.info("Enable doze mode on provider: <{}>.".format(ads[
                0].serial))
            if not enable_doze(ads[0]):
                self.log.error("Failed to enable doze mode.")
                return False
            time.sleep(60)
            if not verify_http_connection(self.log, ads[1]):
                self.log.error("Client have no Internet access.")
                return False
        finally:
            self.log.info("Disable doze mode.")
            if not disable_doze(ads[0]):
                self.log.error("Failed to disable doze mode.")
                return False
            if not wifi_tethering_cleanup(self.log, ads[0], [ads[1]]):
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_msim_switch_data_sim_2g(self):
        """Switch Data SIM on 2G network.

        Steps:
        1. Data on default Data SIM.
        2. Switch Data to another SIM. Make sure data is still available.
        3. Switch Data back to previous SIM. Make sure data is still available.

        Expected Results:
        1. Verify Data on Cell
        2. Verify Data on Wifi

        Returns:
            True if success.
            False if failed.
        """
        ad = self.android_devices[0]
        current_data_sub_id = ad.droid.subscriptionGetDefaultDataSubId()
        current_sim_slot_index = get_slot_index_from_subid(self.log, ad,
            current_data_sub_id)
        if current_sim_slot_index == SIM1_SLOT_INDEX:
            next_sim_slot_index = SIM2_SLOT_INDEX
        else:
            next_sim_slot_index = SIM1_SLOT_INDEX
        next_data_sub_id = get_subid_from_slot_index(self.log, ad,
            next_sim_slot_index)
        self.log.info("Current Data is on subId: {}, SIM slot: {}".format(
            current_data_sub_id, current_sim_slot_index))
        if not ensure_network_generation_for_subscription(
            self.log, ad, ad.droid.subscriptionGetDefaultDataSubId(), GEN_2G,
            voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("Device data does not attach to 2G.")
            return False
        if not verify_http_connection(self.log, ad):
            self.log.error("No Internet access on default Data SIM.")
            return False

        self.log.info("Change Data to subId: {}, SIM slot: {}".format(
            next_data_sub_id, next_sim_slot_index))
        if not change_data_sim_and_verify_data(self.log, ad, next_sim_slot_index):
            self.log.error("Failed to change data SIM.")
            return False

        next_data_sub_id = current_data_sub_id
        next_sim_slot_index = current_sim_slot_index
        self.log.info("Change Data back to subId: {}, SIM slot: {}".format(
            next_data_sub_id, next_sim_slot_index))
        if not change_data_sim_and_verify_data(self.log, ad, next_sim_slot_index):
            self.log.error("Failed to change data SIM.")
            return False

        return True

    def _test_wifi_connect_disconnect(self):
        """Perform multiple connects and disconnects from WiFi and verify that
            data switches between WiFi and Cell.

        Steps:
        1. Reset Wifi on DUT
        2. Connect DUT to a WiFi AP
        3. Repeat steps 1-2, alternately disconnecting and disabling wifi

        Expected Results:
        1. Verify Data on Cell
        2. Verify Data on Wifi

        Returns:
            True if success.
            False if failed.
        """
        ad = self.android_devices[0]

        wifi_toggles = [True, False, True, False, False, True, False, False,
                        False, False, True, False, False, False, False, False,
                        False, False, False]

        for toggle in wifi_toggles:

            WifiUtils.wifi_reset(self.log, ad, toggle)

            if not wait_for_cell_data_connection(
                    self.log, ad, True, MAX_WAIT_TIME_WIFI_CONNECTION):
                self.log.error("Failed wifi connection, aborting!")
                return False

            if not verify_http_connection(self.log, ad,
                                          'http://www.google.com', 100, .1):
                self.log.error("Failed to get user-plane traffic, aborting!")
                return False

            if toggle:
                WifiUtils.wifi_toggle_state(self.log, ad, True)

            WifiUtils.wifi_connect(self.log, ad, self.wifi_network_ssid,
                                   self.wifi_network_pass)

            if not wait_for_wifi_data_connection(
                    self.log, ad, True, MAX_WAIT_TIME_WIFI_CONNECTION):
                self.log.error("Failed wifi connection, aborting!")
                return False

            if not verify_http_connection(self.log, ad,
                                          'http://www.google.com', 100, .1):
                self.log.error("Failed to get user-plane traffic, aborting!")
                return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_connect_disconnect_4g(self):
        """Perform multiple connects and disconnects from WiFi and verify that
            data switches between WiFi and Cell.

        Steps:
        1. DUT Cellular Data is on 4G. Reset Wifi on DUT
        2. Connect DUT to a WiFi AP
        3. Repeat steps 1-2, alternately disconnecting and disabling wifi

        Expected Results:
        1. Verify Data on Cell
        2. Verify Data on Wifi

        Returns:
            True if success.
            False if failed.
        """

        ad = self.android_devices[0]
        if not ensure_network_generation_for_subscription(self.log, ad,
            ad.droid.subscriptionGetDefaultDataSubId(), GEN_4G,
            MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            self.log.error("Device {} failed to reselect in {}s.".format(
                ad.serial, MAX_WAIT_TIME_NW_SELECTION))
            return False
        return self._test_wifi_connect_disconnect()

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_connect_disconnect_3g(self):
        """Perform multiple connects and disconnects from WiFi and verify that
            data switches between WiFi and Cell.

        Steps:
        1. DUT Cellular Data is on 3G. Reset Wifi on DUT
        2. Connect DUT to a WiFi AP
        3. Repeat steps 1-2, alternately disconnecting and disabling wifi

        Expected Results:
        1. Verify Data on Cell
        2. Verify Data on Wifi

        Returns:
            True if success.
            False if failed.
        """

        ad = self.android_devices[0]
        if not ensure_network_generation_for_subscription(self.log, ad,
            ad.droid.subscriptionGetDefaultDataSubId(), GEN_3G,
            MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            self.log.error("Device {} failed to reselect in {}s.".format(
                ad.serial, MAX_WAIT_TIME_NW_SELECTION))
            return False
        return self._test_wifi_connect_disconnect()

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_connect_disconnect_2g(self):
        """Perform multiple connects and disconnects from WiFi and verify that
            data switches between WiFi and Cell.

        Steps:
        1. DUT Cellular Data is on 2G. Reset Wifi on DUT
        2. Connect DUT to a WiFi AP
        3. Repeat steps 1-2, alternately disconnecting and disabling wifi

        Expected Results:
        1. Verify Data on Cell
        2. Verify Data on Wifi

        Returns:
            True if success.
            False if failed.
        """
        ad = self.android_devices[0]
        if not ensure_network_generation_for_subscription(self.log, ad,
            ad.droid.subscriptionGetDefaultDataSubId(), GEN_2G,
            MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            self.log.error("Device {} failed to reselect in {}s.".format(
                ad.serial, MAX_WAIT_TIME_NW_SELECTION))
            return False
        return self._test_wifi_connect_disconnect()

    def _test_wifi_tethering_enabled_add_voice_call(self, network_generation,
        voice_call_direction, is_data_available_during_call):
        """Tethering enabled + voice call.

        Steps:
        1. DUT data is on <network_generation>. Start WiFi Tethering.
        2. PhoneB connect to DUT's softAP
        3. DUT make a MO/MT (<voice_call_direction>) phone call.
        4. DUT end phone call.

        Expected Results:
        1. DUT is able to start WiFi tethering.
        2. PhoneB connected to DUT's softAP and able to browse Internet.
        3. DUT WiFi tethering is still on. Phone call works OK.
            If is_data_available_during_call is True, then PhoneB still has
            Internet access.
            Else, then Data is suspend, PhoneB has no Internet access.
        4. WiFi Tethering still on, voice call stopped, and PhoneB have Internet
            access.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not self._test_setup_tethering(ads, network_generation):
            self.log.error("Verify Internet access failed.")
            return False
        try:
            # Start WiFi Tethering
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                    [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False

            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False

            # Make a voice call
            if voice_call_direction == DIRECTION_MOBILE_ORIGINATED:
                ad_caller = ads[0]
                ad_callee = ads[1]
            else:
                ad_caller = ads[1]
                ad_callee = ads[0]
            if not call_setup_teardown(self.log, ad_caller, ad_callee, None,
                                       None, None):
                self.log.error("Failed to Establish {} Voice Call".format(
                    voice_call_direction))
                return False

            # Tethering should still be on.
            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False
            if not is_data_available_during_call:
                if verify_http_connection(self.log, ads[1], retry=0):
                    self.log.error("Client should not have Internet Access.")
                    return False
            else:
                if not verify_http_connection(self.log, ads[1]):
                    self.log.error("Client should have Internet Access.")
                    return False

            # Hangup call. Client should have data.
            if not hangup_call(self.log, ads[0]):
                self.log.error("Failed to hang up call")
                return False
            if not ads[0].droid.wifiIsApEnabled():
                self.log.error("Provider WiFi tethering stopped.")
                return False
            if not verify_http_connection(self.log, ads[1]):
                self.log.error("Client should have Internet Access.")
                return False
        finally:
            ads[1].droid.telephonyToggleDataConnection(True)
            WifiUtils.wifi_reset(self.log, ads[1])
            if ads[0].droid.wifiIsApEnabled():
                WifiUtils.stop_wifi_tethering(self.log, ads[0])
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_tethering_enabled_add_mo_voice_call_2g_dsds(self):
        """Tethering enabled + voice call

        Steps:
        1. DUT is DSDS device, Data on 2G. Start WiFi Tethering on <Data SIM>
        2. PhoneB connect to DUT's softAP
        3. DUT make a mo phone call on <Voice SIM>
        4. DUT end phone call.

        Expected Results:
        1. DUT is able to start WiFi tethering.
        2. PhoneB connected to DUT's softAP and able to browse Internet.
        3. DUT WiFi tethering is still on. Phone call works OK. Data is suspend,
            PhoneB still connected to DUT's softAP, but no data available.
        4. DUT data resumes, and PhoneB have Internet access.

        Returns:
            True if success.
            False if failed.
        """

        return self._test_wifi_tethering_enabled_add_voice_call(GEN_2G,
            DIRECTION_MOBILE_ORIGINATED, False)

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_tethering_enabled_add_mt_voice_call_2g_dsds(self):
        """Tethering enabled + voice call

        Steps:
        1. DUT is DSDS device, Data on 2G. Start WiFi Tethering on <Data SIM>
        2. PhoneB connect to DUT's softAP
        3. DUT make a mt phone call on <Voice SIM>
        4. DUT end phone call.

        Expected Results:
        1. DUT is able to start WiFi tethering.
        2. PhoneB connected to DUT's softAP and able to browse Internet.
        3. DUT WiFi tethering is still on. Phone call works OK. Data is suspend,
            PhoneB still connected to DUT's softAP, but no data available.
        4. DUT data resumes, and PhoneB have Internet access.

        Returns:
            True if success.
            False if failed.
        """

        return self._test_wifi_tethering_enabled_add_voice_call(GEN_2G,
            DIRECTION_MOBILE_TERMINATED, False)

    @TelephonyBaseTest.tel_test_wrap
    def test_wifi_tethering_msim_switch_data_sim(self):
        """Tethering enabled + switch data SIM.

        Steps:
        1. Start WiFi Tethering on <Default Data SIM>
        2. PhoneB connect to DUT's softAP
        3. DUT change Default Data SIM.

        Expected Results:
        1. DUT is able to start WiFi tethering.
        2. PhoneB connected to DUT's softAP and able to browse Internet.
        3. DUT Data changed to 2nd SIM, WiFi tethering should continues,
            PhoneB should have Internet access.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        current_data_sub_id = ads[0].droid.subscriptionGetDefaultDataSubId()
        current_sim_slot_index = get_slot_index_from_subid(self.log, ads[0],
            current_data_sub_id)
        self.log.info("Current Data is on subId: {}, SIM slot: {}".format(
            current_data_sub_id, current_sim_slot_index))
        if not self._test_setup_tethering(ads):
            self.log.error("Verify Internet access failed.")
            return False
        try:
            # Start WiFi Tethering
            if not wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                    [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=2,
                    do_cleanup=False):
                self.log.error("WiFi Tethering failed.")
                return False
            for i in range(0, 2):
                next_sim_slot_index = \
                    {SIM1_SLOT_INDEX : SIM2_SLOT_INDEX,
                     SIM2_SLOT_INDEX : SIM1_SLOT_INDEX}[current_sim_slot_index]
                self.log.info("Change Data to SIM slot: {}".
                    format(next_sim_slot_index))
                if not change_data_sim_and_verify_data(self.log, ads[0],
                    next_sim_slot_index):
                    self.log.error("Failed to change data SIM.")
                    return False
                current_sim_slot_index = next_sim_slot_index
                if not verify_http_connection(self.log, ads[1]):
                    self.log.error("Client should have Internet Access.")
                    return False
        finally:
            ads[1].droid.telephonyToggleDataConnection(True)
            WifiUtils.wifi_reset(self.log, ads[1])
            if ads[0].droid.wifiIsApEnabled():
                WifiUtils.stop_wifi_tethering(self.log, ads[0])
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_msim_cell_data_switch_to_wifi_switch_data_sim_2g(self):
        """Switch Data SIM on 2G network.

        Steps:
        1. Data on default Data SIM.
        2. Turn on WiFi, then data should be on WiFi.
        3. Switch Data to another SIM. Disable WiFi.

        Expected Results:
        1. Verify Data on Cell
        2. Verify Data on WiFi
        3. After WiFi disabled, Cell Data is available on 2nd SIM.

        Returns:
            True if success.
            False if failed.
        """
        ad = self.android_devices[0]
        current_data_sub_id = ad.droid.subscriptionGetDefaultDataSubId()
        current_sim_slot_index = get_slot_index_from_subid(self.log, ad,
                                                           current_data_sub_id)
        if current_sim_slot_index == SIM1_SLOT_INDEX:
            next_sim_slot_index = SIM2_SLOT_INDEX
        else:
            next_sim_slot_index = SIM1_SLOT_INDEX
        next_data_sub_id = get_subid_from_slot_index(self.log, ad,
                                                     next_sim_slot_index)
        self.log.info("Current Data is on subId: {}, SIM slot: {}".format(
            current_data_sub_id, current_sim_slot_index))
        if not ensure_network_generation_for_subscription(
                self.log,
                ad,
                ad.droid.subscriptionGetDefaultDataSubId(),
                GEN_2G,
                voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("Device data does not attach to 2G.")
            return False
        if not verify_http_connection(self.log, ad):
            self.log.error("No Internet access on default Data SIM.")
            return False

        self.log.info("Connect to WiFi and verify Internet access.")
        if not ensure_wifi_connected(self.log, ad, self.wifi_network_ssid,
                                     self.wifi_network_pass):
            self.log.error("WiFi connect fail.")
            return False
        if (not wait_for_wifi_data_connection(self.log, ad, True) or
                not verify_http_connection(self.log, ad)):
            self.log.error("Data is not on WiFi")
            return False

        try:
            self.log.info(
                "Change Data SIM, Disable WiFi and verify Internet access.")
            set_subid_for_data(ad, next_data_sub_id)
            WifiUtils.wifi_toggle_state(self.log, ad, False)
            if not wait_for_data_attach_for_subscription(
                    self.log, ad, next_data_sub_id,
                    MAX_WAIT_TIME_NW_SELECTION):
                self.log.error("Failed to attach data on subId:{}".format(
                    next_data_sub_id))
                return False
            if not verify_http_connection(self.log, ad):
                self.log.error("No Internet access after changing Data SIM.")
                return False

        finally:
            self.log.info("Change Data SIM back.")
            set_subid_for_data(ad, current_data_sub_id)

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_disable_data_on_non_active_data_sim(self):
        """Switch Data SIM on 2G network.

        Steps:
        1. Data on default Data SIM.
        2. Disable data on non-active Data SIM.

        Expected Results:
        1. Verify Data Status on Default Data SIM and non-active Data SIM.
        1. Verify Data Status on Default Data SIM and non-active Data SIM.

        Returns:
            True if success.
            False if failed.
        """
        ad = self.android_devices[0]
        current_data_sub_id = ad.droid.subscriptionGetDefaultDataSubId()
        current_sim_slot_index = get_slot_index_from_subid(self.log, ad,
                                                           current_data_sub_id)
        if current_sim_slot_index == SIM1_SLOT_INDEX:
            non_active_sim_slot_index = SIM2_SLOT_INDEX
        else:
            non_active_sim_slot_index = SIM1_SLOT_INDEX
        non_active_sub_id = get_subid_from_slot_index(
            self.log, ad, non_active_sim_slot_index)
        self.log.info("Current Data is on subId: {}, SIM slot: {}".format(
            current_data_sub_id, current_sim_slot_index))

        if not ensure_network_generation_for_subscription(
                self.log,
                ad,
                ad.droid.subscriptionGetDefaultDataSubId(),
                GEN_2G,
                voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("Device data does not attach to 2G.")
            return False
        if not verify_http_connection(self.log, ad):
            self.log.error("No Internet access on default Data SIM.")
            return False

        if ad.droid.telephonyGetDataConnectionState() != DATA_STATE_CONNECTED:
            self.log.error("Data Connection State should be connected.")
            return False
        # TODO: Check Data state for non-active subId.

        try:
            self.log.info("Disable Data on Non-Active Sub ID")
            ad.droid.telephonyToggleDataConnectionForSubscription(
                non_active_sub_id, False)
            # TODO: Check Data state for non-active subId.
            if ad.droid.telephonyGetDataConnectionState(
            ) != DATA_STATE_CONNECTED:
                self.log.error("Data Connection State should be connected.")
                return False
        finally:
            self.log.info("Enable Data on Non-Active Sub ID")
            ad.droid.telephonyToggleDataConnectionForSubscription(
                non_active_sub_id, True)
        return True
        """ Tests End """

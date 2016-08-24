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
    Test Script for epdg RF shield box related tests.
"""

import time
from queue import Empty
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_atten_utils import set_rssi
from acts.test_utils.tel.tel_defines import CELL_STRONG_RSSI_VALUE
from acts.test_utils.tel.tel_defines import CELL_WEAK_RSSI_VALUE
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_ORIGINATED
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_TERMINATED
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import INVALID_WIFI_RSSI
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_DROP
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import MAX_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import MIN_RSSI_RESERVED_VALUE
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_BACKGROUND
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_FOREGROUND
from acts.test_utils.tel.tel_defines import PRECISE_CALL_STATE_LISTEN_LEVEL_RINGING
from acts.test_utils.tel.tel_defines import RAT_LTE
from acts.test_utils.tel.tel_defines import RAT_IWLAN
from acts.test_utils.tel.tel_defines import RAT_WCDMA
from acts.test_utils.tel.tel_defines import WAIT_TIME_BETWEEN_REG_AND_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_WIFI_RSSI_CALIBRATION_SCREEN_ON
from acts.test_utils.tel.tel_defines import WAIT_TIME_WIFI_RSSI_CALIBRATION_WIFI_CONNECTED
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_defines import WIFI_WEAK_RSSI_VALUE
from acts.test_utils.tel.tel_defines import EventNetworkCallback
from acts.test_utils.tel.tel_defines import NetworkCallbackAvailable
from acts.test_utils.tel.tel_defines import NetworkCallbackLost
from acts.test_utils.tel.tel_defines import SignalStrengthContainer
from acts.test_utils.tel.tel_test_utils import WifiUtils
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_network_rat
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import is_network_call_back_event_match
from acts.test_utils.tel.tel_test_utils import is_phone_in_call
from acts.test_utils.tel.tel_test_utils import is_phone_not_in_call
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import wait_for_droid_not_in_call
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_disabled
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_enabled
from acts.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_not_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_csfb
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.utils import load_config

# Attenuator name
ATTEN_NAME_FOR_WIFI = 'wifi0'
ATTEN_NAME_FOR_CELL = 'cell0'

# WiFi RSSI settings for ROVE_IN test
WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN = -60
WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN = -70

# WiFi RSSI settings for ROVE_OUT test
WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE = -60
WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT = -70
WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT = -90

# WiFi RSSI settings for HAND_IN test
WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN = -80
WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN = -50

# WiFi RSSI settings for HAND_OUT test
WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT = -70
WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT = -85


class TelWifiVoiceTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            # WFC Call Routing tests.
            # epdg, WFC, APM, WiFi strong
            "test_call_epdg_wfc_wifi_only_wifi_strong_apm",
            "test_call_epdg_wfc_wifi_preferred_wifi_strong_apm",
            "test_call_epdg_wfc_cellular_preferred_wifi_strong_apm",

            # epdg, WFC, APM, WiFi Absent
            "test_call_epdg_wfc_wifi_only_wifi_absent_apm",
            "test_call_epdg_wfc_wifi_preferred_wifi_absent_apm",
            "test_call_epdg_wfc_cellular_preferred_wifi_absent_apm",

            # epdg, WFC, APM, WiFi Disabled
            "test_call_epdg_wfc_wifi_only_wifi_disabled_apm",
            "test_call_epdg_wfc_wifi_preferred_wifi_disabled_apm",
            "test_call_epdg_wfc_cellular_preferred_wifi_disabled_apm",

            # epdg, WFC, cellular strong, WiFi strong
            "test_call_epdg_wfc_wifi_preferred_wifi_strong_cellular_strong",
            "test_call_epdg_wfc_cellular_preferred_wifi_strong_cellular_strong",

            # epdg, WFC, cellular strong, WiFi weak
            "test_call_epdg_wfc_wifi_preferred_wifi_weak_cellular_strong",
            "test_call_epdg_wfc_cellular_preferred_wifi_weak_cellular_strong",

            # epdg, WFC, cellular strong, WiFi Absent
            "test_call_epdg_wfc_wifi_preferred_wifi_absent_cellular_strong",
            "test_call_epdg_wfc_cellular_preferred_wifi_absent_cellular_strong",

            # epdg, WFC, cellular strong, WiFi Disabled
            "test_call_epdg_wfc_wifi_preferred_wifi_disabled_cellular_strong",
            "test_call_epdg_wfc_cellular_preferred_wifi_disabled_cellular_strong",

            # epdg, WFC, cellular weak, WiFi strong
            "test_call_epdg_wfc_wifi_preferred_wifi_strong_cellular_weak",

            # epdg, WFC, cellular weak, WiFi Absent=
            "test_call_epdg_wfc_wifi_preferred_wifi_absent_cellular_weak",
            "test_call_epdg_wfc_cellular_preferred_wifi_absent_cellular_weak",

            # epdg, WFC, cellular weak, WiFi Disabled
            "test_call_epdg_wfc_wifi_preferred_wifi_disabled_cellular_weak",
            "test_call_epdg_wfc_cellular_preferred_wifi_disabled_cellular_weak",

            # epdg, WiFI strong, WFC disabled
            "test_call_epdg_wfc_disabled_wifi_strong_apm",
            "test_call_epdg_wfc_disabled_wifi_strong_cellular_strong",
            "test_call_epdg_wfc_disabled_wifi_strong_cellular_weak",

            # WFC Idle-Mode Mobility
            # Rove-in, Rove-out test
            "test_rove_in_lte_wifi_preferred",
            "test_rove_in_lte_wifi_only",
            "test_rove_in_wcdma_wifi_preferred",
            "test_rove_in_wcdma_wifi_only",
            "test_rove_out_lte_wifi_preferred",
            "test_rove_out_lte_wifi_only",
            "test_rove_out_wcdma_wifi_preferred",
            "test_rove_out_wcdma_wifi_only",
            "test_rove_out_in_stress",

            # WFC Active-Mode Mobility
            # Hand-in, Hand-out test
            "test_hand_out_wifi_only",
            "test_hand_out_wifi_preferred",
            "test_hand_out_in_wifi_preferred",
            "test_hand_in_wifi_preferred",
            "test_hand_in_out_wifi_preferred",
            "test_hand_out_in_stress",

            # WFC test with E4G disabled
            "test_call_epdg_wfc_wifi_preferred_e4g_disabled",
            "test_call_epdg_wfc_wifi_preferred_e4g_disabled_wifi_not_connected",
            "test_call_epdg_wfc_wifi_preferred_e4g_disabled_leave_wifi_coverage",

            # ePDG Active-Mode Mobility: Hand-in, Hand-out test
            "test_hand_out_cellular_preferred",
            "test_hand_in_cellular_preferred",

            # epdg, WFC, cellular weak, WiFi strong
            "test_call_epdg_wfc_wifi_only_wifi_strong_cellular_weak",
            "test_call_epdg_wfc_cellular_preferred_wifi_strong_cellular_weak",

            # epdg, WFC, cellular weak, WiFi weak
            "test_call_epdg_wfc_wifi_only_wifi_weak_cellular_weak",
            "test_call_epdg_wfc_wifi_preferred_wifi_weak_cellular_weak",
            "test_call_epdg_wfc_cellular_preferred_wifi_weak_cellular_weak",

            # epdg, WFC, cellular weak, WiFi Absent
            "test_call_epdg_wfc_wifi_only_wifi_absent_cellular_weak",

            # epdg, WFC, cellular weak, WiFi Disabled
            "test_call_epdg_wfc_wifi_only_wifi_disabled_cellular_weak",

            # epdg, WFC, cellular absent, WiFi strong
            "test_call_epdg_wfc_wifi_only_wifi_strong_cellular_absent",
            "test_call_epdg_wfc_wifi_preferred_wifi_strong_cellular_absent",
            "test_call_epdg_wfc_cellular_preferred_wifi_strong_cellular_absent",

            # epdg, WFC, cellular absent, WiFi weak
            "test_call_epdg_wfc_wifi_only_wifi_weak_cellular_absent",
            "test_call_epdg_wfc_wifi_preferred_wifi_weak_cellular_absent",
            "test_call_epdg_wfc_cellular_preferred_wifi_weak_cellular_absent",

            # epdg, WFC, cellular absent, WiFi Absent
            "test_call_epdg_wfc_wifi_only_wifi_absent_cellular_absent",
            "test_call_epdg_wfc_wifi_preferred_wifi_absent_cellular_absent",
            "test_call_epdg_wfc_cellular_preferred_wifi_absent_cellular_absent",

            # epdg, WFC, cellular absent, WiFi Disabled
            "test_call_epdg_wfc_wifi_only_wifi_disabled_cellular_absent",
            "test_call_epdg_wfc_wifi_preferred_wifi_disabled_cellular_absent",
            "test_call_epdg_wfc_cellular_preferred_wifi_disabled_cellular_absent",

            # epdg, WiFI strong, WFC disabled
            "test_call_epdg_wfc_disabled_wifi_strong_cellular_absent",

            # Below test fail now, because:
            # 1. wifi weak not working now. (phone don't rove-in)
            # 2. wifi-only mode not working now.
            # epdg, WFC, APM, WiFi weak
            "test_call_epdg_wfc_wifi_only_wifi_weak_apm",
            "test_call_epdg_wfc_wifi_preferred_wifi_weak_apm",
            "test_call_epdg_wfc_cellular_preferred_wifi_weak_apm",

            # epdg, WFC, cellular strong, WiFi strong
            "test_call_epdg_wfc_wifi_only_wifi_strong_cellular_strong",

            # epdg, WFC, cellular strong, WiFi weak
            "test_call_epdg_wfc_wifi_only_wifi_weak_cellular_strong",

            # epdg, WFC, cellular strong, WiFi Absent
            "test_call_epdg_wfc_wifi_only_wifi_absent_cellular_strong",

            # epdg, WFC, cellular strong, WiFi Disabled
            "test_call_epdg_wfc_wifi_only_wifi_disabled_cellular_strong",

            # RSSI monitoring
            "test_rssi_monitoring", )

        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.stress_test_number = int(self.user_params["stress_test_number"])
        self.live_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.live_network_pwd = self.user_params["wifi_network_pass"]
        except KeyError:
            self.live_network_pwd = None

        self.attens = {}
        for atten in self.attenuators:
            self.attens[atten.path] = atten

    def setup_class(self):

        super().setup_class()

        self.log.info("WFC phone: <{}> <{}>".format(self.android_devices[
            0].serial, get_phone_number(self.log, self.android_devices[0])))
        self.android_devices[
            0].droid.telephonyStartTrackingSignalStrengthChange()

        # Do WiFi RSSI calibration.
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL], 0,
                 MAX_RSSI_RESERVED_VALUE)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         GEN_4G, voice_or_data=NETWORK_SERVICE_DATA,
                                         toggle_apm_after_setting=True):
            self.log.error("Setup_class: phone failed to select to LTE.")
            return False
        if not ensure_wifi_connected(self.log, self.android_devices[0],
                                     self.live_network_ssid,
                                     self.live_network_pwd):
            self.log.error("{} connect WiFI failed".format(
                self.android_devices[0].serial))
            return False
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False

        # Delay WAIT_TIME_WIFI_RSSI_CALIBRATION_WIFI_CONNECTED after WiFi
        # Connected to make sure WiFi RSSI reported value is correct.
        time.sleep(WAIT_TIME_WIFI_RSSI_CALIBRATION_WIFI_CONNECTED)
        # Turn On Screen and delay WAIT_TIME_WIFI_RSSI_CALIBRATION_SCREEN_ON
        # then get WiFi RSSI to avoid WiFi RSSI report -127(invalid value).
        self.android_devices[0].droid.wakeUpNow()
        time.sleep(WAIT_TIME_WIFI_RSSI_CALIBRATION_SCREEN_ON)

        setattr(self, "wifi_rssi_with_no_atten",
                self.android_devices[0].droid.wifiGetConnectionInfo()['rssi'])
        if self.wifi_rssi_with_no_atten == INVALID_WIFI_RSSI:
            self.log.error(
                "Initial WiFi RSSI calibration value is wrong: -127.")
            return False
        self.log.info("WiFi RSSI calibration info: atten=0, RSSI={}".format(
            self.wifi_rssi_with_no_atten))
        ensure_phones_default_state(self.log, [self.android_devices[0]])

        # Do Cellular RSSI calibration.
        setattr(self, "cell_rssi_with_no_atten", self.android_devices[
            0].droid.telephonyGetSignalStrength()[
                SignalStrengthContainer.SIGNAL_STRENGTH_LTE_DBM])
        self.log.info(
            "Cellular RSSI calibration info: atten=0, RSSI={}".format(
                self.cell_rssi_with_no_atten))
        return True

    def teardown_class(self):

        super().teardown_class()

        self.android_devices[
            0].droid.telephonyStopTrackingSignalStrengthChange()
        return True

    def teardown_test(self):

        super().teardown_test()

        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI], 0,
                 MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL], 0,
                 MAX_RSSI_RESERVED_VALUE)
        return True

    def _wfc_call_sequence(self, ads, mo_mt, initial_wifi_cellular_setup_func,
                           wfc_phone_setup_func, verify_wfc_initial_idle_func,
                           verify_wfc_in_call_state_func,
                           incall_wifi_cellular_setting_check_func,
                           expected_result):
        """_wfc_call_sequence

        Args:
            ads: list of android devices. This list should have 2 ad.
            mo_mt: indicating this call sequence is MO or MT.
                Valid input: DIRECTION_MOBILE_ORIGINATED and
                DIRECTION_MOBILE_TERMINATED.
            initial_wifi_cellular_setup_func: Initial WiFI router and Attenuator
                setup function before phone setup.
            wfc_phone_setup_func: WFC phone setup function.
            verify_wfc_initial_idle_func: Initial WFC phone idle check function.
            verify_wfc_in_call_state_func: WFC phone in call state check function.
            incall_wifi_cellular_setting_check_func: During call, WiFI router and Attenuator
                change setting  and phone status check function.
                (for test hand-in and hand-out)

            expected_result: expected test result.
                If expect call sequence finish, this value should be set to 'True'.
                If expect call sequence not finish (eg. setup fail, call initial fail),
                    this value should be set to "exception string"
                    Current supported string include:
                        "initial_wifi_cellular_setup_func fail."
                        "wfc_phone_setup_func fail."
                        "phone_setup_voice_general fail."
                        "verify_wfc_initial_idle_func fail."
                        "initiate_call fail."
                        "wait_and_answer_call fail."
                        "verify_wfc_in_call_state_func fail."
                        "PhoneB not in call."
                        "verify_wfc_in_call_state_func fail after 30 seconds."
                        "PhoneB not in call after 30 seconds."
                        "incall_wifi_cellular_setting_func fail."
                        "incall_setting_check_func fail."
                        "hangup_call fail."

        Returns:
            if expected_result is True,
                Return True if call sequence finish without exception. Otherwise False.
            if expected_result is string,
                Return True if expected exception happened. Otherwise False.

        """

        class _WfcCallSequenceException(Exception):
            pass

        if (len(ads) != 2) or (mo_mt not in [
                DIRECTION_MOBILE_ORIGINATED, DIRECTION_MOBILE_TERMINATED
        ]):
            self.log.error("Invalid parameters.")
            return False

        if mo_mt == DIRECTION_MOBILE_ORIGINATED:
            ad_caller = ads[0]
            ad_callee = ads[1]
        else:
            ad_caller = ads[1]
            ad_callee = ads[0]
        caller_number = get_phone_number(self.log, ad_caller)
        callee_number = get_phone_number(self.log, ad_callee)

        self.log.info("-->Begin wfc_call_sequence: {} to {}<--".format(
            caller_number, callee_number))

        try:
            # initial setup wifi router and RF
            if initial_wifi_cellular_setup_func and not initial_wifi_cellular_setup_func(
            ):
                raise _WfcCallSequenceException(
                    "initial_wifi_cellular_setup_func fail.")

            if wfc_phone_setup_func and not wfc_phone_setup_func():
                raise _WfcCallSequenceException("wfc_phone_setup_func fail.")
            if not phone_setup_voice_general(self.log, ads[1]):
                raise _WfcCallSequenceException(
                    "phone_setup_voice_general fail.")
            time.sleep(WAIT_TIME_BETWEEN_REG_AND_CALL)

            # Ensure idle status correct
            if verify_wfc_initial_idle_func and not verify_wfc_initial_idle_func(
            ):
                raise _WfcCallSequenceException(
                    "verify_wfc_initial_idle_func fail.")

            # Make MO/MT call.
            if not initiate_call(self.log, ad_caller, callee_number):
                raise _WfcCallSequenceException("initiate_call fail.")
            if not wait_and_answer_call(self.log, ad_callee, caller_number):
                raise _WfcCallSequenceException("wait_and_answer_call fail.")
            time.sleep(1)

            # Check state, wait 30 seconds, check again.
            if verify_wfc_in_call_state_func and not verify_wfc_in_call_state_func(
            ):
                raise _WfcCallSequenceException(
                    "verify_wfc_in_call_state_func fail.")
            if is_phone_not_in_call(self.log, ads[1]):
                raise _WfcCallSequenceException("PhoneB not in call.")
            time.sleep(WAIT_TIME_IN_CALL)
            if verify_wfc_in_call_state_func and not verify_wfc_in_call_state_func(
            ):
                raise _WfcCallSequenceException(
                    "verify_wfc_in_call_state_func fail after 30 seconds.")
            if is_phone_not_in_call(self.log, ads[1]):
                raise _WfcCallSequenceException(
                    "PhoneB not in call after 30 seconds.")

            # in call change setting and check
            if incall_wifi_cellular_setting_check_func and not incall_wifi_cellular_setting_check_func(
            ):
                raise _WfcCallSequenceException(
                    "incall_wifi_cellular_setting_check_func fail.")

            if is_phone_in_call(self.log, ads[0]):
                # hangup call
                if not hangup_call(self.log, ads[0]):
                    raise _WfcCallSequenceException("hangup_call fail.")
            else:
                # Call drop is unexpected if
                # incall_wifi_cellular_setting_check_func is None
                if incall_wifi_cellular_setting_check_func is None:
                    raise _WfcCallSequenceException("Unexpected call drop.")

        except _WfcCallSequenceException as e:
            if str(e) == expected_result:
                self.log.info(
                    "Expected exception happened: <{}>, return True.".format(
                        e))
                return True
            else:
                self.log.info(
                    "Unexpected exception happened: <{}>, return False.".format(
                        e))
                return False
        finally:
            ensure_phones_default_state(self.log, [ads[0], ads[1]])

        self.log.info("wfc_call_sequence finished, return {}".format(
            expected_result is True))
        return (expected_result is True)

    def _phone_idle_iwlan(self):
        return phone_idle_iwlan(self.log, self.android_devices[0])

    def _phone_idle_not_iwlan(self):
        return not self._phone_idle_iwlan()

    def _phone_idle_volte(self):
        return phone_idle_volte(self.log, self.android_devices[0])

    def _phone_idle_csfb(self):
        return phone_idle_csfb(self.log, self.android_devices[0])

    def _phone_idle_3g(self):
        return phone_idle_3g(self.log, self.android_devices[0])

    def _phone_wait_for_not_wfc(self):
        result = wait_for_wfc_disabled(self.log, self.android_devices[0],
                                       MAX_WAIT_TIME_NW_SELECTION)
        self.log.info("_phone_wait_for_not_wfc: WFC_disabled is {}".format(
            result))
        if not result:
            return False
        # TODO: b/26338343 Need to check Data RAT. Data RAT should not be iwlan.
        return True

    def _phone_wait_for_wfc(self):
        result = wait_for_wfc_enabled(self.log, self.android_devices[0],
                                      MAX_WAIT_TIME_NW_SELECTION)
        self.log.info("_phone_wait_for_wfc: WFC_enabled is {}".format(result))
        if not result:
            return False
        nw_type = get_network_rat(self.log, self.android_devices[0],
                                  NETWORK_SERVICE_DATA)
        if nw_type != RAT_IWLAN:
            self.log.error(
                "_phone_wait_for_wfc Data Rat is {}, expecting {}".format(
                    nw_type, RAT_IWLAN))
            return False
        return True

    def _phone_wait_for_call_drop(self):
        if not wait_for_droid_not_in_call(self.log, self.android_devices[0],
                                          MAX_WAIT_TIME_CALL_DROP):
            self.log.info("_phone_wait_for_call_drop: Call not drop.")
            return False
        return True

    def _is_phone_in_call_iwlan(self):
        return is_phone_in_call_iwlan(self.log, self.android_devices[0])

    def _is_phone_in_call_not_iwlan(self):
        return is_phone_in_call_not_iwlan(self.log, self.android_devices[0])

    def _is_phone_not_in_call(self):
        if is_phone_in_call(self.log, self.android_devices[0]):
            self.log.info("{} in call.".format(self.android_devices[0].serial))
            return False
        self.log.info("{} not in call.".format(self.android_devices[0].serial))
        return True

    def _is_phone_in_call_volte(self):
        return is_phone_in_call_volte(self.log, self.android_devices[0])

    def _is_phone_in_call_3g(self):
        return is_phone_in_call_3g(self.log, self.android_devices[0])

    def _is_phone_in_call_csfb(self):
        return is_phone_in_call_csfb(self.log, self.android_devices[0])

    def _wfc_phone_setup(self, is_airplane_mode, wfc_mode, volte_mode=True):
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        toggle_volte(self.log, self.android_devices[0], volte_mode)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
            return False

        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        toggle_airplane_mode(self.log, self.android_devices[0],
                             is_airplane_mode)

        if not ensure_wifi_connected(self.log, self.android_devices[0],
                                     self.live_network_ssid,
                                     self.live_network_pwd):
            self.log.error("{} connect WiFI failed".format(
                self.android_devices[0].serial))
            return False
        return True

    def _wfc_phone_setup_cellular_absent(self, wfc_mode):
        is_exception_happened = False
        try:
            if not toggle_airplane_mode(self.log, self.android_devices[0],
                                        False):
                raise Exception("Toggle APM failed.")
            if not ensure_network_generation(self.log, self.android_devices[0],
                                             GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
                raise Exception("Ensure LTE failed.")
        except Exception:
            is_exception_happened = True

        if not is_exception_happened:
            self.log.error(
                "_wfc_phone_setup_cellular_absent error:"
                "Phone on LTE, expected Phone have no cellular signal")
            return False
        if not toggle_volte(self.log, self.android_devices[0], True):
            self.log.error(
                "_wfc_phone_setup_cellular_absent: toggle VoLTE fail.")
            raise False

        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        if not ensure_wifi_connected(self.log, self.android_devices[0],
                                     self.live_network_ssid,
                                     self.live_network_pwd):
            self.log.error("{} connect WiFI failed".format(
                self.android_devices[0].serial))
            return False
        return True

    def _wfc_phone_setup_apm_wifi_only(self):
        return self._wfc_phone_setup(True, WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_apm_wifi_preferred(self):
        return self._wfc_phone_setup(True, WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_apm_cellular_preferred(self):
        return self._wfc_phone_setup(True, WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_apm_wfc_disabled(self):
        return self._wfc_phone_setup(True, WFC_MODE_DISABLED)

    def _wfc_phone_setup_wifi_only(self):
        return self._wfc_phone_setup(False, WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_wifi_preferred(self):
        return self._wfc_phone_setup(False, WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_cellular_preferred(self):
        return self._wfc_phone_setup(False, WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_wfc_disabled(self):
        return self._wfc_phone_setup(False, WFC_MODE_DISABLED)

    def _wfc_phone_setup_cellular_absent_wifi_only(self):
        return self._wfc_phone_setup_cellular_absent(WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_cellular_absent_wifi_preferred(self):
        return self._wfc_phone_setup_cellular_absent(WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_cellular_absent_cellular_preferred(self):
        return self._wfc_phone_setup_cellular_absent(
            WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_cellular_absent_wfc_disabled(self):
        return self._wfc_phone_setup_cellular_absent(WFC_MODE_DISABLED)

    def _wfc_phone_setup_wifi_preferred_e4g_disabled(self):
        return self._wfc_phone_setup(False, WFC_MODE_WIFI_PREFERRED, False)

    def _wfc_phone_setup_wifi_absent(self,
                                     is_airplane_mode,
                                     wfc_mode,
                                     volte_mode=True):
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        toggle_volte(self.log, self.android_devices[0], volte_mode)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
            return False

        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        toggle_airplane_mode(self.log, self.android_devices[0],
                             is_airplane_mode)

        if ensure_wifi_connected(self.log, self.android_devices[0],
                                 self.live_network_ssid,
                                 self.live_network_pwd):
            self.log.error(
                "{} connect WiFI succeed, expected not succeed".format(
                    self.android_devices[0].serial))
            return False
        return True

    def _wfc_phone_setup_cellular_absent_wifi_absent(self, wfc_mode):
        is_exception_happened = False
        try:
            if not toggle_airplane_mode(self.log, self.android_devices[0],
                                        False):
                raise Exception("Toggle APM failed.")
            if not ensure_network_generation(self.log, self.android_devices[0],
                                             GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
                raise Exception("Ensure LTE failed.")
        except Exception:
            is_exception_happened = True

        if not is_exception_happened:
            self.log.error(
                "_wfc_phone_setup_cellular_absent_wifi_absent error:"
                "Phone on LTE, expected Phone have no cellular signal")
            return False
        if not toggle_volte(self.log, self.android_devices[0], True):
            self.log.error(
                "_wfc_phone_setup_cellular_absent: toggle VoLTE fail.")
            raise False

        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        if ensure_wifi_connected(self.log, self.android_devices[0],
                                 self.live_network_ssid,
                                 self.live_network_pwd):
            self.log.error(
                "{} connect WiFI succeed, expected not succeed".format(
                    self.android_devices[0].serial))
            return False
        return True

    def _wfc_phone_setup_apm_wifi_absent_wifi_only(self):
        return self._wfc_phone_setup_wifi_absent(True, WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_apm_wifi_absent_wifi_preferred(self):
        return self._wfc_phone_setup_wifi_absent(True, WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_apm_wifi_absent_cellular_preferred(self):
        return self._wfc_phone_setup_wifi_absent(True,
                                                 WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_wifi_absent_wifi_only(self):
        return self._wfc_phone_setup_wifi_absent(False, WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_wifi_absent_wifi_preferred(self):
        return self._wfc_phone_setup_wifi_absent(False,
                                                 WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_wifi_absent_cellular_preferred(self):
        return self._wfc_phone_setup_wifi_absent(False,
                                                 WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_cellular_absent_wifi_absent_wifi_only(self):
        return self._wfc_phone_setup_cellular_absent_wifi_absent(
            WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_cellular_absent_wifi_absent_wifi_preferred(self):
        return self._wfc_phone_setup_cellular_absent_wifi_absent(
            WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_cellular_absent_wifi_absent_cellular_preferred(self):
        return self._wfc_phone_setup_cellular_absent_wifi_absent(
            WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_wifi_absent_wifi_preferred_e4g_disabled(self):
        return self._wfc_phone_setup_wifi_absent(
            False, WFC_MODE_WIFI_PREFERRED, False)

    def _wfc_phone_setup_wifi_disabled(self, is_airplane_mode, wfc_mode):
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        toggle_volte(self.log, self.android_devices[0], True)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
            return False

        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        toggle_airplane_mode(self.log, self.android_devices[0],
                             is_airplane_mode)

        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        return True

    def _wfc_phone_setup_cellular_absent_wifi_disabled(self, wfc_mode):
        is_exception_happened = False
        try:
            if not toggle_airplane_mode(self.log, self.android_devices[0],
                                        False):
                raise Exception("Toggle APM failed.")
            if not ensure_network_generation(self.log, self.android_devices[0],
                                             GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
                raise Exception("Ensure LTE failed.")
        except Exception:
            is_exception_happened = True

        if not is_exception_happened:
            self.log.error(
                "_wfc_phone_setup_cellular_absent_wifi_disabled error:"
                "Phone on LTE, expected Phone have no cellular signal")
            return False
        if not toggle_volte(self.log, self.android_devices[0], True):
            self.log.error(
                "_wfc_phone_setup_cellular_absent: toggle VoLTE fail.")
            raise False

        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        WifiUtils.wifi_toggle_state(self.log, self.android_devices[0], False)
        return True

    def _wfc_phone_setup_apm_wifi_disabled_wifi_only(self):
        return self._wfc_phone_setup_wifi_disabled(True, WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_apm_wifi_disabled_wifi_preferred(self):
        return self._wfc_phone_setup_wifi_disabled(True,
                                                   WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_apm_wifi_disabled_cellular_preferred(self):
        return self._wfc_phone_setup_wifi_disabled(True,
                                                   WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_wifi_disabled_wifi_only(self):
        return self._wfc_phone_setup_wifi_disabled(False, WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_wifi_disabled_wifi_preferred(self):
        return self._wfc_phone_setup_wifi_disabled(False,
                                                   WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_wifi_disabled_cellular_preferred(self):
        return self._wfc_phone_setup_wifi_disabled(False,
                                                   WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_phone_setup_cellular_absent_wifi_disabled_wifi_only(self):
        return self._wfc_phone_setup_cellular_absent_wifi_disabled(
            WFC_MODE_WIFI_ONLY)

    def _wfc_phone_setup_cellular_absent_wifi_disabled_wifi_preferred(self):
        return self._wfc_phone_setup_cellular_absent_wifi_disabled(
            WFC_MODE_WIFI_PREFERRED)

    def _wfc_phone_setup_cellular_absent_wifi_disabled_cellular_preferred(
            self):
        return self._wfc_phone_setup_cellular_absent_wifi_disabled(
            WFC_MODE_CELLULAR_PREFERRED)

    def _wfc_set_wifi_strong_cell_strong(self):
        self.log.info("--->Setting WiFi strong cell strong<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        return True

    def _wfc_set_wifi_strong_cell_weak(self):
        self.log.info("--->Setting WiFi strong cell weak<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, CELL_WEAK_RSSI_VALUE)
        return True

    def _wfc_set_wifi_strong_cell_absent(self):
        self.log.info("--->Setting WiFi strong cell absent<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        return True

    def _wfc_set_wifi_weak_cell_strong(self):
        self.log.info("--->Setting WiFi weak cell strong<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, WIFI_WEAK_RSSI_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        return True

    def _wfc_set_wifi_weak_cell_weak(self):
        self.log.info("--->Setting WiFi weak cell weak<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, CELL_WEAK_RSSI_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, WIFI_WEAK_RSSI_VALUE)
        return True

    def _wfc_set_wifi_weak_cell_absent(self):
        self.log.info("--->Setting WiFi weak cell absent<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, WIFI_WEAK_RSSI_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        return True

    def _wfc_set_wifi_absent_cell_strong(self):
        self.log.info("--->Setting WiFi absent cell strong<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        return True

    def _wfc_set_wifi_absent_cell_weak(self):
        self.log.info("--->Setting WiFi absent cell weak<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, CELL_WEAK_RSSI_VALUE)
        return True

    def _wfc_set_wifi_absent_cell_absent(self):
        self.log.info("--->Setting WiFi absent cell absent<---")
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_strong_apm(self):
        """ Test WFC MO MT, WiFI only mode, WIFI Strong, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """

        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_strong_apm(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI Strong, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """

        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_strong_apm(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI Strong, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """

        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_weak_apm(self):
        """ Test WFC MO MT, WiFI only mode, WIFI weak, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """

        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_apm_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_apm_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_weak_apm(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI weak, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """

        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_apm_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_apm_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_weak_apm(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI weak, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """

        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_apm_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_apm_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_absent_apm(self):
        """ Test WFC MO MT, WiFI only mode, WIFI absent, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_apm_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_apm_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_absent_apm(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI absent, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_apm_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_apm_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_absent_apm(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI absent, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_apm_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_apm_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_disabled_apm(self):
        """ Test WFC MO MT, WiFI only mode, WIFI disabled, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_disabled_apm(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI disabled, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_disabled_apm(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI disabled, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_strong_cellular_strong(self):
        """ Test WFC MO MT, WiFI only mode, WIFI strong, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_strong_cellular_strong(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI strong, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_strong_cellular_strong(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI strong, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_weak_cellular_strong(self):
        """ Test WFC MO MT, WiFI only mode, WIFI weak, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PHoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_weak_cellular_strong(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI weak, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_not_iwlan,
            self._is_phone_in_call_not_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_not_iwlan,
            self._is_phone_in_call_not_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_weak_cellular_strong(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI strong, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_absent_cellular_strong(self):
        """ Test WFC MO MT, WiFI only mode, WIFI absent, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_absent_cellular_strong(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI absent, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_absent_cellular_strong(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI absent, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_disabled_cellular_strong(self):
        """ Test WFC MO MT, WiFI only mode, WIFI disabled, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PHoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_disabled_cellular_strong(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI disabled, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_disabled_cellular_strong(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI disabled, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_strong_cellular_weak(self):
        """ Test WFC MO MT, WiFI only mode, WIFI strong, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_strong_cellular_weak(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI strong, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_strong_cellular_weak(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI strong, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_cellular_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_cellular_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_weak_cellular_weak(self):
        """ Test WFC MO MT, WiFI only mode, WIFI weak, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_weak, self._wfc_phone_setup_wifi_only,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_weak, self._wfc_phone_setup_wifi_only,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_weak_cellular_weak(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI weak, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_weak,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_weak,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_weak_cellular_weak(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI weak, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PhoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_weak,
            self._wfc_phone_setup_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_weak,
            self._wfc_phone_setup_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_absent_cellular_weak(self):
        """ Test WFC MO MT, WiFI only mode, WIFI absent, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_weak,
            self._wfc_phone_setup_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_weak,
            self._wfc_phone_setup_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_absent_cellular_weak(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI absent, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PhoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_weak,
            self._wfc_phone_setup_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_weak,
            self._wfc_phone_setup_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_absent_cellular_weak(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI absent, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PhoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_weak,
            self._wfc_phone_setup_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_weak,
            self._wfc_phone_setup_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_disabled_cellular_weak(self):
        """ Test WFC MO MT, WiFI only mode, WIFI disabled, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_disabled_cellular_weak(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI disabled, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PhoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_disabled_cellular_weak(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI disabled, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PhoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_in_call_not_iwlan, None,
            True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_strong_cellular_absent(self):
        """ Test WFC MO MT, WiFI only mode, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_only,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_only,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_strong_cellular_absent(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_strong_cellular_absent(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_weak_cellular_absent(self):
        """ Test WFC MO MT, WiFI only mode, WIFI weak, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_only,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_only,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_weak_cellular_absent(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI weak, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_weak_cellular_absent(self):
        """ Test WFC MO MT, cellular preferred mode, WIFI weak, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on WiFi.
        Call from PhoneB to PhoneA, call should succeed, call should on WiFi.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_weak_cell_absent,
            self._wfc_phone_setup_cellular_absent_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_weak_cell_absent,
            self._wfc_phone_setup_cellular_absent_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_absent_cellular_absent(self):
        """ Test WFC MO MT, WiFI only mode, WIFI absent, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_absent_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_absent_cellular_absent(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI absent, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_absent_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_absent_cellular_absent(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI absent, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_absent_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_absent_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_absent_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_only_wifi_disabled_cellular_absent(self):
        """ Test WFC MO MT, WiFI only mode, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_disabled_wifi_only,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_wifi_disabled_cellular_absent(self):
        """ Test WFC MO MT, WiFI preferred mode, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_disabled_wifi_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_cellular_preferred_wifi_disabled_cellular_absent(
            self):
        """ Test WFC MO MT, cellular preferred mode, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wifi_disabled_cellular_preferred,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_disabled_wifi_strong_cellular_strong(self):
        """ Test WFC MO MT, WFC disabled, WIFI strong, Cellular strong

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wfc_disabled, self._phone_idle_not_iwlan,
            self._is_phone_in_call_not_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wfc_disabled, self._phone_idle_not_iwlan,
            self._is_phone_in_call_not_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_disabled_wifi_strong_cellular_weak(self):
        """ Test WFC MO MT, WFC disabled, WIFI strong, Cellular weak

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should succeed, call should on Cellular.
        Call from PhoneB to PHoneA, call should succeed, call should on Cellular.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wfc_disabled, self._phone_idle_not_iwlan,
            self._is_phone_in_call_not_iwlan, None, True)

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_weak,
            self._wfc_phone_setup_wfc_disabled, self._phone_idle_not_iwlan,
            self._is_phone_in_call_not_iwlan, None, True)

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_disabled_wifi_strong_cellular_absent(self):
        """ Test WFC MO MT, WFC disabled, WIFI strong, Cellular absent

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wfc_disabled,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_wfc_disabled,
            self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
            "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_disabled_wifi_strong_apm(self):
        """ Test WFC MO MT, WFC disabled, WIFI strong, Phone in APM

        Set WiFi/Cellular network environment.
        Make Sure PhoneA is set correct WFC parameters.
        Make SUre PhoneB is able to make MO/MT call.
        Call from PhoneA to PhoneB, call should fail.
        Call from PhoneB to PhoneA, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        mo_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_ORIGINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wfc_disabled, self._phone_idle_not_iwlan,
            self._is_phone_not_in_call, None, "initiate_call fail.")

        mt_result = self._wfc_call_sequence(
            ads, DIRECTION_MOBILE_TERMINATED,
            self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_apm_wfc_disabled, self._phone_idle_not_iwlan,
            self._is_phone_not_in_call, None, "wait_and_answer_call fail.")

        self.log.info("MO: {}, MT: {}".format(mo_result, mt_result))
        return ((mo_result is True) and (mt_result is True))

    def _rove_in_test(self, cellular_gen, wfc_mode):
        """Test utility for Rove-in Tests.

        Cellular strong, WiFi RSSI < -100 dBm.
        Setup Cellular network and wfc_mode, WiFi enabled but not associated.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN in 10s,
            PhoneA does not rove-in.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        Make WFC call.
        """
        self._wfc_set_wifi_absent_cell_strong()
        # ensure cellular rat, wfc mode, wifi not associated
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        toggle_volte(self.log, self.android_devices[0], True)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         cellular_gen, voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("_rove_in_test: {} failed to be in rat: {}".format(
                self.android_devices[0].serial, cellular_rat))
            return False
        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False

        if ensure_wifi_connected(self.log, self.android_devices[0],
                                 self.live_network_ssid,
                                 self.live_network_pwd):
            self.log.error(
                "{} connect WiFI succeed, expected not succeed".format(
                    self.android_devices[0].serial))
            return False

        # set up wifi to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN in 10 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN, 5, 1)
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False

        if self._phone_idle_iwlan():
            self.log.error("Phone should not report iwlan in WiFi {}Bm".format(
                WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN))
            return False

        # set up wifi to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN, 1, 1)
        if not self._phone_idle_iwlan():
            self.log.error("Phone should report iwlan in WiFi {}dBm".format(
                WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN))
            return False

        # make a wfc call.
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, None, None, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, None, True)

    def _rove_out_test(self, cellular_gen, wfc_mode):
        """Test utility for Rove-out Tests.

        Cellular strong, WiFi RSSI WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE.
        Setup Cellular network and wfc_mode, WiFi associated.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT in 10s,
            PhoneA does not rove-out.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        Make a call.
        """
        # set up cell strong
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, MAX_RSSI_RESERVED_VALUE)
        # set up wifi WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE)
        # ensure cellular rat, wfc mode, wifi associated
        toggle_airplane_mode(self.log, self.android_devices[0], False)
        toggle_volte(self.log, self.android_devices[0], True)
        if not ensure_network_generation(self.log, self.android_devices[0],
                                         GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
            self.log.error("_rove_out_test: {} failed to be in rat: {}".format(
                self.android_devices[0].serial, cellular_rat))
            return False
        if not set_wfc_mode(self.log, self.android_devices[0], wfc_mode):
            self.log.error("{} set WFC mode failed.".format(
                self.android_devices[0].serial))
            return False
        if not ensure_wifi_connected(self.log, self.android_devices[0],
                                     self.live_network_ssid,
                                     self.live_network_pwd):
            self.log.error("{} connect WiFI failed, expected succeed".format(
                self.android_devices[0].serial))
            return False
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        if not self._phone_idle_iwlan():
            self.log.error("Phone failed to report iwlan in {}dBm.".format(
                WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE))
            return False

        # set up wifi to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT in 10 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT, 1, 1)
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        if self._phone_wait_for_not_wfc() or not self._phone_idle_iwlan():
            self.log.error("Phone should not rove-out in {}dBm.".format(
                WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT))
            return False

        # set up wifi to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT, 2, 1)
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False

        if not self._phone_wait_for_not_wfc() or self._phone_idle_iwlan():
            self.log.error("Phone should rove-out in {}dBm.".format(
                WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT))
            return False
        # make a call.
        if wfc_mode == WFC_MODE_WIFI_ONLY:
            return self._wfc_call_sequence(
                [self.android_devices[0], self.android_devices[1]],
                DIRECTION_MOBILE_ORIGINATED, None, None,
                self._phone_idle_not_iwlan, self._is_phone_not_in_call, None,
                "initiate_call fail.")
        elif wfc_mode == WFC_MODE_WIFI_PREFERRED:
            if cellular_gen == GEN_4G:
                return self._wfc_call_sequence(
                    [self.android_devices[0], self.android_devices[1]],
                    DIRECTION_MOBILE_ORIGINATED, None, None,
                    self._phone_idle_volte, self._is_phone_in_call_volte, None,
                    True)
            else:
                return self._wfc_call_sequence(
                    [self.android_devices[0], self.android_devices[1]],
                    DIRECTION_MOBILE_ORIGINATED, None, None,
                    self._phone_idle_3g, self._is_phone_in_call_3g, None, True)
        else:
            return False

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_out_in_stress(self):
        """WiFi Calling Rove out/in stress test.

        Steps:
        1. PhoneA on LTE, VoLTE enabled.
        2. PhoneA WFC mode WiFi preferred, WiFi associated.
        3. Cellular strong, WiFi RSSI WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE.
        4. Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        5. Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        6. Repeat Step 4~5.

        Expected Results:
        4. Phone should rove out.
        5. Phone should rove in.
        6. Stress test pass rate should be higher than pre-defined limit.
        """
        self._wfc_set_wifi_strong_cell_strong()
        if not self._wfc_phone_setup_wifi_preferred():
            self.log.error("Setup WFC failed.")
            return False
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        if not self._phone_idle_iwlan():
            self.log.error("Phone failed to report iwlan in {}dBm.".format(
                WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE))
            return False
        total_iteration = self.stress_test_number
        self.log.info(
            "Rove_out/Rove_in stress test. Total iteration = {}.".format(
                total_iteration))
        current_iteration = 1
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = {}/{}----<".format(
                current_iteration, total_iteration))

            # set up wifi to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10 seconds
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT, 2, 1)
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            if not self._phone_wait_for_not_wfc():
                self.log.error("Phone should rove-out in {}dBm.".format(
                    WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT))
                break
            self.log.info("Rove-out succeed.")
            # set up wifi to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10 seconds
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN, 2, 1)
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            if not self._phone_wait_for_wfc():
                self.log.error("Phone should rove-in in {}dBm.".format(
                    WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN))
                break
            self.log.info("Rove-in succeed.")

            self.log.info(">----Iteration : {}/{} succeed.----<".format(
                current_iteration, total_iteration))
            current_iteration += 1
        if current_iteration <= total_iteration:
            self.log.info(">----Iteration : {}/{} failed.----<".format(
                current_iteration, total_iteration))
            return False
        else:
            return True

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_in_out_stress(self):
        """WiFi Calling Rove in/out stress test.

        Steps:
        1. PhoneA on LTE, VoLTE enabled.
        2. PhoneA WFC mode WiFi preferred, WiFi associated.
        3. Cellular strong, WiFi RSSI weak.
        4. Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        5. Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        6. Repeat Step 4~5.

        Expected Results:
        4. Phone should rove in.
        5. Phone should rove out.
        6. Stress test pass rate should be higher than pre-defined limit.
        """
        self._wfc_set_wifi_weak_cell_strong()
        # ensure cellular rat, wfc mode, wifi not associated
        if not self._wfc_phone_setup_wifi_preferred():
            self.log.error("Failed to setup for rove_in_out_stress")
            return False
        total_iteration = self.stress_test_number
        self.log.info(
            "Rove_in/Rove_out stress test. Total iteration = {}.".format(
                total_iteration))
        current_iteration = 1
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = {}/{}----<".format(
                current_iteration, total_iteration))

            # set up wifi to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10 seconds
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN, 2, 1)
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            if not self._phone_wait_for_wfc():
                self.log.error("Phone should rove-in in {}dBm.".format(
                    WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN))
                break
            self.log.info("Rove-in succeed.")

            # set up wifi to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10 seconds
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT, 2, 1)
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            if not self._phone_wait_for_not_wfc():
                self.log.error("Phone should rove-out in {}dBm.".format(
                    WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT))
                break
            self.log.info("Rove-out succeed.")

            self.log.info(">----Iteration : {}/{} succeed.----<".format(
                current_iteration, total_iteration))
            current_iteration += 1
        if current_iteration <= total_iteration:
            self.log.info(">----Iteration : {}/{} failed.----<".format(
                current_iteration, total_iteration))
            return False
        else:
            return True

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_in_lte_wifi_preferred(self):
        """ Test WFC rove-in features.

        PhoneA on LTE, VoLTE enabled.
        PhoneA WFC mode WiFi preferred, WiFi enabled but not associated.
        Cellular strong, WiFi RSSI < -100 dBm.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN in 10s,
            PhoneA does not rove-in.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        Make WFC call.

        Returns:
            True if pass; False if fail.
        """
        return self._rove_in_test(GEN_4G, WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_in_lte_wifi_only(self):
        """ Test WFC rove-in features.

        PhoneA on LTE, VoLTE enabled.
        PhoneA WFC mode WiFi only, WiFi enabled but not associated.
        Cellular strong, WiFi RSSI < -100 dBm.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN in 10s,
            PhoneA does not rove-in.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        Make WFC call.

        Returns:
            True if pass; False if fail.
        """
        ###########
        return self._rove_in_test(GEN_4G, WFC_MODE_WIFI_ONLY)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_in_wcdma_wifi_preferred(self):
        """ Test WFC rove-in features.

        PhoneA on WCDMA, VoLTE enabled.
        PhoneA WFC mode WiFi preferred, WiFi enabled but not associated.
        Cellular strong, WiFi RSSI < -100 dBm.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN in 10s,
            PhoneA does not rove-in.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        Make WFC call.

        Returns:
            True if pass; False if fail.
        """
        return self._rove_in_test(GEN_3G, WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_in_wcdma_wifi_only(self):
        """ Test WFC rove-in features.

        PhoneA on WCDMA, VoLTE enabled.
        PhoneA WFC mode WiFi only, WiFi enabled but not associated.
        Cellular strong, WiFi RSSI < -100 dBm.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_NOT_ROVE_IN in 10s,
            PhoneA does not rove-in.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_IN_TEST_PHONE_ROVE_IN in 10s,
            PhoneA rove-in.
        Make WFC call.

        Returns:
            True if pass; False if fail.
        """
        ###########
        return self._rove_in_test(GEN_3G, WFC_MODE_WIFI_ONLY)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_out_lte_wifi_preferred(self):
        """ Test WFC rove-out features.

        PhoneA on LTE, VoLTE enabled.
        PhoneA WFC mode WiFi preferred, WiFi associated.
        Cellular strong, WiFi RSSI WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT in 10s,
            PhoneA does not rove-out.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        Make a call, call should succeed by cellular VoLTE.

        Returns:
            True if pass; False if fail.
        """
        return self._rove_out_test(GEN_4G, WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_out_lte_wifi_only(self):
        """ Test WFC rove-out features.

        PhoneA on LTE, VoLTE enabled.
        PhoneA WFC mode WiFi only, WiFi associated.
        Cellular strong, WiFi RSSI WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT in 10s,
            PhoneA does not rove-out.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        Make a call, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        return self._rove_out_test(GEN_4G, WFC_MODE_WIFI_ONLY)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_out_wcdma_wifi_preferred(self):
        """ Test WFC rove-out features.

        PhoneA on WCDMA, VoLTE enabled.
        PhoneA WFC mode WiFi preferred, WiFi associated.
        Cellular strong, WiFi RSSI WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT in 10s,
            PhoneA does not rove-out.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        Make a call, call should succeed by cellular 3g.

        Returns:
            True if pass; False if fail.
        """
        return self._rove_out_test(GEN_3G, WFC_MODE_WIFI_PREFERRED)

    @TelephonyBaseTest.tel_test_wrap
    def test_rove_out_wcdma_wifi_only(self):
        """ Test WFC rove-out features.

        PhoneA on WCDMA, VoLTE enabled.
        PhoneA WFC mode WiFi only, WiFi associated.
        Cellular strong, WiFi RSSI WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_INITIAL_STATE.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_NOT_ROVE_OUT in 10s,
            PhoneA does not rove-out.
        Set WiFI RSSI to WIFI_RSSI_FOR_ROVE_OUT_TEST_PHONE_ROVE_OUT in 10s,
            PhoneA rove-out.
        Make a call, call should fail.

        Returns:
            True if pass; False if fail.
        """
        ###########
        return self._rove_out_test(GEN_3G, WFC_MODE_WIFI_ONLY)

    def _increase_wifi_rssi_check_phone_hand_in(self):
        """Private Test utility for hand_in test.

        Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN in 10s.
        PhoneA should connect to WiFi and have data on WiFi.
        PhoneA should not hand-in to iwlan.
        Increase WiFi RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s.
        PhoneA should hand-in to iwlan.
        PhoneA call should remain active.
        """
        # Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN in 10s
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN, 5, 1)
        # Make sure WiFI connected and data OK.
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        # Make sure phone not hand in to iwlan.
        if self._phone_wait_for_wfc():
            self.log.error("Phone hand-in to wfc.")
            return False
        # Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN, 2, 1)
        # Make sure phone hand in to iwlan.
        if not self._phone_wait_for_wfc():
            self.log.error("Phone failed to hand-in to wfc.")
            return False
        if self._is_phone_not_in_call():
            self.log.error("Phone call dropped.")
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_in_wifi_preferred(self):
        """WiFi Hand-In Threshold - WiFi Preferred

        PhoneA on LTE, VoLTE enabled, WFC WiFi preferred. WiFI not associated.
        Cellular Strong, WiFi <-100 dBm
        Call from PhoneA to PhoneB, PhoneA should be on VoLTE.
        Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN in 10s.
        PhoneA should connect to WiFi and have data on WiFi.
        PhoneA should not hand-in to iwlan.
        Increase WiFi RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s.
        PhoneA should hand-in to iwlan.
        PhoneA call should remain active.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_preferred,
            self._phone_idle_volte, self._is_phone_in_call_volte,
            self._increase_wifi_rssi_check_phone_hand_in, True)

    def _increase_wifi_rssi_hand_in_and_decrease_wifi_rssi_hand_out(self):
        if not self._increase_wifi_rssi_check_phone_hand_in():
            return False
        if not self._decrease_wifi_rssi_check_phone_hand_out():
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_in_out_wifi_preferred(self):
        """WiFi Hand-In-Out Threshold - WiFi Preferred

        PhoneA on LTE, VoLTE enabled, WFC WiFi preferred. WiFI not associated.
        Cellular Strong, WiFi <-100 dBm
        Call from PhoneA to PhoneB, PhoneA should be on VoLTE.

        Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN in 10s.
        PhoneA should connect to WiFi and have data on WiFi.
        PhoneA should not hand-in to iwlan.
        Increase WiFi RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s.
        PhoneA should hand-in to iwlan.
        PhoneA call should remain active.

        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT, in 10s.
        PhoneA should still be in call.
        PhoneA should not hand-out, PhoneA should have data on WiFi.
        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT, in 10s.
        PhoneA should still be in call. PhoneA should hand-out to LTE.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_preferred,
            self._phone_idle_volte, self._is_phone_in_call_volte,
            self._increase_wifi_rssi_hand_in_and_decrease_wifi_rssi_hand_out,
            True)

    def _decrease_lte_rssi_check_phone_not_hand_in(self):
        """Private Test utility for hand_in test.

        Decrease Cellular RSSI to CELL_WEAK_RSSI_VALUE in 30s.
        PhoneA should not hand-in to WFC.
        PhoneA should either drop or hands over to 3g/2g.
        """
        # Decrease LTE RSSI to CELL_WEAK_RSSI_VALUE in 30 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, CELL_WEAK_RSSI_VALUE, 1, 1)
        # Make sure phone not hand in to iwlan.
        if self._phone_wait_for_wfc():
            self.log.error("Phone hand-in to wfc.")
            return False

        if is_phone_not_in_call(self.log, self.android_devices[0]):
            self.log.info("Call drop.")
            return True
        if self._is_phone_in_call_csfb():
            self.log.info("Call hands over to 2g/3g.")
            return True
        return False

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_in_cellular_preferred(self):
        """WiFi Hand-In Not Attempted - Cellular Preferred

        PhoneA on LTE, VoLTE enabled, WFC cellular preferred. WiFI associated.
        Cellular and WiFi signal strong.
        Call from PhoneA to PhoneB, PhoneA should be on VoLTE.
        Decrease Cellular RSSI to CELL_WEAK_RSSI_VALUE in 30s.
        PhoneA should not hand-in to WFC.
        PhoneA should either drop or hands over to 3g/2g.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_cellular_preferred, self._phone_idle_volte,
            self._is_phone_in_call_volte,
            self._decrease_lte_rssi_check_phone_not_hand_in, True)

    def _decrease_wifi_rssi_check_phone_hand_out(self):
        """Private Test utility for hand_out test.

        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT in 10s.
        PhoneA should still be in call.
        PhoneA should not hand-out, PhoneA should have data on WiFi.
        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT in 10s.
        PhoneA should still be in call. PhoneA should hand-out to LTE.
        PhoneA should have data on WiFi.
        """
        # Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT
        # in 10 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT, 2, 1)
        # Make sure WiFi still connected and have data.
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        # Make sure phone not hand-out, not drop call
        if self._phone_wait_for_not_wfc():
            self.log.error("Phone should not hand out.")
            return False
        if self._is_phone_not_in_call():
            self.log.error("Phone should not drop call.")
            return False

        # Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT
        # in 10 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten,
                 WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT, 2, 1)
        # Make sure WiFi still connected and have data.
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        # Make sure phone hand-out, not drop call
        if not self._phone_wait_for_not_wfc():
            self.log.error("Phone should hand out.")
            return False
        if not self._is_phone_in_call_volte():
            self.log.error("Phone should be in volte call.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_out_wifi_preferred(self):
        """WiFi Hand-Out Threshold - WiFi Preferred

        PhoneA on LTE, VoLTE enabled, WFC WiFi preferred, WiFi associated.
        Cellular and WiFi signal strong.
        Call from PhoneA to PhoneB, PhoneA should be on iwlan.
        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT in 10s.
        PhoneA should still be in call.
        PhoneA should not hand-out, PhoneA should have data on WiFi.
        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT in 10s.
        PhoneA should still be in call. PhoneA should hand-out to LTE.
        PhoneA should have data on WiFi.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan,
            self._decrease_wifi_rssi_check_phone_hand_out, True)

    def _decrease_wifi_rssi_hand_out_and_increase_wifi_rssi_hand_in(self):
        if not self._decrease_wifi_rssi_check_phone_hand_out():
            return False
        if not self._increase_wifi_rssi_check_phone_hand_in():
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_out_in_wifi_preferred(self):
        """WiFi Hand-Out Threshold - WiFi Preferred

        PhoneA on LTE, VoLTE enabled, WFC WiFi preferred, WiFi associated.
        Cellular and WiFi signal strong.
        Call from PhoneA to PhoneB, PhoneA should be on iwlan.

        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_NOT_HAND_OUT in 10s.
        PhoneA should still be in call.
        PhoneA should not hand-out, PhoneA should have data on WiFi.
        Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT in 10s.
        PhoneA should still be in call. PhoneA should hand-out to LTE.
        PhoneA should have data on WiFi.

        Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_NOT_HAND_IN in 10s.
        PhoneA should connect to WiFi and have data on WiFi.
        PhoneA should not hand-in to iwlan.
        Increase WiFi RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s.
        PhoneA should hand-in to iwlan.
        PhoneA call should remain active.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan,
            self._decrease_wifi_rssi_hand_out_and_increase_wifi_rssi_hand_in,
            True)

    def _hand_out_hand_in_stress(self):
        total_iteration = self.stress_test_number
        self.log.info(
            "Hand_out/Hand_in stress test. Total iteration = {}.".format(
                total_iteration))
        current_iteration = 1
        if self._phone_wait_for_call_drop():
            self.log.error("Call Drop.")
            return False
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = {}/{}----<".format(
                current_iteration, total_iteration))

            # Decrease WiFi RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT
            # in 10 seconds
            self.log.info("Decrease WiFi RSSI to hand out.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT, 2, 1)
            # Make sure WiFi still connected and have data.
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            # Make sure phone hand-out, not drop call
            if not self._phone_wait_for_not_wfc():
                self.log.error("Phone failed to hand-out in RSSI {}.".format(
                    WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT))
                break
            if self._phone_wait_for_call_drop():
                self.log.error("Call Drop.")
                break
            # Increase WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s
            self.log.info("Increase WiFi RSSI to hand in.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN, 2, 1)
            # Make sure WiFi still connected and have data.
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            # Make sure phone hand in to iwlan.
            if not self._phone_wait_for_wfc():
                self.log.error("Phone failed to hand-in to wfc.")
                break
            if self._phone_wait_for_call_drop():
                self.log.error("Call Drop.")
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

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_out_in_stress(self):
        """WiFi Calling Hand out/in stress test.

        Steps:
        1. PhoneA on LTE, VoLTE enabled.
        2. PhoneA WFC mode WiFi preferred, WiFi associated.
        3. Cellular strong, WiFi RSSI strong. Call from PhoneA to PhoneB,
            call should be on WFC.
        4. Set WiFI RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT in 10s,
            PhoneA hand-out.
        5. Set WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s,
            PhoneA hand-in.
        6. Repeat Step 4~5. Call should not drop.

        Expected Results:
        4. Phone should hand out.
        5. Phone should hand in.
        6. Stress test pass rate should be higher than pre-defined limit.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan, self._hand_out_hand_in_stress, True)

    def _hand_in_hand_out_stress(self):
        total_iteration = self.stress_test_number
        self.log.info(
            "Hand_in/Hand_out stress test. Total iteration = {}.".format(
                total_iteration))
        current_iteration = 1
        if self._phone_wait_for_call_drop():
            self.log.error("Call Drop.")
            return False
        while (current_iteration <= total_iteration):
            self.log.info(">----Current iteration = {}/{}----<".format(
                current_iteration, total_iteration))

            # Increase WiFi RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN
            # in 10 seconds
            self.log.info("Increase WiFi RSSI to hand in.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN, 2, 1)
            # Make sure WiFi still connected and have data.
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            # Make sure phone hand in to iwlan.
            if not self._phone_wait_for_wfc():
                self.log.error("Phone failed to hand-in to wfc.")
                break
            if self._phone_wait_for_call_drop():
                self.log.error("Call Drop.")
                break

            # Decrease WiFI RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT in 10s
            self.log.info("Decrease WiFi RSSI to hand out.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT, 2, 1)
            # Make sure WiFi still connected and have data.
            if (not wait_for_wifi_data_connection(
                    self.log, self.android_devices[0], True) or
                    not verify_http_connection(self.log,
                                               self.android_devices[0])):
                self.log.error("No Data on Wifi")
                break
            # Make sure phone hand-out, not drop call
            if not self._phone_wait_for_not_wfc():
                self.log.error("Phone failed to hand-out in RSSI {}.".format(
                    WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT))
                break
            if self._phone_wait_for_call_drop():
                self.log.error("Call Drop.")
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

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_in_out_stress(self):
        """WiFi Calling Hand in/out stress test.

        Steps:
        1. PhoneA on LTE, VoLTE enabled.
        2. PhoneA WFC mode WiFi preferred, WiFi associated.
        3. Cellular strong, WiFi RSSI weak. Call from PhoneA to PhoneB,
            call should be on VoLTE.
        4. Set WiFI RSSI to WIFI_RSSI_FOR_HAND_IN_TEST_PHONE_HAND_IN in 10s,
            PhoneA hand-in.
        5. Set WiFI RSSI to WIFI_RSSI_FOR_HAND_OUT_TEST_PHONE_HAND_OUT in 10s,
            PhoneA hand-out.
        6. Repeat Step 4~5. Call should not drop.

        Expected Results:
        4. Phone should hand in.
        5. Phone should hand out.
        6. Stress test pass rate should be higher than pre-defined limit.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_weak_cell_strong,
            self._wfc_phone_setup_wifi_preferred, self._phone_idle_volte,
            self._is_phone_in_call_volte, self._hand_in_hand_out_stress, True)

    def _increase_cellular_rssi_check_phone_hand_out(self):
        """Private Test utility for hand_out test.

        Increase Cellular RSSI to CELL_STRONG_RSSI_VALUE, in 30s.
        PhoneA should still be in call. PhoneA should hand-out to LTE.
        PhoneA should have data on WiFi.
        """
        # Increase Cellular RSSI to CELL_STRONG_RSSI_VALUE in 30 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_CELL],
                 self.cell_rssi_with_no_atten, CELL_STRONG_RSSI_VALUE, 1, 1)
        # Make sure phone hand-out, not drop call
        if not self._phone_wait_for_not_wfc():
            self.log.error("Phone should hand out.")
            return False
        if not self._is_phone_in_call_volte():
            self.log.error("Phone should be in volte call.")
            return False
        # Make sure WiFi still connected and have data.
        if (not wait_for_wifi_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("No Data on Wifi")
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_out_cellular_preferred(self):
        """WiFi Hand-Out Threshold - Cellular Preferred

        Cellular signal absent, WiFi signal strong.
        PhoneA VoLTE enabled, WFC Cellular preferred, WiFi associated.
        Call from PhoneA to PhoneB, PhoneA should be on iwlan.
        Increase Cellular RSSI to CELL_STRONG_RSSI_VALUE, in 30s.
        PhoneA should still be in call. PhoneA should hand-out to LTE.
        PhoneA should have data on WiFi.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_absent,
            self._wfc_phone_setup_cellular_absent_cellular_preferred,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan,
            self._increase_cellular_rssi_check_phone_hand_out, True)

    def _decrease_wifi_rssi_check_phone_not_hand_out(self):
        """Private Test utility for hand_out test.

        Decrease WiFi RSSI to <-100dBm, in 30s.
        PhoneA should drop call. PhoneA should not report LTE as voice RAT.
        PhoneA data should be on LTE.
        """
        # Decrease WiFi RSSI to <-100dBm in 30 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        # Make sure PhoneA data is on LTE.
        if (not wait_for_cell_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("Data not on Cell.")
            return False
        # Make sure phone drop.
        self.log.info("Wait for call drop.")
        if not self._phone_wait_for_call_drop():
            self.log.error("Phone should drop call.")
            return False
        # Make sure Voice RAT is not LTE.
        # FIXME: I think there's something wrong with this check
        if RAT_LTE == get_network_rat(self.log, self.android_devices[0],
                                      NETWORK_SERVICE_VOICE):
            self.log.error("Phone should not report lte as voice rat.")
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_hand_out_wifi_only(self):
        """WiFi Hand-Out Not Attempted - WiFi Only

        PhoneA on LTE, VoLTE enabled, WFC WiFi only, WiFi associated.
        Cellular and WiFi signal strong.
        Call from PhoneA to PhoneB, PhoneA should be on iwlan.
        Decrease WiFi RSSI to <-100dBm, in 30s.
        PhoneA should drop call. PhoneA should not report LTE as voice RAT.
        PhoneA data should be on LTE.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_only, self._phone_idle_iwlan,
            self._is_phone_in_call_iwlan,
            self._decrease_wifi_rssi_check_phone_not_hand_out, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_e4g_disabled(self):
        """WiFi Calling with E4G disabled.

        PhoneA on LTE, VoLTE disabled, WFC WiFi preferred, WiFi associated.
        Cellular and WiFi signal strong.
        Call from PhoneA to PhoneB, PhoneA should be on iwlan.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred_e4g_disabled,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan, None, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_e4g_disabled_wifi_not_connected(
            self):
        """WiFi Calling with E4G disabled.

        PhoneA on LTE, VoLTE disabled, WFC WiFi preferred, WiFi not associated.
        Cellular signal strong, WiFi absent.
        Call from PhoneA to PhoneB, PhoneA should be on CSFB.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_absent_cell_strong,
            self._wfc_phone_setup_wifi_absent_wifi_preferred_e4g_disabled,
            self._phone_idle_not_iwlan, self._is_phone_in_call_csfb, None,
            True)

    def _decrease_wifi_rssi_check_phone_drop(self):
        """Private Test utility for e4g_disabled_wfc test.

        Decrease WiFi RSSI to make sure WiFI not connected. Call should Drop.
        """
        # Decrease WiFi RSSI to <-100dBm in 30 seconds
        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, MIN_RSSI_RESERVED_VALUE)
        # Make sure PhoneA data is on cellular.
        if (not wait_for_cell_data_connection(self.log,
                                              self.android_devices[0], True) or
                not verify_http_connection(self.log, self.android_devices[0])):
            self.log.error("Data not on Cell.")
            return False
        # Make sure phone drop.
        self.log.info("Wait for call drop.")
        if not self._phone_wait_for_call_drop():
            self.log.error("Phone should drop call.")
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_wfc_wifi_preferred_e4g_disabled_leave_wifi_coverage(
            self):
        """WiFi Calling with E4G disabled.

        PhoneA on LTE, VoLTE disabled, WFC WiFi preferred, WiFi associated.
        Cellular and WiFi signal strong.
        Call from PhoneA to PhoneB, PhoneA should be on iwlan.
        Decrease WiFi RSSI to make sure WiFI not connected. Call should Drop.
        """
        return self._wfc_call_sequence(
            [self.android_devices[0], self.android_devices[1]],
            DIRECTION_MOBILE_ORIGINATED, self._wfc_set_wifi_strong_cell_strong,
            self._wfc_phone_setup_wifi_preferred_e4g_disabled,
            self._phone_idle_iwlan, self._is_phone_in_call_iwlan,
            self._decrease_wifi_rssi_check_phone_drop, True)

    @TelephonyBaseTest.tel_test_wrap
    def test_rssi_monitoring(self):
        """Test WiFi RSSI Monitoring API and Callback function.

        Steps:
        1. Set WiFi RSSI to INITIAL_RSSI (-60dBm), connect WiFi on DUT.
        2. Start WiFi RSSI Monitoring for HIGHER_RSSI_THRESHOLD (-50dBm) and
            LOWER_RSSI_THRESHOLD (-70dBm)
        3. Increase WiFi RSSI to HIGHER_RSSI_THRESHOLD+5dBm
        4. Decrease WiFi RSSI to HIGHER_RSSI_THRESHOLD-5dBm
        5. Decrease WiFi RSSI to LOWER_RSSI_THRESHOLD-5dBm
        6. Increase WiFi RSSI to LOWER_RSSI_THRESHOLD+5dBm

        Expected Results:
        1. WiFi Connected successfully.
        2. DUT report LOWER_RSSI_THRESHOLD available.
        3. DUT report HIGHER_RSSI_THRESHOLD available.
        4. DUT report HIGHER_RSSI_THRESHOLD lost.
        5. DUT report LOWER_RSSI_THRESHOLD lost.
        6. DUT report LOWER_RSSI_THRESHOLD available.
        """
        INITIAL_RSSI = -60
        HIGHER_RSSI_THRESHOLD = -50
        LOWER_RSSI_THRESHOLD = -70
        RSSI_THRESHOLD_MARGIN = 5

        WIFI_RSSI_CHANGE_STEP_SIZE = 2
        WIFI_RSSI_CHANGE_DELAY_PER_STEP = 1

        ad = self.android_devices[0]

        set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                 self.wifi_rssi_with_no_atten, INITIAL_RSSI)
        if not ensure_wifi_connected(self.log, ad, self.live_network_ssid,
                                     self.live_network_pwd):
            self.log.error("{} connect WiFI failed".format(ad.serial))
            return False
        try:
            rssi_monitoring_id_higher = ad.droid.connectivitySetRssiThresholdMonitor(
                HIGHER_RSSI_THRESHOLD)
            rssi_monitoring_id_lower = ad.droid.connectivitySetRssiThresholdMonitor(
                LOWER_RSSI_THRESHOLD)

            self.log.info(
                "Initial RSSI: {},"
                "rssi_monitoring_id_lower should be available.".format(
                    INITIAL_RSSI))
            try:
                event = ad.ed.wait_for_event(
                    EventNetworkCallback,
                    is_network_call_back_event_match,
                    network_callback_id=rssi_monitoring_id_lower,
                    network_callback_event=NetworkCallbackAvailable)
                self.log.info("Received Event: {}".format(event))
            except Empty:
                self.log.error("No {} event for id {}".format(
                    NetworkCallbackAvailable, rssi_monitoring_id_lower))
                return False

            self.log.info("Set RSSI to HIGHER_RSSI_THRESHOLD+5,"
                          "rssi_monitoring_id_higher should be available.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     HIGHER_RSSI_THRESHOLD + RSSI_THRESHOLD_MARGIN,
                     WIFI_RSSI_CHANGE_STEP_SIZE,
                     WIFI_RSSI_CHANGE_DELAY_PER_STEP)
            try:
                event = ad.ed.wait_for_event(
                    EventNetworkCallback,
                    is_network_call_back_event_match,
                    network_callback_id=rssi_monitoring_id_higher,
                    network_callback_event=NetworkCallbackAvailable)
                self.log.info("Received Event: {}".format(event))
            except Empty:
                self.log.error("No {} event for id {}".format(
                    NetworkCallbackAvailable, rssi_monitoring_id_higher))
                return False

            self.log.info("Set RSSI to HIGHER_RSSI_THRESHOLD-5,"
                          "rssi_monitoring_id_higher should be lost.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     HIGHER_RSSI_THRESHOLD - RSSI_THRESHOLD_MARGIN,
                     WIFI_RSSI_CHANGE_STEP_SIZE,
                     WIFI_RSSI_CHANGE_DELAY_PER_STEP)
            try:
                event = ad.ed.wait_for_event(
                    EventNetworkCallback,
                    is_network_call_back_event_match,
                    network_callback_id=rssi_monitoring_id_higher,
                    network_callback_event=NetworkCallbackLost)
                self.log.info("Received Event: {}".format(event))
            except Empty:
                self.log.error("No {} event for id {}".format(
                    NetworkCallbackLost, rssi_monitoring_id_higher))
                return False

            self.log.info("Set RSSI to LOWER_RSSI_THRESHOLD-5,"
                          "rssi_monitoring_id_lower should be lost.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     LOWER_RSSI_THRESHOLD - RSSI_THRESHOLD_MARGIN,
                     WIFI_RSSI_CHANGE_STEP_SIZE,
                     WIFI_RSSI_CHANGE_DELAY_PER_STEP)
            try:
                event = ad.ed.wait_for_event(
                    EventNetworkCallback,
                    is_network_call_back_event_match,
                    network_callback_id=rssi_monitoring_id_lower,
                    network_callback_event=NetworkCallbackLost)
                self.log.info("Received Event: {}".format(event))
            except Empty:
                self.log.error("No {} event for id {}".format(
                    NetworkCallbackLost, rssi_monitoring_id_lower))
                return False

            self.log.info("Set RSSI to LOWER_RSSI_THRESHOLD+5,"
                          "rssi_monitoring_id_lower should be available.")
            set_rssi(self.log, self.attens[ATTEN_NAME_FOR_WIFI],
                     self.wifi_rssi_with_no_atten,
                     LOWER_RSSI_THRESHOLD + RSSI_THRESHOLD_MARGIN,
                     WIFI_RSSI_CHANGE_STEP_SIZE,
                     WIFI_RSSI_CHANGE_DELAY_PER_STEP)
            try:
                event = ad.ed.wait_for_event(
                    EventNetworkCallback,
                    is_network_call_back_event_match,
                    network_callback_id=rssi_monitoring_id_lower,
                    network_callback_event=NetworkCallbackAvailable)
                self.log.info("Received Event: {}".format(event))
            except Empty:
                self.log.error("No {} event for id {}".format(
                    NetworkCallbackAvailable, rssi_monitoring_id_lower))
                return False
        finally:
            ad.droid.connectivityStopRssiThresholdMonitor(
                rssi_monitoring_id_higher)
            ad.droid.connectivityStopRssiThresholdMonitor(
                rssi_monitoring_id_lower)
        return True


""" Tests End """

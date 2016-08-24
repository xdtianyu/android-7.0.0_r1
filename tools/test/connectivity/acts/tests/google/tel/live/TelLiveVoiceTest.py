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
from acts.utils import load_config
from acts.test_utils.tel.tel_subscription_utils import \
    get_subid_from_slot_index
from acts.test_utils.tel.tel_subscription_utils import set_subid_for_data
from acts.test_utils.tel.tel_subscription_utils import \
    set_subid_for_message
from acts.test_utils.tel.tel_subscription_utils import \
    set_subid_for_outgoing_call
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts.test_utils.tel.tel_defines import CALL_STATE_HOLDING
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_CDMA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_GSM
from acts.test_utils.tel.tel_defines import RAT_3G
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WLAN
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_CHANGE_DATA_SUB_ID
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import \
    call_voicemail_erase_all_pending_voicemail
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import is_droid_in_rat_family
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import num_active_calls
from acts.test_utils.tel.tel_test_utils import phone_number_formatter
from acts.test_utils.tel.tel_test_utils import set_call_state_listen_level
from acts.test_utils.tel.tel_test_utils import set_phone_number
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import setup_sim
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_for_not_network_rat
from acts.test_utils.tel.tel_test_utils import WifiUtils
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_1x
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_not_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_wcdma
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import \
    phone_setup_iwlan_cellular_preferred
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_2g
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_csfb
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.tel_voice_utils import two_phone_call_leave_voice_mail
from acts.test_utils.tel.tel_voice_utils import two_phone_call_long_seq
from acts.test_utils.tel.tel_voice_utils import two_phone_call_short_seq

DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION = 1 * 60 * 60 # default value 1 hour

class TelLiveVoiceTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            "test_call_volte_to_volte",
            "test_call_volte_to_csfb_3g",
            "test_call_volte_to_csfb_for_tmo",
            "test_call_volte_to_csfb_1x_long",
            "test_call_volte_to_csfb_long",
            "test_call_volte_to_3g",
            "test_call_volte_to_3g_1x_long",
            "test_call_volte_to_3g_wcdma_long",
            "test_call_volte_to_2g",
            "test_call_csfb_3g_to_csfb_3g",
            "test_call_3g_to_3g",
            "test_call_volte_to_volte_long",
            "test_call_csfb_3g_to_csfb_3g_long",
            "test_call_3g_to_3g_long",
            "test_call_volte_mo_hold_unhold",
            "test_call_volte_mt_hold_unhold",
            "test_call_wcdma_mo_hold_unhold",
            "test_call_wcdma_mt_hold_unhold",
            "test_call_csfb_mo_hold_unhold",
            "test_call_csfb_mt_hold_unhold",
            "test_call_volte_to_volte_7_digit_dialing",
            "test_call_volte_to_volte_10_digit_dialing",
            "test_call_volte_to_volte_11_digit_dialing",
            "test_call_volte_to_volte_12_digit_dialing",
            "test_call_volte_to_volte_loop",
            "test_call_csfb_3g_to_csfb_3g_loop",
            "test_call_3g_to_3g_loop",

            # APM Live WFC tests
            # epdg, WFC, APM, WiFi only, WiFi strong
            "test_call_epdg_to_epdg_apm_wfc_wifi_only",
            "test_call_epdg_to_volte_apm_wfc_wifi_only",
            "test_call_epdg_to_csfb_3g_apm_wfc_wifi_only",
            "test_call_epdg_to_3g_apm_wfc_wifi_only",
            "test_call_epdg_to_epdg_long_apm_wfc_wifi_only",
            "test_call_epdg_to_epdg_loop_apm_wfc_wifi_only",
            "test_call_epdg_mo_hold_unhold_apm_wfc_wifi_only",
            "test_call_epdg_mt_hold_unhold_apm_wfc_wifi_only",
            # epdg, WFC, APM, WiFi preferred
            "test_call_epdg_to_epdg_apm_wfc_wifi_preferred",
            "test_call_epdg_to_volte_apm_wfc_wifi_preferred",
            "test_call_epdg_to_csfb_3g_apm_wfc_wifi_preferred",
            "test_call_epdg_to_3g_apm_wfc_wifi_preferred",
            "test_call_epdg_to_epdg_long_apm_wfc_wifi_preferred",
            "test_call_epdg_to_epdg_loop_apm_wfc_wifi_preferred",
            "test_call_epdg_mo_hold_unhold_apm_wfc_wifi_preferred",
            "test_call_epdg_mt_hold_unhold_apm_wfc_wifi_preferred",
            # epdg, WFC, APM, Cellular preferred
            "test_call_epdg_to_epdg_apm_wfc_cellular_preferred",

            # Non-APM Live WFC tests
            # epdg, WFC, WiFi only, WiFi strong, cell strong
            "test_call_epdg_to_epdg_wfc_wifi_only",
            "test_call_epdg_to_volte_wfc_wifi_only",
            "test_call_epdg_to_csfb_3g_wfc_wifi_only",
            "test_call_epdg_to_3g_wfc_wifi_only",
            "test_call_epdg_to_epdg_long_wfc_wifi_only",
            "test_call_epdg_to_epdg_loop_wfc_wifi_only",
            "test_call_epdg_mo_hold_unhold_wfc_wifi_only",
            "test_call_epdg_mt_hold_unhold_wfc_wifi_only",
            # epdg, WFC, WiFi preferred
            "test_call_epdg_to_epdg_wfc_wifi_preferred",
            "test_call_epdg_to_volte_wfc_wifi_preferred",
            "test_call_epdg_to_csfb_3g_wfc_wifi_preferred",
            "test_call_epdg_to_3g_wfc_wifi_preferred",
            "test_call_epdg_to_epdg_long_wfc_wifi_preferred",
            "test_call_epdg_to_epdg_loop_wfc_wifi_preferred",
            "test_call_epdg_mo_hold_unhold_wfc_wifi_preferred",
            "test_call_epdg_mt_hold_unhold_wfc_wifi_preferred",
            # epdg, WFC, Cellular preferred
            "test_call_epdg_to_epdg_wfc_cellular_preferred",

            # Voice Mail Indicator
            "test_erase_all_pending_voicemail",
            "test_voicemail_indicator_volte",
            "test_voicemail_indicator_lte",
            "test_voicemail_indicator_3g",
            "test_voicemail_indicator_iwlan",
            "test_voicemail_indicator_apm_iwlan",
            "test_call_2g_to_3g_long",
            "test_call_3g_to_2g_long",
            "test_call_2g_to_2g",
            "test_call_2g_to_2g_long",
            "test_call_gsm_mo_hold_unhold",
            "test_call_gsm_mt_hold_unhold",

            # long duration voice call (to measure drop rate)
            "test_call_long_duration_volte",
            "test_call_long_duration_wfc",
            "test_call_long_duration_3g"
            )

        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.stress_test_number = int(self.user_params["stress_test_number"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

        if "long_duration_call_total_duration" in self.user_params:
            self.long_duration_call_total_duration = self.user_params[
                "long_duration_call_total_duration"]
        else:
            self.long_duration_call_total_duration = DEFAULT_LONG_DURATION_CALL_TOTAL_DURATION

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_call_mo_voice_general(self):
        """ General voice to voice call.

        1. Make Sure PhoneA attached to voice network.
        2. Make Sure PhoneB attached to voice network.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], None, None, ads[1], None, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_mt_voice_general(self):
        """ General voice to voice call.

        1. Make Sure PhoneA attached to voice network.
        2. Make Sure PhoneB attached to voice network.
        3. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        4. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[1], None, None, ads[0], None, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte(self):
        """ VoLTE to VoLTE call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_7_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 7 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 7-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       7)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            result = call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_10_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 10 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 10-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       10)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            result = call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_11_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 11 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 11-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       11)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            result = call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_12_digit_dialing(self):
        """ VoLTE to VoLTE call test, dial with 12 digit number

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB by 12-digit phone number, accept on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        callee_default_number = get_phone_number(self.log, ads[1])
        caller_dialing_number = phone_number_formatter(callee_default_number,
                                                       12)
        try:
            set_phone_number(self.log, ads[1], caller_dialing_number)
            result = call_setup_teardown(
                self.log, ads[0], ads[1], ads[0], is_phone_in_call_volte,
                is_phone_in_call_volte, WAIT_TIME_IN_CALL_FOR_IMS)
        except Exception as e:
            self.log.error("Exception happened: {}".format(e))
        finally:
            set_phone_number(self.log, ads[1], callee_default_number)
        return result

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_3g(self):
        """ VoLTE to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_for_tmo(self):
        """ VoLTE to CSFB 3G call test for TMobile

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE, CSFB to WCDMA/GSM).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(self.log, ads[0], phone_idle_volte,
                                        None, ads[1], phone_idle_csfb,
                                        is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_1x_long(self):
        """ VoLTE to CSFB 1x call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE, CSFB to 1x).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is CDMA phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA:
            self.log.error(
                "PhoneB not cdma phone, can not csfb 1x. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_csfb, is_phone_in_call_1x, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_csfb_long(self):
        """ VoLTE to CSFB WCDMA call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (without VoLTE, CSFB to WCDMA/GSM).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is GSM phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM:
            self.log.error(
                "PhoneB not gsm phone, can not csfb wcdma. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_csfb,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_3g(self):
        """ VoLTE to 3G call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_voice_3g,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_3g_1x_long(self):
        """ VoLTE to 3G 1x call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 3G 1x mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is CDMA phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA:
            self.log.error("PhoneB not cdma phone, can not 3g 1x. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_voice_3g,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_3g, is_phone_in_call_1x, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_3g_wcdma_long(self):
        """ VoLTE to 3G WCDMA call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 3G WCDMA mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # Make Sure PhoneB is GSM phone.
        if ads[1].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM:
            self.log.error(
                "PhoneB not gsm phone, can not 3g wcdma. Stop test.")
            return False

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_voice_3g,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_3g, is_phone_in_call_wcdma, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_2g(self):
        """ VoLTE to 2G call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in 2G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_voice_2g,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    def _call_epdg_to_epdg_wfc(self, ads, apm_mode, wfc_mode, wifi_ssid,
                               wifi_pwd):
        """ Test epdg<->epdg call functionality.

        Make Sure PhoneA is set to make epdg call.
        Make Sure PhoneB is set to make epdg call.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Args:
            ads: list of android objects, this list should have two ad.
            apm_mode: phones' airplane mode.
                if True, phones are in airplane mode during test.
                if False, phones are not in airplane mode during test.
            wfc_mode: phones' wfc mode.
                Valid mode includes: WFC_MODE_WIFI_ONLY, WFC_MODE_CELLULAR_PREFERRED,
                WFC_MODE_WIFI_PREFERRED, WFC_MODE_DISABLED.
            wifi_ssid: WiFi ssid to connect during test.
            wifi_pwd: WiFi password.

        Returns:
            True if pass; False if fail.
        """

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], apm_mode, wfc_mode, wifi_ssid, wifi_pwd)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], apm_mode, wfc_mode, wifi_ssid, wifi_pwd))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Setup PhoneB WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, False, WFC_MODE_WIFI_ONLY,
            self.wifi_network_ssid, self.wifi_network_pass)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, False, WFC_MODE_WIFI_PREFERRED,
            self.wifi_network_ssid, self.wifi_network_pass)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_wfc_cellular_preferred(self):
        """ Cellular Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: CELLULAR_PREFERRED.
        2. Setup PhoneB WFC mode: CELLULAR_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = [self.android_devices[0], self.android_devices[1]]
        tasks = [(phone_setup_iwlan_cellular_preferred, (
            self.log, ads[0], self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan_cellular_preferred,
                  (self.log, ads[1], self.wifi_network_ssid,
                   self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], None, is_phone_in_call_not_iwlan, ads[1], None,
            is_phone_in_call_not_iwlan, None, WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, True, WFC_MODE_WIFI_ONLY,
            self.wifi_network_ssid, self.wifi_network_pass)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, True, WFC_MODE_WIFI_PREFERRED,
            self.wifi_network_ssid, self.wifi_network_pass)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_apm_wfc_cellular_preferred(self):
        """ Airplane + Cellular Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: CELLULAR_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: CELLULAR_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        return self._call_epdg_to_epdg_wfc(
            self.android_devices, True, WFC_MODE_CELLULAR_PREFERRED,
            self.wifi_network_ssid, self.wifi_network_pass)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to VoLTE test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to VoLTE test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to VoLTE test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_volte_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to VoLTE test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE enabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to CSFB 3G test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_csfb, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to CSFB 3G test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_csfb, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to CSFB 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_csfb, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_csfb_3g_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to CSFB 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in LTE mode (with VoLTE disabled).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_csfb, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to 3G test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to 3G test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_3g_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to 3G test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_3g_to_csfb_3g(self):
        """ CSFB 3G to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode, VoLTE disabled.
        2. Make Sure PhoneB is in LTE mode, VoLTE disabled.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_csfb, (self.log, ads[0])), (phone_setup_csfb,
                                                          (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_csfb, is_phone_in_call_csfb, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_3g(self):
        """ 3G to 3G call test

        1. Make Sure PhoneA is in 3G mode.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])), (phone_setup_voice_3g,
                                                        (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_3g, is_phone_in_call_3g, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_long(self):
        """ VoLTE to VoLTE call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_volte, is_phone_in_call_volte, ads[1],
            phone_idle_volte, is_phone_in_call_volte, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Setup PhoneB WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_to_epdg_long_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan, ads[1],
            phone_idle_iwlan, is_phone_in_call_iwlan, None,
            WAIT_TIME_IN_CALL_FOR_IMS)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_3g_to_csfb_3g_long(self):
        """ CSFB 3G to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode, VoLTE disabled.
        2. Make Sure PhoneB is in LTE mode, VoLTE disabled.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_csfb, (self.log, ads[0])), (phone_setup_csfb,
                                                          (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_csfb, is_phone_in_call_csfb, ads[1],
            phone_idle_csfb, is_phone_in_call_csfb, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_3g_long(self):
        """ 3G to 3G call test

        1. Make Sure PhoneA is in 3G mode.
        2. Make Sure PhoneB is in 3G mode.
        3. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneA, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])), (phone_setup_voice_3g,
                                                        (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_3g, is_phone_in_call_3g, ads[1],
            phone_idle_3g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_to_volte_loop(self):
        """ Stress test: VoLTE to VoLTE call test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is in LTE mode (with VoLTE).
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_volte, is_phone_in_call_volte,
                    ads[1], phone_idle_volte, is_phone_in_call_volte, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
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
    def test_call_epdg_to_epdg_loop_wfc_wifi_only(self):
        """ Stress test: WiFi Only, WiFi calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Setup PhoneB WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
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
    def test_call_epdg_to_epdg_loop_wfc_wifi_preferred(self):
        """ Stress test: WiFi Preferred, WiFi Calling to WiFi Calling test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
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
    def test_call_epdg_to_epdg_loop_apm_wfc_wifi_only(self):
        """ Stress test: Airplane + WiFi Only, WiFi Calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_ONLY.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
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
    def test_call_epdg_to_epdg_loop_apm_wfc_wifi_preferred(self):
        """ Stress test: Airplane + WiFi Preferred, WiFi Calling to WiFi Calling test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Setup PhoneB in airplane mode, WFC mode: WIFI_PREFERRED.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_iwlan, is_phone_in_call_iwlan,
                    ads[1], phone_idle_iwlan, is_phone_in_call_iwlan, None,
                    WAIT_TIME_IN_CALL_FOR_IMS):
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
    def test_call_csfb_3g_to_csfb_3g_loop(self):
        """ Stress test: CSFB 3G to CSFB 3G call test

        1. Make Sure PhoneA is in LTE mode, VoLTE disabled.
        2. Make Sure PhoneB is in LTE mode, VoLTE disabled.
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_csfb, (self.log, ads[0])), (phone_setup_csfb,
                                                          (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_csfb, is_phone_in_call_csfb,
                    ads[1], phone_idle_csfb, is_phone_in_call_csfb, None):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {}".format(
            success_count, fail_count))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_3g_loop(self):
        """ Stress test: 3G to 3G call test

        1. Make Sure PhoneA is in 3G mode
        2. Make Sure PhoneB is in 3G mode
        3. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        4. Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        5. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        6. Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.
        7. Repeat step 3~6.

        Returns:
            True if pass; False if fail.
        """

        # TODO: b/26338422 Make this a parameter
        MINIMUM_SUCCESS_RATE = .95
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])), (phone_setup_voice_3g,
                                                        (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        success_count = 0
        fail_count = 0

        for i in range(1, self.stress_test_number + 1):

            if two_phone_call_long_seq(
                    self.log, ads[0], phone_idle_3g, is_phone_in_call_3g,
                    ads[1], phone_idle_3g, is_phone_in_call_3g, None):
                success_count += 1
                result_str = "Succeeded"

            else:
                fail_count += 1
                result_str = "Failed"

            self.log.info("Iteration {} {}. Current: {} / {} passed.".format(
                i, result_str, success_count, self.stress_test_number))

        self.log.info("Final Count - Success: {}, Failure: {}".format(
            success_count, fail_count))
        if success_count / (
                success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
            return True
        else:
            return False

    def _hold_unhold_test(self, ads):
        """ Test hold/unhold functionality.

        PhoneA is in call with PhoneB. The call on PhoneA is active.
        Get call list on PhoneA.
        Hold call_id on PhoneA.
        Check call_id state.
        Unhold call_id on PhoneA.
        Check call_id state.

        Args:
            ads: List of android objects.
                This list should contain 2 android objects.
                ads[0] is the ad to do hold/unhold operation.

        Returns:
            True if pass; False if fail.
        """
        call_list = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(call_list))
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        call_id = call_list[0]

        if ads[0].droid.telecomCallGetCallState(call_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_id, ads[0].droid.telecomCallGetCallState(call_id)))
            return False
        # TODO: b/26296375 add voice check.

        self.log.info("Hold call_id {} on PhoneA".format(call_id))
        ads[0].droid.telecomCallHold(call_id)
        time.sleep(WAIT_TIME_IN_CALL)
        if ads[0].droid.telecomCallGetCallState(call_id) != CALL_STATE_HOLDING:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_HOLDING".format(
                    call_id, ads[0].droid.telecomCallGetCallState(call_id)))
            return False
        # TODO: b/26296375 add voice check.

        self.log.info("Unhold call_id {} on PhoneA".format(call_id))
        ads[0].droid.telecomCallUnhold(call_id)
        time.sleep(WAIT_TIME_IN_CALL)
        if ads[0].droid.telecomCallGetCallState(call_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_id, ads[0].droid.telecomCallGetCallState(call_id)))
            return False
        # TODO: b/26296375 add voice check.

        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            self.log.error("Caller/Callee dropped call.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling MO call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling MO call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling MO call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mo_hold_unhold_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling MO call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_wfc_wifi_only(self):
        """ WiFi Only, WiFi calling MT call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_wfc_wifi_preferred(self):
        """ WiFi Preferred, WiFi calling MT call hold/unhold test

        1. Setup PhoneA WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_apm_wfc_wifi_only(self):
        """ Airplane + WiFi Only, WiFi calling MT call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_ONLY.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_epdg_mt_hold_unhold_apm_wfc_wifi_preferred(self):
        """ Airplane + WiFi Preferred, WiFi calling MT call hold/unhold test

        1. Setup PhoneA in airplane mode, WFC mode: WIFI_PREFERRED.
        2. Make sure PhoneB can make/receive voice call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_iwlan):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_mo_hold_unhold(self):
        """ VoLTE MO call hold/unhold test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_volte,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_volte_mt_hold_unhold(self):
        """ VoLTE MT call hold/unhold test

        1. Make Sure PhoneA is in LTE mode (with VoLTE).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_volte):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_wcdma_mo_hold_unhold(self):
        """ MO WCDMA hold/unhold test

        1. Make Sure PhoneA is in 3G WCDMA mode.
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_3g,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_wcdma_mt_hold_unhold(self):
        """ MT WCDMA hold/unhold test

        1. Make Sure PhoneA is in 3G WCDMA mode.
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_3g):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_mo_hold_unhold(self):
        """ MO CSFB WCDMA/GSM hold/unhold test

        1. Make Sure PhoneA is in LTE mode (VoLTE disabled).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneA to PhoneB, accept on PhoneB.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_csfb,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_csfb_mt_hold_unhold(self):
        """ MT CSFB WCDMA/GSM hold/unhold test

        1. Make Sure PhoneA is in LTE mode (VoLTE disabled).
        2. Make Sure PhoneB is able to make/receive call.
        3. Call from PhoneB to PhoneA, accept on PhoneA.
        4. Hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_csfb):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_erase_all_pending_voicemail(self):
        """Script for TMO/ATT/SPT phone to erase all pending voice mail.
        This script only works if phone have already set up voice mail options,
        and phone should disable password protection for voice mail.

        1. If phone don't have pending voice message, return True.
        2. Dial voice mail number.
            For TMO, the number is '123'.
            For ATT, the number is phone's number.
            For SPT, the number is phone's number.
        3. Use DTMF to delete all pending voice messages.
        4. Check telephonyGetVoiceMailCount result. it should be 0.

        Returns:
            False if error happens. True is succeed.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return call_voicemail_erase_all_pending_voicemail(
            self.log, self.android_devices[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_volte(self):
        """Test Voice Mail notification in LTE (VoLTE enabled).
        This script currently only works for TMO now.

        1. Make sure DUT (ads[1]) in VoLTE mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_volte, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_lte(self):
        """Test Voice Mail notification in LTE (VoLTE disabled).
        This script currently only works for TMO/ATT/SPT now.

        1. Make sure DUT (ads[1]) in LTE (No VoLTE) mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_csfb, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_csfb)

    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_3g(self):
        """Test Voice Mail notification in 3G
        This script currently only works for TMO/ATT/SPT now.

        1. Make sure DUT (ads[1]) in 3G mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_2g(self):
        """Test Voice Mail notification in 2G
        This script currently only works for TMO/ATT/SPT now.

        1. Make sure DUT (ads[0]) in 2G mode. Both PhoneB (ads[1]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_2g, (self.log, ads[0]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[0]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[1], None, None,
                                               ads[0], phone_idle_2g)

    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_iwlan(self):
        """Test Voice Mail notification in WiFI Calling
        This script currently only works for TMO now.

        1. Make sure DUT (ads[1]) in WFC mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_voicemail_indicator_apm_iwlan(self):
        """Test Voice Mail notification in WiFI Calling
        This script currently only works for TMO now.

        1. Make sure DUT (ads[1]) in APM WFC mode. Both PhoneB (ads[0]) and DUT idle.
        2. Make call from PhoneB to DUT, reject on DUT.
        3. On PhoneB, leave a voice mail to DUT.
        4. Verify DUT receive voice mail notification.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False
        if not call_voicemail_erase_all_pending_voicemail(self.log, ads[1]):
            self.log.error("Failed to clear voice mail.")
            return False

        return two_phone_call_leave_voice_mail(self.log, ads[0], None, None,
                                               ads[1], phone_idle_iwlan)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_2g_to_2g(self):
        """ Test 2g<->2g call functionality.

        Make Sure PhoneA is in 2g mode.
        Make Sure PhoneB is in 2g mode.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_2g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_short_seq(
            self.log, ads[0], phone_idle_2g, is_phone_in_call_2g, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_2g_to_2g_long(self):
        """ Test 2g<->2g call functionality.

        Make Sure PhoneA is in 2g mode.
        Make Sure PhoneB is in 2g mode.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_2g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_2g, is_phone_in_call_2g, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_3g_to_2g_long(self):
        """ Test 3g<->2g call functionality.

        Make Sure PhoneA is in 3g mode.
        Make Sure PhoneB is in 2g mode.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_2g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_2g, is_phone_in_call_3g, ads[1],
            phone_idle_2g, is_phone_in_call_2g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_2g_to_3g_long(self):
        """ Test 2g<->3g call functionality.

        Make Sure PhoneA is in 2g mode.
        Make Sure PhoneB is in 3g mode.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
        Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneB.
        Call from PhoneB to PhoneA, accept on PhoneA, hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_3g, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return two_phone_call_long_seq(
            self.log, ads[0], phone_idle_2g, is_phone_in_call_2g, ads[1],
            phone_idle_2g, is_phone_in_call_3g, None)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_gsm_mo_hold_unhold(self):
        """ Test GSM call hold/unhold functionality.

        Make Sure PhoneA is in 2g mode (GSM).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MO Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_2g,
                                   verify_callee_func=None):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_call_gsm_mt_hold_unhold(self):
        """ Test GSM call hold/unhold functionality.

        Make Sure PhoneA is in 2g mode (GSM).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneB to PhoneA, accept on PhoneA, hold and unhold on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma hold/unhold test.")
            return False

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        ads[0].droid.telecomCallClearCallList()
        if num_active_calls(self.log, ads[0]) != 0:
            self.log.error("Phone {} Call List is not empty."
                           .format(ads[0].serial))
            return False

        self.log.info("Begin MT Call Hold/Unhold Test.")
        if not call_setup_teardown(self.log,
                                   ads[1],
                                   ads[0],
                                   ad_hangup=None,
                                   verify_caller_func=None,
                                   verify_callee_func=is_phone_in_call_2g):
            return False

        if not self._hold_unhold_test(ads):
            self.log.error("Hold/Unhold test fail.")
            return False

        return True

    def _test_call_long_duration(self, dut_incall_check_func,
                                 total_duration):
        ads = self.android_devices
        self.log.info("Long Duration Call Test. Total duration = {}".
            format(total_duration))
        return call_setup_teardown(self.log, ads[0], ads[1], ads[0],
            verify_caller_func=dut_incall_check_func,
            wait_time_in_call=total_duration)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_long_duration_volte(self):
        """ Test call drop rate for VoLTE long duration call.

        Steps:
        1. Setup VoLTE for DUT.
        2. Make VoLTE call from DUT to PhoneB.
        3. For <total_duration> time, check if DUT drop call or not.

        Expected Results:
        DUT should not drop call.

        Returns:
        False if DUT call dropped during test.
        Otherwise True.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._test_call_long_duration(is_phone_in_call_volte,
            self.long_duration_call_total_duration)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_long_duration_wfc(self):
        """ Test call drop rate for WiFi Calling long duration call.

        Steps:
        1. Setup WFC for DUT.
        2. Make WFC call from DUT to PhoneB.
        3. For <total_duration> time, check if DUT drop call or not.

        Expected Results:
        DUT should not drop call.

        Returns:
        False if DUT call dropped during test.
        Otherwise True.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._test_call_long_duration(is_phone_in_call_iwlan,
            self.long_duration_call_total_duration)

    @TelephonyBaseTest.tel_test_wrap
    def test_call_long_duration_3g(self):
        """ Test call drop rate for 3G long duration call.

        Steps:
        1. Setup 3G for DUT.
        2. Make CS call from DUT to PhoneB.
        3. For <total_duration> time, check if DUT drop call or not.

        Expected Results:
        DUT should not drop call.

        Returns:
        False if DUT call dropped during test.
        Otherwise True.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._test_call_long_duration(is_phone_in_call_3g,
            self.long_duration_call_total_duration)
""" Tests End """

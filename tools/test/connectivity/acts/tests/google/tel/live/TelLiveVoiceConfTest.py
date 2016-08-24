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
    Test Script for Live Network Telephony Conference Call
"""

import time
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_MANAGE_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_MERGE_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_CAPABILITY_SWAP_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_PROPERTY_CONFERENCE
from acts.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts.test_utils.tel.tel_defines import CALL_STATE_HOLDING
from acts.test_utils.tel.tel_defines import CARRIER_VZW
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import RAT_3G
from acts.test_utils.tel.tel_defines import PHONE_TYPE_CDMA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_GSM
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_reject
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import get_call_uri
from acts.test_utils.tel.tel_test_utils import get_phone_number
from acts.test_utils.tel.tel_test_utils import is_uri_equivalent
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import num_active_calls
from acts.test_utils.tel.tel_test_utils import set_call_state_listen_level
from acts.test_utils.tel.tel_test_utils import setup_sim
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts.test_utils.tel.tel_voice_utils import get_cep_conference_call_id
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_1x
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_wcdma
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import swap_calls
from acts.utils import load_config


class TelLiveVoiceConfTest(TelephonyBaseTest):

    # Note: Currently Conference Call do not verify voice.
    # So even if test cases passed, does not necessarily means
    # conference call functionality is working.
    # Need to add code to check for voice.

    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            # GSM
            "test_gsm_mo_mo_add_merge_drop",
            "test_gsm_mt_mt_add_merge_drop"

            # 1x conference
            "test_1x_mo_mo_add_merge_drop_from_participant",
            "test_1x_mo_mo_add_merge_drop_from_host",
            # 1x multi call
            "test_1x_mo_mt_add_drop_active",
            "test_1x_mo_mt_add_drop_held",
            "test_1x_mo_mt_add_drop_on_dut",
            "test_1x_mt_mt_add_drop_active",
            "test_1x_mt_mt_add_drop_held",
            "test_1x_mt_mt_add_drop_on_dut",
            "test_1x_mo_mt_add_swap_twice_drop_active",
            "test_1x_mo_mt_add_swap_twice_drop_held",
            "test_1x_mo_mt_add_swap_twice_drop_on_dut",
            "test_1x_mt_mt_add_swap_twice_drop_active",
            "test_1x_mt_mt_add_swap_twice_drop_held",
            "test_1x_mt_mt_add_swap_twice_drop_on_dut",
            "test_1x_mo_mt_add_swap_once_drop_active",
            "test_1x_mo_mt_add_swap_once_drop_held",
            "test_1x_mo_mt_add_swap_once_drop_on_dut",
            "test_1x_mt_mt_add_swap_once_drop_active",
            "test_1x_mt_mt_add_swap_once_drop_held",
            "test_1x_mt_mt_add_swap_once_drop_on_dut",

            # WCDMA
            "test_wcdma_mo_mo_add_merge_drop",
            "test_wcdma_mt_mt_add_merge_drop",
            "test_wcdma_mo_mo_add_swap_twice_drop_held",
            "test_wcdma_mo_mo_add_swap_twice_drop_active",
            "test_wcdma_mo_mt_add_swap_twice_drop_held",
            "test_wcdma_mo_mt_add_swap_twice_drop_active",
            "test_wcdma_mo_mo_add_swap_once_drop_held",
            "test_wcdma_mo_mo_add_swap_once_drop_active",
            "test_wcdma_mo_mt_add_swap_once_drop_held",
            "test_wcdma_mo_mt_add_swap_once_drop_active",
            "test_wcdma_mo_mo_add_swap_once_merge_drop",
            "test_wcdma_mo_mo_add_swap_twice_merge_drop",
            "test_wcdma_mo_mt_add_swap_once_merge_drop",
            "test_wcdma_mo_mt_add_swap_twice_merge_drop",
            "test_wcdma_mt_mt_add_swap_once_merge_drop",
            "test_wcdma_mt_mt_add_swap_twice_merge_drop",
            "test_wcdma_mt_mt_add_merge_unmerge_swap_drop",

            # CSFB WCDMA
            "test_csfb_wcdma_mo_mo_add_swap_twice_drop_held",
            "test_csfb_wcdma_mo_mo_add_swap_twice_drop_active",
            "test_csfb_wcdma_mo_mt_add_swap_twice_drop_held",
            "test_csfb_wcdma_mo_mt_add_swap_twice_drop_active",
            "test_csfb_wcdma_mo_mo_add_swap_once_drop_held",
            "test_csfb_wcdma_mo_mo_add_swap_once_drop_active",
            "test_csfb_wcdma_mo_mt_add_swap_once_drop_held",
            "test_csfb_wcdma_mo_mt_add_swap_once_drop_active",
            "test_csfb_wcdma_mo_mo_add_swap_once_merge_drop",
            "test_csfb_wcdma_mo_mo_add_swap_twice_merge_drop",
            "test_csfb_wcdma_mo_mt_add_swap_once_merge_drop",
            "test_csfb_wcdma_mo_mt_add_swap_twice_merge_drop",

            # VoLTE
            "test_volte_mo_mo_add_volte_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_volte_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_volte_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_1x_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_1x_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_1x_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_volte_swap_twice_drop_held",
            "test_volte_mo_mo_add_volte_swap_twice_drop_active",
            "test_volte_mo_mt_add_volte_swap_twice_drop_held",
            "test_volte_mo_mt_add_volte_swap_twice_drop_active",
            "test_volte_mo_mo_add_volte_swap_once_drop_held",
            "test_volte_mo_mo_add_volte_swap_once_drop_active",
            "test_volte_mo_mt_add_volte_swap_once_drop_held",
            "test_volte_mo_mt_add_volte_swap_once_drop_active",
            "test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep",
            "test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep",

            # VoLTE CEP
            "test_volte_mo_mo_add_volte_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_volte_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_volte_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_volte_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_volte_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_volte_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_volte_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_volte_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_volte_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_volte_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_volte_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_volte_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_wcdma_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_wcdma_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_wcdma_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_wcdma_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_wcdma_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_wcdma_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_1x_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_1x_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_1x_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_1x_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_1x_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_1x_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_1x_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_1x_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_1x_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_1x_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_1x_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_1x_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_volte_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_volte_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_volte_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_volte_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_volte_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_volte_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_volte_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_volte_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_volte_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_volte_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_volte_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_volte_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_wcdma_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_wcdma_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_wcdma_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_1x_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_1x_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mo_add_1x_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mo_add_1x_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_1x_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_1x_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mo_mt_add_1x_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mo_mt_add_1x_swap_twice_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_1x_swap_once_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_1x_swap_once_merge_drop_first_call_from_host_cep",
            "test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_cep",
            "test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_host_cep",
            "test_volte_mt_mt_add_1x_swap_twice_merge_drop_first_call_from_participant_cep",
            "test_volte_mt_mt_add_1x_swap_twice_merge_drop_first_call_from_host_cep",

            # WiFi Calling Conference
            # WiFi_Only mode is not supported for now.
            # epdg, WFC, noAPM, WiFi Only, cell strong, wifi strong
            "test_epdg_mo_mo_add_epdg_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_merge_drop_wfc_wifi_only",
            "test_epdg_mt_mt_add_epdg_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_volte_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_volte_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_wcdma_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_wcdma_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_1x_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_1x_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_epdg_swap_twice_drop_held_wfc_wifi_only",
            "test_epdg_mo_mo_add_epdg_swap_twice_drop_active_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_swap_twice_drop_held_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_swap_twice_drop_active_wfc_wifi_only",
            "test_epdg_mo_mo_add_epdg_swap_once_drop_held_wfc_wifi_only",
            "test_epdg_mo_mo_add_epdg_swap_once_drop_active_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_swap_once_drop_held_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_swap_once_drop_active_wfc_wifi_only",
            "test_epdg_mo_mo_add_epdg_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_epdg_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_epdg_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_volte_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_volte_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_volte_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_volte_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_wcdma_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_wcdma_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_wcdma_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_wcdma_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_1x_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_1x_swap_twice_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mo_add_1x_swap_once_merge_drop_wfc_wifi_only",
            "test_epdg_mo_mt_add_1x_swap_once_merge_drop_wfc_wifi_only",

            # epdg, WFC, noAPM, WiFi preferred, cell strong, wifi strong
            "test_epdg_mo_mo_add_epdg_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_merge_drop_wfc_wifi_preferred",
            "test_epdg_mt_mt_add_epdg_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_volte_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_wcdma_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_wcdma_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_1x_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_1x_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_twice_drop_held_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_twice_drop_active_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_twice_drop_held_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_twice_drop_active_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_once_drop_held_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_once_drop_active_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_once_drop_held_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_once_drop_active_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_once_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_once_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_volte_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_volte_swap_once_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_wcdma_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_wcdma_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_wcdma_swap_once_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_wcdma_swap_once_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_1x_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_1x_swap_twice_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mo_add_1x_swap_once_merge_drop_wfc_wifi_preferred",
            "test_epdg_mo_mt_add_1x_swap_once_merge_drop_wfc_wifi_preferred",

            # WiFi Calling Multi Call Swap Only
            "test_epdg_mo_mo_add_epdg_swap_twice_drop_active_apm_wifi_preferred",
            "test_epdg_mo_mt_add_epdg_swap_once_drop_held_apm_wifi_preferred",
            "test_epdg_mo_mo_add_epdg_swap_once_drop_active_apm_wfc_wifi_preferred",

            # WiFi Calling Conference No_CEP
            # Airplane Mode, WiFi Preferred
            "test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep",
            "test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep",
            "test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep",

            # WiFi Calling Multi Call Swap + Conference No_CEP
            # Airplane Mode, WiFi Preferred
            "test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep",

            # WiFi Calling Conference CEP
            # Airplane Mode, WiFi Preferred
            "test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mo_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mo_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mt_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mt_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep",
            "test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep",
            "test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep",
            "test_epdg_mt_mt_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep",
            "test_epdg_mt_mt_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep",

            # WiFi Calling Multi Call Swap + Conference CEP
            # Airplane Mode, WiFi Preferred
            "test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep",
            "test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep",
            )

        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

    """ Private Test Utils """

    def _three_phone_call_mo_add_mo(self, ads, phone_setups, verify_funcs):
        """Use 3 phones to make MO calls.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad)))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    self.log.error("Phone {} Call List is not empty."
                                   .format(ad.serial))
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[1],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_a,
                                       verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            self.log.info("Calls in PhoneA{}".format(calls))
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneA to PhoneC.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[2],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_a,
                                       verify_callee_func=verify_func_c):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except _CallException:
            return None

        return call_ab_id

    def _three_phone_call_mo_add_mt(self, ads, phone_setups, verify_funcs):
        """Use 3 phones to make MO call and MT call.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneC to PhoneA, accept on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad)))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    self.log.error("Phone {} Call List is not empty."
                                   .format(ad.serial))
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[1],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_a,
                                       verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            self.log.info("Calls in PhoneA{}".format(calls))
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneC to PhoneA.")
            if not call_setup_teardown(self.log,
                                       ads[2],
                                       ads[0],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_c,
                                       verify_callee_func=verify_func_a):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except _CallException:
            return None

        return call_ab_id

    def _three_phone_call_mo_add_mt_reject(self, ads, verify_funcs, reject):
        """Use 3 phones to make MO call and MT call.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneC to PhoneA. PhoneA receive incoming call.
            if reject is True, then reject the call on PhoneA.
            if reject if False, then just ignore the incoming call on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            verify_funcs: list of phone call verify functions for
                PhoneA and PhoneB. The list should have two objects.

        Returns:
            True if no error happened.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b = verify_funcs
            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[1],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_a,
                                       verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            self.log.info("Step2: Call From PhoneC to PhoneA then decline.")
            if not call_reject(self.log, ads[2], ads[0], reject):
                raise _CallException("PhoneC call PhoneA then decline failed.")
            time.sleep(WAIT_TIME_IN_CALL)
            if not verify_incall_state(self.log, [ads[0], ads[1]], True):
                raise _CallException("PhoneA and PhoneB are not in call.")

        except _CallException:
            return False

        return True

    def _three_phone_call_mt_add_mt(self, ads, phone_setups, verify_funcs):
        """Use 3 phones to make MT call and MT call.

        Call from PhoneB to PhoneA, accept on PhoneA.
        Call from PhoneC to PhoneA, accept on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad)))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    self.log.error("Phone {} Call List is not empty."
                                   .format(ad.serial))
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneB to PhoneA.")
            if not call_setup_teardown(self.log,
                                       ads[1],
                                       ads[0],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_b,
                                       verify_callee_func=verify_func_a):
                raise _CallException("PhoneB call PhoneA failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            self.log.info("Calls in PhoneA{}".format(calls))
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneC to PhoneA.")
            if not call_setup_teardown(self.log,
                                       ads[2],
                                       ads[0],
                                       ad_hangup=None,
                                       verify_caller_func=verify_func_c,
                                       verify_callee_func=verify_func_a):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except _CallException:
            return None

        return call_ab_id

    def _test_1x_mo_mo_add(self):
        """Test multi call feature in 1x call.

        PhoneA (1x) call PhoneB, accept on PhoneB.
        PhoneA (1x) call PhoneC, accept on PhoneC.

        Returns:
            call_ab_id, call_ac_id, call_conf_id if succeed;
            None, None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("{} not CDMA phone, abort this 1x test.".format(ads[
                0].serial))
            return None, None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_3g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_1x, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 3:
            return None, None, None
        for call_id in calls:
            if (CALL_CAPABILITY_MERGE_CONFERENCE in
                    ads[0].droid.telecomCallGetCapabilities(call_id)):
                call_conf_id = call_id
            elif call_id != call_ab_id:
                call_ac_id = call_id

        return call_ab_id, call_ac_id, call_conf_id

    def _test_1x_mo_mt_add_swap_x(self, num_swaps):
        """Test multi call feature in 1x call.

        PhoneA (1x) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (1x), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Returns:
            call_ab_id, call_ac_id, call_conf_id if succeed;
            None, None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("{} not CDMA phone, abort this 1x test.".format(ads[
                0].serial))
            return None, None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_3g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_1x, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None, None

        call_conf_id = None
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 3:
            return None, None, None
        for call_id in calls:
            if (CALL_CAPABILITY_SWAP_CONFERENCE in
                    ads[0].droid.telecomCallGetCapabilities(call_id)):
                call_conf_id = call_id
            elif call_id != call_ab_id:
                call_ac_id = call_id

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log,
                              ads,
                              call_ab_id,
                              call_ac_id,
                              num_swaps,
                              check_call_status=False):
                self.log.error("Swap test failed.")
                return None, None, None

        return call_ab_id, call_ac_id, call_conf_id

    def _test_1x_mt_mt_add_swap_x(self, num_swaps):
        """Test multi call feature in 1x call.

        PhoneB call PhoneA (1x), accept on PhoneA.
        PhoneC call PhoneA (1x), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Returns:
            call_ab_id, call_ac_id, call_conf_id if succeed;
            None, None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("{} not CDMA phone, abort this 1x test.".format(ads[
                0].serial))
            return None, None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_3g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_1x, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None, None

        call_conf_id = None
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 3:
            return None, None, None
        for call_id in calls:
            if (CALL_CAPABILITY_SWAP_CONFERENCE in
                    ads[0].droid.telecomCallGetCapabilities(call_id)):
                call_conf_id = call_id
            elif call_id != call_ab_id:
                call_ac_id = call_id

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log,
                              ads,
                              call_ab_id,
                              call_ac_id,
                              num_swaps,
                              check_call_status=False):
                self.log.error("Swap test failed.")
                return None, None, None

        return call_ab_id, call_ac_id, call_conf_id

    def _test_1x_multi_call_drop_from_participant(self, host, first_drop_ad,
                                                  second_drop_ad):
        """Test private function to drop call from participant in 1x multi call.

        Host(1x) is in multi call scenario with first_drop_ad and second_drop_ad.
        Drop call on first_drop_ad.
        Verify call continues between host and second_drop_ad.
        Drop call on second_drop_ad and verify host also ends.

        Args:
            host: android device object for multi-call/conference-call host.
            first_drop_ad: android device object for call participant, end call
                on this participant first.
            second_drop_ad: android device object for call participant, end call
                on this participant second.

        Returns:
            True if no error happened. Otherwise False.
        """
        self.log.info("Drop 1st call.")
        first_drop_ad.droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        calls = host.droid.telecomCallGetCallIds()
        self.log.info("Calls in Host{}".format(calls))
        if num_active_calls(self.log, host) != 3:
            return False
        if not verify_incall_state(self.log, [host, second_drop_ad], True):
            return False
        if not verify_incall_state(self.log, [first_drop_ad], False):
            return False

        self.log.info("Drop 2nd call.")
        second_drop_ad.droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(
                self.log, [host, second_drop_ad, first_drop_ad], False):
            return False
        return True

    def _test_1x_multi_call_drop_from_host(self, host, active_participant_ad,
                                           held_participant_ad):
        """Test private function to drop call from host in 1x multi call.

        Host(1x) is in multi call scenario with first_drop_ad and second_drop_ad.
        Drop call on host. Then active_participant_ad should ends as well.
        Host should receive a call back from held_participant_ad. Answer on host.
        Drop call on host. Then verify held_participant_ad ends as well.

        Args:
            host: android device object for multi-call/conference-call host.
            active_participant_ad: android device object for the current active
                call participant.
            held_participant_ad: android device object for the current held
                call participant.

        Returns:
            True if no error happened. Otherwise False.
        """
        self.log.info("Drop current call on DUT.")
        host.droid.telecomEndCall()
        if not wait_and_answer_call(self.log, host, get_phone_number(
                self.log, held_participant_ad)):
            self.log.error("Did not receive call back.")
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [host, held_participant_ad],
                                   True):
            return False
        if not verify_incall_state(self.log, [active_participant_ad], False):
            return False

        self.log.info("Drop current call on DUT.")
        host.droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [host, held_participant_ad,
                                              active_participant_ad], False):
            return False
        return True

    def _test_1x_conf_call_drop_from_host(self, host, participant_list):
        """Test private function to drop call from host in 1x conference call.

        Host(1x) is in conference call scenario with phones in participant_list.
        End call on host. Then all phones in participant_list should end call.

        Args:
            host: android device object for multi-call/conference-call host.
            participant_list: android device objects list for all other
                participants in multi-call/conference-call.

        Returns:
            True if no error happened. Otherwise False.
        """
        self.log.info("Drop conference call on host.")
        host.droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [host], False):
            return False
        if not verify_incall_state(self.log, participant_list, False):
            return False
        return True

    def _test_1x_merge_conference(self, host, participant_list, call_conf_id):
        """Test private function to merge to conference in 1x multi call scenario.

        Host(1x) is in multi call scenario with phones in participant_list.
        Merge to conference on host.
        Verify merge succeed.

        Args:
            host: android device object for multi-call/conference-call host.
            participant_list: android device objects list for all other
                participants in multi-call/conference-call.
            call_conf_id: conference call id in host android device object.

        Returns:
            True if no error happened. Otherwise False.
        """
        host.droid.telecomCallMergeToConf(call_conf_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = host.droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, host) != 3:
            return False
        if not verify_incall_state(self.log, [host], True):
            return False
        if not verify_incall_state(self.log, participant_list, True):
            return False
        if (CALL_CAPABILITY_MERGE_CONFERENCE in
                host.droid.telecomCallGetCapabilities(call_conf_id)):
            self.log.error("Merge conference failed.")
            return False
        return True

    def _test_volte_mo_mo_add_volte_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_volte, phone_setup_volte],
            [is_phone_in_call_volte, is_phone_in_call_volte,
             is_phone_in_call_volte])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_volte, phone_setup_volte],
            [is_phone_in_call_volte, is_phone_in_call_volte,
             is_phone_in_call_volte])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mt_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_volte, phone_setup_volte],
            [is_phone_in_call_volte, is_phone_in_call_volte,
             is_phone_in_call_volte])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mo_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                self.log.error(
                    "{} not GSM phone, abort wcdma swap test.".format(
                        ad.serial))
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_wcdma,
             is_phone_in_call_wcdma])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                self.log.error(
                    "{} not GSM phone, abort wcdma swap test.".format(
                        ad.serial))
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_wcdma,
             is_phone_in_call_wcdma])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mt_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                self.log.error(
                    "{} not GSM phone, abort wcdma swap test.".format(
                        ad.serial))
                return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_wcdma,
             is_phone_in_call_wcdma])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mo_add_1x_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("{} not CDMA phone, abort 1x swap test.".format(
                    ad.serial))
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mo_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("{} not CDMA phone, abort 1x swap test.".format(
                    ad.serial))
                return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_volte_mt_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in VoLTE call.

        PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("{} not CDMA phone, abort 1x swap test.".format(
                    ad.serial))
                return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_volte, phone_setup_voice_3g, phone_setup_voice_3g],
            [is_phone_in_call_volte, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_wcdma_mo_mo_add_swap_x(self, num_swaps):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_3g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_3g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_wcdma_mt_mt_add_swap_x(self, num_swaps):
        """Test swap feature in WCDMA call.

        PhoneB call PhoneA (WCDMA), accept on PhoneA.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_3g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_3g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_wcdma_mo_mt_add_swap_x(self, num_swaps):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_3g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_wcdma, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_csfb_wcdma_mo_mo_add_swap_x(self, num_swaps):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_csfb, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_csfb, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_csfb_wcdma_mo_mt_add_swap_x(self, num_swaps):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_csfb, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_csfb, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_ims_conference_merge_drop_second_call_no_cep(self, call_ab_id,
                                                           call_ac_id):
        """Test conference merge and drop in VoLTE call.

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Total number of call ids in {} is not 1.".format(
                ads[0].serial))
            if get_cep_conference_call_id(ads[0]) is not None:
                self.log.error("CEP enabled.")
            else:
                self.log.error("Merge failed.")
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id, ads[0].droid.telecomCallGetCallState(
                        call_conf_id)))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        ads[2].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        # Because of b/18413009, VZW VoLTE conference host will not drop call
        # even if all participants drop. The reason is VZW network is not
        # providing such information to DUT.
        # So this test probably will fail on the last step for VZW.
        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        ads[1].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _merge_cep_conference_call(self, call_ab_id, call_ac_id):
        """Merge CEP conference call.

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            call_id for conference
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))

        call_conf_id = get_cep_conference_call_id(ads[0])
        if call_conf_id is None:
            self.log.error(
                "No call with children. Probably CEP not enabled or merge failed.")
            return None
        calls.remove(call_conf_id)
        if (set(ads[0].droid.telecomCallGetCallChildren(call_conf_id)) !=
                set(calls)):
            self.log.error(
                "Children list<{}> for conference call is not correct.".format(
                    ads[0].droid.telecomCallGetCallChildren(call_conf_id)))
            return None

        if (CALL_PROPERTY_CONFERENCE not in
                ads[0].droid.telecomCallGetProperties(call_conf_id)):
            self.log.error("Conf call id properties wrong: {}".format(ads[
                0].droid.telecomCallGetProperties(call_conf_id)))
            return None

        if (CALL_CAPABILITY_MANAGE_CONFERENCE not in
                ads[0].droid.telecomCallGetCapabilities(call_conf_id)):
            self.log.error("Conf call id capabilities wrong: {}".format(ads[
                0].droid.telecomCallGetCapabilities(call_conf_id)))
            return None

        if (call_ab_id in calls) or (call_ac_id in calls):
            self.log.error(
                "Previous call ids should not in new call list after merge.")
            return None

        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return None

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id, ads[0].droid.telecomCallGetCallState(
                        call_conf_id)))
            return None

        return call_conf_id

    def _test_ims_conference_merge_drop_second_call_from_participant_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        ads[2].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        ads[1].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_ims_conference_merge_drop_first_call_from_participant_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        Hangup on PhoneB, check call continues between AC.
        Hangup on PhoneC, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        self.log.info("Step5: End call on PhoneB and verify call continues.")
        ads[1].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not verify_incall_state(self.log, [ads[1]], False):
            return False

        self.log.info("Step6: End call on PhoneC and verify PhoneA end.")
        ads[2].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_ims_conference_merge_drop_second_call_from_host_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        On PhoneA, disconnect call between A-C, verify PhoneA PhoneB still in call.
        On PhoneA, disconnect call between A-B, verify PhoneA PhoneB disconnected.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_ab_uri = get_call_uri(ads[0], call_ab_id)
        call_ac_uri = get_call_uri(ads[0], call_ac_id)

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        calls = ads[0].droid.telecomCallGetCallIds()
        calls.remove(call_conf_id)

        self.log.info("Step5: Disconnect call A-C and verify call continues.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ac_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-C.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info(
            "Step6: Disconnect call A-B and verify PhoneA PhoneB end.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ab_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-B.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_ims_conference_merge_drop_first_call_from_host_cep(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (CEP enabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA (CEP enabled IMS conference).
        On PhoneA, disconnect call between A-B, verify PhoneA PhoneC still in call.
        On PhoneA, disconnect call between A-C, verify PhoneA PhoneC disconnected.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_ab_uri = get_call_uri(ads[0], call_ab_id)
        call_ac_uri = get_call_uri(ads[0], call_ac_id)

        call_conf_id = self._merge_cep_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        calls = ads[0].droid.telecomCallGetCallIds()
        calls.remove(call_conf_id)

        self.log.info("Step5: Disconnect call A-B and verify call continues.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ab_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-B.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[2]], True):
            return False
        if not verify_incall_state(self.log, [ads[1]], False):
            return False

        self.log.info(
            "Step6: Disconnect call A-C and verify PhoneA PhoneC end.")
        call_to_disconnect = None
        for call in calls:
            if is_uri_equivalent(call_ac_uri, get_call_uri(ads[0], call)):
                call_to_disconnect = call
                calls.remove(call_to_disconnect)
                break
        if call_to_disconnect is None:
            self.log.error("Can NOT find call on host represents A-C.")
            return False
        else:
            ads[0].droid.telecomCallDisconnect(call_to_disconnect)
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_wcdma_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in WCDMA/CSFB_WCDMA call.

        PhoneA in WCDMA (or CSFB_WCDMA) call with PhoneB.
        PhoneA in WCDMA (or CSFB_WCDMA) call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 3:
            self.log.error("Total number of call ids in {} is not 3.".format(
                ads[0].serial))
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id, ads[0].droid.telecomCallGetCallState(
                        call_conf_id)))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        ads[2].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        ads[1].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _three_phone_hangup_call_verify_call_state(
            self, ad_hangup, ad_verify, call_id, call_state, ads_active):
        """Private Test utility for swap test.

        Hangup on 'ad_hangup'.
        Verify 'call_id' on 'ad_verify' is in expected 'call_state'
        Verify each ad in ads_active are 'in-call'.

        Args:
            ad_hangup: android object to hangup call.
            ad_verify: android object to verify call id state.
            call_id: call id in 'ad_verify'.
            call_state: expected state for 'call_id'.
                'call_state' is either CALL_STATE_HOLDING or CALL_STATE_ACTIVE.
            ads_active: list of android object.
                Each one of them should be 'in-call' after 'hangup' operation.

        Returns:
            True if no error happened. Otherwise False.

        """

        self.log.info("Hangup at {}, verify call continues.".format(
            ad_hangup.serial))
        ad_hangup.droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)

        if ad_verify.droid.telecomCallGetCallState(call_id) != call_state:
            self.log.error("Call_id:{}, state:{}, expected: {}".format(
                call_id, ad_verify.droid.telecomCallGetCallState(
                    call_id), call_state))
            return False
        # TODO: b/26296375 add voice check.

        if not verify_incall_state(self.log, ads_active, True):
            return False
        if not verify_incall_state(self.log, [ad_hangup], False):
            return False

        return True

    def _test_epdg_mo_mo_add_epdg_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_epdg_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_iwlan,
             is_phone_in_call_iwlan])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_epdg_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_epdg_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_iwlan,
             is_phone_in_call_iwlan])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_epdg_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (epdg) call PhoneA (epdg), accept on PhoneA.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mt_mt_add_epdg_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_iwlan,
             is_phone_in_call_iwlan])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mo_add_volte_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_volte_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_volte,
             is_phone_in_call_volte])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_volte_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_volte,
             is_phone_in_call_volte])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_volte_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (VoLTE) call PhoneA (epdg), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mt_mt_add_volte_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_volte,
             is_phone_in_call_volte])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mo_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                self.log.error(
                    "{} not GSM phone, abort wcdma swap test.".format(
                        ad.serial))
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_wcdma_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_wcdma,
             is_phone_in_call_wcdma])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                self.log.error(
                    "{} not GSM phone, abort wcdma swap test.".format(
                        ad.serial))
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_wcdma_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_wcdma,
             is_phone_in_call_wcdma])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_wcdma_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (WCDMA) call PhoneA (epdg), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are GSM phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
                self.log.error(
                    "{} not GSM phone, abort wcdma swap test.".format(
                        ad.serial))
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_wcdma_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_wcdma,
             is_phone_in_call_wcdma])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mo_add_1x_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("{} not CDMA phone, abort 1x swap test.".format(
                    ad.serial))
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mo_add_1x_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mo_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("{} not CDMA phone, abort 1x swap test.".format(
                    ad.serial))
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_1x_swap_x in test cases.
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_mt_mt_add_1x_swap_x(self, num_swaps):
        """Test swap feature in epdg call.

        PhoneB (1x) call PhoneA (epdg), accept on PhoneA.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.(N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneB and PhoneC are CDMA phone before proceed.
        for ad in [ads[1], ads[2]]:
            if (ad.droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
                self.log.error("{} not CDMA phone, abort 1x swap test.".format(
                    ad.serial))
                return None, None

        # To make thing simple, for epdg, setup should be called before calling
        # _test_epdg_mo_mt_add_1x_swap_x in test cases.
        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]], [None, None, None],
            [is_phone_in_call_iwlan, is_phone_in_call_1x, is_phone_in_call_1x])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_epdg_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in epdg call.

        PhoneA in epdg call with PhoneB.
        PhoneA in epdg call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """

        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            self.log.error("Total number of call ids in {} is not 1.".format(
                ads[0].serial))
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id, ads[0].droid.telecomCallGetCallState(
                        call_conf_id)))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        ads[2].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        ads[1].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mt_mt_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneB to PhoneA, accept on PhoneA.
        Call from PhoneC to PhoneA, accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mo_add_merge_drop_from_participant(self):
        """ Test 1x Conf Call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from DUT to PhoneC, accept on PhoneC.
        4. On DUT, merge to conference call.
        5. End call PhoneC, verify call continues on DUT and PhoneB.
        6. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        4. Merge Call succeed on DUT.
        5. PhoneC drop call, DUT and PhoneB call continues.
        6. PhoneB drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mo_add()
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Merge to Conf Call and verify Conf Call.")
        if not self._test_1x_merge_conference(ads[0], [ads[1], ads[2]],
                                              call_conf_id):
            self.log.error("1x Conference merge failed.")

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mo_add_merge_drop_from_host(self):
        """ Test 1x Conf Call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from DUT to PhoneC, accept on PhoneC.
        4. On DUT, merge to conference call.
        5. End call on DUT, make sure all participants drop.

        Expected Results:
        4. Merge Call succeed on DUT.
        5. Make sure DUT and all participants drop call.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mo_add()
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Merge to Conf Call and verify Conf Call.")
        if not self._test_1x_merge_conference(ads[0], [ads[1], ads[2]],
                                              call_conf_id):
            self.log.error("1x Conference merge failed.")

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_conf_call_drop_from_host(ads[0], [ads[2], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_drop_active(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneC, verify call continues on DUT and PhoneB.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        4. PhoneC drop call, DUT and PhoneB call continues.
        5. PhoneB drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_swap_twice_drop_active(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneC.
        7. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneB continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_swap_once_drop_active(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneB.
        6. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneC continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[1],
                                                              ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_drop_held(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneB, verify call continues on DUT and PhoneC.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        4. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        5. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[1],
                                                              ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_swap_twice_drop_held(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneB.
        7. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneC continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[1],
                                                              ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_swap_once_drop_held(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneC.
        6. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneB continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_drop_on_dut(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from DUT to PhoneB, accept on PhoneB.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call on DUT.
        5. End call on DUT.

        Expected Results:
        4. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        5. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_swap_twice_drop_on_dut(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop current call on DUT.
        7. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        7. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mo_mt_add_swap_once_drop_on_dut(self):
        """ Test 1x MO+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. DUT MO call to PhoneB, answer on PhoneB.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop current call on DUT.
        6. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. DUT drop call, PhoneB also end. Then DUT receive callback from PhoneC.
        6. DUT drop call, call also end on PhoneC.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mo_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_drop_active(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from PhoneB to DUT, accept on DUT.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneC, verify call continues on DUT and PhoneB.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        4. PhoneC drop call, DUT and PhoneB call continues.
        5. PhoneB drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_swap_twice_drop_active(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneC.
        7. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneB continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_swap_once_drop_active(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneB.
        6. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneC continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[1],
                                                              ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_drop_held(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from PhoneB to DUT, accept on DUT.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call PhoneB, verify call continues on DUT and PhoneC.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        4. PhoneB drop call, DUT and PhoneC call continues.
        5. PhoneC drop call, call also end on DUT.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[1],
                                                              ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_swap_twice_drop_held(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop on PhoneB.
        7. Drop on PhoneC.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. Call between DUT and PhoneC continues.
        7. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneB, and end call on PhoneC.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[1],
                                                              ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_swap_once_drop_held(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop on PhoneC.
        6. Drop on PhoneB.

        Expected Results:
        4. Swap call succeed.
        5. Call between DUT and PhoneB continues.
        6. All participant call end.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on PhoneC, and end call on PhoneB.")
        return self._test_1x_multi_call_drop_from_participant(ads[0], ads[2],
                                                              ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_drop_on_dut(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. Call from PhoneB to DUT, accept on DUT.
        3. Call from PhoneC to DUT, accept on DUT.
        4. End call on DUT.
        5. End call on DUT.

        Expected Results:
        4. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        5. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            0)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_swap_twice_drop_on_dut(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Swap active call on DUT.
        6. Drop current call on DUT.
        7. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. Swap call succeed.
        6. DUT drop call, PhoneC also end. Then DUT receive callback from PhoneB.
        7. DUT drop call, call also end on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            2)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[2], ads[1])

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_mt_mt_add_swap_once_drop_on_dut(self):
        """ Test 1x MT+MT call among three phones.

        Steps:
        1. DUT in 1x idle, PhoneB and PhoneC idle.
        2. PhoneB call to DUT, answer on DUT.
        3. PhoneC call to DUT, answer on DUT
        4. Swap active call on DUT.
        5. Drop current call on DUT.
        6. Drop current call on DUT.

        Expected Results:
        4. Swap call succeed.
        5. DUT drop call, PhoneB also end. Then DUT receive callback from PhoneC.
        6. DUT drop call, call also end on PhoneC.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        call_ab_id, call_ac_id, call_conf_id = self._test_1x_mt_mt_add_swap_x(
            1)
        if ((call_ab_id is None) or (call_ac_id is None) or
            (call_conf_id is None)):
            self.log.error("Failed to setup 3 way call.")
            return False

        self.log.info("Verify no one dropped call.")
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        self.log.info("End call on DUT, DUT should receive callback.")
        return self._test_1x_multi_call_drop_from_host(ads[0], ads[1], ads[2])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (VoLTE), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (VoLTE), accept on PhoneB.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (VoLTE) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneA (VOLTE) to PhoneC (WCDMA), accept on PhoneC.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (WCDMA), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (WCDMA), accept on PhoneB.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (WCDMA) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        Call from PhoneA (VOLTE) to PhoneC (1x), accept on PhoneC.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneA (VoLTE) to PhoneC (1x), accept on PhoneC.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneA (VoLTE) to PhoneB (1x), accept on PhoneB.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_merge_drop_second_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_merge_drop_second_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_merge_drop_first_call_from_participant_cep(
            self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_merge_drop_first_call_from_host_cep(self):
        """ Test VoLTE Conference Call among three phones. CEP enabled.

        1. Call from PhoneB (1x) to PhoneA (VoLTE), accept on PhoneA.
        2. Call from PhoneC (1x) to PhoneA (VoLTE), accept on PhoneA.
        3. On PhoneA, merge to conference call (VoLTE CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_drop_held(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_drop_active(self):
        """Test swap feature in VoLTE call.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_swap_twice_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_swap_twice_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mt_add_swap_twice_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mt_add_swap_twice_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_swap_once_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_swap_once_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mt_add_swap_once_drop_held(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mt_add_swap_once_drop_active(self):
        """Test swap feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mo_add_swap_twice_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mo_add_swap_twice_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mt_add_swap_twice_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mt_add_swap_twice_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mo_add_swap_once_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mo_add_swap_once_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mt_add_swap_once_drop_held(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mt_add_swap_once_drop_active(self):
        """Test swap feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_volte_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (VoLTE), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_volte_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (VoLTE), accept on PhoneB.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_volte_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (VoLTE) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (WCDMA), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (WCDMA), accept on PhoneB.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test VoLTE Conference Call among three phones. No CEP.

        PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_wcdma_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (WCDMA) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mo_add_1x_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneA (VoLTE) call PhoneC (1x), accept on PhoneC.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mo_mt_add_1x_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneA (VoLTE) call PhoneB (1x), accept on PhoneB.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. End call on PhoneB, verify call continues.
        6. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_once_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (VoLTE CEP conference call).
        5. On PhoneA disconnect call between A-B, verify call continues.
        6. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_no_cep(
            self):
        """ Test swap and merge features in VoLTE call. No CEP.

        PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        On PhoneA, merge to conference call (No CEP).
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneC, verify call continues.
        7. End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_second_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-C, verify call continues.
        7. On PhoneA disconnect call between A-B, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_first_call_from_participant_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. End call on PhoneB, verify call continues.
        7. End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_mt_mt_add_1x_swap_twice_merge_drop_first_call_from_host_cep(
            self):
        """ Test swap and merge features in VoLTE call. CEP enabled.

        1. PhoneB (1x) call PhoneA (VoLTE), accept on PhoneA.
        2. PhoneC (1x) call PhoneA (VoLTE), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. Swap active call on PhoneA.
        5. On PhoneA, merge to conference call (VoLTE CEP conference call).
        6. On PhoneA disconnect call between A-B, verify call continues.
        7. On PhoneA disconnect call between A-C, verify call continues.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_volte_mt_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mo_add_swap_once_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB_WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB_WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mo_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (CSFB WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mt_add_swap_once_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_csfb_wcdma_mo_mt_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in CSFB WCDMA call.

        PhoneA (CSFB WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (CSFB WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_csfb_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_swap_once_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mo_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneA (WCDMA) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mo_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mt_add_swap_once_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mo_mt_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneA (WCDMA) call PhoneB, accept on PhoneB.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mo_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mt_mt_add_swap_once_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneB call PhoneA (WCDMA), accept on PhoneA.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mt_mt_add_swap_twice_merge_drop(self):
        """Test swap and merge feature in WCDMA call.

        PhoneB call PhoneA (WCDMA), accept on PhoneA.
        PhoneC call PhoneA (WCDMA), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_wcdma_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_mt_mt_add_merge_unmerge_swap_drop(self):
        """Test Conference Call Unmerge operation.

        Phones A, B, C are in WCDMA Conference call (MT-MT-Merge)
        Unmerge call with B on PhoneA
        Check the number of Call Ids to be 2 on PhoneA
        Check if call AB is active since 'B' was unmerged
        Swap call to C
        Check if call AC is active
        Tear down calls
        All Phones should be in Idle

        """
        call_ab_id, call_ac_id = self._test_wcdma_mt_mt_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            self.log.error("Either of Call AB ID or Call AC ID is None.")
            return False

        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 3:
            self.log.error("Total number of call ids in {} is not 3.".format(
                ads[0].serial))
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id, ads[0].droid.telecomCallGetCallState(
                        call_conf_id)))
            return False

        # Unmerge
        self.log.info("Step5: UnMerge Conf Call into individual participants.")
        ads[0].droid.telecomCallSplitFromConf(call_ab_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))

        # Are there 2 calls?
        if num_active_calls(self.log, ads[0]) != 2:
            self.log.error("Total number of call ids in {} is not 2".format(
                ads[0].serial))
            return False

        # Unmerged calls not dropped?
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            self.log.error("Either Call_AB or Call_AC was dropped")
            return False

        # Unmerged call in call state ACTIVE?
        if ads[0].droid.telecomCallGetCallState(
                call_ab_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_ab_id, ads[0].droid.telecomCallGetCallState(
                        call_ab_id)))
            return False

        # Swap call
        self.log.info("Step6: Swap call and see if Call_AC is ACTIVE.")
        num_swaps = 1
        if not swap_calls(self.log, ads, call_ac_id, call_ab_id, num_swaps):
            self.log.error("Failed to swap calls.")
            return False

        # Other call in call state ACTIVE?
        if ads[0].droid.telecomCallGetCallState(
                call_ac_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_ac_id, ads[0].droid.telecomCallGetCallState(
                        call_ac_id)))
            return False

        # All calls still CONNECTED?
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (epdg), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (epdg), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (epdg), accept on PhoneB.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneB (epdg) to PhoneA (epdg), accept on PhoneA.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneB (epdg) to PhoneA (epdg), accept on PhoneA.
        Call from PhoneC (epdg) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_volte_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (VoLTE), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_volte_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (VoLTE), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_volte_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneC (VoLTE) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_volte_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (VoLTE), accept on PhoneB.
        Call from PhoneC (VoLTE) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_wcdma_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (WCDMA), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_wcdma_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (WCDMA), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_wcdma_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneC (WCDMA) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_wcdma_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (WCDMA), accept on PhoneB.
        Call from PhoneC (WCDMA) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_1x_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (1x), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])),
                 (phone_setup_voice_3g, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_1x_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneA (epdg) to PhoneC (1x), accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_1x_merge_drop_wfc_wifi_only(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneC (1x) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_1x_merge_drop_wfc_wifi_preferred(self):
        """ Test Conf Call among three phones.

        Call from PhoneA (epdg) to PhoneB (1x), accept on PhoneB.
        Call from PhoneC (1x) to PhoneA (epdg), accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_volte_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_volte_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_volte_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_volte_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneA (epdg) call PhoneC (VoLTE), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_volte_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_volte_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_volte_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_volte_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (VoLTE), accept on PhoneB.
        PhoneC (VoLTE) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_volte, (self.log, ads[1])), (phone_setup_volte,
                                                           (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_volte_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_wcdma_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_wcdma_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_wcdma_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_wcdma_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneA (epdg) call PhoneC (WCDMA), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_wcdma_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_wcdma_swap_once_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_wcdma_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_wcdma_swap_twice_merge_drop_wfc_wifi_preferred(
            self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (WCDMA), accept on PhoneB.
        PhoneC (WCDMA) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_wcdma_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_1x_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_1x_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_1x_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_1x_swap_twice_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneA (epdg) call PhoneC (1x), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_1x_swap_once_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_1x_swap_once_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_1x_swap_twice_merge_drop_wfc_wifi_only(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_1x_swap_twice_merge_drop_wfc_wifi_preferred(self):
        """Test swap and merge feature in epdg call.

        PhoneA (epdg) call PhoneB (1x), accept on PhoneB.
        PhoneC (1x) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_3g, (self.log, ads[1])), (phone_setup_voice_3g,
                                                        (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_1x_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_epdg_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_twice_drop_active_apm_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_twice_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(2)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_swap_once_drop_active_apm_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneA (epdg) call PhoneC (epdg), accept on PhoneC.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_drop_held_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_drop_held_wfc_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_drop_held_apm_wifi_preferred(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneC, check if call continues between AB.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False
        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_drop_active_wfc_wifi_only(self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_ONLY,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_drop_active_wfc_wifi_preferred(
            self):
        """Test swap feature in epdg call.

        PhoneA (epdg) call PhoneB (epdg), accept on PhoneB.
        PhoneC (epdg) call PhoneA (epdg), accept on PhoneA.
        Swap active call on PhoneA.
        Hangup call from PhoneB, check if call continues between AC.

        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[1],
            ad_verify=ads[0],
            call_id=call_ac_id,
            call_state=CALL_STATE_HOLDING,
            ads_active=[ads[0], ads[2]])

    def _test_gsm_mo_mo_add_swap_x(self, num_swaps):
        """Test swap feature in GSM call.

        PhoneA (GSM) call PhoneB, accept on PhoneB.
        PhoneA (GSM) call PhoneC, accept on PhoneC.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mo_add_mo(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_2g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_2g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_gsm_mt_mt_add_swap_x(self, num_swaps):
        """Test swap feature in GSM call.

        PhoneB call PhoneA (GSM), accept on PhoneA.
        PhoneC call PhoneA (GSM), accept on PhoneA.
        Swap active call on PhoneA. (N times)

        Args:
            num_swaps: do swap for 'num_swaps' times.
                This value can be 0 (no swap operation).

        Returns:
            call_ab_id, call_ac_id if succeed;
            None, None if failed.

        """
        ads = self.android_devices

        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("{} not GSM phone, abort wcdma swap test.".format(
                ads[0].serial))
            return None, None

        call_ab_id = self._three_phone_call_mt_add_mt(
            [ads[0], ads[1], ads[2]],
            [phone_setup_voice_2g, phone_setup_voice_general,
             phone_setup_voice_general], [is_phone_in_call_2g, None, None])
        if call_ab_id is None:
            self.log.error("Failed to get call_ab_id")
            return None, None

        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 2:
            return None, None
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if num_swaps > 0:
            self.log.info("Step3: Begin Swap x{} test.".format(num_swaps))
            if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                              num_swaps):
                self.log.error("Swap test failed.")
                return None, None

        return call_ab_id, call_ac_id

    def _test_gsm_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in GSM call.

        PhoneA in GSM call with PhoneB.
        PhoneA in GSM call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 3:
            self.log.error("Total number of call ids in {} is not 3.".format(
                ads[0].serial))
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        # Check if Conf Call is currently active
        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            self.log.error(
                "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                    call_conf_id, ads[0].droid.telecomCallGetCallState(
                        call_conf_id)))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        ads[2].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        self.log.info("Calls in PhoneA{}".format(calls))
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        ads[1].droid.telecomEndCall()
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_gsm_mo_mo_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_gsm_mo_mo_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_gsm_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_gsm_mo_mo_add_swap_once_drop_held(self):
        """ Test Conf Call among three phones.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneA to PhoneC, accept on PhoneC.
        On PhoneA, swap active call.
        End call on PhoneB, verify call continues.
        End call on PhoneC, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        call_ab_id, call_ac_id = self._test_gsm_mo_mo_add_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._three_phone_hangup_call_verify_call_state(
            ad_hangup=ads[2],
            ad_verify=ads[0],
            call_id=call_ab_id,
            call_state=CALL_STATE_ACTIVE,
            ads_active=[ads[0], ads[1]])

    @TelephonyBaseTest.tel_test_wrap
    def test_gsm_mt_mt_add_merge_drop(self):
        """ Test Conf Call among three phones.

        Call from PhoneB to PhoneA, accept on PhoneA.
        Call from PhoneC to PhoneA, accept on PhoneA.
        On PhoneA, merge to conference call.
        End call on PhoneC, verify call continues.
        End call on PhoneB, verify call end on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        call_ab_id, call_ac_id = self._test_gsm_mt_mt_add_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_gsm_conference_merge_drop(call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test WFC Conference Call among three phones. No CEP.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (No CEP).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mo_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneA (WFC APM WiFi Preferred) call PhoneC (WFC APM WiFi Preferred), accept on PhoneC.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mo_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test WFC Conference Call among three phones. No CEP.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (No CEP).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test WFC Conference Call among three phones. No CEP.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (No CEP).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneC, verify call continues.
        5. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-C, verify call continues.
        5. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-B continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_first_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. End call on PhoneB, verify call continues.
        5. End call on PhoneC, verify call end on PhoneA.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mt_mt_add_epdg_merge_drop_first_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test WFC Conference Call among three phones. CEP enabled.

        Steps:
        1. PhoneB (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. On PhoneA, merge to conference call (WFC CEP conference call).
        4. On PhoneA disconnect call between A-B, verify call continues.
        5. On PhoneA disconnect call between A-C, verify call continues.

        Expected Results:
        3. Conference merged successfully.
        4. Drop calls succeeded. Call between A-C continues.
        5. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mt_mt_add_epdg_swap_x(0)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_first_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_no_cep(
            self):
        """ Test swap and merge features in WFC call. No CEP.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (No CEP).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Swap operation succeeded.
        4. Conference merged successfully.
        5. Drop calls succeeded. Call between A-B continues.
        6. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, No_CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_no_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_host_wfc_apm_wifi_preferred_cep(
            self):
        """ Test swap and merge features in WFC call. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (WFC CEP conference call).
        5. On PhoneA disconnect call between A-C, verify call continues.
        6. On PhoneA disconnect call between A-B, verify call continues.

        Expected Results:
        3. Swap operation succeeded.
        4. Conference merged successfully.
        5. Drop calls succeeded. Call between A-B continues.
        6. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_host_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_epdg_mo_mt_add_epdg_swap_once_merge_drop_second_call_from_participant_wfc_apm_wifi_preferred_cep(
            self):
        """ Test swap and merge features in WFC call. CEP enabled.

        Steps:
        1. PhoneA (WFC APM WiFi Preferred) call PhoneB (WFC APM WiFi Preferred), accept on PhoneB.
        2. PhoneC (WFC APM WiFi Preferred) call PhoneA (WFC APM WiFi Preferred), accept on PhoneA.
        3. Swap active call on PhoneA.
        4. On PhoneA, merge to conference call (WFC CEP conference call).
        5. End call on PhoneC, verify call continues.
        6. End call on PhoneB, verify call end on PhoneA.

        Expected Results:
        3. Swap operation succeeded.
        4. Conference merged successfully.
        5. Drop calls succeeded. Call between A-B continues.
        6. Drop calls succeeded, all call participants drop.

        Returns:
            True if pass; False if fail.

        TAGS: Telephony, WFC, Conference, CEP
        Priority: 1
        """
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_iwlan,
                  (self.log, ads[2], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        call_ab_id, call_ac_id = self._test_epdg_mo_mt_add_epdg_swap_x(1)
        if call_ab_id is None or call_ac_id is None:
            return False

        return self._test_ims_conference_merge_drop_second_call_from_participant_cep(
            call_ab_id, call_ac_id)

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_wcdma, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wcdma_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_wcdma, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_1x, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_1x_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_1x, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_volte_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wfc_lte_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wfc_lte_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], False):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wfc_apm_add_mt_decline(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], True):
            return False
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_wfc_apm_add_mt_ignore(self):
        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], False, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1])),
                 (phone_setup_voice_general, (self.log, ads[2]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not self._three_phone_call_mo_add_mt_reject(
            [ads[0], ads[1], ads[2]], [is_phone_in_call_volte, None], False):
            return False
        return True

    """ Tests End """

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

import time
from acts.test_utils.tel.tel_defines import CALL_PROPERTY_HIGH_DEF_AUDIO
from acts.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts.test_utils.tel.tel_defines import CALL_STATE_HOLDING
from acts.test_utils.tel.tel_defines import GEN_2G
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOLTE_ENABLED
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_ENABLED
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_CDMA2000
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WCDMA
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WLAN
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import RAT_IWLAN
from acts.test_utils.tel.tel_defines import RAT_LTE
from acts.test_utils.tel.tel_defines import RAT_UMTS
from acts.test_utils.tel.tel_defines import WAIT_TIME_BETWEEN_REG_AND_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_LEAVE_VOICE_MAIL
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import NETWORK_MODE_CDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_CDMA_EVDO
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_subscription_utils import get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import get_default_data_sub_id
from acts.test_utils.tel.tel_test_utils import call_reject_leave_message
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_network_gen_for_subscription
from acts.test_utils.tel.tel_test_utils import get_network_rat
from acts.test_utils.tel.tel_test_utils import get_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import is_wfc_enabled
from acts.test_utils.tel.tel_test_utils import \
    reset_preferred_network_type_to_allowable_range
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import toggle_volte_for_subscription
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import \
    wait_for_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import wait_for_not_network_rat
from acts.test_utils.tel.tel_test_utils import wait_for_network_rat
from acts.test_utils.tel.tel_test_utils import \
    wait_for_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import wait_for_volte_enabled
from acts.test_utils.tel.tel_test_utils import \
    wait_for_voice_attach_for_subscription
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_enabled
from acts.test_utils.tel.tel_test_utils import wait_for_wfc_disabled


def two_phone_call_leave_voice_mail(
        log,
        caller,
        caller_idle_func,
        caller_in_call_check_func,
        callee,
        callee_idle_func,
        wait_time_in_call=WAIT_TIME_LEAVE_VOICE_MAIL):
    """Call from caller to callee, reject on callee, caller leave a voice mail.

    1. Caller call Callee.
    2. Callee reject incoming call.
    3. Caller leave a voice mail.
    4. Verify callee received the voice mail notification.

    Args:
        caller: caller android device object.
        caller_idle_func: function to check caller's idle state.
        caller_in_call_check_func: function to check caller's in-call state.
        callee: callee android device object.
        callee_idle_func: function to check callee's idle state.
        wait_time_in_call: time to wait when leaving a voice mail.
            This is optional, default is WAIT_TIME_LEAVE_VOICE_MAIL

    Returns:
        True: if voice message is received on callee successfully.
        False: for errors
    """

    ads = [caller, callee]

    # Make sure phones are idle.
    ensure_phones_idle(log, ads)
    if caller_idle_func and not caller_idle_func(log, caller):
        log.error("Caller Failed to Reselect")
        return False
    if callee_idle_func and not callee_idle_func(log, callee):
        log.error("Callee Failed to Reselect")
        return False

    # TODO: b/26337871 Need to use proper API to check phone registered.
    time.sleep(WAIT_TIME_BETWEEN_REG_AND_CALL)

    # Make call and leave a message.
    if not call_reject_leave_message(
            log, caller, callee, caller_in_call_check_func, wait_time_in_call):
        log.error("make a call and leave a message failed.")
        return False
    return True


def two_phone_call_short_seq(log,
                             phone_a,
                             phone_a_idle_func,
                             phone_a_in_call_check_func,
                             phone_b,
                             phone_b_idle_func,
                             phone_b_in_call_check_func,
                             call_sequence_func=None,
                             wait_time_in_call=WAIT_TIME_IN_CALL):
    """Call process short sequence.
    1. Ensure phone idle and in idle_func check return True.
    2. Call from PhoneA to PhoneB, accept on PhoneB.
    3. Check phone state, hangup on PhoneA.
    4. Ensure phone idle and in idle_func check return True.
    5. Call from PhoneA to PhoneB, accept on PhoneB.
    6. Check phone state, hangup on PhoneB.

    Args:
        phone_a: PhoneA's android device object.
        phone_a_idle_func: function to check PhoneA's idle state.
        phone_a_in_call_check_func: function to check PhoneA's in-call state.
        phone_b: PhoneB's android device object.
        phone_b_idle_func: function to check PhoneB's idle state.
        phone_b_in_call_check_func: function to check PhoneB's in-call state.
        call_sequence_func: default parameter, not implemented.
        wait_time_in_call: time to wait in call.
            This is optional, default is WAIT_TIME_IN_CALL

    Returns:
        True: if call sequence succeed.
        False: for errors
    """
    ads = [phone_a, phone_b]

    call_params = [(ads[0], ads[1], ads[0], phone_a_in_call_check_func,
                    phone_b_in_call_check_func),
                   (ads[0], ads[1], ads[1], phone_a_in_call_check_func,
                    phone_b_in_call_check_func), ]

    for param in call_params:
        # Make sure phones are idle.
        ensure_phones_idle(log, ads)
        if phone_a_idle_func and not phone_a_idle_func(log, phone_a):
            log.error("Phone A Failed to Reselect")
            return False
        if phone_b_idle_func and not phone_b_idle_func(log, phone_b):
            log.error("Phone B Failed to Reselect")
            return False

        # TODO: b/26337871 Need to use proper API to check phone registered.
        time.sleep(WAIT_TIME_BETWEEN_REG_AND_CALL)

        # Make call.
        log.info("---> Call test: {} to {} <---".format(param[0].serial, param[
            1].serial))
        if not call_setup_teardown(log,
                                   *param,
                                   wait_time_in_call=wait_time_in_call):
            log.error("Call Iteration Failed")
            return False

    return True


def two_phone_call_long_seq(log,
                            phone_a,
                            phone_a_idle_func,
                            phone_a_in_call_check_func,
                            phone_b,
                            phone_b_idle_func,
                            phone_b_in_call_check_func,
                            call_sequence_func=None,
                            wait_time_in_call=WAIT_TIME_IN_CALL):
    """Call process long sequence.
    1. Ensure phone idle and in idle_func check return True.
    2. Call from PhoneA to PhoneB, accept on PhoneB.
    3. Check phone state, hangup on PhoneA.
    4. Ensure phone idle and in idle_func check return True.
    5. Call from PhoneA to PhoneB, accept on PhoneB.
    6. Check phone state, hangup on PhoneB.
    7. Ensure phone idle and in idle_func check return True.
    8. Call from PhoneB to PhoneA, accept on PhoneA.
    9. Check phone state, hangup on PhoneA.
    10. Ensure phone idle and in idle_func check return True.
    11. Call from PhoneB to PhoneA, accept on PhoneA.
    12. Check phone state, hangup on PhoneB.

    Args:
        phone_a: PhoneA's android device object.
        phone_a_idle_func: function to check PhoneA's idle state.
        phone_a_in_call_check_func: function to check PhoneA's in-call state.
        phone_b: PhoneB's android device object.
        phone_b_idle_func: function to check PhoneB's idle state.
        phone_b_in_call_check_func: function to check PhoneB's in-call state.
        call_sequence_func: default parameter, not implemented.
        wait_time_in_call: time to wait in call.
            This is optional, default is WAIT_TIME_IN_CALL

    Returns:
        True: if call sequence succeed.
        False: for errors
    """
    ads = [phone_a, phone_b]

    call_params = [(ads[0], ads[1], ads[0], phone_a_in_call_check_func,
                    phone_b_in_call_check_func),
                   (ads[0], ads[1], ads[1], phone_a_in_call_check_func,
                    phone_b_in_call_check_func),
                   (ads[1], ads[0], ads[0], phone_b_in_call_check_func,
                    phone_a_in_call_check_func),
                   (ads[1], ads[0], ads[1], phone_b_in_call_check_func,
                    phone_a_in_call_check_func), ]

    for param in call_params:
        # Make sure phones are idle.
        ensure_phones_idle(log, ads)
        if phone_a_idle_func and not phone_a_idle_func(log, phone_a):
            log.error("Phone A Failed to Reselect")
            return False
        if phone_b_idle_func and not phone_b_idle_func(log, phone_b):
            log.error("Phone B Failed to Reselect")
            return False

        # TODO: b/26337871 Need to use proper API to check phone registered.
        time.sleep(WAIT_TIME_BETWEEN_REG_AND_CALL)

        # Make call.
        log.info("---> Call test: {} to {} <---".format(param[0].serial, param[
            1].serial))
        if not call_setup_teardown(log,
                                   *param,
                                   wait_time_in_call=wait_time_in_call):
            log.error("Call Iteration Failed")
            return False

    return True

def phone_setup_iwlan(log,
                      ad,
                      is_airplane_mode,
                      wfc_mode,
                      wifi_ssid=None,
                      wifi_pwd=None):
    """Phone setup function for epdg call test.
    Set WFC mode according to wfc_mode.
    Set airplane mode according to is_airplane_mode.
    Make sure phone connect to WiFi. (If wifi_ssid is not None.)
    Wait for phone to be in iwlan data network type.
    Wait for phone to report wfc enabled flag to be true.

    Args:
        log: Log object.
        ad: Android device object.
        is_airplane_mode: True to turn on airplane mode. False to turn off airplane mode.
        wfc_mode: WFC mode to set to.
        wifi_ssid: WiFi network SSID. This is optional.
            If wifi_ssid is None, then phone_setup_iwlan will not attempt to connect to wifi.
        wifi_pwd: WiFi network password. This is optional.

    Returns:
        True if success. False if fail.
    """
    return phone_setup_iwlan_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad), is_airplane_mode, wfc_mode,
        wifi_ssid, wifi_pwd)


def phone_setup_iwlan_for_subscription(log,
                                       ad,
                                       sub_id,
                                       is_airplane_mode,
                                       wfc_mode,
                                       wifi_ssid=None,
                                       wifi_pwd=None):
    """Phone setup function for epdg call test for subscription id.
    Set WFC mode according to wfc_mode.
    Set airplane mode according to is_airplane_mode.
    Make sure phone connect to WiFi. (If wifi_ssid is not None.)
    Wait for phone to be in iwlan data network type.
    Wait for phone to report wfc enabled flag to be true.

    Args:
        log: Log object.
        ad: Android device object.
        sub_id: subscription id.
        is_airplane_mode: True to turn on airplane mode. False to turn off airplane mode.
        wfc_mode: WFC mode to set to.
        wifi_ssid: WiFi network SSID. This is optional.
            If wifi_ssid is None, then phone_setup_iwlan will not attempt to connect to wifi.
        wifi_pwd: WiFi network password. This is optional.

    Returns:
        True if success. False if fail.
    """

    toggle_airplane_mode(log, ad, False)

    if ad.droid.imsIsEnhanced4gLteModeSettingEnabledByPlatform():
        toggle_volte(log, ad, True)

    if not is_airplane_mode and not ensure_network_generation_for_subscription(
            log,
            ad,
            sub_id,
            GEN_4G,
            voice_or_data=NETWORK_SERVICE_DATA):
        return False

    if not set_wfc_mode(log, ad, wfc_mode):
        log.error("{} set WFC mode failed.".format(ad.serial))
        return False

    toggle_airplane_mode(log, ad, is_airplane_mode)

    if wifi_ssid is not None:
        if not ensure_wifi_connected(log, ad, wifi_ssid, wifi_pwd):
            log.error("{} connect to WiFi failed.".format(ad.serial))
            return False

    return phone_idle_iwlan_for_subscription(log, ad, sub_id)


def phone_setup_iwlan_cellular_preferred(log,
                                         ad,
                                         wifi_ssid=None,
                                         wifi_pwd=None):
    """Phone setup function for iwlan Non-APM CELLULAR_PREFERRED test.
    Set WFC mode according to CELLULAR_PREFERRED.
    Set airplane mode according to False.
    Make sure phone connect to WiFi. (If wifi_ssid is not None.)
    Make sure phone don't report iwlan data network type.
    Make sure phone don't report wfc enabled flag to be true.

    Args:
        log: Log object.
        ad: Android device object.
        wifi_ssid: WiFi network SSID. This is optional.
            If wifi_ssid is None, then phone_setup_iwlan will not attempt to connect to wifi.
        wifi_pwd: WiFi network password. This is optional.

    Returns:
        True if success. False if fail.
    """
    toggle_airplane_mode(log, ad, False)
    toggle_volte(log, ad, True)
    if not ensure_network_generation(log,
                                     ad,
                                     GEN_4G,
                                     voice_or_data=NETWORK_SERVICE_DATA):
        return False
    if not set_wfc_mode(log, ad, WFC_MODE_CELLULAR_PREFERRED):
        log.error("{} set WFC mode failed.".format(ad.serial))
        return False
    if wifi_ssid is not None:
        if not ensure_wifi_connected(log, ad, wifi_ssid, wifi_pwd):
            log.error("{} connect to WiFi failed.".format(ad.serial))
            return False
    if not wait_for_not_network_rat(log,
                                    ad,
                                    RAT_FAMILY_WLAN,
                                    voice_or_data=NETWORK_SERVICE_DATA):
        log.error("{} data rat in iwlan mode.".format(ad.serial))
        return False
    elif not wait_for_wfc_disabled(log, ad, MAX_WAIT_TIME_WFC_ENABLED):
        log.error("{} should report wifi calling disabled within {}s.".format(
            ad.serial, MAX_WAIT_TIME_WFC_ENABLED))
        return False
    return True

def phone_setup_data_for_subscription(log, ad, sub_id, network_generation):
    """Setup Phone <sub_id> Data to <network_generation>

    Args:
        log: log object
        ad: android device object
        sub_id: subscription id
        network_generation: network generation, e.g. GEN_2G, GEN_3G, GEN_4G

    Returns:
        True if success, False if fail.
    """
    toggle_airplane_mode(log, ad, False)
    if not set_wfc_mode(log, ad, WFC_MODE_DISABLED):
        log.error("{} Disable WFC failed.".format(ad.serial))
        return False
    if not ensure_network_generation_for_subscription(
            log,
            ad,
            sub_id,
            network_generation,
            voice_or_data=NETWORK_SERVICE_DATA):
        return False
    return True

def phone_setup_4g(log, ad):
    """Setup Phone default data sub_id data to 4G.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    return phone_setup_4g_for_subscription(
        log, ad, get_default_data_sub_id(ad))

def phone_setup_4g_for_subscription(log, ad, sub_id):
    """Setup Phone <sub_id> Data to 4G.

    Args:
        log: log object
        ad: android device object
        sub_id: subscription id

    Returns:
        True if success, False if fail.
    """
    return phone_setup_data_for_subscription(log, ad, sub_id, GEN_4G)

def phone_setup_3g(log, ad):
    """Setup Phone default data sub_id data to 3G.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    return phone_setup_3g_for_subscription(
        log, ad, get_default_data_sub_id(ad))

def phone_setup_3g_for_subscription(log, ad, sub_id):
    """Setup Phone <sub_id> Data to 3G.

    Args:
        log: log object
        ad: android device object
        sub_id: subscription id

    Returns:
        True if success, False if fail.
    """
    return phone_setup_data_for_subscription(log, ad, sub_id, GEN_3G)

def phone_setup_2g(log, ad):
    """Setup Phone default data sub_id data to 2G.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    return phone_setup_2g_for_subscription(
        log, ad, get_default_data_sub_id(ad))

def phone_setup_2g_for_subscription(log, ad, sub_id):
    """Setup Phone <sub_id> Data to 3G.

    Args:
        log: log object
        ad: android device object
        sub_id: subscription id

    Returns:
        True if success, False if fail.
    """
    return phone_setup_data_for_subscription(log, ad, sub_id, GEN_2G)

def phone_setup_csfb(log, ad):
    """Setup phone for CSFB call test.

    Setup Phone to be in 4G mode.
    Disabled VoLTE.

    Args:
        log: log object
        ad: Android device object.

    Returns:
        True if setup successfully.
        False for errors.
    """
    return phone_setup_csfb_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad))


def phone_setup_csfb_for_subscription(log, ad, sub_id):
    """Setup phone for CSFB call test for subscription id.

    Setup Phone to be in 4G mode.
    Disabled VoLTE.

    Args:
        log: log object
        ad: Android device object.
        sub_id: subscription id.

    Returns:
        True if setup successfully.
        False for errors.
    """
    if not phone_setup_4g_for_subscription(log, ad, sub_id):
        log.error("{} failed to set to 4G data.".format(ad.serial))
        return False
    if ad.droid.imsIsEnhanced4gLteModeSettingEnabledByPlatform():
        toggle_volte(log, ad, False)
    if not ensure_network_generation_for_subscription(
            log,
            ad,
            sub_id,
            GEN_4G,
            voice_or_data=NETWORK_SERVICE_DATA):
        return False

    if not wait_for_voice_attach_for_subscription(log, ad, sub_id,
                                                  MAX_WAIT_TIME_NW_SELECTION):
        return False

    return phone_idle_csfb_for_subscription(log, ad, sub_id)

def phone_setup_volte(log, ad):
    """Setup VoLTE enable.

    Args:
        log: log object
        ad: android device object.

    Returns:
        True: if VoLTE is enabled successfully.
        False: for errors
    """
    return phone_setup_volte_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def phone_setup_volte_for_subscription(log, ad, sub_id):
    """Setup VoLTE enable for subscription id.

    Args:
        log: log object
        ad: android device object.
        sub_id: subscription id.

    Returns:
        True: if VoLTE is enabled successfully.
        False: for errors
    """
    if not phone_setup_4g_for_subscription(log, ad, sub_id):
        log.error("{} failed to set to 4G data.".format(ad.serial))
        return False
    toggle_volte_for_subscription(log, ad, sub_id, True)
    return phone_idle_volte_for_subscription(log, ad, sub_id)

def phone_setup_voice_3g(log, ad):
    """Setup phone voice to 3G.

    Args:
        log: log object
        ad: Android device object.

    Returns:
        True if setup successfully.
        False for errors.
    """
    return phone_setup_voice_3g_for_subscription(log, ad,
                                                 get_outgoing_voice_sub_id(ad))

def phone_setup_voice_3g_for_subscription(log, ad, sub_id):
    """Setup phone voice to 3G for subscription id.

    Args:
        log: log object
        ad: Android device object.
        sub_id: subscription id.

    Returns:
        True if setup successfully.
        False for errors.
    """
    if not phone_setup_3g_for_subscription(log, ad, sub_id):
        log.error("{} failed to set to 3G data.".format(ad.serial))
        return False
    if not wait_for_voice_attach_for_subscription(log, ad, sub_id,
                                                  MAX_WAIT_TIME_NW_SELECTION):
        return False
    return phone_idle_3g_for_subscription(log, ad, sub_id)

def phone_setup_voice_2g(log, ad):
    """Setup phone voice to 2G.

    Args:
        log: log object
        ad: Android device object.

    Returns:
        True if setup successfully.
        False for errors.
    """
    return phone_setup_voice_2g_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def phone_setup_voice_2g_for_subscription(log, ad, sub_id):
    """Setup phone voice to 2G for subscription id.

    Args:
        log: log object
        ad: Android device object.
        sub_id: subscription id.

    Returns:
        True if setup successfully.
        False for errors.
    """
    if not phone_setup_2g_for_subscription(log, ad, sub_id):
        log.error("{} failed to set to 2G data.".format(ad.serial))
        return False
    if not wait_for_voice_attach_for_subscription(log, ad, sub_id,
                                                  MAX_WAIT_TIME_NW_SELECTION):
        return False
    return phone_idle_2g_for_subscription(log, ad, sub_id)

def phone_setup_voice_general(log, ad):
    """Setup phone for voice general call test.

    Make sure phone attached to voice.
    Make necessary delay.

    Args:
        ad: Android device object.

    Returns:
        True if setup successfully.
        False for errors.
    """
    return phone_setup_voice_general_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def phone_setup_voice_general_for_subscription(log, ad, sub_id):
    """Setup phone for voice general call test for subscription id.

    Make sure phone attached to voice.
    Make necessary delay.

    Args:
        ad: Android device object.
        sub_id: subscription id.

    Returns:
        True if setup successfully.
        False for errors.
    """
    toggle_airplane_mode(log, ad, False)
    if not wait_for_voice_attach_for_subscription(log, ad, sub_id,
                                                  MAX_WAIT_TIME_NW_SELECTION):
        # if phone can not attach voice, try phone_setup_voice_3g
        return phone_setup_voice_3g_for_subscription(log, ad, sub_id)
    return True

def phone_setup_data_general(log, ad):
    """Setup phone for data general test.

    Make sure phone attached to data.
    Make necessary delay.

    Args:
        ad: Android device object.

    Returns:
        True if setup successfully.
        False for errors.
    """
    return phone_setup_data_general_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultDataSubId())

def phone_setup_data_general_for_subscription(log, ad, sub_id):
    """Setup phone for data general test for subscription id.

    Make sure phone attached to data.
    Make necessary delay.

    Args:
        ad: Android device object.
        sub_id: subscription id.

    Returns:
        True if setup successfully.
        False for errors.
    """
    toggle_airplane_mode(log, ad, False)
    if not wait_for_data_attach_for_subscription(log, ad, sub_id,
                                                 MAX_WAIT_TIME_NW_SELECTION):
        # if phone can not attach data, try reset network preference settings
        reset_preferred_network_type_to_allowable_range(log, ad)

    return wait_for_data_attach_for_subscription(log, ad, sub_id,
                                                 MAX_WAIT_TIME_NW_SELECTION)

def phone_setup_rat_for_subscription(log, ad, sub_id, network_preference,
                                     rat_family):
    toggle_airplane_mode(log, ad, False)
    if not set_wfc_mode(log, ad, WFC_MODE_DISABLED):
        log.error("{} Disable WFC failed.".format(ad.serial))
        return False
    return ensure_network_rat_for_subscription(log, ad, sub_id,
                                               network_preference, rat_family)


def phone_setup_lte_gsm_wcdma(log, ad):
    return phone_setup_lte_gsm_wcdma_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId())


def phone_setup_lte_gsm_wcdma_for_subscription(log, ad, sub_id):
    return phone_setup_rat_for_subscription(
        log, ad, sub_id, NETWORK_MODE_LTE_GSM_WCDMA, RAT_FAMILY_LTE)


def phone_setup_gsm_umts(log, ad):
    return phone_setup_gsm_umts_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId())


def phone_setup_gsm_umts_for_subscription(log, ad, sub_id):
    return phone_setup_rat_for_subscription(
        log, ad, sub_id, NETWORK_MODE_GSM_UMTS, RAT_FAMILY_WCDMA)


def phone_setup_gsm_only(log, ad):
    return phone_setup_gsm_only_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId())


def phone_setup_gsm_only_for_subscription(log, ad, sub_id):
    return phone_setup_rat_for_subscription(
        log, ad, sub_id, NETWORK_MODE_GSM_ONLY, RAT_FAMILY_GSM)


def phone_setup_lte_cdma_evdo(log, ad):
    return phone_setup_lte_cdma_evdo_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId())


def phone_setup_lte_cdma_evdo_for_subscription(log, ad, sub_id):
    return phone_setup_rat_for_subscription(
        log, ad, sub_id, NETWORK_MODE_LTE_CDMA_EVDO, RAT_FAMILY_LTE)


def phone_setup_cdma(log, ad):
    return phone_setup_cdma_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId())


def phone_setup_cdma_for_subscription(log, ad, sub_id):
    return phone_setup_rat_for_subscription(log, ad, sub_id, NETWORK_MODE_CDMA,
                                            RAT_FAMILY_CDMA2000)


def phone_idle_volte(log, ad):
    """Return if phone is idle for VoLTE call test.

    Args:
        ad: Android device object.
    """
    return phone_idle_volte_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad))


def phone_idle_volte_for_subscription(log, ad, sub_id):
    """Return if phone is idle for VoLTE call test for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not wait_for_network_rat_for_subscription(
            log,
            ad,
            sub_id,
            RAT_FAMILY_LTE,
            voice_or_data=NETWORK_SERVICE_VOICE):
        log.error("{} voice rat not in LTE mode.".format(ad.serial))
        return False
    if not wait_for_volte_enabled(log, ad, MAX_WAIT_TIME_VOLTE_ENABLED):
        log.error(
            "{} failed to <report volte enabled true> within {}s.".format(
                ad.serial, MAX_WAIT_TIME_VOLTE_ENABLED))
        return False
    return True


def phone_idle_iwlan(log, ad):
    """Return if phone is idle for WiFi calling call test.

    Args:
        ad: Android device object.
    """
    return phone_idle_iwlan_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad))


def phone_idle_iwlan_for_subscription(log, ad, sub_id):
    """Return if phone is idle for WiFi calling call test for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not wait_for_network_rat_for_subscription(
            log,
            ad,
            sub_id,
            RAT_FAMILY_WLAN,
            voice_or_data=NETWORK_SERVICE_DATA):
        log.error("{} data rat not in iwlan mode.".format(ad.serial))
        return False
    if not wait_for_wfc_enabled(log, ad, MAX_WAIT_TIME_WFC_ENABLED):
        log.error("{} failed to <report wfc enabled true> within {}s.".format(
            ad.serial, MAX_WAIT_TIME_WFC_ENABLED))
        return False
    return True


def phone_idle_csfb(log, ad):
    """Return if phone is idle for CSFB call test.

    Args:
        ad: Android device object.
    """
    return phone_idle_csfb_for_subscription(log, ad,
                                            get_outgoing_voice_sub_id(ad))


def phone_idle_csfb_for_subscription(log, ad, sub_id):
    """Return if phone is idle for CSFB call test for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not wait_for_network_rat_for_subscription(
            log,
            ad,
            sub_id,
            RAT_FAMILY_LTE,
            voice_or_data=NETWORK_SERVICE_DATA):
        log.error("{} data rat not in lte mode.".format(ad.serial))
        return False
    return True


def phone_idle_3g(log, ad):
    """Return if phone is idle for 3G call test.

    Args:
        ad: Android device object.
    """
    return phone_idle_3g_for_subscription(log, ad,
                                          get_outgoing_voice_sub_id(ad))


def phone_idle_3g_for_subscription(log, ad, sub_id):
    """Return if phone is idle for 3G call test for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    return wait_for_network_generation_for_subscription(
        log,
        ad,
        sub_id,
        GEN_3G,
        voice_or_data=NETWORK_SERVICE_VOICE)


def phone_idle_2g(log, ad):
    """Return if phone is idle for 2G call test.

    Args:
        ad: Android device object.
    """
    return phone_idle_2g_for_subscription(log, ad,
                                          get_outgoing_voice_sub_id(ad))


def phone_idle_2g_for_subscription(log, ad, sub_id):
    """Return if phone is idle for 2G call test for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    return wait_for_network_generation_for_subscription(
        log,
        ad,
        sub_id,
        GEN_2G,
        voice_or_data=NETWORK_SERVICE_VOICE)


def is_phone_in_call_volte(log, ad):
    """Return if phone is in VoLTE call.

    Args:
        ad: Android device object.
    """
    return is_phone_in_call_volte_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_volte_for_subscription(log, ad, sub_id):
    """Return if phone is in VoLTE call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_type = get_network_rat_for_subscription(log, ad, sub_id,
                                               NETWORK_SERVICE_VOICE)
    if nw_type != RAT_LTE:
        log.error("{} voice rat on: {}. Expected: LTE".format(ad.serial,
                                                              nw_type))
        return False
    return True


def is_phone_in_call_csfb(log, ad):
    """Return if phone is in CSFB call.

    Args:
        ad: Android device object.
    """
    return is_phone_in_call_csfb_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_csfb_for_subscription(log, ad, sub_id):
    """Return if phone is in CSFB call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_type = get_network_rat_for_subscription(log, ad, sub_id,
                                               NETWORK_SERVICE_VOICE)
    if nw_type == RAT_LTE:
        log.error("{} voice rat on: {}. Expected: not LTE".format(ad.serial,
                                                                  nw_type))
        return False
    return True


def is_phone_in_call_3g(log, ad):
    """Return if phone is in 3G call.

    Args:
        ad: Android device object.
    """
    return is_phone_in_call_3g_for_subscription(log, ad,
                                                get_outgoing_voice_sub_id(ad))


def is_phone_in_call_3g_for_subscription(log, ad, sub_id):
    """Return if phone is in 3G call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_gen = get_network_gen_for_subscription(log, ad, sub_id,
                                              NETWORK_SERVICE_VOICE)
    if nw_gen != GEN_3G:
        log.error("{} voice rat on: {}. Expected: 3g".format(ad.serial,
                                                             nw_gen))
        return False
    return True


def is_phone_in_call_2g(log, ad):
    """Return if phone is in 2G call.

    Args:
        ad: Android device object.
    """
    return is_phone_in_call_2g_for_subscription(log, ad,
                                                get_outgoing_voice_sub_id(ad))


def is_phone_in_call_2g_for_subscription(log, ad, sub_id):
    """Return if phone is in 2G call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_gen = get_network_gen_for_subscription(log, ad, sub_id,
                                              NETWORK_SERVICE_VOICE)
    if nw_gen != GEN_2G:
        log.error("{} voice rat on: {}. Expected: 2g".format(ad.serial,
                                                             nw_gen))
        return False
    return True


def is_phone_in_call_1x(log, ad):
    """Return if phone is in 1x call.

    Args:
        ad: Android device object.
    """
    return is_phone_in_call_1x_for_subscription(log, ad,
                                                get_outgoing_voice_sub_id(ad))


def is_phone_in_call_1x_for_subscription(log, ad, sub_id):
    """Return if phone is in 1x call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_type = get_network_rat_for_subscription(log, ad, sub_id,
                                               NETWORK_SERVICE_VOICE)
    if nw_type != RAT_1XRTT:
        log.error("{} voice rat on: {}. Expected: 1xrtt".format(ad.serial,
                                                                nw_type))
        return False
    return True


def is_phone_in_call_wcdma(log, ad):
    """Return if phone is in WCDMA call.

    Args:
        ad: Android device object.
    """
    return is_phone_in_call_wcdma_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_wcdma_for_subscription(log, ad, sub_id):
    """Return if phone is in WCDMA call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    # Currently checking 'umts'.
    # Changes may needed in the future.
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_type = get_network_rat_for_subscription(log, ad, sub_id,
                                               NETWORK_SERVICE_VOICE)
    if nw_type != RAT_UMTS:
        log.error("{} voice rat on: {}. Expected: umts".format(ad.serial,
                                                               nw_type))
        return False
    return True


def is_phone_in_call_iwlan(log, ad):
    """Return if phone is in WiFi call.

    Args:
        ad: Android device object.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_type = get_network_rat(log, ad, NETWORK_SERVICE_DATA)
    if nw_type != RAT_IWLAN:
        log.error("{} data rat on: {}. Expected: iwlan".format(ad.serial,
                                                               nw_type))
        return False
    if not is_wfc_enabled(log, ad):
        log.error("{} WiFi Calling feature bit is False.".format(ad.serial))
        return False
    return True


def is_phone_in_call_not_iwlan(log, ad):
    """Return if phone is in WiFi call for subscription id.

    Args:
        ad: Android device object.
        sub_id: subscription id.
    """
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    nw_type = get_network_rat(log, ad, NETWORK_SERVICE_DATA)
    if nw_type == RAT_IWLAN:
        log.error("{} data rat on: {}. Expected: not iwlan".format(ad.serial,
                                                                   nw_type))
        return False
    if is_wfc_enabled(log, ad):
        log.error("{} WiFi Calling feature bit is True.".format(ad.serial))
        return False
    return True


def swap_calls(log,
               ads,
               call_hold_id,
               call_active_id,
               num_swaps=1,
               check_call_status=True):
    """PhoneA in call with B and C. Swap active/holding call on PhoneA.

    Swap call and check status on PhoneA.
        (This step may have multiple times according to 'num_swaps'.)
    Check if all 3 phones are 'in-call'.

    Args:
        ads: list of ad object, at least three need to pass in.
            Swap operation will happen on ads[0].
            ads[1] and ads[2] are call participants.
        call_hold_id: id for the holding call in ads[0].
            call_hold_id should be 'STATE_HOLDING' when calling this function.
        call_active_id: id for the active call in ads[0].
            call_active_id should be 'STATE_ACTIVE' when calling this function.
        num_swaps: how many swap/check operations will be done before return.
        check_call_status: This is optional. Default value is True.
            If this value is True, then call status (active/hold) will be
            be checked after each swap operation.

    Returns:
        If no error happened, return True, otherwise, return False.
    """
    if check_call_status:
        # Check status before swap.
        if ads[0].droid.telecomCallGetCallState(
                call_active_id) != CALL_STATE_ACTIVE:
            log.error("Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                call_active_id, ads[0].droid.telecomCallGetCallState(
                    call_active_id)))
            return False
        if ads[0].droid.telecomCallGetCallState(
                call_hold_id) != CALL_STATE_HOLDING:
            log.error("Call_id:{}, state:{}, expected: STATE_HOLDING".format(
                call_hold_id, ads[0].droid.telecomCallGetCallState(
                    call_hold_id)))
            return False

    i = 1
    while (i <= num_swaps):
        log.info("swap_test: {}. {} swap and check call status.".format(i, ads[
            0].serial))
        ads[0].droid.telecomCallHold(call_active_id)
        time.sleep(WAIT_TIME_IN_CALL)
        # Swap object reference
        call_active_id, call_hold_id = call_hold_id, call_active_id
        if check_call_status:
            # Check status
            if ads[0].droid.telecomCallGetCallState(
                    call_active_id) != CALL_STATE_ACTIVE:
                log.error(
                    "Call_id:{}, state:{}, expected: STATE_ACTIVE".format(
                        call_active_id, ads[0].droid.telecomCallGetCallState(
                            call_active_id)))
                return False
            if ads[0].droid.telecomCallGetCallState(
                    call_hold_id) != CALL_STATE_HOLDING:
                log.error(
                    "Call_id:{}, state:{}, expected: STATE_HOLDING".format(
                        call_hold_id, ads[0].droid.telecomCallGetCallState(
                            call_hold_id)))
                return False
        # TODO: b/26296375 add voice check.

        i += 1

    #In the end, check all three phones are 'in-call'.
    if not verify_incall_state(log, [ads[0], ads[1], ads[2]], True):
        return False

    return True


def get_audio_route(log, ad):
    """Gets the audio route for the active call

    Args:
        log: logger object
        ad: android_device object

    Returns:
        Audio route string ["BLUETOOTH", "EARPIECE", "SPEAKER", "WIRED_HEADSET"
            "WIRED_OR_EARPIECE"]
    """

    audio_state = ad.droid.telecomCallGetAudioState()
    return audio_state["AudioRoute"]


def set_audio_route(log, ad, route):
    """Sets the audio route for the active call

    Args:
        log: logger object
        ad: android_device object
        route: string ["BLUETOOTH", "EARPIECE", "SPEAKER", "WIRED_HEADSET"
            "WIRED_OR_EARPIECE"]

    Returns:
        If no error happened, return True, otherwise, return False.
    """
    ad.droid.telecomCallSetAudioRoute(route)
    return True


def is_property_in_call_properties(log, ad, call_id, expected_property):
    """Return if the call_id has the expected property

    Args:
        log: logger object
        ad: android_device object
        call_id: call id.
        expected_property: expected property.

    Returns:
        True if call_id has expected_property. False if not.
    """
    properties = ad.droid.telecomCallGetProperties(call_id)
    return (expected_property in properties)


def is_call_hd(log, ad, call_id):
    """Return if the call_id is HD call.

    Args:
        log: logger object
        ad: android_device object
        call_id: call id.

    Returns:
        True if call_id is HD call. False if not.
    """
    return is_property_in_call_properties(log, ad, call_id,
                                          CALL_PROPERTY_HIGH_DEF_AUDIO)


def get_cep_conference_call_id(ad):
    """Get CEP conference call id if there is an ongoing CEP conference call.

    Args:
        ad: android device object.

    Returns:
        call id for CEP conference call if there is an ongoing CEP conference call.
        None otherwise.
    """
    for call in ad.droid.telecomCallGetCallIds():
        if len(ad.droid.telecomCallGetCallChildren(call)) != 0:
            return call
    return None

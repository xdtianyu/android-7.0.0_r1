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

from future import standard_library
standard_library.install_aliases()

import concurrent.futures
import urllib.parse
import time

from queue import Empty
from acts.controllers.android_device import AndroidDevice
from acts.controllers.event_dispatcher import EventDispatcher
from acts.test_utils.tel.tel_defines import AOSP_PREFIX
from acts.test_utils.tel.tel_defines import CARRIER_UNKNOWN
from acts.test_utils.tel.tel_defines import DATA_STATE_CONNECTED
from acts.test_utils.tel.tel_defines import DATA_STATE_DISCONNECTED
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import GEN_UNKNOWN
from acts.test_utils.tel.tel_defines import INCALL_UI_DISPLAY_BACKGROUND
from acts.test_utils.tel.tel_defines import INCALL_UI_DISPLAY_FOREGROUND
from acts.test_utils.tel.tel_defines import INVALID_SIM_SLOT_INDEX
from acts.test_utils.tel.tel_defines import INVALID_SUB_ID
from acts.test_utils.tel.tel_defines import MAX_SAVED_VOICE_MAIL
from acts.test_utils.tel.tel_defines import MAX_SCREEN_ON_TIME
from acts.test_utils.tel.tel_defines import \
    MAX_WAIT_TIME_ACCEPT_CALL_TO_OFFHOOK_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_AIRPLANEMODE_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_INITIATION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALLEE_RINGING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CONNECTION_STATE_UPDATE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_DATA_SUB_CHANGE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_IDLE_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_SMS_RECEIVE
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_SMS_SENT_SUCCESS
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TELECOM_RINGING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOICE_MAIL_COUNT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_DISABLED
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_WFC_ENABLED
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_CONNECTION_TYPE_CELL
from acts.test_utils.tel.tel_defines import NETWORK_CONNECTION_TYPE_WIFI
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_7_DIGIT
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_10_DIGIT
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_11_DIGIT
from acts.test_utils.tel.tel_defines import PHONE_NUMBER_STRING_FORMAT_12_DIGIT
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WLAN
from acts.test_utils.tel.tel_defines import RAT_FAMILY_WCDMA
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import RAT_UNKNOWN
from acts.test_utils.tel.tel_defines import SERVICE_STATE_EMERGENCY_ONLY
from acts.test_utils.tel.tel_defines import SERVICE_STATE_IN_SERVICE
from acts.test_utils.tel.tel_defines import SERVICE_STATE_OUT_OF_SERVICE
from acts.test_utils.tel.tel_defines import SERVICE_STATE_POWER_OFF
from acts.test_utils.tel.tel_defines import SIM_STATE_READY
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_IDLE
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_OFFHOOK
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_RINGING
from acts.test_utils.tel.tel_defines import VOICEMAIL_DELETE_DIGIT
from acts.test_utils.tel.tel_defines import WAIT_TIME_1XRTT_VOICE_ATTACH
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_CHANGE_DATA_SUB_ID
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_LEAVE_VOICE_MAIL
from acts.test_utils.tel.tel_defines import WAIT_TIME_REJECT_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import EventCallStateChanged
from acts.test_utils.tel.tel_defines import EventConnectivityChanged
from acts.test_utils.tel.tel_defines import EventDataConnectionStateChanged
from acts.test_utils.tel.tel_defines import EventDataSmsReceived
from acts.test_utils.tel.tel_defines import EventMessageWaitingIndicatorChanged
from acts.test_utils.tel.tel_defines import EventServiceStateChanged
from acts.test_utils.tel.tel_defines import EventMmsSentSuccess
from acts.test_utils.tel.tel_defines import EventSmsReceived
from acts.test_utils.tel.tel_defines import EventSmsSentSuccess
from acts.test_utils.tel.tel_defines import CallStateContainer
from acts.test_utils.tel.tel_defines import DataConnectionStateContainer
from acts.test_utils.tel.tel_defines import MessageWaitingIndicatorContainer
from acts.test_utils.tel.tel_defines import NetworkCallbackContainer
from acts.test_utils.tel.tel_defines import ServiceStateContainer
from acts.test_utils.tel.tel_lookup_tables import \
    connection_type_from_type_string
from acts.test_utils.tel.tel_lookup_tables import is_valid_rat
from acts.test_utils.tel.tel_lookup_tables import get_allowable_network_preference
from acts.test_utils.tel.tel_lookup_tables import \
    get_voice_mail_count_check_function
from acts.test_utils.tel.tel_lookup_tables import get_voice_mail_number_function
from acts.test_utils.tel.tel_lookup_tables import \
    network_preference_for_generaton
from acts.test_utils.tel.tel_lookup_tables import operator_name_from_plmn_id
from acts.test_utils.tel.tel_lookup_tables import \
    rat_families_for_network_preference
from acts.test_utils.tel.tel_lookup_tables import rat_family_for_generation
from acts.test_utils.tel.tel_lookup_tables import rat_family_from_rat
from acts.test_utils.tel.tel_lookup_tables import rat_generation_from_rat
from acts.test_utils.tel.tel_subscription_utils import \
    get_default_data_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_outgoing_message_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_incoming_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import \
    get_incoming_message_sub_id
from acts.utils import load_config
from acts.logger import LoggerProxy
log = LoggerProxy()


class TelTestUtilsError(Exception):
    pass


def setup_droid_properties(log, ad, sim_filename):

    # Check to see if droid already has this property
    if hasattr(ad, 'cfg'):
        return

    device_props = {}
    device_props['subscription'] = {}

    try:
        sim_data = load_config(sim_filename)
    except Exception:
        log.warning("Failed to load {}!".format(sim_filename))
        sim_data = None
    sub_info_list = ad.droid.subscriptionGetAllSubInfoList()
    found_sims = 0
    for sub_info in sub_info_list:
        sub_id = sub_info['subscriptionId']
        if sub_info['simSlotIndex'] is not INVALID_SIM_SLOT_INDEX:
            found_sims += 1
            sim_record = {}
            try:
                sim_serial = ad.droid.telephonyGetSimSerialNumberForSubscription(
                    sub_id)
                if not sim_serial:
                    log.error("Unable to find ICC-ID for SIM on {}!".format(
                        ad.serial))
                if sim_data is not None:
                    number = sim_data[sim_serial]["phone_num"]
                else:
                    raise KeyError("No file to load phone number info!")
            except KeyError:
                number = ad.droid.telephonyGetLine1NumberForSubscription(
                    sub_id)
            if not number or number == "":
                raise TelTestUtilsError(
                    "Failed to find valid phone number for {}"
                    .format(ad.serial))

            sim_record['phone_num'] = number
            sim_record['operator'] = get_operator_name(log, ad, sub_id)
            device_props['subscription'][sub_id] = sim_record
            log.info(
                "phone_info: <{}:{}>, <subId:{}> {} <{}>, ICC-ID:<{}>".format(
                    ad.model, ad.serial, sub_id, number, get_operator_name(
                        log, ad, sub_id),
                    ad.droid.telephonyGetSimSerialNumberForSubscription(
                        sub_id)))

    if found_sims == 0:
        log.warning("No Valid SIMs found in device {}".format(ad.serial))

    setattr(ad, 'cfg', device_props)


def refresh_droid_config(log, ad):
    """ Update Android Device cfg records for each sub_id.
    1. Update Phone Number using Line1Number (if Line1Number is valid).
    2. Update Operator name.

    Args:
        log: log object
        ad: android device object

    Returns:
        None
    """
    for sub_id in ad.cfg['subscription']:
        # Update Phone number
        number = ad.droid.telephonyGetLine1NumberForSubscription(sub_id)
        if number:
            number = phone_number_formatter(number)
            ad.cfg['subscription'][sub_id]['phone_num'] = number
        # Update Operator Name
        ad.cfg['subscription'][sub_id]['operator'] = get_operator_name(log, ad,
                                                                       sub_id)


def get_slot_index_from_subid(log, ad, sub_id):
    try:
        info = ad.droid.subscriptionGetSubInfoForSubscriber(sub_id)
        return info['simSlotIndex']
    except KeyError:
        return INVALID_SIM_SLOT_INDEX


def get_num_active_sims(log, ad):
    """ Get the number of active SIM cards by counting slots

    Args:
        ad: android_device object.

    Returns:
        result: The number of loaded (physical) SIM cards
    """
    # using a dictionary as a cheap way to prevent double counting
    # in the situation where multiple subscriptions are on the same SIM.
    # yes, this is a corner corner case.
    valid_sims = {}
    subInfo = ad.droid.subscriptionGetAllSubInfoList()
    for info in subInfo:
        ssidx = info['simSlotIndex']
        if ssidx == INVALID_SIM_SLOT_INDEX:
            continue
        valid_sims[ssidx] = True
    return len(valid_sims.keys())


def toggle_airplane_mode(log, ad, new_state=None):
    """ Toggle the state of airplane mode.

    Args:
        ad: android_device object.
        new_state: Airplane mode state to set to.
            If None, opposite of the current state.

    Returns:
        result: True if operation succeed. False if error happens.
    """
    return toggle_airplane_mode_msim(log, ad, new_state)


def is_expected_event(event_to_check, events_list):
    """ check whether event is present in the event list

    Args:
        event_to_check: event to be checked.
        events_list: list of events
    Returns:
        result: True if event present in the list. False if not.
    """
    for event in events_list:
        if event in event_to_check['name']:
            return True
    return False


def is_sim_ready(log, ad, sim_slot_id=None):
    """ check whether SIM is ready.

    Args:
        ad: android_device object.
        sim_slot_id: check the SIM status for sim_slot_id
            This is optional. If this is None, check default SIM.

    Returns:
        result: True if all SIMs are ready. False if not.
    """
    if sim_slot_id is None:
        status = ad.droid.telephonyGetSimState()
    else:
        status = ad.droid.telephonyGetSimStateForSlotId(sim_slot_id)
    if status != SIM_STATE_READY:
        log.info("Sim not ready")
        return False
    return True


def _is_expecting_event(event_recv_list):
    """ check for more event is expected in event list

    Args:
        event_recv_list: list of events
    Returns:
        result: True if more events are expected. False if not.
    """
    for state in event_recv_list:
        if state is False:
            return True
    return False


def _set_event_list(event_recv_list, sub_id_list, sub_id, value):
    """ set received event in expected event list

    Args:
        event_recv_list: list of received events
        sub_id_list: subscription ID list
        sub_id: subscription id of current event
        value: True or False
    Returns:
        None.
    """
    for i in range(len(sub_id_list)):
        if sub_id_list[i] == sub_id:
            event_recv_list[i] = value


def toggle_airplane_mode_msim(log, ad, new_state=None):
    """ Toggle the state of airplane mode.

    Args:
        ad: android_device object.
        new_state: Airplane mode state to set to.
            If None, opposite of the current state.

    Returns:
        result: True if operation succeed. False if error happens.
    """
    serial_number = ad.serial

    ad.ed.clear_all_events()
    sub_id_list = []

    active_sub_info = ad.droid.subscriptionGetAllSubInfoList()
    for info in active_sub_info:
        sub_id_list.append(info['subscriptionId'])

    cur_state = ad.droid.connectivityCheckAirplaneMode()
    if cur_state == new_state:
        log.info("Airplane mode already <{}> on {}".format(new_state,
                                                           serial_number))
        return True
    elif new_state is None:
        log.info("Current State {} New state {}".format(cur_state, new_state))

    if new_state is None:
        new_state = not cur_state

    service_state_list = []
    if new_state:
        service_state_list.append(SERVICE_STATE_POWER_OFF)
        log.info("Turn on airplane mode: " + serial_number)

    else:
        # If either one of these 3 events show up, it should be OK.
        # Normal SIM, phone in service
        service_state_list.append(SERVICE_STATE_IN_SERVICE)
        # NO SIM, or Dead SIM, or no Roaming coverage.
        service_state_list.append(SERVICE_STATE_OUT_OF_SERVICE)
        service_state_list.append(SERVICE_STATE_EMERGENCY_ONLY)
        log.info("Turn off airplane mode: " + serial_number)

    for sub_id in sub_id_list:
        ad.droid.telephonyStartTrackingServiceStateChangeForSubscription(
            sub_id)
    ad.droid.connectivityToggleAirplaneMode(new_state)

    event = None

    try:
        try:
            event = ad.ed.wait_for_event(
                EventServiceStateChanged,
                is_event_match_for_list,
                timeout=MAX_WAIT_TIME_AIRPLANEMODE_EVENT,
                field=ServiceStateContainer.SERVICE_STATE,
                value_list=service_state_list)
        except Empty:
            pass
        if event is None:
            log.error("Did not get expected service state {}".format(
                service_state_list))
        log.info("Received event: {}".format(event))
    finally:
        for sub_id in sub_id_list:
            ad.droid.telephonyStopTrackingServiceStateChangeForSubscription(
                sub_id)

    if new_state:
        if (not ad.droid.connectivityCheckAirplaneMode() or
                ad.droid.wifiCheckState() or ad.droid.bluetoothCheckState()):
            log.error("Airplane mode ON fail on {}".format(ad.serial))
            return False
    else:
        if ad.droid.connectivityCheckAirplaneMode():
            log.error("Airplane mode OFF fail on {}".format(ad.serial))
            return False
    return True


def wait_and_answer_call(log,
                         ad,
                         incoming_number=None,
                         incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND):
    """Wait for an incoming call on default voice subscription and
       accepts the call.

    Args:
        ad: android device object.
        incoming_number: Expected incoming number.
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True: if incoming call is received and answered successfully.
        False: for errors
        """
    return wait_and_answer_call_for_subscription(
        log, ad, get_incoming_voice_sub_id(ad), incoming_number,
        incall_ui_display)


def wait_for_ringing_event(log, ad, wait_time):
    """Wait for ringing event.

    Args:
        log: log object.
        ad: android device object.
        wait_time: max time to wait for ringing event.

    Returns:
        event_ringing if received ringing event.
        otherwise return None.
    """
    log.info("Wait for ringing.")
    start_time = time.time()
    remaining_time = wait_time
    event_iter_timeout = 4
    event_ringing = None

    while remaining_time > 0:
        try:
            event_ringing = ad.ed.wait_for_event(
                EventCallStateChanged,
                is_event_match,
                timeout=event_iter_timeout,
                field=CallStateContainer.CALL_STATE,
                value=TELEPHONY_STATE_RINGING)
        except Empty:
            if ad.droid.telecomIsRinging():
                log.error("No Ringing event. But Callee in Ringing state.")
                log.error("Test framework dropped event.")
                return None
        remaining_time = start_time + wait_time - time.time()
        if event_ringing is not None:
            break
    if event_ringing is None:
        log.error("No Ringing Event, Callee not in ringing state.")
        log.error("No incoming call.")
        return None

    return event_ringing


def wait_and_answer_call_for_subscription(
        log,
        ad,
        sub_id,
        incoming_number=None,
        incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND):
    """Wait for an incoming call on specified subscription and
       accepts the call.

    Args:
        ad: android device object.
        sub_id: subscription ID
        incoming_number: Expected incoming number.
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True: if incoming call is received and answered successfully.
        False: for errors
    """
    ad.ed.clear_all_events()
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    if (not ad.droid.telecomIsRinging() and
            ad.droid.telephonyGetCallStateForSubscription(sub_id) !=
            TELEPHONY_STATE_RINGING):
        try:
            event_ringing = wait_for_ringing_event(
                log, ad, MAX_WAIT_TIME_CALLEE_RINGING)
            if event_ringing is None:
                log.error("No Ringing Event.")
                return False
        finally:
            ad.droid.telephonyStopTrackingCallStateChangeForSubscription(
                sub_id)

        if not incoming_number:
            result = True
        else:
            result = check_phone_number_match(
                event_ringing['data'][CallStateContainer.INCOMING_NUMBER],
                incoming_number)

        if not result:
            log.error("Incoming Number not match")
            log.error("Expected number:{}, actual number:{}".format(
                incoming_number, event_ringing['data'][
                    CallStateContainer.INCOMING_NUMBER]))
            return False

    ad.ed.clear_all_events()
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    if not wait_for_telecom_ringing(log, ad, MAX_WAIT_TIME_TELECOM_RINGING):
        log.error("Telecom is not ringing.")
        return False
    log.info("Accept on callee.")
    ad.droid.telecomAcceptRingingCall()
    try:
        ad.ed.wait_for_event(
            EventCallStateChanged,
            is_event_match,
            timeout=MAX_WAIT_TIME_ACCEPT_CALL_TO_OFFHOOK_EVENT,
            field=CallStateContainer.CALL_STATE,
            value=TELEPHONY_STATE_OFFHOOK)
    except Empty:
        if not ad.droid.telecomIsInCall():
            log.error("Accept call failed.")
            return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChangeForSubscription(sub_id)
    if incall_ui_display == INCALL_UI_DISPLAY_FOREGROUND:
        ad.droid.telecomShowInCallScreen()
    elif incall_ui_display == INCALL_UI_DISPLAY_BACKGROUND:
        ad.droid.showHomeScreen()
    return True


def wait_and_reject_call(log,
                         ad,
                         incoming_number=None,
                         delay_reject=WAIT_TIME_REJECT_CALL,
                         reject=True):
    """Wait for an incoming call on default voice subscription and
       reject the call.

    Args:
        ad: android device object.
        incoming_number: Expected incoming number.
            Optional. Default is None
        delay_reject: time to wait before rejecting the call
            Optional. Default is WAIT_TIME_REJECT_CALL

    Returns:
        True: if incoming call is received and reject successfully.
        False: for errors
    """
    return wait_and_reject_call_for_subscription(
        log, ad, get_incoming_voice_sub_id(ad), incoming_number, delay_reject,
        reject)


def wait_and_reject_call_for_subscription(log,
                                          ad,
                                          sub_id,
                                          incoming_number=None,
                                          delay_reject=WAIT_TIME_REJECT_CALL,
                                          reject=True):
    """Wait for an incoming call on specific subscription and
       reject the call.

    Args:
        ad: android device object.
        sub_id: subscription ID
        incoming_number: Expected incoming number.
            Optional. Default is None
        delay_reject: time to wait before rejecting the call
            Optional. Default is WAIT_TIME_REJECT_CALL

    Returns:
        True: if incoming call is received and reject successfully.
        False: for errors
    """
    ad.ed.clear_all_events()
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    if (not ad.droid.telecomIsRinging() and
            ad.droid.telephonyGetCallStateForSubscription(sub_id) !=
            TELEPHONY_STATE_RINGING):
        try:
            event_ringing = wait_for_ringing_event(
                log, ad, MAX_WAIT_TIME_CALLEE_RINGING)
            if event_ringing is None:
                log.error("No Ringing Event.")
                return False
        finally:
            ad.droid.telephonyStopTrackingCallStateChangeForSubscription(
                sub_id)

        if not incoming_number:
            result = True
        else:
            result = check_phone_number_match(
                event_ringing['data'][CallStateContainer.INCOMING_NUMBER],
                incoming_number)

        if not result:
            log.error("Incoming Number not match")
            log.error("Expected number:{}, actual number:{}".format(
                incoming_number, event_ringing['data'][
                    CallStateContainer.INCOMING_NUMBER]))
            return False

    ad.ed.clear_all_events()
    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)
    if reject is True:
        # Delay between ringing and reject.
        time.sleep(delay_reject)
        log.info("Reject on callee.")
        is_find = False
        # Loop the call list and find the matched one to disconnect.
        for call in ad.droid.telecomCallGetCallIds():
            if check_phone_number_match(
                    get_number_from_tel_uri(get_call_uri(ad, call)),
                    incoming_number):
                ad.droid.telecomCallDisconnect(call)
                is_find = True
        if is_find is False:
            log.error("Did not find matching call to reject.")
            return False
    else:
        # don't reject on callee. Just ignore the incoming call.
        log.info("Received incoming call. Ignore it.")
    try:
        ad.ed.wait_for_event(
            EventCallStateChanged,
            is_event_match_for_list,
            timeout=MAX_WAIT_TIME_CALL_IDLE_EVENT,
            field=CallStateContainer.CALL_STATE,
            value_list=[TELEPHONY_STATE_IDLE, TELEPHONY_STATE_OFFHOOK])
    except Empty:
        log.error("No onCallStateChangedIdle event received.")
        return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChangeForSubscription(sub_id)
    return True


def hangup_call(log, ad):
    """Hang up ongoing active call.
    """
    ad.ed.clear_all_events()
    ad.droid.telephonyStartTrackingCallState()
    log.info("Hangup call.")
    ad.droid.telecomEndCall()

    try:
        ad.ed.wait_for_event(EventCallStateChanged,
                             is_event_match,
                             timeout=MAX_WAIT_TIME_CALL_IDLE_EVENT,
                             field=CallStateContainer.CALL_STATE,
                             value=TELEPHONY_STATE_IDLE)
    except Empty:
        if ad.droid.telecomIsInCall():
            log.error("Hangup call failed.")
            return False
    finally:
        ad.droid.telephonyStopTrackingCallStateChange()
    return True


def disconnect_call_by_id(log, ad, call_id):
    """Disconnect call by call id.
    """
    ad.droid.telecomCallDisconnect(call_id)
    return True

def _phone_number_remove_prefix(number):
    """Remove the country code and other prefix from the input phone number.
    Currently only handle phone number with the following formats:
        (US phone number format)
        +1abcxxxyyyy
        1abcxxxyyyy
        abcxxxyyyy
        abc xxx yyyy
        abc.xxx.yyyy
        abc-xxx-yyyy
        (EEUK phone number format)
        +44abcxxxyyyy
        0abcxxxyyyy

    Args:
        number: input phone number

    Returns:
        Phone number without country code or prefix
    """
    if number is None:
        return None, None
    country_code_list = ["+1", "+44"]
    for country_code in country_code_list:
        if number.startswith(country_code):
            return number[len(country_code):], country_code
    if number[0] == "1" or number[0] == "0":
        return number[1:], None
    return number, None


def check_phone_number_match(number1, number2):
    """Check whether two input phone numbers match or not.

    Compare the two input phone numbers.
    If they match, return True; otherwise, return False.
    Currently only handle phone number with the following formats:
        (US phone number format)
        +1abcxxxyyyy
        1abcxxxyyyy
        abcxxxyyyy
        abc xxx yyyy
        abc.xxx.yyyy
        abc-xxx-yyyy
        (EEUK phone number format)
        +44abcxxxyyyy
        0abcxxxyyyy

        There are some scenarios we can not verify, one example is:
            number1 = +15555555555, number2 = 5555555555
            (number2 have no country code)

    Args:
        number1: 1st phone number to be compared.
        number2: 2nd phone number to be compared.

    Returns:
        True if two phone numbers match. Otherwise False.
    """
    # Remove country code and prefix
    number1, country_code1 = _phone_number_remove_prefix(number1)
    number2, country_code2 = _phone_number_remove_prefix(number2)
    if ((country_code1 is not None) and
        (country_code2 is not None) and
        (country_code1 != country_code2)):
        return False
    # Remove white spaces, dashes, dots
    number1 = phone_number_formatter(number1)
    number2 = phone_number_formatter(number2)
    return number1 == number2


def initiate_call(log, ad_caller, callee_number, emergency=False):
    """Make phone call from caller to callee.

    Args:
        ad_caller: Caller android device object.
        callee_number: Callee phone number.
        emergency : specify the call is emergency.
            Optional. Default value is False.

    Returns:
        result: if phone call is placed successfully.
    """
    ad_caller.ed.clear_all_events()
    sub_id = get_outgoing_voice_sub_id(ad_caller)
    ad_caller.droid.telephonyStartTrackingCallStateForSubscription(sub_id)

    wait_time_for_incall_state = MAX_WAIT_TIME_CALL_INITIATION

    try:
        # Make a Call
        if emergency:
            ad_caller.droid.telecomCallEmergencyNumber(callee_number)
        else:
            ad_caller.droid.telecomCallNumber(callee_number)

        # Verify OFFHOOK event
        if ad_caller.droid.telephonyGetCallState() != TELEPHONY_STATE_OFFHOOK:
            event_offhook = ad_caller.ed.wait_for_event(
                EventCallStateChanged,
                is_event_match,
                timeout=wait_time_for_incall_state,
                field=CallStateContainer.CALL_STATE,
                value=TELEPHONY_STATE_OFFHOOK)
    except Empty:
        log.error("initiate_call did not receive Telephony OFFHOOK event.")
        return False
    finally:
        ad_caller.droid.telephonyStopTrackingCallStateChangeForSubscription(
            sub_id)

    # Verify call state
    while wait_time_for_incall_state > 0:
        wait_time_for_incall_state -= 1
        if (ad_caller.droid.telecomIsInCall() and
            (ad_caller.droid.telephonyGetCallState() ==
             TELEPHONY_STATE_OFFHOOK) and
            (ad_caller.droid.telecomGetCallState() ==
             TELEPHONY_STATE_OFFHOOK)):
            return True
        time.sleep(1)
    log.error("Make call fail. telecomIsInCall:{}, Telecom State:{},"
              " Telephony State:{}".format(ad_caller.droid.telecomIsInCall(
              ), ad_caller.droid.telephonyGetCallState(
              ), ad_caller.droid.telecomGetCallState()))
    return False


def call_reject(log, ad_caller, ad_callee, reject=True):
    """Caller call Callee, then reject on callee.


    """
    subid_caller = ad_caller.droid.subscriptionGetDefaultVoiceSubId()
    subid_callee = ad_callee.incoming_voice_sub_id
    log.info("Sub-ID Caller {}, Sub-ID Callee {}".format(subid_caller,
                                                         subid_callee))
    return call_reject_for_subscription(log, ad_caller, ad_callee,
                                        subid_caller, subid_callee, reject)


def call_reject_for_subscription(log,
                                 ad_caller,
                                 ad_callee,
                                 subid_caller,
                                 subid_callee,
                                 reject=True):
    """
    """

    class _CallSequenceException(Exception):
        pass

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    log.info("Call from {} to {}".format(caller_number, callee_number))
    try:
        if not initiate_call(log, ad_caller, callee_number):
            raise _CallSequenceException("Initiate call failed.")

        if not wait_and_reject_call_for_subscription(
                log, ad_callee, subid_callee, caller_number,
                WAIT_TIME_REJECT_CALL, reject):
            raise _CallSequenceException("Reject call fail.")
        # Check if incoming call is cleared on callee or not.
        if ad_callee.droid.telephonyGetCallStateForSubscription(
                subid_callee) == TELEPHONY_STATE_RINGING:
            raise _CallSequenceException("Incoming call is not cleared.")
        # Hangup on caller
        hangup_call(log, ad_caller)
    except _CallSequenceException as e:
        log.error(e)
        return False
    return True


def call_reject_leave_message(log,
                              ad_caller,
                              ad_callee,
                              verify_caller_func=None,
                              wait_time_in_call=WAIT_TIME_LEAVE_VOICE_MAIL):
    """On default voice subscription, Call from caller to callee,
    reject on callee, caller leave a voice mail.

    1. Caller call Callee.
    2. Callee reject incoming call.
    3. Caller leave a voice mail.
    4. Verify callee received the voice mail notification.

    Args:
        ad_caller: caller android device object.
        ad_callee: callee android device object.
        verify_caller_func: function to verify caller is in correct state while in-call.
            This is optional, default is None.
        wait_time_in_call: time to wait when leaving a voice mail.
            This is optional, default is WAIT_TIME_LEAVE_VOICE_MAIL

    Returns:
        True: if voice message is received on callee successfully.
        False: for errors
    """
    subid_caller = get_outgoing_voice_sub_id(ad_caller)
    subid_callee = get_incoming_voice_sub_id(ad_callee)
    return call_reject_leave_message_for_subscription(
        log, ad_caller, ad_callee, subid_caller, subid_callee,
        verify_caller_func, wait_time_in_call)


def call_reject_leave_message_for_subscription(
        log,
        ad_caller,
        ad_callee,
        subid_caller,
        subid_callee,
        verify_caller_func=None,
        wait_time_in_call=WAIT_TIME_LEAVE_VOICE_MAIL):
    """On specific voice subscription, Call from caller to callee,
    reject on callee, caller leave a voice mail.

    1. Caller call Callee.
    2. Callee reject incoming call.
    3. Caller leave a voice mail.
    4. Verify callee received the voice mail notification.

    Args:
        ad_caller: caller android device object.
        ad_callee: callee android device object.
        subid_caller: caller's subscription id.
        subid_callee: callee's subscription id.
        verify_caller_func: function to verify caller is in correct state while in-call.
            This is optional, default is None.
        wait_time_in_call: time to wait when leaving a voice mail.
            This is optional, default is WAIT_TIME_LEAVE_VOICE_MAIL

    Returns:
        True: if voice message is received on callee successfully.
        False: for errors
    """

    class _CallSequenceException(Exception):
        pass
    # Currently this test utility only works for TMO and ATT and SPT.
    # It does not work for VZW (see b/21559800)
    # "with VVM TelephonyManager APIs won't work for vm"

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    log.info("Call from {} to {}".format(caller_number, callee_number))

    try:

        if not initiate_call(log, ad_caller, callee_number):
            raise _CallSequenceException("Initiate call failed.")

        if not wait_and_reject_call_for_subscription(
                log,
                ad_callee,
                subid_callee,
                incoming_number=caller_number):
            raise _CallSequenceException("Reject call fail.")

        ad_callee.droid.telephonyStartTrackingVoiceMailStateChangeForSubscription(
            subid_callee)
        voice_mail_count_before = ad_callee.droid.telephonyGetVoiceMailCountForSubscription(
            subid_callee)

        # -1 means there are unread voice mail, but the count is unknown
        # 0 means either this API not working (VZW) or no unread voice mail.
        if voice_mail_count_before != 0:
            log.warning("--Pending new Voice Mail, please clear on phone.--")

        # ensure that all internal states are updated in telecom
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        ad_callee.ed.clear_all_events()

        if verify_caller_func and not verify_caller_func(log, ad_caller):
            raise _CallSequenceException("Caller not in correct state!")

        # TODO: b/26293512 Need to play some sound to leave message.
        # Otherwise carrier voice mail server may drop this voice mail.

        time.sleep(wait_time_in_call)

        if not verify_caller_func:
            caller_state_result = ad_caller.droid.telecomIsInCall()
        else:
            caller_state_result = verify_caller_func(log, ad_caller)
        if not caller_state_result:
            raise _CallSequenceException(
                "Caller not in correct state after {} seconds".format(
                    wait_time_in_call))

        if not hangup_call(log, ad_caller):
            raise _CallSequenceException("Error in Hanging-Up Call")

        log.info("Wait for voice mail indicator on callee.")
        try:
            event = ad_callee.ed.wait_for_event(
                EventMessageWaitingIndicatorChanged,
                _is_on_message_waiting_event_true)
            log.info(event)
        except Empty:
            raise _CallSequenceException("No expected event {}.".format(
                EventMessageWaitingIndicatorChanged))
        voice_mail_count_after = ad_callee.droid.telephonyGetVoiceMailCountForSubscription(
            subid_callee)
        log.info(
            "telephonyGetVoiceMailCount output - before: {}, after: {}".format(
                voice_mail_count_before, voice_mail_count_after))

        # voice_mail_count_after should:
        # either equals to (voice_mail_count_before + 1) [For ATT and SPT]
        # or equals to -1 [For TMO]
        # -1 means there are unread voice mail, but the count is unknown
        if not check_voice_mail_count(log, ad_callee, voice_mail_count_before,
                                      voice_mail_count_after):
            log.error("telephonyGetVoiceMailCount output is incorrect.")
            return False

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        ad_callee.droid.telephonyStopTrackingVoiceMailStateChangeForSubscription(
            subid_callee)
    return True


def call_voicemail_erase_all_pending_voicemail(log, ad):
    """Script for phone to erase all pending voice mail.
    This script only works for TMO and ATT and SPT currently.
    This script only works if phone have already set up voice mail options,
    and phone should disable password protection for voice mail.

    1. If phone don't have pending voice message, return True.
    2. Dial voice mail number.
        For TMO, the number is '123'
        For ATT, the number is phone's number
        For SPT, the number is phone's number
    3. Wait for voice mail connection setup.
    4. Wait for voice mail play pending voice message.
    5. Send DTMF to delete one message.
        The digit is '7'.
    6. Repeat steps 4 and 5 until voice mail server drop this call.
        (No pending message)
    6. Check telephonyGetVoiceMailCount result. it should be 0.

    Args:
        log: log object
        ad: android device object
    Returns:
        False if error happens. True is succeed.
    """
    log.info("Erase all pending voice mail.")
    if ad.droid.telephonyGetVoiceMailCount() == 0:
        log.info("No Pending voice mail.")
        return True

    voice_mail_number = get_voice_mail_number(log, ad)

    if not initiate_call(log, ad, voice_mail_number):
        log.error("Initiate call failed.")
        return False
    time.sleep(WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE)
    callId = ad.droid.telecomCallGetCallIds()[0]
    time.sleep(WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE)
    count = MAX_SAVED_VOICE_MAIL
    while (is_phone_in_call(log, ad) and (count > 0)):
        log.info("Press 7 to delete voice mail.")
        ad.droid.telecomCallPlayDtmfTone(callId, VOICEMAIL_DELETE_DIGIT)
        ad.droid.telecomCallStopDtmfTone(callId)
        time.sleep(WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE)
        count -= 1
    log.info("Voice mail server dropped this call.")
    # wait for telephonyGetVoiceMailCount to update correct result
    remaining_time = MAX_WAIT_TIME_VOICE_MAIL_COUNT
    while ((remaining_time > 0) and
           (ad.droid.telephonyGetVoiceMailCount() != 0)):
        time.sleep(1)
        remaining_time -= 1
    current_voice_mail_count = ad.droid.telephonyGetVoiceMailCount()
    log.info("telephonyGetVoiceMailCount: {}".format(current_voice_mail_count))
    return (current_voice_mail_count == 0)


def _is_on_message_waiting_event_true(event):
    """Private function to return if the received EventMessageWaitingIndicatorChanged
    event MessageWaitingIndicatorContainer.IS_MESSAGE_WAITING field is True.
    """
    return event['data'][MessageWaitingIndicatorContainer.IS_MESSAGE_WAITING]


def call_setup_teardown(log,
                        ad_caller,
                        ad_callee,
                        ad_hangup=None,
                        verify_caller_func=None,
                        verify_callee_func=None,
                        wait_time_in_call=WAIT_TIME_IN_CALL,
                        incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND):
    """ Call process, including make a phone call from caller,
    accept from callee, and hang up. The call is on default voice subscription

    In call process, call from <droid_caller> to <droid_callee>,
    accept the call, (optional)then hang up from <droid_hangup>.

    Args:
        ad_caller: Caller Android Device Object.
        ad_callee: Callee Android Device Object.
        ad_hangup: Android Device Object end the phone call.
            Optional. Default value is None, and phone call will continue.
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True if call process without any error.
        False if error happened.

    """
    subid_caller = get_outgoing_voice_sub_id(ad_caller)
    subid_callee = get_incoming_voice_sub_id(ad_callee)
    log.info("Sub-ID Caller {}, Sub-ID Callee {}".format(subid_caller,
                                                         subid_callee))
    return call_setup_teardown_for_subscription(
        log, ad_caller, ad_callee, subid_caller, subid_callee, ad_hangup,
        verify_caller_func, verify_callee_func, wait_time_in_call,
        incall_ui_display)


def call_setup_teardown_for_subscription(
        log,
        ad_caller,
        ad_callee,
        subid_caller,
        subid_callee,
        ad_hangup=None,
        verify_caller_func=None,
        verify_callee_func=None,
        wait_time_in_call=WAIT_TIME_IN_CALL,
        incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND):
    """ Call process, including make a phone call from caller,
    accept from callee, and hang up. The call is on specified subscription

    In call process, call from <droid_caller> to <droid_callee>,
    accept the call, (optional)then hang up from <droid_hangup>.

    Args:
        ad_caller: Caller Android Device Object.
        ad_callee: Callee Android Device Object.
        subid_caller: Caller subscription ID
        subid_callee: Callee subscription ID
        ad_hangup: Android Device Object end the phone call.
            Optional. Default value is None, and phone call will continue.
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        verify_call_mode_caller: func_ptr to verify caller in correct mode
            Optional. Default is None
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True if call process without any error.
        False if error happened.

    """
    CHECK_INTERVAL = 3

    class _CallSequenceException(Exception):
        pass

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    log.info("Call from {} to {}".format(caller_number, callee_number))

    try:
        if not initiate_call(log, ad_caller, callee_number):
            raise _CallSequenceException("Initiate call failed.")

        if not wait_and_answer_call_for_subscription(
                log,
                ad_callee,
                subid_callee,
                incoming_number=caller_number,
                incall_ui_display=incall_ui_display):
            raise _CallSequenceException("Answer call fail.")

        # ensure that all internal states are updated in telecom
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if verify_caller_func and not verify_caller_func(log, ad_caller):
            raise _CallSequenceException("Caller not in correct state!")
        if verify_callee_func and not verify_callee_func(log, ad_callee):
            raise _CallSequenceException("Callee not in correct state!")

        elapsed_time = 0
        while (elapsed_time < wait_time_in_call):
            CHECK_INTERVAL = min(CHECK_INTERVAL,
                                 wait_time_in_call - elapsed_time)
            time.sleep(CHECK_INTERVAL)
            elapsed_time += CHECK_INTERVAL
            if not verify_caller_func:
                caller_state_result = ad_caller.droid.telecomIsInCall()
            else:
                caller_state_result = verify_caller_func(log, ad_caller)
            if not caller_state_result:
                raise _CallSequenceException(
                    "Caller not in correct state at <{}>/<{}> second.".format(
                        elapsed_time, wait_time_in_call))
            if not verify_callee_func:
                callee_state_result = ad_callee.droid.telecomIsInCall()
            else:
                callee_state_result = verify_callee_func(log, ad_callee)
            if not callee_state_result:
                raise _CallSequenceException(
                    "Callee not in correct state at <{}>/<{}> second.".format(
                        elapsed_time, wait_time_in_call))

        if not ad_hangup:
            return True

        if not hangup_call(log, ad_hangup):
            raise _CallSequenceException("Error in Hanging-Up Call")

        return True

    except _CallSequenceException as e:
        log.error(e)
        return False
    finally:
        if ad_hangup:
            for ad in [ad_caller, ad_callee]:
                try:
                    if ad.droid.telecomIsInCall():
                        ad.droid.telecomEndCall()
                except Exception as e:
                    log.error(str(e))


def phone_number_formatter(input_string, format=None):
    """Get expected format of input phone number string.

    Args:
        input_string: (string) input phone number.
            The input could be 10/11/12 digital, with or without " "/"-"/"."
        format: (int) expected format, this could be 7/10/11/12
            if format is 7: output string would be 7 digital number.
            if format is 10: output string would be 10 digital (standard) number.
            if format is 11: output string would be "1" + 10 digital number.
            if format is 12: output string would be "+1" + 10 digital number.

    Returns:
        If no error happen, return phone number in expected format.
        Else, return None.
    """
    # make sure input_string is 10 digital
    # Remove white spaces, dashes, dots
    input_string = input_string.replace(" ", "").replace("-", "").replace(".",
                                                                          "")
    if not format:
        return input_string
    # Remove "1"  or "+1"from front
    if (len(input_string) == PHONE_NUMBER_STRING_FORMAT_11_DIGIT and
            input_string[0] == "1"):
        input_string = input_string[1:]
    elif (len(input_string) == PHONE_NUMBER_STRING_FORMAT_12_DIGIT and
          input_string[0:2] == "+1"):
        input_string = input_string[2:]
    elif (len(input_string) == PHONE_NUMBER_STRING_FORMAT_7_DIGIT and
          format == PHONE_NUMBER_STRING_FORMAT_7_DIGIT):
        return input_string
    elif len(input_string) != PHONE_NUMBER_STRING_FORMAT_10_DIGIT:
        return None
    # change input_string according to format
    if format == PHONE_NUMBER_STRING_FORMAT_12_DIGIT:
        input_string = "+1" + input_string
    elif format == PHONE_NUMBER_STRING_FORMAT_11_DIGIT:
        input_string = "1" + input_string
    elif format == PHONE_NUMBER_STRING_FORMAT_10_DIGIT:
        input_string = input_string
    elif format == PHONE_NUMBER_STRING_FORMAT_7_DIGIT:
        input_string = input_string[3:]
    else:
        return None
    return input_string


def get_internet_connection_type(log, ad):
    """Get current active connection type name.

    Args:
        log: Log object.
        ad: Android Device Object.
    Returns:
        current active connection type name.
    """
    if not ad.droid.connectivityNetworkIsConnected():
        return 'none'
    return connection_type_from_type_string(
        ad.droid.connectivityNetworkGetActiveConnectionTypeName())


def verify_http_connection(log,
                           ad,
                           url="http://www.google.com/",
                           retry=3,
                           retry_interval=5):
    """Make ping request and return status.

    Args:
        ad: Android Device Object.
        url: Optional. The ping request will be made to this URL.
            Default Value is "http://www.google.com/".

    """
    for i in range(0, retry + 1):

        try:
            http_response = ad.droid.httpPing(url)
        except:
            http_response = None

        # If httpPing failed, it may return {} (if phone just turn off APM) or
        # None (regular fail)
        # So here use "if http_response" to see if it pass or fail
        if http_response:
            log.info("Verify Internet succeeded after {}s.".format(
                i * retry_interval) if i > 0 else "Verify Internet succeeded.")
            return True
        else:
            if i < retry:
                time.sleep(retry_interval)
    log.info("Verify Internet retry failed after {}s"
             .format(i * retry_interval))
    return False


def _connection_state_change(_event, target_state, connection_type):
    if connection_type:
        if 'TypeName' not in _event['data']:
            return False
        connection_type_string_in_event = _event['data']['TypeName']
        cur_type = connection_type_from_type_string(
            connection_type_string_in_event)
        if cur_type != connection_type:
            log.info(
                "_connection_state_change expect: {}, received: {} <type {}>".format(
                    connection_type, connection_type_string_in_event,
                    cur_type))
            return False

    if 'isConnected' in _event['data'] and _event['data'][
            'isConnected'] == target_state:
        return True
    return False


def wait_for_cell_data_connection(
        log,
        ad,
        state,
        timeout_value=EventDispatcher.DEFAULT_TIMEOUT):
    """Wait for data connection status to be expected value for default
       data subscription.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for cell data timeout value.
            This is optional, default value is EventDispatcher.DEFAULT_TIMEOUT

    Returns:
        True if success.
        False if failed.
    """
    sub_id = get_default_data_sub_id(ad)
    return wait_for_cell_data_connection_for_subscription(log, ad, sub_id,
                                                          state, timeout_value)


def _is_data_connection_state_match(log, ad, expected_data_connection_state):
    return (expected_data_connection_state ==
            ad.droid.telephonyGetDataConnectionState())


def _is_network_connected_state_match(log, ad,
                                      expected_network_connected_state):
    return (expected_network_connected_state ==
            ad.droid.connectivityNetworkIsConnected())


def wait_for_cell_data_connection_for_subscription(
        log,
        ad,
        sub_id,
        state,
        timeout_value=EventDispatcher.DEFAULT_TIMEOUT):
    """Wait for data connection status to be expected value for specified
       subscrption id.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        sub_id: subscription Id
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for cell data timeout value.
            This is optional, default value is EventDispatcher.DEFAULT_TIMEOUT

    Returns:
        True if success.
        False if failed.
    """
    state_str = {
        True: DATA_STATE_CONNECTED,
        False: DATA_STATE_DISCONNECTED
    }[state]

    ad.ed.clear_all_events()
    ad.droid.telephonyStartTrackingDataConnectionStateChangeForSubscription(
        sub_id)
    ad.droid.connectivityStartTrackingConnectivityStateChange()
    try:
        # TODO: b/26293147 There is no framework API to get data connection
        # state by sub id
        data_state = ad.droid.telephonyGetDataConnectionState()
        if data_state == state_str:
            return _wait_for_nw_data_connection(
                log, ad, state, NETWORK_CONNECTION_TYPE_CELL, timeout_value)

        try:
            event = ad.ed.wait_for_event(
                EventDataConnectionStateChanged,
                is_event_match,
                timeout=timeout_value,
                field=DataConnectionStateContainer.DATA_CONNECTION_STATE,
                value=state_str)
        except Empty:
            log.debug(
                "No expected event EventDataConnectionStateChanged {}.".format(
                    state_str))

        # TODO: Wait for <MAX_WAIT_TIME_CONNECTION_STATE_UPDATE> seconds for
        # data connection state.
        # Otherwise, the network state will not be correct.
        # The bug is tracked here: b/20921915

        # Previously we use _is_data_connection_state_match,
        # but telephonyGetDataConnectionState sometimes return wrong value.
        # The bug is tracked here: b/22612607
        # So we use _is_network_connected_state_match.

        if _wait_for_droid_in_state(log, ad,
                                    MAX_WAIT_TIME_CONNECTION_STATE_UPDATE,
                                    _is_network_connected_state_match, state):
            return _wait_for_nw_data_connection(
                log, ad, state, NETWORK_CONNECTION_TYPE_CELL, timeout_value)
        else:
            return False

    finally:
        ad.droid.telephonyStopTrackingDataConnectionStateChangeForSubscription(
            sub_id)


def wait_for_wifi_data_connection(
        log,
        ad,
        state,
        timeout_value=EventDispatcher.DEFAULT_TIMEOUT):
    """Wait for data connection status to be expected value and connection is by WiFi.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for network data timeout value.
            This is optional, default value is EventDispatcher.DEFAULT_TIMEOUT

    Returns:
        True if success.
        False if failed.
    """
    log.info("{} wait_for_wifi_data_connection".format(ad.serial))
    return _wait_for_nw_data_connection(
        log, ad, state, NETWORK_CONNECTION_TYPE_WIFI, timeout_value)


def wait_for_data_connection(log,
                             ad,
                             state,
                             timeout_value=EventDispatcher.DEFAULT_TIMEOUT):
    """Wait for data connection status to be expected value.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        state: Expected status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        timeout_value: wait for network data timeout value.
            This is optional, default value is EventDispatcher.DEFAULT_TIMEOUT

    Returns:
        True if success.
        False if failed.
    """
    return _wait_for_nw_data_connection(log, ad, state, None, timeout_value)


def _wait_for_nw_data_connection(
        log,
        ad,
        is_connected,
        connection_type=None,
        timeout_value=EventDispatcher.DEFAULT_TIMEOUT):
    """Wait for data connection status to be expected value.

    Wait for the data connection status to be DATA_STATE_CONNECTED
        or DATA_STATE_DISCONNECTED.

    Args:
        log: Log object.
        ad: Android Device Object.
        is_connected: Expected connection status: True or False.
            If True, it will wait for status to be DATA_STATE_CONNECTED.
            If False, it will wait for status ti be DATA_STATE_DISCONNECTED.
        connection_type: expected connection type.
            This is optional, if it is None, then any connection type will return True.
        timeout_value: wait for network data timeout value.
            This is optional, default value is EventDispatcher.DEFAULT_TIMEOUT

    Returns:
        True if success.
        False if failed.
    """
    ad.ed.clear_all_events()
    ad.droid.connectivityStartTrackingConnectivityStateChange()
    try:
        cur_data_connection_state = ad.droid.connectivityNetworkIsConnected()
        if is_connected == cur_data_connection_state:
            current_type = get_internet_connection_type(log, ad)
            log.info(
                "_wait_for_nw_data_connection: current connection type: {}".format(
                    current_type))
            if not connection_type:
                return True
            else:
                if not is_connected and current_type != connection_type:
                    log.info(
                        "wait_for_nw_data_connection success: {} data not on {}!".format(
                            ad.serial, connection_type))
                    return True
                elif is_connected and current_type == connection_type:
                    log.info(
                        "wait_for_nw_data_connection success: {} data on {}!".format(
                            ad.serial, connection_type))
                    return True
        else:
            log.info("{} current state: {} target: {}".format(
                ad.serial, cur_data_connection_state, is_connected))

        try:
            event = ad.ed.wait_for_event(
                EventConnectivityChanged, _connection_state_change,
                timeout_value, is_connected, connection_type)
            log.info("_wait_for_nw_data_connection received event:{}".format(
                event))
        except Empty:
            pass

        log.info(
            "_wait_for_nw_data_connection: check connection after wait event.")
        # TODO: Wait for <MAX_WAIT_TIME_CONNECTION_STATE_UPDATE> seconds for
        # data connection state.
        # Otherwise, the network state will not be correct.
        # The bug is tracked here: b/20921915
        if _wait_for_droid_in_state(
                log, ad, MAX_WAIT_TIME_CONNECTION_STATE_UPDATE,
                _is_network_connected_state_match, is_connected):
            current_type = get_internet_connection_type(log, ad)
            log.info(
                "_wait_for_nw_data_connection: current connection type: {}".format(
                    current_type))
            if not connection_type:
                return True
            else:
                if not is_connected and current_type != connection_type:
                    log.info(
                        "wait_for_nw_data_connection after event wait, success: {} data not on {}!".format(
                            ad.serial, connection_type))
                    return True
                elif is_connected and current_type == connection_type:
                    log.info(
                        "wait_for_nw_data_connection after event wait, success: {} data on {}!".format(
                            ad.serial, connection_type))
                    return True
                else:
                    return False
        else:
            return False
    except Exception as e:
        log.error(
            "tel_test_utils._wait_for_nw_data_connection threw Random exception {}".format(
                str(e)))
        return False
    finally:
        ad.droid.connectivityStopTrackingConnectivityStateChange()


def verify_incall_state(log, ads, expected_status):
    """Verify phones in incall state or not.

    Verify if all phones in the array <ads> are in <expected_status>.

    Args:
        log: Log object.
        ads: Array of Android Device Object. All droid in this array will be tested.
        expected_status: If True, verify all Phones in incall state.
            If False, verify all Phones not in incall state.

    """
    result = True
    for ad in ads:
        if ad.droid.telecomIsInCall() is not expected_status:
            log.error("Verify_incall_state: {} status:{}, expected:{}".format(
                ad.serial, ad.droid.telecomIsInCall(), expected_status))
            result = False
    return result


def verify_active_call_number(log, ad, expected_number):
    """Verify the number of current active call.

    Verify if the number of current active call in <ad> is
        equal to <expected_number>.

    Args:
        ad: Android Device Object.
        expected_number: Expected active call number.
    """
    calls = ad.droid.telecomCallGetCallIds()
    if calls is None:
        actual_number = 0
    else:
        actual_number = len(calls)
    if actual_number != expected_number:
        log.error("Active Call number in {}".format(ad.serial))
        log.error("Expected:{}, Actual:{}".format(expected_number,
                                                  actual_number))
        return False
    return True


def num_active_calls(log, ad):
    """Get the count of current active calls.

    Args:
        log: Log object.
        ad: Android Device Object.

    Returns:
        Count of current active calls.
    """
    calls = ad.droid.telecomCallGetCallIds()
    return len(calls) if calls else 0


def toggle_volte(log, ad, new_state=None):
    """Toggle enable/disable VoLTE for default voice subscription.

    Args:
        ad: Android device object.
        new_state: VoLTE mode state to set to.
            True for enable, False for disable.
            If None, opposite of the current state.

    Raises:
        TelTestUtilsError if platform does not support VoLTE.
    """
    return toggle_volte_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad), new_state)


def toggle_volte_for_subscription(log, ad, sub_id, new_state=None):
    """Toggle enable/disable VoLTE for specified voice subscription.

    Args:
        ad: Android device object.
        sub_id: subscription ID
        new_state: VoLTE mode state to set to.
            True for enable, False for disable.
            If None, opposite of the current state.

    Raises:
        TelTestUtilsError if platform does not support VoLTE.
    """
    # TODO: b/26293960 No framework API available to set IMS by SubId.
    if not ad.droid.imsIsEnhanced4gLteModeSettingEnabledByPlatform():
        raise TelTestUtilsError("VoLTE not supported by platform.")
    current_state = ad.droid.imsIsEnhanced4gLteModeSettingEnabledByUser()
    if new_state is None:
        new_state = not current_state
    if new_state != current_state:
        ad.droid.imsSetEnhanced4gMode(new_state)
    return True


def set_wfc_mode(log, ad, wfc_mode):
    """Set WFC enable/disable and mode.

    Args:
        log: Log object
        ad: Android device object.
        wfc_mode: WFC mode to set to.
            Valid mode includes: WFC_MODE_WIFI_ONLY, WFC_MODE_CELLULAR_PREFERRED,
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_DISABLED.

    Returns:
        True if success. False if ad does not support WFC or error happened.
    """
    try:
        log.info("{} set wfc mode to {}".format(ad.serial, wfc_mode))
        if not ad.droid.imsIsWfcEnabledByPlatform():
            if wfc_mode == WFC_MODE_DISABLED:
                return True
            else:
                log.error("WFC not supported by platform.")
                return False

        ad.droid.imsSetWfcMode(wfc_mode)

    except Exception as e:
        log.error(e)
        return False

    return True


def _wait_for_droid_in_state(log, ad, max_time, state_check_func, *args,
                             **kwargs):
    while max_time > 0:
        if state_check_func(log, ad, *args, **kwargs):
            return True

        time.sleep(1)
        max_time -= 1

    return False


def _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_time, state_check_func, *args, **kwargs):
    while max_time > 0:
        if state_check_func(log, ad, sub_id, *args, **kwargs):
            return True

        time.sleep(1)
        max_time -= 1

    return False


def _wait_for_droids_in_state(log, ads, max_time, state_check_func, *args,
                              **kwargs):
    while max_time > 0:
        success = True
        for ad in ads:
            if not state_check_func(log, ad, *args, **kwargs):
                success = False
                break
        if success:
            return True

        time.sleep(1)
        max_time -= 1

    return False


def is_phone_in_call(log, ad):
    """Return True if phone in call.

    Args:
        log: log object.
        ad:  android device.
    """
    return ad.droid.telecomIsInCall()


def is_phone_not_in_call(log, ad):
    """Return True if phone not in call.

    Args:
        log: log object.
        ad:  android device.
    """
    return not ad.droid.telecomIsInCall()


def wait_for_droid_in_call(log, ad, max_time):
    """Wait for android to be in call state.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        If phone become in call state within max_time, return True.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_phone_in_call)


def wait_for_telecom_ringing(log, ad, max_time=MAX_WAIT_TIME_TELECOM_RINGING):
    """Wait for android to be in telecom ringing state.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time. This is optional.
            Default Value is MAX_WAIT_TIME_TELECOM_RINGING.

    Returns:
        If phone become in telecom ringing state within max_time, return True.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(
        log, ad, max_time, lambda log, ad: ad.droid.telecomIsRinging())


def wait_for_droid_not_in_call(log, ad, max_time):
    """Wait for android to be not in call state.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        If phone become not in call state within max_time, return True.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_phone_not_in_call)


def _is_attached(log, ad, voice_or_data):
    return _is_attached_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), voice_or_data)


def _is_attached_for_subscription(log, ad, sub_id, voice_or_data):
    if get_network_rat_for_subscription(log, ad, sub_id,
                                        voice_or_data) != RAT_UNKNOWN:
        return True
    else:
        return False


def wait_for_voice_attach(log, ad, max_time):
    """Wait for android device to attach on voice.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device attach voice within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, _is_attached,
                                    NETWORK_SERVICE_VOICE)


def wait_for_voice_attach_for_subscription(log, ad, sub_id, max_time):
    """Wait for android device to attach on voice in subscription id.

    Args:
        log: log object.
        ad:  android device.
        sub_id: subscription id.
        max_time: maximal wait time.

    Returns:
        Return True if device attach voice within max_time.
        Return False if timeout.
    """
    if not _wait_for_droid_in_state_for_subscription(
            log, ad, sub_id, max_time, _is_attached_for_subscription,
            NETWORK_SERVICE_VOICE):
        return False

    # TODO: b/26295983 if pone attach to 1xrtt from unknown, phone may not
    # receive incoming call immediately.
    if ad.droid.telephonyGetCurrentVoiceNetworkType() == RAT_1XRTT:
        time.sleep(WAIT_TIME_1XRTT_VOICE_ATTACH)
    return True


def wait_for_data_attach(log, ad, max_time):
    """Wait for android device to attach on data.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device attach data within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, _is_attached,
                                    NETWORK_SERVICE_DATA)


def wait_for_data_attach_for_subscription(log, ad, sub_id, max_time):
    """Wait for android device to attach on data in subscription id.

    Args:
        log: log object.
        ad:  android device.
        sub_id: subscription id.
        max_time: maximal wait time.

    Returns:
        Return True if device attach data within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_time, _is_attached_for_subscription,
        NETWORK_SERVICE_DATA)


def is_ims_registered(log, ad):
    """Return True if IMS registered.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if IMS registered.
        Return False if IMS not registered.
    """
    return ad.droid.telephonyIsImsRegistered()


def wait_for_ims_registered(log, ad, max_time):
    """Wait for android device to register on ims.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device register ims successfully within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_ims_registered)


def is_volte_enabled(log, ad):
    """Return True if VoLTE feature bit is True.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if VoLTE feature bit is True and IMS registered.
        Return False if VoLTE feature bit is False or IMS not registered.
    """
    volte_status = ad.droid.telephonyIsVolteAvailable()
    ims_status = is_ims_registered(log, ad)
    if volte_status is True and ims_status is False:
        log.error("Error! VoLTE is Available, but IMS is not registered.")
        return False
    return volte_status


def is_video_enabled(log, ad):
    """Return True if Video Calling feature bit is True.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if Video Calling feature bit is True and IMS registered.
        Return False if Video Calling feature bit is False or IMS not registered.
    """
    video_status = ad.droid.telephonyIsVideoCallingAvailable()
    ims_status = is_ims_registered(log, ad)
    if video_status is True and ims_status is False:
        log.error("Error! Video Call is Available, but IMS is not registered.")
        return False
    return video_status


def wait_for_volte_enabled(log, ad, max_time):
    """Wait for android device to report VoLTE enabled bit true.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device report VoLTE enabled bit true within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_volte_enabled)


def wait_for_video_enabled(log, ad, max_time):
    """Wait for android device to report Video Telephony enabled bit true.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.

    Returns:
        Return True if device report Video Telephony enabled bit true within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_video_enabled)


def is_wfc_enabled(log, ad):
    """Return True if WiFi Calling feature bit is True.

    Args:
        log: log object.
        ad: android device.

    Returns:
        Return True if WiFi Calling feature bit is True and IMS registered.
        Return False if WiFi Calling feature bit is False or IMS not registered.
    """
    wfc_status = ad.droid.telephonyIsWifiCallingAvailable()
    ims_status = is_ims_registered(log, ad)
    if wfc_status is True and ims_status is False:
        log.error(
            "Error! WiFi Calling is Available, but IMS is not registered.")
        return False
    return wfc_status


def wait_for_wfc_enabled(log, ad, max_time=MAX_WAIT_TIME_WFC_ENABLED):
    """Wait for android device to report WiFi Calling enabled bit true.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.
            Default value is MAX_WAIT_TIME_WFC_ENABLED.

    Returns:
        Return True if device report WiFi Calling enabled bit true within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(log, ad, max_time, is_wfc_enabled)


def wait_for_wfc_disabled(log, ad, max_time=MAX_WAIT_TIME_WFC_DISABLED):
    """Wait for android device to report WiFi Calling enabled bit false.

    Args:
        log: log object.
        ad:  android device.
        max_time: maximal wait time.
            Default value is MAX_WAIT_TIME_WFC_DISABLED.

    Returns:
        Return True if device report WiFi Calling enabled bit false within max_time.
        Return False if timeout.
    """
    return _wait_for_droid_in_state(
        log, ad, max_time, lambda log, ad: not is_wfc_enabled(log, ad))


def get_phone_number(log, ad):
    """Get phone number for default subscription

    Args:
        log: log object.
        ad: Android device object.

    Returns:
        Phone number.
    """
    return get_phone_number_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad))


def get_phone_number_for_subscription(log, ad, subid):
    """Get phone number for subscription

    Args:
        log: log object.
        ad: Android device object.
        subid: subscription id.

    Returns:
        Phone number.
    """
    number = None
    try:
        number = ad.cfg['subscription'][subid]['phone_num']
    except KeyError:
        number = ad.droid.telephonyGetLine1NumberForSubscription(subid)
    return number


def set_phone_number(log, ad, phone_num):
    """Set phone number for default subscription

    Args:
        log: log object.
        ad: Android device object.
        phone_num: phone number string.

    Returns:
        True if success.
    """
    return set_phone_number_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad), phone_num)


def set_phone_number_for_subscription(log, ad, subid, phone_num):
    """Set phone number for subscription

    Args:
        log: log object.
        ad: Android device object.
        subid: subscription id.
        phone_num: phone number string.

    Returns:
        True if success.
    """
    try:
        ad.cfg['subscription'][subid]['phone_num'] = phone_num
    except Exception:
        return False
    return True


def get_operator_name(log, ad, subId=None):
    """Get operator name (e.g. vzw, tmo) of droid.

    Args:
        ad: Android device object.
        sub_id: subscription ID
            Optional, default is None

    Returns:
        Operator name.
    """
    try:
        if subId is not None:
            result = operator_name_from_plmn_id(
                ad.droid.telephonyGetSimOperatorForSubscription(subId))
        else:
            result = operator_name_from_plmn_id(
                ad.droid.telephonyGetSimOperator())
    except KeyError:
        result = CARRIER_UNKNOWN
    return result


def get_model_name(ad):
    """Get android device model name

    Args:
        ad: Android device object

    Returns:
        model name string
    """
    # TODO: Create translate table.
    model = ad.model
    if (model.startswith(AOSP_PREFIX)):
        model = model[len(AOSP_PREFIX):]
    return model


def is_sms_match(event, phonenumber_tx, text):
    """Return True if 'text' equals to event['data']['Text']
        and phone number match.

    Args:
        event: Event object to verify.
        phonenumber_tx: phone number for sender.
        text: text string to verify.

    Returns:
        Return True if 'text' equals to event['data']['Text']
            and phone number match.
    """
    return (
        check_phone_number_match(event['data']['Sender'], phonenumber_tx) and
        event['data']['Text'] == text)


def is_sms_partial_match(event, phonenumber_tx, text):
    """Return True if 'text' starts with event['data']['Text']
        and phone number match.

    Args:
        event: Event object to verify.
        phonenumber_tx: phone number for sender.
        text: text string to verify.

    Returns:
        Return True if 'text' starts with event['data']['Text']
            and phone number match.
    """
    return (
        check_phone_number_match(event['data']['Sender'], phonenumber_tx) and
        text.startswith(event['data']['Text']))


def sms_send_receive_verify(log, ad_tx, ad_rx, array_message):
    """Send SMS, receive SMS, and verify content and sender's number.

        Send (several) SMS from droid_tx to droid_rx.
        Verify SMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object
        ad_rx: Receiver's Android Device Object
        array_message: the array of message to send/receive
    """
    subid_tx = get_outgoing_message_sub_id(ad_tx)
    subid_rx = get_incoming_message_sub_id(ad_rx)
    return sms_send_receive_verify_for_subscription(
        log, ad_tx, ad_rx, subid_tx, subid_rx, array_message)


def wait_for_matching_sms(log,
                          ad_rx,
                          phonenumber_tx,
                          text,
                          allow_multi_part_long_sms=True):
    """Wait for matching incoming SMS.

    Args:
        log: Log object.
        ad_rx: Receiver's Android Device Object
        phonenumber_tx: Sender's phone number.
        text: SMS content string.
        allow_multi_part_long_sms: is long SMS allowed to be received as
            multiple short SMS. This is optional, default value is True.

    Returns:
        True if matching incoming SMS is received.
    """
    if not allow_multi_part_long_sms:
        try:
            ad_rx.ed.wait_for_event(EventSmsReceived, is_sms_match,
                                    MAX_WAIT_TIME_SMS_RECEIVE, phonenumber_tx,
                                    text)
            return True
        except Empty:
            log.error("No matched SMS received event.")
            return False
    else:
        try:
            received_sms = ''
            while (text != ''):
                event = ad_rx.ed.wait_for_event(
                    EventSmsReceived, is_sms_partial_match,
                    MAX_WAIT_TIME_SMS_RECEIVE, phonenumber_tx, text)
                text = text[len(event['data']['Text']):]
                received_sms += event['data']['Text']
            return True
        except Empty:
            log.error("No matched SMS received event.")
            if received_sms != '':
                log.error("Only received partial matched SMS: {}".format(
                    received_sms))
            return False


def sms_send_receive_verify_for_subscription(log, ad_tx, ad_rx, subid_tx,
                                             subid_rx, array_message):
    """Send SMS, receive SMS, and verify content and sender's number.

        Send (several) SMS from droid_tx to droid_rx.
        Verify SMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object..
        ad_rx: Receiver's Android Device Object.
        subid_tx: Sender's subsciption ID to be used for SMS
        subid_rx: Receiver's subsciption ID to be used for SMS
        array_message: the array of message to send/receive
    """

    phonenumber_tx = ad_tx.cfg['subscription'][subid_tx]['phone_num']
    phonenumber_rx = ad_rx.cfg['subscription'][subid_rx]['phone_num']
    for text in array_message:
        log.info("Sending SMS {} to {}, len: {}, content: {}.".format(
            phonenumber_tx, phonenumber_rx, len(text), text))
        result = False
        ad_rx.ed.clear_all_events()
        ad_rx.droid.smsStartTrackingIncomingSmsMessage()
        try:
            ad_tx.droid.smsSendTextMessage(phonenumber_rx, text, True)

            try:
                ad_tx.ed.pop_event(EventSmsSentSuccess,
                                   MAX_WAIT_TIME_SMS_SENT_SUCCESS)
            except Empty:
                log.error("No sent_success event.")
                return False

            if not wait_for_matching_sms(log,
                                         ad_rx,
                                         phonenumber_tx,
                                         text,
                                         allow_multi_part_long_sms=True):
                return False
        finally:
            ad_rx.droid.smsStopTrackingIncomingSmsMessage()
    return True


def mms_send_receive_verify(log, ad_tx, ad_rx, array_message):
    """Send SMS, receive SMS, and verify content and sender's number.

        Send (several) SMS from droid_tx to droid_rx.
        Verify SMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object
        ad_rx: Receiver's Android Device Object
        array_message: the array of message to send/receive
    """
    return mms_send_receive_verify_for_subscription(
        log, ad_tx, ad_rx, get_outgoing_message_sub_id(ad_tx),
        get_incoming_message_sub_id(ad_rx), array_message)


#TODO: b/21569494 This function is still a WIP and is disabled
def mms_send_receive_verify_for_subscription(log, ad_tx, ad_rx, subid_tx,
                                             subid_rx, array_payload):
    """Send SMS, receive SMS, and verify content and sender's number.

        Send (several) SMS from droid_tx to droid_rx.
        Verify SMS is sent, delivered and received.
        Verify received content and sender's number are correct.

    Args:
        log: Log object.
        ad_tx: Sender's Android Device Object..
        ad_rx: Receiver's Android Device Object.
        subid_tx: Sender's subsciption ID to be used for SMS
        subid_rx: Receiver's subsciption ID to be used for SMS
        array_message: the array of message to send/receive
    """

    log.error("Function is non-working: b/21569494")
    return False

    phonenumber_tx = ad_tx.cfg['subscription'][subid_tx]['phone_num']
    phonenumber_rx = ad_rx.cfg['subscription'][subid_rx]['phone_num']
    for subject, message, filename in array_payload:
        log.info("Sending MMS {} to {}, subject: {}, message: {}.".format(
            phonenumber_tx, phonenumber_rx, subject, message))
        result = False
        ad_rx.ed.clear_all_events()
        ad_rx.droid.smsStartTrackingIncomingMmsMessage()
        ad_rx.droid.smsStartTrackingIncomingSmsMessage()
        try:
            ad_tx.droid.smsSendMultimediaMessage(
                phonenumber_rx, subject, message, phonenumber_tx, filename)

            ad_tx.ed.pop_event(EventMmsSentSuccess,
                               MAX_WAIT_TIME_SMS_SENT_SUCCESS)

            start_time = time.time()
            remaining_time = MAX_WAIT_TIME_SMS_RECEIVE
            while remaining_time > 0:
                event = ad_rx.ed.pop_event(EventSmsReceived, remaining_time)
                if check_phone_number_match(event['data']['Sender'],
                                            phonenumber_tx):
                    log.debug("Received SMS Indication")
                    while remaining_time > 0:
                        event = ad_rx.ed.pop_event(EventDataSmsReceived,
                                                   remaining_time)
                        if check_phone_number_match(event['data']['Sender'],
                                                    phonenumber_tx):
                            result = True
                            break
                        remaining_time = time.time() - start_time
                remaining_time = time.time() - start_time

            if not result:
                log.info("Expected sender:" + phonenumber_tx)
                log.error("Received sender:" + event['data']['Sender'])
                log.error("Failed in verify receiving MMS.")
                return False
        finally:
            ad_rx.droid.smsStopTrackingIncomingSmsMessage()
            ad_rx.droid.smsStopTrackingIncomingMmsMessage()
    return True


def ensure_network_rat(log,
                       ad,
                       network_preference,
                       rat_family,
                       voice_or_data=None,
                       max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                       toggle_apm_after_setting=False):
    """Ensure ad's current network is in expected rat_family.
    """
    return ensure_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), network_preference,
        rat_family, voice_or_data, max_wait_time, toggle_apm_after_setting)


def ensure_network_rat_for_subscription(
        log,
        ad,
        sub_id,
        network_preference,
        rat_family,
        voice_or_data=None,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        toggle_apm_after_setting=False):
    """Ensure ad's current network is in expected rat_family.
    """
    if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
            network_preference, sub_id):
        log.error("Set Preferred Networks failed.")
        return False
    if is_droid_in_rat_family_for_subscription(log, ad, sub_id, rat_family,
                                               voice_or_data):
        return True

    if toggle_apm_after_setting:
        toggle_airplane_mode(log, ad, True)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        toggle_airplane_mode(log, ad, False)

    result = wait_for_network_rat_for_subscription(
        log, ad, sub_id, rat_family, max_wait_time, voice_or_data)

    log.info(
        "End of ensure_network_rat_for_subscription for {}. "
        "Setting to {}, Expecting {} {}. Current: voice: {}(family: {}), "
        "data: {}(family: {})".format(
            ad.serial, network_preference, rat_family, voice_or_data,
            ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                sub_id),
            rat_family_from_rat(
                ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                    sub_id)),
            ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                sub_id), rat_family_from_rat(
                    ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                        sub_id))))
    return result


def ensure_network_preference(log,
                              ad,
                              network_preference,
                              voice_or_data=None,
                              max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                              toggle_apm_after_setting=False):
    """Ensure that current rat is within the device's preferred network rats.
    """
    return ensure_network_preference_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), network_preference,
        voice_or_data, max_wait_time, toggle_apm_after_setting)


def ensure_network_preference_for_subscription(
        log,
        ad,
        sub_id,
        network_preference,
        voice_or_data=None,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        toggle_apm_after_setting=False):
    """Ensure ad's network preference is <network_preference> for sub_id.
    """
    rat_family_list = rat_families_for_network_preference(network_preference)
    if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
            network_preference, sub_id):
        log.error("Set Preferred Networks failed.")
        return False
    if is_droid_in_rat_family_list_for_subscription(
            log, ad, sub_id, rat_family_list, voice_or_data):
        return True

    if toggle_apm_after_setting:
        toggle_airplane_mode(log, ad, True)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        toggle_airplane_mode(log, ad, False)

    result = wait_for_preferred_network_for_subscription(
        log, ad, sub_id, network_preference, max_wait_time, voice_or_data)

    log.info(
        "End of ensure_network_preference_for_subscription for {}. "
        "Setting to {}, Expecting {} {}. Current: voice: {}(family: {}), "
        "data: {}(family: {})".format(
            ad.serial, network_preference, rat_family_list, voice_or_data,
            ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                sub_id),
            rat_family_from_rat(
                ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                    sub_id)),
            ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                sub_id), rat_family_from_rat(
                    ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                        sub_id))))
    return result


def ensure_network_generation(log,
                              ad,
                              generation,
                              max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                              voice_or_data=None,
                              toggle_apm_after_setting=False):
    """Ensure ad's network is <network generation> for default subscription ID.

    Set preferred network generation to <generation>.
    Toggle ON/OFF airplane mode if necessary.
    Wait for ad in expected network type.
    """
    return ensure_network_generation_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), generation,
        max_wait_time, voice_or_data, toggle_apm_after_setting)


def ensure_network_generation_for_subscription(
        log,
        ad,
        sub_id,
        generation,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None,
        toggle_apm_after_setting=False):
    """Ensure ad's network is <network generation> for specified subscription ID.

    Set preferred network generation to <generation>.
    Toggle ON/OFF airplane mode if necessary.
    Wait for ad in expected network type.
    """
    if is_droid_in_network_generation_for_subscription(
            log, ad, sub_id, generation, voice_or_data):
        return True

    operator = get_operator_name(log, ad, sub_id)
    try:
        network_preference = network_preference_for_generaton(generation,
                                                              operator)
        rat_family = rat_family_for_generation(generation, operator)
    except KeyError:
        log.error("Failed to find a rat_family entry for "
                  "PLMN: {}, operator:{}, generation: {}".format(
                      ad.droid.telephonyGetSimOperatorForSubscription(
                          sub_id), operator, generation))
        return False

    if not ad.droid.telephonySetPreferredNetworkTypesForSubscription(
            network_preference, sub_id):
        log.error("Set Preferred Networks failed.")
        return False

    if toggle_apm_after_setting:
        toggle_airplane_mode(log, ad, True)
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        toggle_airplane_mode(log, ad, False)

    result = wait_for_network_generation_for_subscription(
        log, ad, sub_id, generation, max_wait_time, voice_or_data)

    log.info(
        "End of ensure_network_generation_for_subscription for {}. "
        "Setting to {}, Expecting {} {}. Current: voice: {}(family: {}), "
        "data: {}(family: {})".format(
            ad.serial, network_preference, generation, voice_or_data,
            ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                sub_id),
            rat_generation_from_rat(
                ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
                    sub_id)),
            ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                sub_id), rat_generation_from_rat(
                    ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
                        sub_id))))

    return result


def wait_for_network_rat(log,
                         ad,
                         rat_family,
                         max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                         voice_or_data=None):
    return wait_for_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family,
        max_wait_time, voice_or_data)


def wait_for_network_rat_for_subscription(
        log,
        ad,
        sub_id,
        rat_family,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        is_droid_in_rat_family_for_subscription, rat_family, voice_or_data)


def wait_for_not_network_rat(log,
                             ad,
                             rat_family,
                             max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                             voice_or_data=None):
    return wait_for_not_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family,
        max_wait_time, voice_or_data)


def wait_for_not_network_rat_for_subscription(
        log,
        ad,
        sub_id,
        rat_family,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        lambda log, ad, sub_id, *args, **kwargs: not is_droid_in_rat_family_for_subscription(log, ad, sub_id, rat_family, voice_or_data))


def wait_for_preferred_network(log,
                               ad,
                               network_preference,
                               max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                               voice_or_data=None):
    return wait_for_preferred_network_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), network_preference,
        max_wait_time, voice_or_data)


def wait_for_preferred_network_for_subscription(
        log,
        ad,
        sub_id,
        network_preference,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    rat_family_list = rat_families_for_network_preference(network_preference)
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        is_droid_in_rat_family_list_for_subscription, rat_family_list,
        voice_or_data)


def wait_for_network_generation(log,
                                ad,
                                generation,
                                max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
                                voice_or_data=None):
    return wait_for_network_generation_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), generation,
        max_wait_time, voice_or_data)


def wait_for_network_generation_for_subscription(
        log,
        ad,
        sub_id,
        generation,
        max_wait_time=MAX_WAIT_TIME_NW_SELECTION,
        voice_or_data=None):
    return _wait_for_droid_in_state_for_subscription(
        log, ad, sub_id, max_wait_time,
        is_droid_in_network_generation_for_subscription, generation,
        voice_or_data)


def is_droid_in_rat_family(log, ad, rat_family, voice_or_data=None):
    return is_droid_in_rat_family_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family,
        voice_or_data)


def is_droid_in_rat_family_for_subscription(log,
                                            ad,
                                            sub_id,
                                            rat_family,
                                            voice_or_data=None):
    return is_droid_in_rat_family_list_for_subscription(
        log, ad, sub_id, [rat_family], voice_or_data)


def is_droid_in_rat_familiy_list(log, ad, rat_family_list, voice_or_data=None):
    return is_droid_in_rat_family_list_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), rat_family_list,
        voice_or_data)


def is_droid_in_rat_family_list_for_subscription(log,
                                                 ad,
                                                 sub_id,
                                                 rat_family_list,
                                                 voice_or_data=None):
    service_list = [NETWORK_SERVICE_DATA, NETWORK_SERVICE_VOICE]
    if voice_or_data:
        service_list = [voice_or_data]

    for service in service_list:
        nw_rat = get_network_rat_for_subscription(log, ad, sub_id, service)
        if nw_rat == RAT_UNKNOWN or not is_valid_rat(nw_rat):
            continue
        if rat_family_from_rat(nw_rat) in rat_family_list:
            return True
    return False


def is_droid_in_network_generation(log, ad, nw_gen, voice_or_data):
    """Checks if a droid in expected network generation ("2g", "3g" or "4g").

    Args:
        log: log object.
        ad: android device.
        nw_gen: expected generation "4g", "3g", "2g".
        voice_or_data: check voice network generation or data network generation
            This parameter is optional. If voice_or_data is None, then if
            either voice or data in expected generation, function will return True.

    Returns:
        True if droid in expected network generation. Otherwise False.
    """
    return is_droid_in_network_generation_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), nw_gen, voice_or_data)


def is_droid_in_network_generation_for_subscription(log, ad, sub_id, nw_gen,
                                                    voice_or_data):
    """Checks if a droid in expected network generation ("2g", "3g" or "4g").

    Args:
        log: log object.
        ad: android device.
        nw_gen: expected generation "4g", "3g", "2g".
        voice_or_data: check voice network generation or data network generation
            This parameter is optional. If voice_or_data is None, then if
            either voice or data in expected generation, function will return True.

    Returns:
        True if droid in expected network generation. Otherwise False.
    """
    service_list = [NETWORK_SERVICE_DATA, NETWORK_SERVICE_VOICE]

    if voice_or_data:
        service_list = [voice_or_data]

    for service in service_list:
        nw_rat = get_network_rat_for_subscription(log, ad, sub_id, service)

        if nw_rat == RAT_UNKNOWN or not is_valid_rat(nw_rat):
            continue

        if rat_generation_from_rat(nw_rat) == nw_gen:
            return True
        else:
            return False

    return False


def get_network_rat(log, ad, voice_or_data):
    """Get current network type (Voice network type, or data network type)
       for default subscription id

    Args:
        ad: Android Device Object
        voice_or_data: Input parameter indicating to get voice network type or
            data network type.

    Returns:
        Current voice/data network type.
    """
    return get_network_rat_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), voice_or_data)


def get_network_rat_for_subscription(log, ad, sub_id, voice_or_data):
    """Get current network type (Voice network type, or data network type)
       for specified subscription id

    Args:
        ad: Android Device Object
        sub_id: subscription ID
        voice_or_data: Input parameter indicating to get voice network type or
            data network type.

    Returns:
        Current voice/data network type.
    """
    if voice_or_data == NETWORK_SERVICE_VOICE:
        ret_val = ad.droid.telephonyGetCurrentVoiceNetworkTypeForSubscription(
            sub_id)
    elif voice_or_data == NETWORK_SERVICE_DATA:
        ret_val = ad.droid.telephonyGetCurrentDataNetworkTypeForSubscription(
            sub_id)
    else:
        ret_val = ad.droid.telephonyGetNetworkTypeForSubscription(sub_id)

    if ret_val is None:
        log.error("get_network_rat(): Unexpected null return value")
        return RAT_UNKNOWN
    else:
        return ret_val


def get_network_gen(log, ad, voice_or_data):
    """Get current network generation string (Voice network type, or data network type)

    Args:
        ad: Android Device Object
        voice_or_data: Input parameter indicating to get voice network generation
            or data network generation.

    Returns:
        Current voice/data network generation.
    """
    return get_network_gen_for_subscription(
        log, ad, ad.droid.subscriptionGetDefaultSubId(), voice_or_data)


def get_network_gen_for_subscription(log, ad, sub_id, voice_or_data):
    """Get current network generation string (Voice network type, or data network type)

    Args:
        ad: Android Device Object
        voice_or_data: Input parameter indicating to get voice network generation
            or data network generation.

    Returns:
        Current voice/data network generation.
    """
    try:
        return rat_generation_from_rat(get_network_rat_for_subscription(
            log, ad, sub_id, voice_or_data))
    except KeyError:
        log.error(
            "KeyError happened in get_network_gen, ad:{}, d/v: {}, rat: {}".format(
                ad.serial, voice_or_data, get_network_rat_for_subscription(
                    log, ad, sub_id, voice_or_data)))
        return GEN_UNKNOWN


def check_voice_mail_count(log, ad, voice_mail_count_before,
                           voice_mail_count_after):
    """function to check if voice mail count is correct after leaving a new voice message.
    """
    return get_voice_mail_count_check_function(get_operator_name(log, ad))(
        voice_mail_count_before, voice_mail_count_after)


def get_voice_mail_number(log, ad):
    """function to get the voice mail number
    """
    voice_mail_number = get_voice_mail_number_function(get_operator_name(log,
                                                                         ad))()
    if voice_mail_number is None:
        return get_phone_number(log, ad)
    return voice_mail_number


def ensure_phones_idle(log,
                       ads,
                       settling_time=WAIT_TIME_ANDROID_STATE_SETTLING):
    """Ensure ads idle (not in call).
    """
    for ad in ads:
        if ad.droid.telecomIsInCall():
            ad.droid.telecomEndCall()
    # Leave the delay time to make sure droid can recover to idle from ongoing call.
    time.sleep(settling_time)
    return True


def ensure_phone_idle(log, ad, settling_time=WAIT_TIME_ANDROID_STATE_SETTLING):
    """Ensure ad idle (not in call).
    """
    return ensure_phones_idle(log, [ad], settling_time)


def ensure_phone_default_state(log, ad):
    """Ensure ad in default state.
    Phone not in call.
    Phone have no stored WiFi network and WiFi disconnected.
    Phone not in airplane mode.
    """
    result = True
    if ad.droid.telecomIsInCall():
        ad.droid.telecomEndCall()
    set_wfc_mode(log, ad, WFC_MODE_DISABLED)

    if not wait_for_not_network_rat(log,
                                    ad,
                                    RAT_FAMILY_WLAN,
                                    voice_or_data=NETWORK_SERVICE_DATA):
        log.error(
            "ensure_phones_default_state: wait_for_droid_not_in iwlan fail {}.".format(
                ad.serial))
        result = False
    if ((not WifiUtils.wifi_reset(log, ad)) or
        (not WifiUtils.wifi_toggle_state(log, ad, False))):
        log.error("ensure_phones_default_state:reset WiFi fail {}.".format(
            ad.serial))
        result = False
    if not toggle_airplane_mode(log, ad, False):
        log.error(
            "ensure_phones_default_state:turn off airplane mode fail {}.".format(
                ad.serial))
        result = False
    # make sure phone data is on
    ad.droid.telephonyToggleDataConnection(True)

    # Leave the delay time to make sure droid can recover to idle from ongoing call.
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    return result


def ensure_phones_default_state(log, ads):
    """Ensure ads in default state.
    Phone not in call.
    Phone have no stored WiFi network and WiFi disconnected.
    Phone not in airplane mode.
    """
    tasks = []
    for ad in ads:
        tasks.append((ensure_phone_default_state, (log, ad)))
    if not multithread_func(log, tasks):
        log.error("Ensure_phones_default_state Fail.")
        return False
    return True


def ensure_wifi_connected(log, ad, wifi_ssid, wifi_pwd=None, retry=1):
    """Ensure ad connected to wifi.

    Args:
        log: Log object.
        ad: Android device object.
        wifi_ssid: WiFi network SSID.
        wifi_pwd: WiFi network password. This is optional.

    """
    while (retry >= 0):
        WifiUtils.wifi_reset(log, ad)
        WifiUtils.wifi_toggle_state(log, ad, False)
        WifiUtils.wifi_toggle_state(log, ad, True)
        if WifiUtils.wifi_connect(log, ad, wifi_ssid, wifi_pwd):
            return True
        else:
            log.info("ensure_wifi_connected: Connect WiFi failed, retry + 1.")
            retry -= 1
    return False


def reset_preferred_network_type_to_allowable_range(log, ad):
    """If preferred network type is not in allowable range, reset to GEN_4G
    preferred network type.

    Args:
        log: log object
        ad: android device object

    Returns:
        None
    """
    sub_info_list = ad.droid.subscriptionGetAllSubInfoList()
    for sub_info in sub_info_list:
        sub_id = sub_info['subscriptionId']
        operator = get_operator_name(log, ad, sub_id)
        current_preference = \
            ad.droid.telephonyGetPreferredNetworkTypesForSubscription(sub_id)
        try:
            if current_preference not in get_allowable_network_preference(
                    operator):
                network_preference = network_preference_for_generaton(GEN_4G,
                                                                      operator)
                ad.droid.telephonySetPreferredNetworkTypesForSubscription(
                    network_preference, sub_id)
        except KeyError:
            pass


def task_wrapper(task):
    """Task wrapper for multithread_func

    Args:
        task[0]: function to be wrapped.
        task[1]: function args.

    Returns:
        Return value of wrapped function call.
    """
    func = task[0]
    params = task[1]
    return func(*params)


def multithread_func(log, tasks):
    """Multi-thread function wrapper.

    Args:
        log: log object.
        tasks: tasks to be executed in parallel.

    Returns:
        True if all tasks return True.
        False if any task return False.
    """
    MAX_NUMBER_OF_WORKERS = 4
    number_of_workers = min(MAX_NUMBER_OF_WORKERS, len(tasks))
    executor = concurrent.futures.ThreadPoolExecutor(
        max_workers=number_of_workers)
    results = list(executor.map(task_wrapper, tasks))
    executor.shutdown()
    log.info("multithread_func result: {}".format(results))
    for r in results:
        if not r:
            return False
    return True


def set_phone_screen_on(log, ad, screen_on_time=MAX_SCREEN_ON_TIME):
    """Set phone screen on time.

    Args:
        log: Log object.
        ad: Android device object.
        screen_on_time: screen on time.
            This is optional, default value is MAX_SCREEN_ON_TIME.
    Returns:
        True if set successfully.
    """
    ad.droid.setScreenTimeout(screen_on_time)
    return screen_on_time == ad.droid.getScreenTimeout()


def set_phone_silent_mode(log, ad, silent_mode=True):
    """Set phone silent mode.

    Args:
        log: Log object.
        ad: Android device object.
        silent_mode: set phone silent or not.
            This is optional, default value is True (silent mode on).
    Returns:
        True if set successfully.
    """
    ad.droid.toggleRingerSilentMode(silent_mode)
    return silent_mode == ad.droid.checkRingerSilentMode()


def set_preferred_subid_for_sms(log, ad, sub_id):
    """set subscription id for SMS

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.

    """
    log.info("Setting subscription:{} as Message SIM for {}".format(sub_id,
                                                                    ad.serial))
    ad.droid.subscriptionSetDefaultSmsSubId(sub_id)
    # Wait to make sure settings take effect
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    return sub_id == ad.droid.subscriptionGetDefaultSmsSubId()


def set_preferred_subid_for_data(log, ad, sub_id):
    """set subscription id for data

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.

    """
    log.info("Setting subscription:{} as Data SIM for {}".format(sub_id,
                                                                 ad.serial))
    ad.droid.subscriptionSetDefaultDataSubId(sub_id)
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    # Wait to make sure settings take effect
    # Data SIM change takes around 1 min
    # Check whether data has changed to selected sim
    if not wait_for_data_connection(log, ad, True,
                                    MAX_WAIT_TIME_DATA_SUB_CHANGE):
        log.error("Data Connection failed - Not able to switch Data SIM")
        return False
    return True


def set_preferred_subid_for_voice(log, ad, sub_id):
    """set subscription id for voice

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.

    """
    log.info("Setting subscription:{} as Voice SIM for {}".format(sub_id,
                                                                  ad.serial))
    ad.droid.subscriptionSetDefaultVoiceSubId(sub_id)
    ad.droid.telecomSetUserSelectedOutgoingPhoneAccountBySubId(sub_id)
    # Wait to make sure settings take effect
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    return True


def set_call_state_listen_level(log, ad, value, sub_id):
    """Set call state listen level for subscription id.

    Args:
        log: Log object.
        ad: Android device object.
        value: True or False
        sub_id :Subscription ID.

    Returns:
        True or False
    """
    if sub_id == INVALID_SUB_ID:
        log.error("Invalid Subscription ID")
        return False
    ad.droid.telephonyAdjustPreciseCallStateListenLevelForSubscription(
        "Foreground", value, sub_id)
    ad.droid.telephonyAdjustPreciseCallStateListenLevelForSubscription(
        "Ringing", value, sub_id)
    ad.droid.telephonyAdjustPreciseCallStateListenLevelForSubscription(
        "Background", value, sub_id)
    return True


def setup_sim(log, ad, sub_id, voice=False, sms=False, data=False):
    """set subscription id for voice, sms and data

    Args:
        log: Log object.
        ad: Android device object.
        sub_id :Subscription ID.
        voice: True if to set subscription as default voice subscription
        sms: True if to set subscription as default sms subscription
        data: True if to set subscription as default data subscription

    """
    if sub_id == INVALID_SUB_ID:
        log.error("Invalid Subscription ID")
        return False
    else:
        if voice:
            if not set_preferred_subid_for_voice(log, ad, sub_id):
                return False
        if sms:
            if not set_preferred_subid_for_sms(log, ad, sub_id):
                return False
        if data:
            if not set_preferred_subid_for_data(log, ad, sub_id):
                return False
    return True


def is_event_match(event, field, value):
    """Return if <field> in "event" match <value> or not.

    Args:
        event: event to test. This event need to have <field>.
        field: field to match.
        value: value to match.

    Returns:
        True if <field> in "event" match <value>.
        False otherwise.
    """
    return is_event_match_for_list(event, field, [value])


def is_event_match_for_list(event, field, value_list):
    """Return if <field> in "event" match any one of the value
        in "value_list" or not.

    Args:
        event: event to test. This event need to have <field>.
        field: field to match.
        value_list: a list of value to match.

    Returns:
        True if <field> in "event" match one of the value in "value_list".
        False otherwise.
    """
    try:
        value_in_event = event['data'][field]
    except KeyError:
        return False
    for value in value_list:
        if value_in_event == value:
            return True
    return False


def is_network_call_back_event_match(event, network_callback_id,
                                     network_callback_event):
    try:
        return ((network_callback_id == event[
            'data'][NetworkCallbackContainer.ID]) and
                (network_callback_event == event['data'][
                    NetworkCallbackContainer.NETWORK_CALLBACK_EVENT]))
    except KeyError:
        return False


def is_build_id(log, ad, build_id):
    """Return if ad's build id is the same as input parameter build_id.

    Args:
        log: log object.
        ad: android device object.
        build_id: android build id.

    Returns:
        True if ad's build id is the same as input parameter build_id.
        False otherwise.
    """
    actual_bid = ad.droid.getBuildID()

    log.info("{} BUILD DISPLAY: {}"
             .format(ad.serial, ad.droid.getBuildDisplay()))
    #In case we want to log more stuff/more granularity...
    #log.info("{} BUILD ID:{} ".format(ad.serial, ad.droid.getBuildID()))
    #log.info("{} BUILD FINGERPRINT: {} "
    # .format(ad.serial), ad.droid.getBuildFingerprint())
    #log.info("{} BUILD TYPE: {} "
    # .format(ad.serial), ad.droid.getBuildType())
    #log.info("{} BUILD NUMBER: {} "
    # .format(ad.serial), ad.droid.getBuildNumber())
    if actual_bid.upper() != build_id.upper():
        log.error("{}: Incorrect Build ID".format(ad.model))
        return False
    return True


def is_uri_equivalent(uri1, uri2):
    """Check whether two input uris match or not.

    Compare Uris.
        If Uris are tel URI, it will only take the digit part
        and compare as phone number.
        Else, it will just do string compare.

    Args:
        uri1: 1st uri to be compared.
        uri2: 2nd uri to be compared.

    Returns:
        True if two uris match. Otherwise False.
    """

    #If either is None/empty we return false
    if not uri1 or not uri2:
        return False

    try:
        if uri1.startswith('tel:') and uri2.startswith('tel:'):
            uri1_number = get_number_from_tel_uri(uri1)
            uri2_number = get_number_from_tel_uri(uri2)
            return check_phone_number_match(uri1_number, uri2_number)
        else:
            return uri1 == uri2
    except AttributeError as e:
        return False


def get_call_uri(ad, call_id):
    """Get call's uri field.

    Get Uri for call_id in ad.

    Args:
        ad: android device object.
        call_id: the call id to get Uri from.

    Returns:
        call's Uri if call is active and have uri field. None otherwise.
    """
    try:
        call_detail = ad.droid.telecomCallGetDetails(call_id)
        return call_detail["Handle"]["Uri"]
    except:
        return None


def get_number_from_tel_uri(uri):
    """Get Uri number from tel uri

    Args:
        uri: input uri

    Returns:
        If input uri is tel uri, return the number part.
        else return None.
    """
    if uri.startswith('tel:'):
        uri_number = ''.join(i for i in urllib.parse.unquote(uri)
                             if i.isdigit())
        return uri_number
    else:
        return None


# TODO: b/26294018 Remove wrapper class once wifi_utils methods updated
class WifiUtils():

    from acts.test_utils.wifi.wifi_test_utils \
        import reset_wifi as _reset_wifi
    from acts.test_utils.wifi.wifi_test_utils \
        import wifi_connect as _wifi_connect
    from acts.test_utils.wifi.wifi_test_utils \
        import wifi_toggle_state as _wifi_toggle_state
    from acts.test_utils.wifi.wifi_test_utils \
        import start_wifi_tethering as _start_wifi_tethering
    from acts.test_utils.wifi.wifi_test_utils \
        import stop_wifi_tethering as _stop_wifi_tethering
    from acts.test_utils.wifi.wifi_test_utils \
        import WifiEnums as _WifiEnums

    WIFI_CONFIG_APBAND_2G = _WifiEnums.WIFI_CONFIG_APBAND_2G
    WIFI_CONFIG_APBAND_5G = _WifiEnums.WIFI_CONFIG_APBAND_5G
    SSID_KEY = _WifiEnums.SSID_KEY
    PWD_KEY = _WifiEnums.PWD_KEY

    @staticmethod
    def wifi_toggle_state(log, ad, state):
        try:
            WifiUtils._wifi_toggle_state(ad, state)
        except Exception as e:
            log.error("WifiUtils.wifi_toggle_state exception: {}".format(e))
            return False
        return True

    @staticmethod
    def wifi_reset(log, ad, disable_wifi=True):
        try:
            WifiUtils._reset_wifi(ad)
        except Exception as e:
            log.error("WifiUtils.wifi_reset exception: {}".format(e))
            return False
        finally:
            if disable_wifi is True:
                ad.droid.wifiToggleState(False)
                # Ensure toggle state has human-time to take effect
            time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        return True

    @staticmethod
    def wifi_connect(log, ad, ssid, password=None):
        if password == "":
            password = None
        try:
            network = {WifiUtils.SSID_KEY: ssid}
            if password:
                network[WifiUtils.PWD_KEY] = password
            WifiUtils._wifi_connect(ad, network)
        except Empty:
            # did not get event, then check connection info
            try:
                if ad.droid.wifiGetConnectionInfo()[
                        WifiUtils.SSID_KEY] == ssid:
                    return True
                else:
                    log.error(
                        "WifiUtils.wifi_connect not connected."
                        "No event received. Expected SSID: {}, current SSID:{}".format(
                            ssid, ad.droid.wifiGetConnectionInfo()[
                                WifiUtils.SSID_KEY]))
                    return False
            except Exception as e:
                log.error("WifiUtils.wifi_connect not connected, no event.")
                return False
        except Exception as e:
            log.error("WifiUtils.wifi_connect exception: {}".format(e))
            return False
        return True

    @staticmethod
    def start_wifi_tethering(log, ad, ssid, password, ap_band=None):
        try:
            return WifiUtils._start_wifi_tethering(ad, ssid, password, ap_band)
        except Exception as e:
            log.error("WifiUtils.start_wifi_tethering exception: {}".format(e))
            return False

    @staticmethod
    def stop_wifi_tethering(log, ad):
        try:
            WifiUtils._stop_wifi_tethering(ad)
            return True
        except Exception as e:
            log.error("WifiUtils.stop_wifi_tethering exception: {}".format(e))
            return False

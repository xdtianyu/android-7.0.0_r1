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
from queue import Empty
from acts.test_utils.tel.tel_defines import AUDIO_ROUTE_EARPIECE
from acts.test_utils.tel.tel_defines import CALL_STATE_RINGING
from acts.test_utils.tel.tel_defines import INCALL_UI_DISPLAY_BACKGROUND
from acts.test_utils.tel.tel_defines import INCALL_UI_DISPLAY_FOREGROUND
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_ACCEPT_CALL_TO_OFFHOOK_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALL_INITIATION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_CALLEE_RINGING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TELECOM_RINGING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VIDEO_SESSION_EVENT
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_VOLTE_ENABLED
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_OFFHOOK
from acts.test_utils.tel.tel_defines import TELEPHONY_STATE_RINGING
from acts.test_utils.tel.tel_defines import VT_STATE_AUDIO_ONLY
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL_PAUSED
from acts.test_utils.tel.tel_defines import VT_STATE_RX_ENABLED
from acts.test_utils.tel.tel_defines import VT_STATE_RX_PAUSED
from acts.test_utils.tel.tel_defines import VT_STATE_TX_ENABLED
from acts.test_utils.tel.tel_defines import VT_STATE_TX_PAUSED
from acts.test_utils.tel.tel_defines import VT_STATE_STATE_INVALID
from acts.test_utils.tel.tel_defines import VT_VIDEO_QUALITY_DEFAULT
from acts.test_utils.tel.tel_defines import WAIT_TIME_ACCEPT_VIDEO_CALL_TO_CHECK_STATE
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import EventCallStateChanged
from acts.test_utils.tel.tel_defines import EventTelecomVideoCallSessionModifyRequestReceived
from acts.test_utils.tel.tel_defines import EventTelecomVideoCallSessionModifyResponseReceived
from acts.test_utils.tel.tel_defines import EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED
from acts.test_utils.tel.tel_defines import EVENT_VIDEO_SESSION_MODIFY_REQUEST_RECEIVED
from acts.test_utils.tel.tel_defines import CallStateContainer
from acts.test_utils.tel.tel_subscription_utils import get_outgoing_voice_sub_id
from acts.test_utils.tel.tel_subscription_utils import get_incoming_voice_sub_id
from acts.test_utils.tel.tel_test_utils import check_phone_number_match
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import is_event_match
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import set_wfc_mode
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import toggle_volte
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_test_utils import wait_for_network_generation
from acts.test_utils.tel.tel_test_utils import wait_for_network_rat_for_subscription
from acts.test_utils.tel.tel_test_utils import wait_for_ringing_event
from acts.test_utils.tel.tel_test_utils import wait_for_telecom_ringing
from acts.test_utils.tel.tel_test_utils import wait_for_video_enabled
from acts.test_utils.tel.tel_voice_utils import is_call_hd


def phone_setup_video(log, ad):
    """Setup phone default sub_id to make video call

    Args:
        log: log object.
        ad: android device object

    Returns:
        True if ad (default sub_id) is setup correctly and idle for video call.
    """
    return phone_setup_video_for_subscription(log, ad,
                                              get_outgoing_voice_sub_id(ad))


def phone_setup_video_for_subscription(log, ad, sub_id):
    """Setup phone sub_id to make video call

    Args:
        log: log object.
        ad: android device object
        sub_id: ad's sub id.

    Returns:
        True if ad (sub_id) is setup correctly and idle for video call.
    """

    # FIXME: temporarily disable selinux due to b/26953532
    ad.adb.shell("setenforce", "0")

    toggle_airplane_mode(log, ad, False)
    if not set_wfc_mode(log, ad, WFC_MODE_DISABLED):
        log.error("{} Disable WFC failed.".format(ad.serial))
        return False
    toggle_volte(log, ad, True)

    if not ensure_network_generation(log,
                                     ad,
                                     GEN_4G,
                                     voice_or_data=NETWORK_SERVICE_DATA):
        log.error("{} voice not in LTE mode.".format(ad.serial))
        return False

    return phone_idle_video_for_subscription(log, ad, sub_id)


def phone_idle_video(log, ad):
    """Return if phone (default sub_id) is idle for video call.

    Args:
        log: log object.
        ad: android device object

    Returns:
        True if ad is idle for video call.
    """
    return phone_idle_video_for_subscription(log, ad,
                                             get_outgoing_voice_sub_id(ad))


def phone_idle_video_for_subscription(log, ad, sub_id):
    """Return if phone (sub_id) is idle for video call.

    Args:
        log: log object.
        ad: android device object
        sub_id: ad's sub id

    Returns:
        True if ad (sub_id) is idle for video call.
    """

    if not wait_for_network_generation(log, ad, GEN_4G):
        log.error("{} voice not in LTE mode.".format(ad.serial))
        return False

    if not wait_for_video_enabled(log, ad, MAX_WAIT_TIME_VOLTE_ENABLED):
        log.error(
            "{} failed to <report volte enabled true> within {}s.".format(
                ad.serial, MAX_WAIT_TIME_VOLTE_ENABLED))
        return False
    return True


def is_phone_in_call_video(log, ad):
    """Return if ad is in a video call (in expected video state).

    Args:
        log: log object.
        ad: android device object
        video_state: Expected Video call state.
            This is optional, if it's None,
            then TX_ENABLED/RX_ENABLED/BIDIRECTIONAL video call state will
            return True.

    Returns:
        True if ad (for sub_id) is in a video call (in expected video state).
    """
    return is_phone_in_call_video_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_video_for_subscription(log, ad, sub_id, video_state=None):
    """Return if ad (for sub_id) is in a video call (in expected video state).
    Args:
        log: log object.
        ad: android device object
        sub_id: device sub_id
        video_state: Expected Video call state.
            This is optional, if it's None,
            then TX_ENABLED/RX_ENABLED/BIDIRECTIONAL video call state will
            return True.

    Returns:
        True if ad is in a video call (in expected video state).
    """

    if video_state is None:
        log.info("Verify if {}(subid {}) in video call.".format(ad.serial,
                                                                sub_id))
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    call_list = ad.droid.telecomCallGetCallIds()
    for call in call_list:
        state = ad.droid.telecomCallVideoGetState(call)
        if video_state is None:
            if {
                    VT_STATE_AUDIO_ONLY: False,
                    VT_STATE_TX_ENABLED: True,
                    VT_STATE_TX_PAUSED: True,
                    VT_STATE_RX_ENABLED: True,
                    VT_STATE_RX_PAUSED: True,
                    VT_STATE_BIDIRECTIONAL: True,
                    VT_STATE_BIDIRECTIONAL_PAUSED: True,
                    VT_STATE_STATE_INVALID: False
            }[state]:
                return True
        else:
            if state == video_state:
                return True
        log.info("Non-Video-State: {}".format(state))
    log.error("Phone not in video call. Call list: {}".format(call_list))
    return False


def is_phone_in_call_video_bidirectional(log, ad):
    """Return if phone in bi-directional video call.

    Args:
        log: log object.
        ad: android device object

    Returns:
        True if phone in bi-directional video call.
    """
    return is_phone_in_call_video_bidirectional_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_video_bidirectional_for_subscription(log, ad, sub_id):
    """Return if phone in bi-directional video call for subscription id.

    Args:
        log: log object.
        ad: android device object
        sub_id: subscription id.

    Returns:
        True if phone in bi-directional video call.
    """
    log.info("Verify if {}(subid {}) in bi-directional video call.".format(
        ad.serial, sub_id))
    return is_phone_in_call_video_for_subscription(log, ad, sub_id,
                                                   VT_STATE_BIDIRECTIONAL)


def is_phone_in_call_video_tx_enabled(log, ad):
    """Return if phone in tx_enabled video call.

    Args:
        log: log object.
        ad: android device object

    Returns:
        True if phone in tx_enabled video call.
    """
    return is_phone_in_call_video_tx_enabled_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_video_tx_enabled_for_subscription(log, ad, sub_id):
    """Return if phone in tx_enabled video call for subscription id.

    Args:
        log: log object.
        ad: android device object
        sub_id: subscription id.

    Returns:
        True if phone in tx_enabled video call.
    """
    log.info("Verify if {}(subid {}) in tx_enabled video call.".format(
        ad.serial, sub_id))
    return is_phone_in_call_video_for_subscription(log, ad, sub_id,
                                                   VT_STATE_TX_ENABLED)


def is_phone_in_call_video_rx_enabled(log, ad):
    """Return if phone in rx_enabled video call.

    Args:
        log: log object.
        ad: android device object

    Returns:
        True if phone in rx_enabled video call.
    """
    return is_phone_in_call_video_rx_enabled_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_video_rx_enabled_for_subscription(log, ad, sub_id):
    """Return if phone in rx_enabled video call for subscription id.

    Args:
        log: log object.
        ad: android device object
        sub_id: subscription id.

    Returns:
        True if phone in rx_enabled video call.
    """
    log.info("Verify if {}(subid {}) in rx_enabled video call.".format(
        ad.serial, sub_id))
    return is_phone_in_call_video_for_subscription(log, ad, sub_id,
                                                   VT_STATE_RX_ENABLED)


def is_phone_in_call_voice_hd(log, ad):
    """Return if phone in hd voice call.

    Args:
        log: log object.
        ad: android device object

    Returns:
        True if phone in hd voice call.
    """
    return is_phone_in_call_voice_hd_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad))


def is_phone_in_call_voice_hd_for_subscription(log, ad, sub_id):
    """Return if phone in hd voice call for subscription id.

    Args:
        log: log object.
        ad: android device object
        sub_id: subscription id.

    Returns:
        True if phone in hd voice call.
    """
    log.info("Verify if {}(subid {}) in hd voice call.".format(ad.serial,
                                                               sub_id))
    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return False
    for call in ad.droid.telecomCallGetCallIds():
        state = ad.droid.telecomCallVideoGetState(call)
        if (state == VT_STATE_AUDIO_ONLY and is_call_hd(log, ad, call)):
            return True
        log.info("Non-HDAudio-State: {}, property: {}".format(
            state, ad.droid.telecomCallGetProperties(call)))
    return False


def initiate_video_call(log, ad_caller, callee_number):
    """Make phone call from caller to callee.

    Args:
        ad_caller: Caller android device object.
        callee_number: Callee phone number.
        emergency : specify the call is emergency.
            Optional. Default value is False.

    Returns:
        result: if phone call is placed successfully.
    """

    wait_time_for_incall_state = MAX_WAIT_TIME_CALL_INITIATION
    ad_caller.droid.telecomCallNumber(callee_number, True)
    while wait_time_for_incall_state > 0:
        wait_time_for_incall_state -= 1
        if ad_caller.droid.telecomIsInCall():
            return True
        time.sleep(1)
    log.error("Make call fail.")
    return False


def wait_and_answer_video_call(log,
                               ad,
                               incoming_number=None,
                               video_state=VT_STATE_BIDIRECTIONAL,
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
    return wait_and_answer_video_call_for_subscription(
        log, ad, get_outgoing_voice_sub_id(ad), incoming_number, video_state,
        incall_ui_display)


def wait_and_answer_video_call_for_subscription(
        log,
        ad,
        sub_id,
        incoming_number=None,
        video_state=VT_STATE_BIDIRECTIONAL,
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
                event_ringing['data']['incomingNumber'], incoming_number)

        if not result:
            log.error("Incoming Number not match")
            log.error("Expected number:{}, actual number:{}".format(
                incoming_number, event_ringing['data']['incomingNumber']))
            return False

    ad.ed.clear_all_events()

    ad.droid.telephonyStartTrackingCallStateForSubscription(sub_id)

    if not wait_for_telecom_ringing(log, ad, MAX_WAIT_TIME_TELECOM_RINGING):
        log.error("Telecom is not ringing.")
        return False

    log.info("Accept on callee.")
    ad.droid.telecomAcceptRingingCall(video_state)

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


def video_call_setup_teardown(log,
                              ad_caller,
                              ad_callee,
                              ad_hangup=None,
                              video_state=VT_STATE_BIDIRECTIONAL,
                              verify_caller_func=None,
                              verify_callee_func=None,
                              wait_time_in_call=WAIT_TIME_IN_CALL,
                              incall_ui_display=INCALL_UI_DISPLAY_FOREGROUND):
    """ Call process, including make a phone call from caller,
    accept from callee, and hang up. The call is on default subscription

    In call process, call from <droid_caller> to <droid_callee>,
    accept the call, (optional)then hang up from <droid_hangup>.

    Args:
        ad_caller: Caller Android Device Object.
        ad_callee: Callee Android Device Object.
        ad_hangup: Android Device Object end the phone call.
            Optional. Default value is None, and phone call will continue.
        video_state: video state for VT call.
            Optional. Default value is VT_STATE_BIDIRECTIONAL
        verify_caller_func: func_ptr to verify caller in correct mode
            Optional. Default is None
        verify_callee_func: func_ptr to verify callee in correct mode
            Optional. Default is None
        wait_time_in_call: wait time during call.
            Optional. Default is WAIT_TIME_IN_CALL.
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True if call process without any error.
        False if error happened.

    """
    return video_call_setup_teardown_for_subscription(
        log, ad_caller, ad_callee, get_outgoing_voice_sub_id(ad_caller),
        get_incoming_voice_sub_id(ad_callee), ad_hangup, video_state,
        verify_caller_func, verify_callee_func, wait_time_in_call,
        incall_ui_display)


# TODO: b/26337151 Might be able to re-factor call_setup_teardown and add.
# Minimal changes.
def video_call_setup_teardown_for_subscription(
        log,
        ad_caller,
        ad_callee,
        subid_caller,
        subid_callee,
        ad_hangup=None,
        video_state=VT_STATE_BIDIRECTIONAL,
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
        video_state: video state for VT call.
            Optional. Default value is VT_STATE_BIDIRECTIONAL
        verify_caller_func: func_ptr to verify caller in correct mode
            Optional. Default is None
        verify_callee_func: func_ptr to verify callee in correct mode
            Optional. Default is None
        wait_time_in_call: wait time during call.
            Optional. Default is WAIT_TIME_IN_CALL.
        incall_ui_display: after answer the call, bring in-call UI to foreground or
            background. Optional, default value is INCALL_UI_DISPLAY_FOREGROUND.
            if = INCALL_UI_DISPLAY_FOREGROUND, bring in-call UI to foreground.
            if = INCALL_UI_DISPLAY_BACKGROUND, bring in-call UI to background.
            else, do nothing.

    Returns:
        True if call process without any error.
        False if error happened.

    """

    class _CallSequenceException(Exception):
        pass

    caller_number = ad_caller.cfg['subscription'][subid_caller]['phone_num']
    callee_number = ad_callee.cfg['subscription'][subid_callee]['phone_num']

    log.info("Call from {} to {}".format(caller_number, callee_number))

    try:
        if not initiate_video_call(log, ad_caller, callee_number):
            raise _CallSequenceException("Initiate call failed.")

        if not wait_and_answer_video_call_for_subscription(
                log,
                ad_callee,
                subid_callee,
                incoming_number=caller_number,
                video_state=video_state,
                incall_ui_display=incall_ui_display):
            raise _CallSequenceException("Answer call fail.")

        # ensure that all internal states are updated in telecom
        time.sleep(WAIT_TIME_ACCEPT_VIDEO_CALL_TO_CHECK_STATE)

        # Check if caller/callee dropped call.
        if not verify_incall_state(log, [ad_callee, ad_caller], True):
            raise _CallSequenceException("Call Drop!")
        # Check Callee first
        # in case of VT call drop, it usually start from callee
        if verify_callee_func and not verify_callee_func(log, ad_callee):
            raise _CallSequenceException("Callee not in correct state!")
        if verify_caller_func and not verify_caller_func(log, ad_caller):
            raise _CallSequenceException("Caller not in correct state!")

        # TODO: b/26291165 Replace with reducing the volume as we want
        # to test route switching
        ad_caller.droid.telecomCallSetAudioRoute(AUDIO_ROUTE_EARPIECE)
        ad_callee.droid.telecomCallSetAudioRoute(AUDIO_ROUTE_EARPIECE)

        time.sleep(wait_time_in_call)

        # Check Callee first
        # in case of VT call drop, it usually start from callee
        if not verify_callee_func:
            callee_state_result = ad_callee.droid.telecomIsInCall()
        else:
            callee_state_result = verify_callee_func(log, ad_callee)
        if not callee_state_result:
            raise _CallSequenceException(
                "Callee not in correct state after {} seconds"
                .format(wait_time_in_call))

        if not verify_caller_func:
            caller_state_result = ad_caller.droid.telecomIsInCall()
        else:
            caller_state_result = verify_caller_func(log, ad_caller)
        if not caller_state_result:
            raise _CallSequenceException(
                "Caller not in correct state after {} seconds"
                .format(wait_time_in_call))

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


def video_call_modify_video(log,
                            ad_requester,
                            call_id_requester,
                            ad_responder,
                            call_id_responder,
                            video_state_request,
                            video_quality_request=VT_VIDEO_QUALITY_DEFAULT,
                            video_state_response=None,
                            video_quality_response=None,
                            verify_func_between_request_and_response=None):
    """Modifies an ongoing call to change the video_call state

    Args:
        log: logger object
        ad_requester: android_device object of the requester
        call_id_requester: the call_id of the call placing the modify request
        ad_requester: android_device object of the responder
        call_id_requester: the call_id of the call receiving the modify request
        video_state_request: the requested video state
        video_quality_request: the requested video quality, defaults to
            QUALITY_DEFAULT
        video_state_response: the responded video state or, or (default)
            match the request if None
        video_quality_response: the responded video quality, or (default)
            match the request if None

    Returns:
        A call_id corresponding to the first call in the state, or None
    """

    if not video_state_response:
        video_state_response = video_state_request
    if not video_quality_response:
        video_quality_response = video_quality_request

    cur_video_state = ad_requester.droid.telecomCallVideoGetState(
        call_id_requester)

    log.info("State change request from {} to {} requested"
             .format(cur_video_state, video_state_request))

    if cur_video_state == video_state_request:
        return True

    ad_responder.ed.clear_events(
        EventTelecomVideoCallSessionModifyRequestReceived)

    ad_responder.droid.telecomCallVideoStartListeningForEvent(
        call_id_responder, EVENT_VIDEO_SESSION_MODIFY_REQUEST_RECEIVED)

    ad_requester.droid.telecomCallVideoSendSessionModifyRequest(
        call_id_requester, video_state_request, video_quality_request)

    try:
        request_event = ad_responder.ed.pop_event(
            EventTelecomVideoCallSessionModifyRequestReceived,
            MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
        log.info(request_event)
    except Empty:
        log.error("Failed to receive SessionModifyRequest!")
        return False
    finally:
        ad_responder.droid.telecomCallVideoStopListeningForEvent(
            call_id_responder, EVENT_VIDEO_SESSION_MODIFY_REQUEST_RECEIVED)

    if (verify_func_between_request_and_response and
            not verify_func_between_request_and_response()):
        log.error("verify_func_between_request_and_response failed.")
        return False

    # TODO: b/26291165 Replace with reducing the volume as we want
    # to test route switching
    ad_requester.droid.telecomCallSetAudioRoute(AUDIO_ROUTE_EARPIECE)

    ad_requester.droid.telecomCallVideoStartListeningForEvent(
        call_id_requester, EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED)

    ad_responder.droid.telecomCallVideoSendSessionModifyResponse(
        call_id_responder, video_state_response, video_quality_response)

    try:
        response_event = ad_requester.ed.pop_event(
            EventTelecomVideoCallSessionModifyResponseReceived,
            MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
        log.info(response_event)
    except Empty:
        log.error("Failed to receive SessionModifyResponse!")
        return False
    finally:
        ad_requester.droid.telecomCallVideoStopListeningForEvent(
            call_id_requester, EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED)

    # TODO: b/26291165 Replace with reducing the volume as we want
    # to test route switching
    ad_responder.droid.telecomCallSetAudioRoute(AUDIO_ROUTE_EARPIECE)

    return True


def is_call_id_in_video_state(log, ad, call_id, video_state):
    """Return is the call_id is in expected video_state

    Args:
        log: logger object
        ad: android_device object
        call_id: call id
        video_state: valid VIDEO_STATE

    Returns:
        True is call_id in expected video_state; False if not.
    """
    return video_state == ad.droid.telecomCallVideoGetState(call_id)


def get_call_id_in_video_state(log, ad, video_state):
    """Gets the first call reporting a given video_state
        from among the active calls

    Args:
        log: logger object
        ad: android_device object
        video_state: valid VIDEO_STATE

    Returns:
        A call_id corresponding to the first call in the state, or None
    """

    if not ad.droid.telecomIsInCall():
        log.error("{} not in call.".format(ad.serial))
        return None
    for call in ad.droid.telecomCallGetCallIds():
        if is_call_id_in_video_state(log, ad, call, video_state):
            return call
    return None


def video_call_downgrade(log,
                         ad_requester,
                         call_id_requester,
                         ad_responder,
                         call_id_responder,
                         video_state_request=None,
                         video_quality_request=VT_VIDEO_QUALITY_DEFAULT):
    """Downgrade Video call to video_state_request.
    Send telecomCallVideoSendSessionModifyRequest from ad_requester.
    Get video call state from ad_requester and ad_responder.
    Verify video calls states are correct and downgrade succeed.

    Args:
        log: logger object
        ad_requester: android_device object of the requester
        call_id_requester: the call_id of the call placing the modify request
        ad_requester: android_device object of the responder
        call_id_requester: the call_id of the call receiving the modify request
        video_state_request: the requested downgrade video state
            This parameter is optional. If this parameter is None:
                if call_id_requester current is bi-directional, will downgrade to RX_ENABLED
                if call_id_requester current is RX_ENABLED, will downgrade to AUDIO_ONLY
        video_quality_request: the requested video quality, defaults to
            QUALITY_DEFAULT
    Returns:
        True if downgrade succeed.
    """
    if (call_id_requester is None) or (call_id_responder is None):
        log.error("call_id_requester: {}, call_id_responder: {}".format(
            call_id_requester, call_id_responder))
        return False
    current_video_state_requester = ad_requester.droid.telecomCallVideoGetState(
        call_id_requester)
    if video_state_request is None:
        if (current_video_state_requester == VT_STATE_BIDIRECTIONAL or
                current_video_state_requester ==
                VT_STATE_BIDIRECTIONAL_PAUSED):
            video_state_request = VT_STATE_RX_ENABLED
        elif (current_video_state_requester == VT_STATE_TX_ENABLED or
              current_video_state_requester == VT_STATE_TX_PAUSED):
            video_state_request = VT_STATE_AUDIO_ONLY
        else:
            log.error("Can Not Downgrade. ad: {}, current state {}".format(
                ad_requester.serial, current_video_state_requester))
            return False
    expected_video_state_responder = {
        VT_STATE_AUDIO_ONLY: VT_STATE_AUDIO_ONLY,
        VT_STATE_RX_ENABLED: VT_STATE_TX_ENABLED
    }[video_state_request]

    ad_requester.droid.telecomCallVideoStartListeningForEvent(
        call_id_requester, EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED)

    ad_requester.droid.telecomCallVideoSendSessionModifyRequest(
        call_id_requester, video_state_request, video_quality_request)

    try:
        response_event = ad_requester.ed.pop_event(
            EventTelecomVideoCallSessionModifyResponseReceived,
            MAX_WAIT_TIME_VIDEO_SESSION_EVENT)
        log.info(response_event)
    except Empty:
        log.error("Failed to receive SessionModifyResponse!")
        return False
    finally:
        ad_requester.droid.telecomCallVideoStopListeningForEvent(
            call_id_requester, EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED)

    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    # TODO: b/26291165 Replace with reducing the volume as we want
    # to test route switching
    ad_requester.droid.telecomCallSetAudioRoute(AUDIO_ROUTE_EARPIECE)
    ad_responder.droid.telecomCallSetAudioRoute(AUDIO_ROUTE_EARPIECE)

    time.sleep(WAIT_TIME_IN_CALL)
    if video_state_request != ad_requester.droid.telecomCallVideoGetState(
            call_id_requester):
        log.error("requester not in correct state. expected:{}, current:{}"
                  .format(video_state_request,
                          ad_requester.droid.telecomCallVideoGetState(
                              call_id_requester)))
        return False
    if (expected_video_state_responder !=
            ad_responder.droid.telecomCallVideoGetState(call_id_responder)):
        log.error(
            "responder not in correct state. expected:{}, current:{}".format(
                expected_video_state_responder,
                ad_responder.droid.telecomCallVideoGetState(
                    call_id_responder)))
        return False

    return True


def verify_video_call_in_expected_state(log, ad, call_id, call_video_state,
                                        call_state):
    """Return True if video call is in expected video state and call state.

    Args:
        log: logger object
        ad: android_device object
        call_id: ad's call id
        call_video_state: video state to validate.
        call_state: call state to validate.

    Returns:
        True if video call is in expected video state and call state.
    """
    if not is_call_id_in_video_state(log, ad, call_id, call_video_state):
        log.error("Call is not in expected {} state. Current state {}".format(
            call_video_state, ad.droid.telecomCallVideoGetState(call_id)))
        return False
    if ad.droid.telecomCallGetCallState(call_id) != call_state:
        log.error("Call is not in expected {} state. Current state {}".format(
            call_state, ad.droid.telecomCallGetCallState(call_id)))
        return False
    return True

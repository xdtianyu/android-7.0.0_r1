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

###############################################
# TIMERS
###############################################
# Max time to wait for phone data/network connection state update
MAX_WAIT_TIME_CONNECTION_STATE_UPDATE = 20

# Max time to wait for network reselection
MAX_WAIT_TIME_NW_SELECTION = 120

# Max time to wait for call drop
MAX_WAIT_TIME_CALL_DROP = 60

# Max time to wait after caller make a call and before
# callee start ringing
MAX_WAIT_TIME_CALLEE_RINGING = 30

# Max time to wait after caller make a call and before
# callee start ringing
MAX_WAIT_TIME_ACCEPT_CALL_TO_OFFHOOK_EVENT = 30

# Max time to wait for "onCallStatehangedIdle" event after reject or ignore
# incoming call
MAX_WAIT_TIME_CALL_IDLE_EVENT = 60

# Max time to wait after initiating a call for telecom to report in-call
MAX_WAIT_TIME_CALL_INITIATION = 25

# Max time to wait after toggle airplane mode and before
# get expected event
MAX_WAIT_TIME_AIRPLANEMODE_EVENT = 90

# Max time to wait after device sent an SMS and before
# get "onSmsSentSuccess" event
MAX_WAIT_TIME_SMS_SENT_SUCCESS = 60

# Max time to wait after MT SMS was sent and before device
# actually receive this MT SMS.
MAX_WAIT_TIME_SMS_RECEIVE = 120

# Max time to wait for IMS registration
MAX_WAIT_TIME_IMS_REGISTRATION = 120

# TODO: b/26338156 MAX_WAIT_TIME_VOLTE_ENABLED and MAX_WAIT_TIME_WFC_ENABLED should only
# be used for wait after IMS registration.

# Max time to wait for VoLTE enabled flag to be True
MAX_WAIT_TIME_VOLTE_ENABLED = MAX_WAIT_TIME_IMS_REGISTRATION + 20

# Max time to wait for WFC enabled flag to be True
MAX_WAIT_TIME_WFC_ENABLED = MAX_WAIT_TIME_IMS_REGISTRATION + 50

# Max time to wait for WFC enabled flag to be False
MAX_WAIT_TIME_WFC_DISABLED = 60

# Max time to wait for WiFi Manager to Connect to an AP
MAX_WAIT_TIME_WIFI_CONNECTION = 30

# Max time to wait for Video Session Modify Messaging
MAX_WAIT_TIME_VIDEO_SESSION_EVENT = 10

# Max time to wait after a network connection for ConnectivityManager to
# report a working user plane data connection
MAX_WAIT_TIME_USER_PLANE_DATA = 20

# Max time to wait for tethering entitlement check
MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK = 15

# Max time to wait for voice mail count report correct result.
MAX_WAIT_TIME_VOICE_MAIL_COUNT = 30

# Max time to wait for data SIM change
MAX_WAIT_TIME_DATA_SUB_CHANGE = 150

# Max time to wait for telecom Ringing status after receive ringing event
MAX_WAIT_TIME_TELECOM_RINGING = 5

# Max time to wait for phone get provisioned.
MAX_WAIT_TIME_PROVISIONING = 300

# Time to wait after call setup before declaring
# that the call is actually successful
WAIT_TIME_IN_CALL = 15

# (For IMS, e.g. VoLTE-VoLTE, WFC-WFC, VoLTE-WFC test only)
# Time to wait after call setup before declaring
# that the call is actually successful
WAIT_TIME_IN_CALL_FOR_IMS = 30

# Time to wait after phone receive incoming call before phone reject this call.
WAIT_TIME_REJECT_CALL = 2

# Time to leave a voice message after callee reject the incoming call
WAIT_TIME_LEAVE_VOICE_MAIL = 30

# Time to wait after accept video call and before checking state
WAIT_TIME_ACCEPT_VIDEO_CALL_TO_CHECK_STATE = 2

# Time delay to ensure user actions are performed in
# 'human' time rather than at the speed of the script
WAIT_TIME_ANDROID_STATE_SETTLING = 1

# Time to wait after registration to ensure the phone
# has sufficient time to reconfigure based on new network
WAIT_TIME_BETWEEN_REG_AND_CALL = 5

# Time to wait for 1xrtt voice attach check
# After DUT voice network type report 1xrtt (from unknown), it need to wait for
# several seconds before the DUT can receive incoming call.
WAIT_TIME_1XRTT_VOICE_ATTACH = 30

# Time to wait for data status change during wifi tethering,.
WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING = 30

# Time to wait for rssi calibration.
# This is the delay between <WiFi Connected> and <Turn on Screen to get RSSI>.
WAIT_TIME_WIFI_RSSI_CALIBRATION_WIFI_CONNECTED = 10
# This is the delay between <Turn on Screen> and <Call API to get WiFi RSSI>.
WAIT_TIME_WIFI_RSSI_CALIBRATION_SCREEN_ON = 2

# Time to wait for each operation on voice mail box.
WAIT_TIME_VOICE_MAIL_SERVER_RESPONSE = 10

# Time to wait for radio to up and running after reboot
WAIT_TIME_AFTER_REBOOT = 10

# Time to wait for tethering test after reboot
WAIT_TIME_TETHERING_AFTER_REBOOT = 10

# Time to wait after changing data sub id
WAIT_TIME_CHANGE_DATA_SUB_ID = 30

# These are used in phone_number_formatter
PHONE_NUMBER_STRING_FORMAT_7_DIGIT = 7
PHONE_NUMBER_STRING_FORMAT_10_DIGIT = 10
PHONE_NUMBER_STRING_FORMAT_11_DIGIT = 11
PHONE_NUMBER_STRING_FORMAT_12_DIGIT = 12

# MAX screen-on time during test (in unit of second)
MAX_SCREEN_ON_TIME = 1800

# In Voice Mail box, press this digit to delete one message.
VOICEMAIL_DELETE_DIGIT = '7'

# MAX number of saved voice mail in voice mail box.
MAX_SAVED_VOICE_MAIL = 25

# SIM1 slot index
SIM1_SLOT_INDEX = 0

# SIM2 slot index
SIM2_SLOT_INDEX = 1

# invalid Subscription ID
INVALID_SUB_ID = -1

# invalid SIM slot index
INVALID_SIM_SLOT_INDEX = -1

# WiFI RSSI is -127 if WiFi is not connected
INVALID_WIFI_RSSI = -127

# MAX and MIN value for attenuator settings
ATTEN_MAX_VALUE = 90
ATTEN_MIN_VALUE = 0

MAX_RSSI_RESERVED_VALUE = 100
MIN_RSSI_RESERVED_VALUE = -200

# cellular weak RSSI value
CELL_WEAK_RSSI_VALUE = -120
# cellular strong RSSI value
CELL_STRONG_RSSI_VALUE = -70
# WiFi weak RSSI value
WIFI_WEAK_RSSI_VALUE = -80

# Emergency call number
EMERGENCY_CALL_NUMBER = "911"

AOSP_PREFIX = "aosp_"

INCALL_UI_DISPLAY_FOREGROUND = "foreground"
INCALL_UI_DISPLAY_BACKGROUND = "background"
INCALL_UI_DISPLAY_DEFAULT = "default"

NETWORK_CONNECTION_TYPE_WIFI = 'wifi'
NETWORK_CONNECTION_TYPE_CELL = 'cell'
NETWORK_CONNECTION_TYPE_MMS = 'mms'
NETWORK_CONNECTION_TYPE_HIPRI = 'hipri'
NETWORK_CONNECTION_TYPE_UNKNOWN = 'unknown'

TETHERING_MODE_WIFI = 'wifi'

NETWORK_SERVICE_VOICE = 'voice'
NETWORK_SERVICE_DATA = 'data'

CARRIER_VZW = 'vzw'
CARRIER_ATT = 'att'
CARRIER_TMO = 'tmo'
CARRIER_SPT = 'spt'
CARRIER_EEUK = 'eeuk'
CARRIER_VFUK = 'vfuk'
CARRIER_UNKNOWN = 'unknown'

RAT_FAMILY_CDMA = 'cdma'
RAT_FAMILY_CDMA2000 = 'cdma2000'
RAT_FAMILY_IDEN = 'iden'
RAT_FAMILY_GSM = 'gsm'
RAT_FAMILY_WCDMA = 'wcdma'
RAT_FAMILY_UMTS = RAT_FAMILY_WCDMA
RAT_FAMILY_WLAN = 'wlan'
RAT_FAMILY_LTE = 'lte'
RAT_FAMILY_TDSCDMA = 'tdscdma'
RAT_FAMILY_UNKNOWN = 'unknown'

CAPABILITY_PHONE = 'phone'
CAPABILITY_VOLTE = 'volte'
CAPABILITY_VT = 'vt'
CAPABILITY_WFC = 'wfc'
CAPABILITY_MSIM = 'msim'
CAPABILITY_OMADM = 'omadm'

# Constant for operation direction
DIRECTION_MOBILE_ORIGINATED = "MO"
DIRECTION_MOBILE_TERMINATED = "MT"

# Constant for call teardown side
CALL_TEARDOWN_PHONE = "PHONE"
CALL_TEARDOWN_REMOTE = "REMOTE"

WIFI_VERBOSE_LOGGING_ENABLED = 1
WIFI_VERBOSE_LOGGING_DISABLED = 0
"""
Begin shared constant define for both Python and Java
"""

# Constant for WiFi Calling WFC mode
WFC_MODE_WIFI_ONLY = "WIFI_ONLY"
WFC_MODE_CELLULAR_PREFERRED = "CELLULAR_PREFERRED"
WFC_MODE_WIFI_PREFERRED = "WIFI_PREFERRED"
WFC_MODE_DISABLED = "DISABLED"
WFC_MODE_UNKNOWN = "UNKNOWN"

# Constant for Video Telephony VT state
VT_STATE_AUDIO_ONLY = "AUDIO_ONLY"
VT_STATE_TX_ENABLED = "TX_ENABLED"
VT_STATE_RX_ENABLED = "RX_ENABLED"
VT_STATE_BIDIRECTIONAL = "BIDIRECTIONAL"
VT_STATE_TX_PAUSED = "TX_PAUSED"
VT_STATE_RX_PAUSED = "RX_PAUSED"
VT_STATE_BIDIRECTIONAL_PAUSED = "BIDIRECTIONAL_PAUSED"
VT_STATE_STATE_INVALID = "INVALID"

# Constant for Video Telephony Video quality
VT_VIDEO_QUALITY_DEFAULT = "DEFAULT"
VT_VIDEO_QUALITY_UNKNOWN = "UNKNOWN"
VT_VIDEO_QUALITY_HIGH = "HIGH"
VT_VIDEO_QUALITY_MEDIUM = "MEDIUM"
VT_VIDEO_QUALITY_LOW = "LOW"
VT_VIDEO_QUALITY_INVALID = "INVALID"

# Constant for Call State (for call object)
CALL_STATE_ACTIVE = "ACTIVE"
CALL_STATE_NEW = "NEW"
CALL_STATE_DIALING = "DIALING"
CALL_STATE_RINGING = "RINGING"
CALL_STATE_HOLDING = "HOLDING"
CALL_STATE_DISCONNECTED = "DISCONNECTED"
CALL_STATE_PRE_DIAL_WAIT = "PRE_DIAL_WAIT"
CALL_STATE_CONNECTING = "CONNECTING"
CALL_STATE_DISCONNECTING = "DISCONNECTING"
CALL_STATE_UNKNOWN = "UNKNOWN"
CALL_STATE_INVALID = "INVALID"

# Constant for PRECISE Call State (for call object)
PRECISE_CALL_STATE_ACTIVE = "ACTIVE"
PRECISE_CALL_STATE_ALERTING = "ALERTING"
PRECISE_CALL_STATE_DIALING = "DIALING"
PRECISE_CALL_STATE_INCOMING = "INCOMING"
PRECISE_CALL_STATE_HOLDING = "HOLDING"
PRECISE_CALL_STATE_DISCONNECTED = "DISCONNECTED"
PRECISE_CALL_STATE_WAITING = "WAITING"
PRECISE_CALL_STATE_DISCONNECTING = "DISCONNECTING"
PRECISE_CALL_STATE_IDLE = "IDLE"
PRECISE_CALL_STATE_UNKNOWN = "UNKNOWN"
PRECISE_CALL_STATE_INVALID = "INVALID"

# Constant for DC POWER STATE
DC_POWER_STATE_LOW = "LOW"
DC_POWER_STATE_HIGH = "HIGH"
DC_POWER_STATE_MEDIUM = "MEDIUM"
DC_POWER_STATE_UNKNOWN = "UNKNOWN"

# Constant for Audio Route
AUDIO_ROUTE_EARPIECE = "EARPIECE"
AUDIO_ROUTE_BLUETOOTH = "BLUETOOTH"
AUDIO_ROUTE_SPEAKER = "SPEAKER"
AUDIO_ROUTE_WIRED_HEADSET = "WIRED_HEADSET"
AUDIO_ROUTE_WIRED_OR_EARPIECE = "WIRED_OR_EARPIECE"

# Constant for Call Capability
CALL_CAPABILITY_HOLD = "HOLD"
CALL_CAPABILITY_SUPPORT_HOLD = "SUPPORT_HOLD"
CALL_CAPABILITY_MERGE_CONFERENCE = "MERGE_CONFERENCE"
CALL_CAPABILITY_SWAP_CONFERENCE = "SWAP_CONFERENCE"
CALL_CAPABILITY_UNUSED_1 = "UNUSED_1"
CALL_CAPABILITY_RESPOND_VIA_TEXT = "RESPOND_VIA_TEXT"
CALL_CAPABILITY_MUTE = "MUTE"
CALL_CAPABILITY_MANAGE_CONFERENCE = "MANAGE_CONFERENCE"
CALL_CAPABILITY_SUPPORTS_VT_LOCAL_RX = "SUPPORTS_VT_LOCAL_RX"
CALL_CAPABILITY_SUPPORTS_VT_LOCAL_TX = "SUPPORTS_VT_LOCAL_TX"
CALL_CAPABILITY_SUPPORTS_VT_LOCAL_BIDIRECTIONAL = "SUPPORTS_VT_LOCAL_BIDIRECTIONAL"
CALL_CAPABILITY_SUPPORTS_VT_REMOTE_RX = "SUPPORTS_VT_REMOTE_RX"
CALL_CAPABILITY_SUPPORTS_VT_REMOTE_TX = "SUPPORTS_VT_REMOTE_TX"
CALL_CAPABILITY_SUPPORTS_VT_REMOTE_BIDIRECTIONAL = "SUPPORTS_VT_REMOTE_BIDIRECTIONAL"
CALL_CAPABILITY_SEPARATE_FROM_CONFERENCE = "SEPARATE_FROM_CONFERENCE"
CALL_CAPABILITY_DISCONNECT_FROM_CONFERENCE = "DISCONNECT_FROM_CONFERENCE"
CALL_CAPABILITY_SPEED_UP_MT_AUDIO = "SPEED_UP_MT_AUDIO"
CALL_CAPABILITY_CAN_UPGRADE_TO_VIDEO = "CAN_UPGRADE_TO_VIDEO"
CALL_CAPABILITY_CAN_PAUSE_VIDEO = "CAN_PAUSE_VIDEO"
CALL_CAPABILITY_UNKOWN = "UNKOWN"

# Constant for Call Property
CALL_PROPERTY_HIGH_DEF_AUDIO = "HIGH_DEF_AUDIO"
CALL_PROPERTY_CONFERENCE = "CONFERENCE"
CALL_PROPERTY_GENERIC_CONFERENCE = "GENERIC_CONFERENCE"
CALL_PROPERTY_WIFI = "WIFI"
CALL_PROPERTY_EMERGENCY_CALLBACK_MODE = "EMERGENCY_CALLBACK_MODE"
CALL_PROPERTY_UNKNOWN = "UNKNOWN"

# Constant for Call Presentation
CALL_PRESENTATION_ALLOWED = "ALLOWED"
CALL_PRESENTATION_RESTRICTED = "RESTRICTED"
CALL_PRESENTATION_PAYPHONE = "PAYPHONE"
CALL_PRESENTATION_UNKNOWN = "UNKNOWN"

# Constant for Network Generation
GEN_2G = "2G"
GEN_3G = "3G"
GEN_4G = "4G"
GEN_UNKNOWN = "UNKNOWN"

# Constant for Network RAT
RAT_IWLAN = "IWLAN"
RAT_LTE = "LTE"
RAT_4G = "4G"
RAT_3G = "3G"
RAT_2G = "2G"
RAT_WCDMA = "WCDMA"
RAT_UMTS = "UMTS"
RAT_1XRTT = "1XRTT"
RAT_EDGE = "EDGE"
RAT_GPRS = "GPRS"
RAT_HSDPA = "HSDPA"
RAT_HSUPA = "HSUPA"
RAT_CDMA = "CDMA"
RAT_EVDO = "EVDO"
RAT_EVDO_0 = "EVDO_0"
RAT_EVDO_A = "EVDO_A"
RAT_EVDO_B = "EVDO_B"
RAT_IDEN = "IDEN"
RAT_EHRPD = "EHRPD"
RAT_HSPA = "HSPA"
RAT_HSPAP = "HSPAP"
RAT_GSM = "GSM"
RAT_TD_SCDMA = "TD_SCDMA"
RAT_GLOBAL = "GLOBAL"
RAT_UNKNOWN = "UNKNOWN"

# Constant for Phone Type
PHONE_TYPE_GSM = "GSM"
PHONE_TYPE_NONE = "NONE"
PHONE_TYPE_CDMA = "CDMA"
PHONE_TYPE_SIP = "SIP"

# Constant for SIM State
SIM_STATE_READY = "READY"
SIM_STATE_UNKNOWN = "UNKNOWN"
SIM_STATE_ABSENT = "ABSENT"
SIM_STATE_PUK_REQUIRED = "PUK_REQUIRED"
SIM_STATE_PIN_REQUIRED = "PIN_REQUIRED"
SIM_STATE_NETWORK_LOCKED = "NETWORK_LOCKED"
SIM_STATE_NOT_READY = "NOT_READY"
SIM_STATE_PERM_DISABLED = "PERM_DISABLED"
SIM_STATE_CARD_IO_ERROR = "CARD_IO_ERROR"

# Constant for Data Connection State
DATA_STATE_CONNECTED = "CONNECTED"
DATA_STATE_DISCONNECTED = "DISCONNECTED"
DATA_STATE_CONNECTING = "CONNECTING"
DATA_STATE_SUSPENDED = "SUSPENDED"
DATA_STATE_UNKNOWN = "UNKNOWN"

# Constant for Telephony Manager Call State
TELEPHONY_STATE_RINGING = "RINGING"
TELEPHONY_STATE_IDLE = "IDLE"
TELEPHONY_STATE_OFFHOOK = "OFFHOOK"
TELEPHONY_STATE_UNKNOWN = "UNKNOWN"

# Constant for TTY Mode
TTY_MODE_FULL = "FULL"
TTY_MODE_HCO = "HCO"
TTY_MODE_OFF = "OFF"
TTY_MODE_VCO = "VCO"

# Constant for Service State
SERVICE_STATE_EMERGENCY_ONLY = "EMERGENCY_ONLY"
SERVICE_STATE_IN_SERVICE = "IN_SERVICE"
SERVICE_STATE_OUT_OF_SERVICE = "OUT_OF_SERVICE"
SERVICE_STATE_POWER_OFF = "POWER_OFF"
SERVICE_STATE_UNKNOWN = "UNKNOWN"

# Constant for VoLTE Hand-over Service State
VOLTE_SERVICE_STATE_HANDOVER_STARTED = "STARTED"
VOLTE_SERVICE_STATE_HANDOVER_COMPLETED = "COMPLETED"
VOLTE_SERVICE_STATE_HANDOVER_FAILED = "FAILED"
VOLTE_SERVICE_STATE_HANDOVER_CANCELED = "CANCELED"
VOLTE_SERVICE_STATE_HANDOVER_UNKNOWN = "UNKNOWN"

# Constant for precise call state state listen level
PRECISE_CALL_STATE_LISTEN_LEVEL_FOREGROUND = "FOREGROUND"
PRECISE_CALL_STATE_LISTEN_LEVEL_RINGING = "RINGING"
PRECISE_CALL_STATE_LISTEN_LEVEL_BACKGROUND = "BACKGROUND"

# Constants used to register or de-register for video call callback events
EVENT_VIDEO_SESSION_MODIFY_REQUEST_RECEIVED = "EVENT_VIDEO_SESSION_MODIFY_REQUEST_RECEIVED"
EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED = "EVENT_VIDEO_SESSION_MODIFY_RESPONSE_RECEIVED"
EVENT_VIDEO_SESSION_EVENT = "EVENT_VIDEO_SESSION_EVENT"
EVENT_VIDEO_PEER_DIMENSIONS_CHANGED = "EVENT_VIDEO_PEER_DIMENSIONS_CHANGED"
EVENT_VIDEO_QUALITY_CHANGED = "EVENT_VIDEO_QUALITY_CHANGED"
EVENT_VIDEO_DATA_USAGE_CHANGED = "EVENT_VIDEO_DATA_USAGE_CHANGED"
EVENT_VIDEO_CAMERA_CAPABILITIES_CHANGED = "EVENT_VIDEO_CAMERA_CAPABILITIES_CHANGED"
EVENT_VIDEO_INVALID = "EVENT_VIDEO_INVALID"

# Constant for Video Call Session Event Name
SESSION_EVENT_RX_PAUSE = "SESSION_EVENT_RX_PAUSE"
SESSION_EVENT_RX_RESUME = "SESSION_EVENT_RX_RESUME"
SESSION_EVENT_TX_START = "SESSION_EVENT_TX_START"
SESSION_EVENT_TX_STOP = "SESSION_EVENT_TX_STOP"
SESSION_EVENT_CAMERA_FAILURE = "SESSION_EVENT_CAMERA_FAILURE"
SESSION_EVENT_CAMERA_READY = "SESSION_EVENT_CAMERA_READY"
SESSION_EVENT_UNKNOWN = "SESSION_EVENT_UNKNOWN"

NETWORK_MODE_WCDMA_PREF = "NETWORK_MODE_WCDMA_PREF"
NETWORK_MODE_GSM_ONLY = "NETWORK_MODE_GSM_ONLY"
NETWORK_MODE_WCDMA_ONLY = "NETWORK_MODE_WCDMA_ONLY"
NETWORK_MODE_GSM_UMTS = "NETWORK_MODE_GSM_UMTS"
NETWORK_MODE_CDMA = "NETWORK_MODE_CDMA"
NETWORK_MODE_CDMA_NO_EVDO = "NETWORK_MODE_CDMA_NO_EVDO"
NETWORK_MODE_EVDO_NO_CDMA = "NETWORK_MODE_EVDO_NO_CDMA"
NETWORK_MODE_GLOBAL = "NETWORK_MODE_GLOBAL"
NETWORK_MODE_LTE_CDMA_EVDO = "NETWORK_MODE_LTE_CDMA_EVDO"
NETWORK_MODE_LTE_GSM_WCDMA = "NETWORK_MODE_LTE_GSM_WCDMA"
NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA = "NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA"
NETWORK_MODE_LTE_ONLY = "NETWORK_MODE_LTE_ONLY"
NETWORK_MODE_LTE_WCDMA = "NETWORK_MODE_LTE_WCDMA"
NETWORK_MODE_TDSCDMA_ONLY = "NETWORK_MODE_TDSCDMA_ONLY"
NETWORK_MODE_TDSCDMA_WCDMA = "NETWORK_MODE_TDSCDMA_WCDMA"
NETWORK_MODE_LTE_TDSCDMA = "NETWORK_MODE_LTE_TDSCDMA"
NETWORK_MODE_TDSCDMA_GSM = "NETWORK_MODE_TDSCDMA_GSM"
NETWORK_MODE_LTE_TDSCDMA_GSM = "NETWORK_MODE_LTE_TDSCDMA_GSM"
NETWORK_MODE_TDSCDMA_GSM_WCDMA = "NETWORK_MODE_TDSCDMA_GSM_WCDMA"
NETWORK_MODE_LTE_TDSCDMA_WCDMA = "NETWORK_MODE_LTE_TDSCDMA_WCDMA"
NETWORK_MODE_LTE_TDSCDMA_GSM_WCDMA = "NETWORK_MODE_LTE_TDSCDMA_GSM_WCDMA"
NETWORK_MODE_TDSCDMA_CDMA_EVDO_WCDMA = "NETWORK_MODE_TDSCDMA_CDMA_EVDO_WCDMA"
NETWORK_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA = "NETWORK_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA"

# Constant for Messaging Event Name
EventSmsDeliverSuccess = "SmsDeliverSuccess"
EventSmsDeliverFailure = "SmsDeliverFailure"
EventSmsSentSuccess = "SmsSentSuccess"
EventSmsSentFailure = "SmsSentFailure"
EventSmsReceived = "SmsReceived"
EventMmsSentSuccess = "MmsSentSuccess"
EventMmsSentFailure = "MmsSentFailure"
EventMmsDownloaded = "MmsDownloaded"
EventWapPushReceived = "WapPushReceived"
EventDataSmsReceived = "DataSmsReceived"
EventCmasReceived = "CmasReceived"
EventEtwsReceived = "EtwsReceived"

# Constant for Telecom Call Event Name
EventTelecomCallStateChanged = "TelecomCallStateChanged"
EventTelecomCallParentChanged = "TelecomCallParentChanged"
EventTelecomCallChildrenChanged = "TelecomCallChildrenChanged"
EventTelecomCallDetailsChanged = "TelecomCallDetailsChanged"
EventTelecomCallCannedTextResponsesLoaded = "TelecomCallCannedTextResponsesLoaded"
EventTelecomCallPostDialWait = "TelecomCallPostDialWait"
EventTelecomCallVideoCallChanged = "TelecomCallVideoCallChanged"
EventTelecomCallDestroyed = "TelecomCallDestroyed"
EventTelecomCallConferenceableCallsChanged = "TelecomCallConferenceableCallsChanged"

# Constant for Video Call Event Name
EventTelecomVideoCallSessionModifyRequestReceived = "TelecomVideoCallSessionModifyRequestReceived"
EventTelecomVideoCallSessionModifyResponseReceived = "TelecomVideoCallSessionModifyResponseReceived"
EventTelecomVideoCallSessionEvent = "TelecomVideoCallSessionEvent"
EventTelecomVideoCallPeerDimensionsChanged = "TelecomVideoCallPeerDimensionsChanged"
EventTelecomVideoCallVideoQualityChanged = "TelecomVideoCallVideoQualityChanged"
EventTelecomVideoCallDataUsageChanged = "TelecomVideoCallDataUsageChanged"
EventTelecomVideoCallCameraCapabilities = "TelecomVideoCallCameraCapabilities"

# Constant for Other Event Name
EventCallStateChanged = "CallStateChanged"
EventPreciseStateChanged = "PreciseStateChanged"
EventDataConnectionRealTimeInfoChanged = "DataConnectionRealTimeInfoChanged"
EventDataConnectionStateChanged = "DataConnectionStateChanged"
EventServiceStateChanged = "ServiceStateChanged"
EventSignalStrengthChanged = "SignalStrengthChanged"
EventVolteServiceStateChanged = "VolteServiceStateChanged"
EventMessageWaitingIndicatorChanged = "MessageWaitingIndicatorChanged"
EventConnectivityChanged = "ConnectivityChanged"

# Constant for Packet Keep Alive Call Back
EventPacketKeepaliveCallback = "PacketKeepaliveCallback"
PacketKeepaliveCallbackStarted = "Started"
PacketKeepaliveCallbackStopped = "Stopped"
PacketKeepaliveCallbackError = "Error"
PacketKeepaliveCallbackInvalid = "Invalid"

# Constant for Network Call Back
EventNetworkCallback = "NetworkCallback"
NetworkCallbackPreCheck = "PreCheck"
NetworkCallbackAvailable = "Available"
NetworkCallbackLosing = "Losing"
NetworkCallbackLost = "Lost"
NetworkCallbackUnavailable = "Unavailable"
NetworkCallbackCapabilitiesChanged = "CapabilitiesChanged"
NetworkCallbackSuspended = "Suspended"
NetworkCallbackResumed = "Resumed"
NetworkCallbackLinkPropertiesChanged = "LinkPropertiesChanged"
NetworkCallbackInvalid = "Invalid"


class SignalStrengthContainer:
    SIGNAL_STRENGTH_GSM = "gsmSignalStrength"
    SIGNAL_STRENGTH_GSM_DBM = "gsmDbm"
    SIGNAL_STRENGTH_GSM_LEVEL = "gsmLevel"
    SIGNAL_STRENGTH_GSM_ASU_LEVEL = "gsmAsuLevel"
    SIGNAL_STRENGTH_GSM_BIT_ERROR_RATE = "gsmBitErrorRate"
    SIGNAL_STRENGTH_CDMA_DBM = "cdmaDbm"
    SIGNAL_STRENGTH_CDMA_LEVEL = "cdmaLevel"
    SIGNAL_STRENGTH_CDMA_ASU_LEVEL = "cdmaAsuLevel"
    SIGNAL_STRENGTH_CDMA_ECIO = "cdmaEcio"
    SIGNAL_STRENGTH_EVDO_DBM = "evdoDbm"
    SIGNAL_STRENGTH_EVDO_ECIO = "evdoEcio"
    SIGNAL_STRENGTH_LTE = "lteSignalStrength"
    SIGNAL_STRENGTH_LTE_DBM = "lteDbm"
    SIGNAL_STRENGTH_LTE_LEVEL = "lteLevel"
    SIGNAL_STRENGTH_LTE_ASU_LEVEL = "lteAsuLevel"
    SIGNAL_STRENGTH_DBM = "dbm"
    SIGNAL_STRENGTH_LEVEL = "level"
    SIGNAL_STRENGTH_ASU_LEVEL = "asuLevel"


class MessageWaitingIndicatorContainer:
    IS_MESSAGE_WAITING = "isMessageWaiting"


class CallStateContainer:
    INCOMING_NUMBER = "incomingNumber"
    SUBSCRIPTION_ID = "subscriptionId"
    CALL_STATE = "callState"


class PreciseCallStateContainer:
    TYPE = "type"
    CAUSE = "cause"
    SUBSCRIPTION_ID = "subscriptionId"
    PRECISE_CALL_STATE = "preciseCallState"


class DataConnectionRealTimeInfoContainer:
    TYPE = "type"
    TIME = "time"
    SUBSCRIPTION_ID = "subscriptionId"
    DATA_CONNECTION_POWER_STATE = "dataConnectionPowerState"


class DataConnectionStateContainer:
    TYPE = "type"
    DATA_NETWORK_TYPE = "dataNetworkType"
    STATE_CODE = "stateCode"
    SUBSCRIPTION_ID = "subscriptionId"
    DATA_CONNECTION_STATE = "dataConnectionState"


class ServiceStateContainer:
    VOICE_REG_STATE = "voiceRegState"
    VOICE_NETWORK_TYPE = "voiceNetworkType"
    DATA_REG_STATE = "dataRegState"
    DATA_NETWORK_TYPE = "dataNetworkType"
    OPERATOR_NAME = "operatorName"
    OPERATOR_ID = "operatorId"
    IS_MANUAL_NW_SELECTION = "isManualNwSelection"
    ROAMING = "roaming"
    IS_EMERGENCY_ONLY = "isEmergencyOnly"
    NETWORK_ID = "networkId"
    SYSTEM_ID = "systemId"
    SUBSCRIPTION_ID = "subscriptionId"
    SERVICE_STATE = "serviceState"


class PacketKeepaliveContainer:
    ID = "id"
    PACKET_KEEPALIVE_EVENT = "packetKeepaliveEvent"


class NetworkCallbackContainer:
    ID = "id"
    NETWORK_CALLBACK_EVENT = "networkCallbackEvent"
    MAX_MS_TO_LIVE = "maxMsToLive"
    RSSI = "rssi"


"""
End shared constant define for both Python and Java
"""

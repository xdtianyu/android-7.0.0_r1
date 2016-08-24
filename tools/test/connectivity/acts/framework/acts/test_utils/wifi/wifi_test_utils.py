#!/usr/bin/env python3.4
#
#   Copyright 2016 Google, Inc.
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
import pprint

from enum import IntEnum
from queue import Empty

from acts import asserts
from acts import signals
from acts.logger import LoggerProxy
from acts.utils import exe_cmd
from acts.utils import require_sl4a
from acts.utils import sync_device_time
from acts.utils import trim_model_name

log = LoggerProxy()

# Number of seconds to wait for events that are supposed to happen quickly.
# Like onSuccess for start background scan and confirmation on wifi state
# change.
SHORT_TIMEOUT = 30

# The currently supported devices that existed before release
#TODO: (navtejsingh) Need to clean up the below lists going forward
K_DEVICES = ["hammerhead", "razor", "razorg"]
L_DEVICES = ["shamu", "ryu"]
L_TAP_DEVICES = ["volantis", "volantisg"]
M_DEVICES = ["angler"]

# Speed of light in m/s.
SPEED_OF_LIGHT = 299792458

DEFAULT_PING_ADDR = "http://www.google.com/robots.txt"

class WifiEnums():

    SSID_KEY = "SSID"
    BSSID_KEY = "BSSID"
    PWD_KEY = "password"
    frequency_key = "frequency"
    APBAND_KEY = "apBand"

    WIFI_CONFIG_APBAND_2G = 0
    WIFI_CONFIG_APBAND_5G = 1

    WIFI_WPS_INFO_PBC     = 0;
    WIFI_WPS_INFO_DISPLAY = 1;
    WIFI_WPS_INFO_KEYPAD  = 2;
    WIFI_WPS_INFO_LABEL   = 3;
    WIFI_WPS_INFO_INVALID = 4;

    class CountryCode():
        CHINA = "CN"
        JAPAN = "JP"
        UK = "GB"
        US = "US"
        UNKNOWN = "UNKNOWN"

    # Start of Macros for EAP
    # EAP types
    class Eap(IntEnum):
        NONE = -1
        PEAP = 0
        TLS  = 1
        TTLS = 2
        PWD  = 3
        SIM  = 4
        AKA  = 5
        AKA_PRIME = 6
        UNAUTH_TLS = 7

    # EAP Phase2 types
    class EapPhase2(IntEnum):
        NONE        = 0
        PAP         = 1
        MSCHAP      = 2
        MSCHAPV2    = 3
        GTC         = 4

    class Enterprise:
    # Enterprise Config Macros
        EMPTY_VALUE      = "NULL"
        EAP              = "eap"
        PHASE2           = "phase2"
        IDENTITY         = "identity"
        ANON_IDENTITY    = "anonymous_identity"
        PASSWORD         = "password"
        SUBJECT_MATCH    = "subject_match"
        ALTSUBJECT_MATCH = "altsubject_match"
        DOM_SUFFIX_MATCH = "domain_suffix_match"
        CLIENT_CERT      = "client_cert"
        CA_CERT          = "ca_cert"
        ENGINE           = "engine"
        ENGINE_ID        = "engine_id"
        PRIVATE_KEY_ID   = "key_id"
        REALM            = "realm"
        PLMN             = "plmn"
        FQDN             = "FQDN"
        FRIENDLY_NAME    = "providerFriendlyName"
        ROAMING_IDS      = "roamingConsortiumIds"
    # End of Macros for EAP

    # Macros for wifi p2p.
    WIFI_P2P_SERVICE_TYPE_ALL = 0
    WIFI_P2P_SERVICE_TYPE_BONJOUR = 1
    WIFI_P2P_SERVICE_TYPE_UPNP = 2
    WIFI_P2P_SERVICE_TYPE_VENDOR_SPECIFIC = 255

    class ScanResult:
        CHANNEL_WIDTH_20MHZ = 0
        CHANNEL_WIDTH_40MHZ = 1
        CHANNEL_WIDTH_80MHZ = 2
        CHANNEL_WIDTH_160MHZ = 3
        CHANNEL_WIDTH_80MHZ_PLUS_MHZ = 4

    # Macros for wifi rtt.
    class RttType(IntEnum):
        TYPE_ONE_SIDED      = 1
        TYPE_TWO_SIDED      = 2

    class RttPeerType(IntEnum):
        PEER_TYPE_AP        = 1
        PEER_TYPE_STA       = 2 # Requires NAN.
        PEER_P2P_GO         = 3
        PEER_P2P_CLIENT     = 4
        PEER_NAN            = 5

    class RttPreamble(IntEnum):
        PREAMBLE_LEGACY  = 0x01
        PREAMBLE_HT      = 0x02
        PREAMBLE_VHT     = 0x04

    class RttBW(IntEnum):
        BW_5_SUPPORT   = 0x01
        BW_10_SUPPORT  = 0x02
        BW_20_SUPPORT  = 0x04
        BW_40_SUPPORT  = 0x08
        BW_80_SUPPORT  = 0x10
        BW_160_SUPPORT = 0x20

    class Rtt(IntEnum):
        STATUS_SUCCESS                  = 0
        STATUS_FAILURE                  = 1
        STATUS_FAIL_NO_RSP              = 2
        STATUS_FAIL_REJECTED            = 3
        STATUS_FAIL_NOT_SCHEDULED_YET   = 4
        STATUS_FAIL_TM_TIMEOUT          = 5
        STATUS_FAIL_AP_ON_DIFF_CHANNEL  = 6
        STATUS_FAIL_NO_CAPABILITY       = 7
        STATUS_ABORTED                  = 8
        STATUS_FAIL_INVALID_TS          = 9
        STATUS_FAIL_PROTOCOL            = 10
        STATUS_FAIL_SCHEDULE            = 11
        STATUS_FAIL_BUSY_TRY_LATER      = 12
        STATUS_INVALID_REQ              = 13
        STATUS_NO_WIFI                  = 14
        STATUS_FAIL_FTM_PARAM_OVERRIDE  = 15

        REASON_UNSPECIFIED              = -1
        REASON_NOT_AVAILABLE            = -2
        REASON_INVALID_LISTENER         = -3
        REASON_INVALID_REQUEST          = -4

    class RttParam:
        device_type = "deviceType"
        request_type = "requestType"
        BSSID = "bssid"
        channel_width = "channelWidth"
        frequency = "frequency"
        center_freq0 = "centerFreq0"
        center_freq1 = "centerFreq1"
        number_burst = "numberBurst"
        interval = "interval"
        num_samples_per_burst = "numSamplesPerBurst"
        num_retries_per_measurement_frame = "numRetriesPerMeasurementFrame"
        num_retries_per_FTMR = "numRetriesPerFTMR"
        lci_request = "LCIRequest"
        lcr_request = "LCRRequest"
        burst_timeout = "burstTimeout"
        preamble = "preamble"
        bandwidth = "bandwidth"
        margin = "margin"

    RTT_MARGIN_OF_ERROR = {
        RttBW.BW_80_SUPPORT: 2,
        RttBW.BW_40_SUPPORT: 5,
        RttBW.BW_20_SUPPORT: 5
    }

    # Macros as specified in the WifiScanner code.
    WIFI_BAND_UNSPECIFIED = 0      # not specified
    WIFI_BAND_24_GHZ = 1           # 2.4 GHz band
    WIFI_BAND_5_GHZ = 2            # 5 GHz band without DFS channels
    WIFI_BAND_5_GHZ_DFS_ONLY  = 4  # 5 GHz band with DFS channels
    WIFI_BAND_5_GHZ_WITH_DFS  = 6  # 5 GHz band with DFS channels
    WIFI_BAND_BOTH = 3             # both bands without DFS channels
    WIFI_BAND_BOTH_WITH_DFS = 7    # both bands with DFS channels

    REPORT_EVENT_AFTER_BUFFER_FULL = 0
    REPORT_EVENT_AFTER_EACH_SCAN = 1
    REPORT_EVENT_FULL_SCAN_RESULT = 2

    # US Wifi frequencies
    ALL_2G_FREQUENCIES = [2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452,
                          2457, 2462]
    DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520, 5540, 5560, 5580,
                          5600, 5620, 5640, 5660, 5680, 5700, 5720]
    NONE_DFS_5G_FREQUENCIES = [5180, 5200, 5220, 5240, 5745, 5765, 5785, 5805,
                               5825]
    ALL_5G_FREQUENCIES = DFS_5G_FREQUENCIES + NONE_DFS_5G_FREQUENCIES

    band_to_frequencies = {
      WIFI_BAND_24_GHZ: ALL_2G_FREQUENCIES,
      WIFI_BAND_5_GHZ: NONE_DFS_5G_FREQUENCIES,
      WIFI_BAND_5_GHZ_DFS_ONLY: DFS_5G_FREQUENCIES,
      WIFI_BAND_5_GHZ_WITH_DFS: ALL_5G_FREQUENCIES,
      WIFI_BAND_BOTH: ALL_2G_FREQUENCIES + NONE_DFS_5G_FREQUENCIES,
      WIFI_BAND_BOTH_WITH_DFS: ALL_5G_FREQUENCIES + ALL_2G_FREQUENCIES
    }

    # All Wifi frequencies to channels lookup.
    freq_to_channel = {
        2412: 1,
        2417: 2,
        2422: 3,
        2427: 4,
        2432: 5,
        2437: 6,
        2442: 7,
        2447: 8,
        2452: 9,
        2457: 10,
        2462: 11,
        2467: 12,
        2472: 13,
        2484: 14,
        4915: 183,
        4920: 184,
        4925: 185,
        4935: 187,
        4940: 188,
        4945: 189,
        4960: 192,
        4980: 196,
        5035: 7,
        5040: 8,
        5045: 9,
        5055: 11,
        5060: 12,
        5080: 16,
        5170: 34,
        5180: 36,
        5190: 38,
        5200: 40,
        5210: 42,
        5220: 44,
        5230: 46,
        5240: 48,
        5260: 52,
        5280: 56,
        5300: 60,
        5320: 64,
        5500: 100,
        5520: 104,
        5540: 108,
        5560: 112,
        5580: 116,
        5600: 120,
        5620: 124,
        5640: 128,
        5660: 132,
        5680: 136,
        5700: 140,
        5745: 149,
        5765: 153,
        5785: 157,
        5805: 161,
        5825: 165,
    }

    # All Wifi channels to frequencies lookup.
    channel_2G_to_freq = {
        1: 2412,
        2: 2417,
        3: 2422,
        4: 2427,
        5: 2432,
        6: 2437,
        7: 2442,
        8: 2447,
        9: 2452,
        10: 2457,
        11: 2462,
        12: 2467,
        13: 2472,
        14: 2484
    }

    channel_5G_to_freq = {
        183: 4915,
        184: 4920,
        185: 4925,
        187: 4935,
        188: 4940,
        189: 4945,
        192: 4960,
        196: 4980,
        7: 5035,
        8: 5040,
        9: 5045,
        11: 5055,
        12: 5060,
        16: 5080,
        34: 5170,
        36: 5180,
        38: 5190,
        40: 5200,
        42: 5210,
        44: 5220,
        46: 5230,
        48: 5240,
        52: 5260,
        56: 5280,
        60: 5300,
        64: 5320,
        100: 5500,
        104: 5520,
        108: 5540,
        112: 5560,
        116: 5580,
        120: 5600,
        124: 5620,
        128: 5640,
        132: 5660,
        136: 5680,
        140: 5700,
        149: 5745,
        153: 5765,
        157: 5785,
        161: 5805,
        165: 5825
    }

class WifiEventNames:
    WIFI_CONNECTED = "WifiNetworkConnected"
    SUPPLICANT_CON_CHANGED = "SupplicantConnectionChanged"
    WIFI_FORGET_NW_SUCCESS = "WifiManagerForgetNetworkOnSuccess"

class WifiTestUtilsError(Exception):
    pass

class WifiChannelBase:
    ALL_2G_FREQUENCIES = []
    DFS_5G_FREQUENCIES = []
    NONE_DFS_5G_FREQUENCIES = []
    ALL_5G_FREQUENCIES = DFS_5G_FREQUENCIES + NONE_DFS_5G_FREQUENCIES
    MIX_CHANNEL_SCAN = []

    def band_to_freq(self, band):
        _band_to_frequencies = {
            WifiEnums.WIFI_BAND_24_GHZ: self.ALL_2G_FREQUENCIES,
            WifiEnums.WIFI_BAND_5_GHZ: self.NONE_DFS_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_5_GHZ_DFS_ONLY: self.DFS_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_5_GHZ_WITH_DFS: self.ALL_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_BOTH: self.ALL_2G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES,
            WifiEnums.WIFI_BAND_BOTH_WITH_DFS: self.ALL_5G_FREQUENCIES + self.ALL_2G_FREQUENCIES
        }
        return _band_to_frequencies[band]

class WifiChannelUS(WifiChannelBase):
    # US Wifi frequencies
    ALL_2G_FREQUENCIES = [2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452,
                          2457, 2462]
    NONE_DFS_5G_FREQUENCIES = [5180, 5200, 5220, 5240, 5745, 5765, 5785, 5805,
                               5825]
    MIX_CHANNEL_SCAN = [2412, 2437, 2462, 5180, 5200, 5280, 5260, 5300,5500, 5320,
                        5520, 5560, 5700, 5745, 5805]

    def __init__(self, model=None):
        if model and trim_model_name(model) in K_DEVICES:
            self.DFS_5G_FREQUENCIES = []
            self.ALL_5G_FREQUENCIES = self.NONE_DFS_5G_FREQUENCIES
            self.MIX_CHANNEL_SCAN = [2412, 2437, 2462, 5180, 5200, 5240, 5745, 5765]
        elif model and trim_model_name(model) in L_DEVICES:
            self.DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520,
                                       5540, 5560, 5580, 5660, 5680, 5700]
            self.ALL_5G_FREQUENCIES = self.DFS_5G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES
        elif model and trim_model_name(model) in L_TAP_DEVICES:
            self.DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520,
                                       5540, 5560, 5580, 5660, 5680, 5700, 5720]
            self.ALL_5G_FREQUENCIES = self.DFS_5G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES
        elif model and trim_model_name(model) in M_DEVICES:
            self.DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520, 5540, 5560,5580,
                                       5600, 5620, 5640, 5660, 5680, 5700]
            self.ALL_5G_FREQUENCIES = self.DFS_5G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES
        else:
            self.DFS_5G_FREQUENCIES = [5260, 5280, 5300, 5320, 5500, 5520, 5540, 5560,5580,
                                       5600, 5620, 5640, 5660, 5680, 5700, 5720]
            self.ALL_5G_FREQUENCIES = self.DFS_5G_FREQUENCIES + self.NONE_DFS_5G_FREQUENCIES

def match_networks(target_params, networks):
    """Finds the WiFi networks that match a given set of parameters in a list
    of WiFi networks.

    To be considered a match, a network needs to have all the target parameters
    and the values of those parameters need to equal to those of the target
    parameters.

    Args:
        target_params: The target parameters to match networks against.
        networks: A list of dict objects representing WiFi networks.

    Returns:
        The networks that match the target parameters.
    """
    results = []
    for n in networks:
        for k, v in target_params.items():
            if k not in n:
                continue
            if n[k] != v:
                continue
            results.append(n)
    return results

def wifi_toggle_state(ad, new_state=None):
    """Toggles the state of wifi.

    Args:
        ad: An AndroidDevice object.
        new_state: Wifi state to set to. If None, opposite of the current state.

    Returns:
        True if the toggle was successful, False otherwise.
    """
    # Check if the new_state is already achieved, so we don't wait for the
    # state change event by mistake.
    if new_state == ad.droid.wifiCheckState():
        return True
    ad.droid.wifiStartTrackingStateChange()
    log.info("Setting wifi state to {}".format(new_state))
    ad.droid.wifiToggleState(new_state)
    try:
        event = ad.ed.pop_event(WifiEventNames.SUPPLICANT_CON_CHANGED, SHORT_TIMEOUT)
        return event['data']['Connected'] == new_state
    except Empty:
      # Supplicant connection event is not always reliable. We double check here
      # and call it a success as long as the new state equals the expected state.
        return new_state == ad.droid.wifiCheckState()
    finally:
        ad.droid.wifiStopTrackingStateChange()

def reset_wifi(ad):
    """Clears all saved networks on a device.

    Args:
        ad: An AndroidDevice object.

    Raises:
        WifiTestUtilsError is raised if forget network operation failed.
    """
    ad.droid.wifiToggleState(True)
    networks = ad.droid.wifiGetConfiguredNetworks()
    if not networks:
        return
    for n in networks:
        ad.droid.wifiForgetNetwork(n['networkId'])
        try:
            event = ad.ed.pop_event(WifiEventNames.WIFI_FORGET_NW_SUCCESS,
              SHORT_TIMEOUT)
        except Empty:
            raise WifiTestUtilsError("Failed to remove network {}.".format(n))

def wifi_forget_network(ad, net_ssid):
    """Remove configured Wifi network on an android device.

    Args:
        ad: android_device object for forget network.
        net_ssid: ssid of network to be forget

    Raises:
        WifiTestUtilsError is raised if forget network operation failed.
    """
    droid, ed = ad.droid, ad.ed
    droid.wifiToggleState(True)
    networks = droid.wifiGetConfiguredNetworks()
    if not networks:
        return
    for n in networks:
        if net_ssid in n[WifiEnums.SSID_KEY]:
            droid.wifiForgetNetwork(n['networkId'])
            try:
                event = ed.pop_event(WifiEventNames.WIFI_FORGET_NW_SUCCESS,
                        SHORT_TIMEOUT)
            except Empty:
                raise WifiTestUtilsError("Failed to remove network %s." % n)

def wifi_test_device_init(ad):
    """Initializes an android device for wifi testing.

    0. Make sure SL4A connection is established on the android device.
    1. Disable location service's WiFi scan.
    2. Turn WiFi on.
    3. Clear all saved networks.
    4. Set country code to US.
    5. Enable WiFi verbose logging.
    6. Sync device time with computer time.
    7. Turn off cellular data.
    """
    require_sl4a((ad,))
    ad.droid.wifiScannerToggleAlwaysAvailable(False)
    msg = "Failed to turn off location service's scan."
    assert not ad.droid.wifiScannerIsAlwaysAvailable(), msg
    msg = "Failed to turn WiFi on %s" % ad.serial
    assert wifi_toggle_state(ad, True), msg
    reset_wifi(ad)
    msg = "Failed to clear configured networks."
    assert not ad.droid.wifiGetConfiguredNetworks(), msg
    ad.droid.wifiEnableVerboseLogging(1)
    msg = "Failed to enable WiFi verbose logging."
    assert ad.droid.wifiGetVerboseLoggingLevel() == 1, msg
    ad.droid.wifiScannerToggleAlwaysAvailable(False)
    # We don't verify the following settings since they are not critical.
    sync_device_time(ad)
    ad.droid.telephonyToggleDataConnection(False)
    # TODO(angli): need to verify the country code was actually set. No generic
    # way to check right now.
    ad.adb.shell("halutil -country %s" % WifiEnums.CountryCode.US)

def sort_wifi_scan_results(results, key="level"):
    """Sort wifi scan results by key.

    Args:
        results: A list of results to sort.
        key: Name of the field to sort the results by.

    Returns:
        A list of results in sorted order.
    """
    return sorted(results, lambda d: (key not in d, d[key]))

def start_wifi_connection_scan(ad):
    """Starts a wifi connection scan and wait for results to become available.

    Args:
        ad: An AndroidDevice object.
    """
    ad.droid.wifiStartScan()
    ad.ed.pop_event("WifiManagerScanResultsAvailable", 60)

def start_wifi_background_scan(ad, scan_setting):
    """Starts wifi background scan.

    Args:
        ad: android_device object to initiate connection on.
        scan_setting: A dict representing the settings of the scan.

    Returns:
        If scan was started successfully, event data of success event is returned.
    """
    droid, ed = ad.droids[0], ad.eds[0]
    idx = droid.wifiScannerStartBackgroundScan(scan_setting)
    event = ed.pop_event("WifiScannerScan{}onSuccess".format(idx),
                         SHORT_TIMEOUT)
    return event['data']

def start_wifi_tethering(ad, ssid, password, band=None):
    """Starts wifi tethering on an android_device.

    Args:
        ad: android_device to start wifi tethering on.
        ssid: The SSID the soft AP should broadcast.
        password: The password the soft AP should use.
        band: The band the soft AP should be set on. It should be either
            WifiEnums.WIFI_CONFIG_APBAND_2G or WifiEnums.WIFI_CONFIG_APBAND_5G.

    Returns:
        True if soft AP was started successfully, False otherwise.
    """
    droid, ed = ad.droid, ad.ed
    droid.wifiStartTrackingStateChange()
    config = {
        WifiEnums.SSID_KEY: ssid
    }
    if password:
        config[WifiEnums.PWD_KEY] = password
    if band:
        config[WifiEnums.APBAND_KEY] = band
    if not droid.wifiSetApEnabled(True, config):
        return False
    ed.pop_event("WifiManagerApEnabled", 30)
    ed.wait_for_event("TetherStateChanged",
        lambda x : x["data"]["ACTIVE_TETHER"], 30)
    droid.wifiStopTrackingStateChange()
    return True

def stop_wifi_tethering(ad):
    """Stops wifi tethering on an android_device.

    Args:
        ad: android_device to stop wifi tethering on.
    """
    droid, ed = ad.droid, ad.ed
    droid.wifiStartTrackingStateChange()
    droid.wifiSetApEnabled(False, None)
    ed.pop_event("WifiManagerApDisabled", 30)
    ed.wait_for_event("TetherStateChanged",
        lambda x : not x["data"]["ACTIVE_TETHER"], 30)
    droid.wifiStopTrackingStateChange()

def wifi_connect(ad, network):
    """Connect an Android device to a wifi network.

    Initiate connection to a wifi network, wait for the "connected" event, then
    confirm the connected ssid is the one requested.

    Args:
        ad: android_device object to initiate connection on.
        network: A dictionary representing the network to connect to. The
            dictionary must have the key "SSID".
    """
    assert WifiEnums.SSID_KEY in network, ("Key '%s' must be present in "
        "network definition.") % WifiEnums.SSID_KEY
    ad.droid.wifiStartTrackingStateChange()
    try:
        assert ad.droid.wifiConnect(network), "WiFi connect returned false."
        connect_result = ad.ed.pop_event(WifiEventNames.WIFI_CONNECTED)
        log.debug("Connection result: %s." % connect_result)
        expected_ssid = network[WifiEnums.SSID_KEY]
        actual_ssid = connect_result['data'][WifiEnums.SSID_KEY]
        assert actual_ssid == expected_ssid, ("Expected to connect to %s, "
            "connected to %s") % (expected_ssid, actual_ssid)
        log.info("Successfully connected to %s" % actual_ssid)
    finally:
        ad.droid.wifiStopTrackingStateChange()

def start_wifi_single_scan(ad, scan_setting):
    """Starts wifi single shot scan.

    Args:
        ad: android_device object to initiate connection on.
        scan_setting: A dict representing the settings of the scan.

    Returns:
        If scan was started successfully, event data of success event is returned.
    """
    droid, ed = ad.droid, ad.ed
    idx = droid.wifiScannerStartScan(scan_setting)
    event = ed.pop_event("WifiScannerScan{}onSuccess".format(idx),
                         SHORT_TIMEOUT)
    log.debug("event {}".format(event))
    return event['data']

def track_connection(ad, network_ssid, check_connection_count):
    """Track wifi connection to network changes for given number of counts

    Args:
        ad: android_device object for forget network.
        network_ssid: network ssid to which connection would be tracked
        check_connection_count: Integer for maximum number network connection
            check.
    Returns:

        True if connection to given network happen, else return False.
    """
    droid, ed = ad.droid, ad.ed
    droid.wifiStartTrackingStateChange()
    while check_connection_count > 0:
        connect_network = ed.pop_event("WifiNetworkConnected", 120)
        log.info("connect_network {}".format(connect_network))
        if (WifiEnums.SSID_KEY in connect_network['data']
            and connect_network['data'][WifiEnums.SSID_KEY] == network_ssid):
                return True
        check_connection_count -= 1
    droid.wifiStopTrackingStateChange()
    return False

def get_scan_time_and_channels(wifi_chs, scan_setting, stime_channel):
    """Calculate the scan time required based on the band or channels in scan
    setting

    Args:
        wifi_chs: Object of channels supported
        scan_setting: scan setting used for start scan
        stime_channel: scan time per channel

    Returns:
        scan_time: time required for completing a scan
        scan_channels: channel used for scanning
    """
    scan_time = 0
    scan_channels = []
    if "band" in scan_setting and "channels" not in scan_setting:
        scan_channels = wifi_chs.band_to_freq(scan_setting["band"])
    elif "channels" in scan_setting and "band" not in scan_setting:
        scan_channels = scan_setting["channels"]
    scan_time = len(scan_channels) * stime_channel
    for channel in scan_channels:
        if channel in WifiEnums.DFS_5G_FREQUENCIES:
            scan_time += 132 #passive scan time on DFS
    return scan_time, scan_channels

def start_wifi_track_bssid(ad, track_setting):
    """Start tracking Bssid for the given settings.

    Args:
      ad: android_device object.
      track_setting: Setting for which the bssid tracking should be started

    Returns:
      If tracking started successfully, event data of success event is returned.
    """
    droid, ed = ad.droid, ad.ed
    idx = droid.wifiScannerStartTrackingBssids(
        track_setting["bssidInfos"],
        track_setting["apLostThreshold"]
        )
    event = ed.pop_event("WifiScannerBssid{}onSuccess".format(idx),
                         SHORT_TIMEOUT)
    return event['data']

def convert_pem_key_to_pkcs8(in_file, out_file):
    """Converts the key file generated by us to the format required by
    Android using openssl.

    The input file must have the extension "pem". The output file must
    have the extension "der".

    Args:
        in_file: The original key file.
        out_file: The full path to the converted key file, including
        filename.
    """
    cmd = ("openssl pkcs8 -inform PEM -in {} -outform DER -out {} -nocrypt"
           " -topk8").format(in_file, out_file)
    exe_cmd(cmd)

def check_internet_connection(ad, ping_addr):
    """Validate internet connection by pinging the address provided.

    Args:
        ad: android_device object.
        ping_addr: address on internet for pinging.

    Returns:
        True, if address ping successful
    """
    droid, ed = ad.droid, ad.ed
    ping = droid.httpPing(ping_addr)
    log.info("Http ping result: {}".format(ping))
    return ping

#TODO(angli): This can only verify if an actual value is exactly the same.
# Would be nice to be able to verify an actual value is one of serveral.
def verify_wifi_connection_info(ad, expected_con):
    """Verifies that the information of the currently connected wifi network is
    as expected.

    Args:
        expected_con: A dict representing expected key-value pairs for wifi
            connection. e.g. {"SSID": "test_wifi"}
    """
    current_con = ad.droid.wifiGetConnectionInfo()
    case_insensitive = ["BSSID", "supplicant_state"]
    log.debug("Current connection: %s" % current_con)
    for k, expected_v in expected_con.items():
        # Do not verify authentication related fields.
        if k == "password":
            continue
        msg = "Field %s does not exist in wifi connection info %s." % (k,
            current_con)
        if k not in current_con:
            raise signals.TestFailure(msg)
        actual_v = current_con[k]
        if k in case_insensitive:
            actual_v = actual_v.lower()
            expected_v = expected_v.lower()
        msg = "Expected %s to be %s, actual %s is %s." % (k, expected_v, k,
            actual_v)
        if actual_v != expected_v:
            raise signals.TestFailure(msg)

def eap_connect(config, ad, validate_con=True, ping_addr=DEFAULT_PING_ADDR):
    """Connects to an enterprise network and verify connection.

    This logic expect the enterprise network to have Internet access.

    Args:
        config: A dict representing a wifi enterprise configuration.
        ad: The android_device to operate with.
        validate_con: If True, validate Internet connection after connecting to
            the network.

    Returns:
        True if the connection is successful and Internet access works.
    """
    droid, ed = ad.droid, ad.ed
    start_wifi_connection_scan(ad)
    expect_ssid = None
    if WifiEnums.SSID_KEY in config:
        expect_ssid = config[WifiEnums.SSID_KEY]
        log.info("Connecting to %s." % expect_ssid)
    else:
        log.info("Connecting.")
    log.debug(pprint.pformat(config, indent=4))
    ad.droid.wifiEnterpriseConnect(config)
    try:
        event = ed.pop_event("WifiManagerEnterpriseConnectOnSuccess", 30)
        log.info("Started connecting...")
        event = ed.pop_event(WifiEventNames.WIFI_CONNECTED, 60)
    except Empty:
        asserts.fail("Failed to connect to %s" % config)
    log.debug(event)
    if expect_ssid:
        actual_ssid = event["data"][WifiEnums.SSID_KEY]
        asserts.assert_equal(expect_ssid, actual_ssid, "SSID mismatch.")
    else:
        log.info("Connected to %s." % expect_ssid)
    if validate_con:
        log.info("Checking Internet access.")
        # Wait for data connection to stabilize.
        time.sleep(4)
        ping = ad.droid.httpPing(ping_addr)
        log.info("Http ping result: {}".format(ping))
        asserts.assert_true(ping, "No Internet access.")

def expand_enterprise_config_by_phase2(config):
    """Take an enterprise config and generate a list of configs, each with
    a different phase2 auth type.

    Args:
        config: A dict representing enterprise config.

    Returns
        A list of enterprise configs.
    """
    results = []
    phase2_types = WifiEnums.EapPhase2
    if config[WifiEnums.Enterprise.EAP] == WifiEnums.Eap.PEAP:
        # Skip unsupported phase2 types for PEAP.
        phase2_types = [WifiEnums.EapPhase2.GTC, WifiEnums.EapPhase2.MSCHAPV2]
    for phase2_type in phase2_types:
        # Skip a special case for passpoint TTLS.
        if (WifiEnums.Enterprise.FQDN in config and
            phase2_type == WifiEnums.EapPhase2.GTC):
            continue
        c = dict(config)
        c[WifiEnums.Enterprise.PHASE2] = phase2_type
        results.append(c)
    return results

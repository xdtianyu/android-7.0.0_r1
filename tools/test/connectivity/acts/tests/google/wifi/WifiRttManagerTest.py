#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import pprint
import queue

import acts.base_test
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils
from acts import asserts
from acts.controllers.android import SL4AAPIError

WifiEnums = wutils.WifiEnums

# Macros for RttParam keywords
RttParam = WifiEnums.RttParam
# Macros for RttManager
Rtt = WifiEnums.Rtt
RttBW = WifiEnums.RttBW
RttPreamble = WifiEnums.RttPreamble
RttPeerType = WifiEnums.RttPeerType
RttType = WifiEnums.RttType

ScanResult = WifiEnums.ScanResult
RTT_MARGIN_OF_ERROR = WifiEnums.RTT_MARGIN_OF_ERROR

class WifiRTTRangingError (Exception):
     """Error in WifiScanner Rtt."""

class WifiRttManagerTest(acts.base_test.BaseTestClass):
    """Tests for wifi's RttManager APIs."""
    tests = None
    MAX_RTT_AP = 10

    def __init__(self, controllers):
        acts.base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_support_check",
            "test_invalid_params",
            "test_capability_check",
            "test_rtt_ranging_single_AP_stress",
            "test_regular_scan_then_rtt_ranging_stress",
            "test_gscan_then_rtt_ranging_stress"
        )

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        required_params = (
            "support_models",
            "stress_num",
            "vht80_5g",
            "actual_distance"
        )
        self.unpack_userparams(required_params)
        asserts.assert_true(self.actual_distance >= 5,
            "Actual distance should be no shorter than 5 meters.")
        self.visible_networks = (
            self.vht80_5g,
        )
        self.default_rtt_params = {
            RttParam.request_type: RttType.TYPE_TWO_SIDED,
            RttParam.device_type: RttPeerType.PEER_TYPE_AP,
            RttParam.preamble: RttPreamble.PREAMBLE_HT,
            RttParam.bandwidth: RttBW.BW_80_SUPPORT
        }
        # Expected capability for devices that don't support RTT.
        rtt_cap_neg = {
            'lcrSupported': False,
            'bwSupported': 0,
            'twoSided11McRttSupported': False,
            'preambleSupported': 0,
            'oneSidedRttSupported': False,
            'lciSupported': False
        }
        rtt_cap_shamu = {
            'lcrSupported': False,
            'bwSupported': 0x1C,
            'twoSided11McRttSupported': True,
            'preambleSupported': 6,
            'oneSidedRttSupported': False,
            'lciSupported': False
        }
        rtt_cap_bullhead = {
            'lcrSupported': True,
            'bwSupported': 0x1C,
            'twoSided11McRttSupported': True,
            'preambleSupported': 7,
            'oneSidedRttSupported': True,
            'lciSupported': True
        }
        rtt_cap_angler = {
            'lcrSupported': True,
            'bwSupported': 0x1C,
            'twoSided11McRttSupported': True,
            'preambleSupported': 6,
            'oneSidedRttSupported': False,
            'lciSupported': True
        }
        self.rtt_cap_table = {
            "hammerhead": rtt_cap_neg,
            "shamu": rtt_cap_shamu,
            "volantis": rtt_cap_neg,
            "volantisg": rtt_cap_neg,
            "bullhead": rtt_cap_bullhead,
            "angler": rtt_cap_angler
        }

    """Helper Functions"""
    def invalid_params_logic(self, rtt_params):
        try:
            self.dut.droid.wifiRttStartRanging([rtt_params])
        except SL4AAPIError as e:
            e_str = str(e)
            asserts.assert_true("IllegalArgumentException" in e_str,
                "Missing IllegalArgumentException in %s." % e_str)
            msg = "Got expected exception with invalid param %s." % rtt_params
            self.log.info(msg)

    def get_rtt_results(self, rtt_params):
        """Starts RTT ranging and get results.

        Args:
            rtt_params: A list of dicts each representing an RttParam.

        Returns:
            Rtt ranging results.
        """
        self.log.debug("Start ranging with:\n%s" % pprint.pformat(rtt_params))
        idx = self.dut.droid.wifiRttStartRanging(rtt_params)
        event = None
        try:
            event = self.dut.ed.pop_events("WifiRttRanging%d" % idx, 30)
            if event[0]["name"].endswith("onSuccess"):
                results = event[0]["data"]["Results"]
                result_len = len(results)
                param_len = len(rtt_params)
                asserts.assert_true(result_len == param_len,
                    "Expected %d results, got %d." % (param_len, result_len))
                # Add acceptable margin of error to results, which will be used
                # during result processing.
                for i, r in enumerate(results):
                    bw_mode = rtt_params[i][RttParam.bandwidth]
                    r[RttParam.margin] = RTT_MARGIN_OF_ERROR[bw_mode]
            self.log.debug(pprint.pformat(event))
            return event
        except queue.Empty:
            self.log.error("Waiting for RTT event timed out.")
            return None

    def network_selector(self, network_info):
        """Decides if a network should be used for rtt ranging.

        There are a few conditions:
        1. This network supports 80211mc.
        2. This network's info matches certain conditions.

        This is added to better control which networks to range against instead
        of blindly use all 80211mc networks in air.

        Args:
            network_info: A dict representing a WiFi network.

        Returns:
            True if the input network should be used for ranging, False
            otherwise.
        """
        target_params = {
            "is80211McRTTResponder": True,
            WifiEnums.BSSID_KEY: self.vht80_5g[WifiEnums.BSSID_KEY],
        }
        for k, v in target_params.items():
            if k not in network_info:
                return False
            if type(network_info[k]) is str:
                network_info[k] = network_info[k].lower()
                v = v.lower()
            if network_info[k] != v:
                return False
        return True

    def regular_scan_for_rtt_networks(self):
        """Scans for 11mc-capable WiFi networks using regular wifi scan.

        Networks are selected based on self.network_selector.

        Returns:
            A list of networks that have RTTResponders.
        """
        wutils.start_wifi_connection_scan(self.dut)
        networks = self.dut.droid.wifiGetScanResults()
        rtt_networks = []
        for nw in networks:
            if self.network_selector(nw):
                rtt_networks.append(nw)
        return rtt_networks

    def gscan_for_rtt_networks(self):
        """Scans for 11mc-capable WiFi networks using wifi gscan.

        Networks are selected based on self.network_selector.

        Returns:
            A list of networks that have RTTResponders.
        """
        s = {
            "reportEvents" : WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT,
            "band": WifiEnums.WIFI_BAND_BOTH,
            "periodInMs": 10000,
            "numBssidsPerScan": 32
        }
        idx = wutils.start_wifi_single_scan(self.android_devices[0], s)["Index"]
        self.log.info("Scan index is %d" % idx)
        event_name = "WifiScannerScan%donFullResult" % idx
        def condition(event):
            nw = event["data"]["Results"][0]
            return self.network_selector(nw)
        rtt_networks = []
        try:
            for i in range(len(self.visible_networks)):
                event = self.dut.ed.wait_for_event(event_name, condition, 30)
                rtt_networks.append(event["data"]["Results"][0])
            self.log.info("Waiting for gscan to finish.")
            event_name = "WifiScannerScan%donResults" % idx
            event = self.dut.ed.pop_event(event_name, 300)
            total_network_cnt = len(event["data"]["Results"][0]["ScanResults"])
            self.log.info("Found %d networks in total." % total_network_cnt)
            self.log.debug(rtt_networks)
            return rtt_networks
        except queue.Empty:
            self.log.error("Timed out waiting for gscan result.")

    def process_rtt_events(self, events):
        """Processes rtt ranging events.

        Validates RTT event types.
        Validates RTT response status and measured RTT values.
        Enforces success rate.

        Args:
            events: A list of callback results from RTT ranging.
        """
        total = aborted = failure = invalid = out_of_range = 0
        for e in events:
            if e["name"].endswith("onAborted"):
                aborted += 1
            if e["name"].endswith("onFailure"):
                failure += 1
            if e["name"].endswith("onSuccess"):
                results = e["data"]["Results"]
                for r in results:
                    total += 1
                    # Status needs to be "success".
                    status = r["status"]
                    if status != Rtt.STATUS_SUCCESS:
                        self.log.warning("Got error status %d." % status)
                        invalid += 1
                        continue
                    # RTT value should be positive.
                    value = r["rtt"]
                    if value <= 0:
                        self.log.warning("Got error RTT value %d." % value)
                        invalid += 1
                        continue
                    # Vadlidate values in successful responses.
                    acd = self.actual_distance
                    margin = r[RttParam.margin]
                    # If the distance is >= 0, check distance only.
                    d = r["distance"] / 100.0
                    if d > 0:
                        # Distance should be in acceptable range.
                        is_d_valid = (acd - margin) <= d <= acd + (margin)
                        if not is_d_valid:
                            self.log.warning(("Reported distance %.2fm is out of the"
                                " acceptable range %.2f±%.2fm.") % (d, acd, margin))
                            out_of_range += 1
                        continue
                    # Check if the RTT value is in range.
                    d = (value / 2) / 1E10 * wutils.SPEED_OF_LIGHT
                    is_rtt_valid = (acd - margin) <= d <= (acd + margin)
                    if not is_rtt_valid:
                        self.log.warning(
                           ("Distance calculated from RTT value %d - %.2fm is "
                            "out of the acceptable range %.2f±%dm.") % (
                            value, d, acd, margin))
                        out_of_range += 1
                        continue
                    # Check if the RSSI value is in range.
                    rssi = r["rssi"]
                    # average rssi in 0.5dB steps, e.g. 143 implies -71.5dB,
                    # so the valid range is 0 to 200
                    is_rssi_valid = 0 <= rssi <= 200
                    if not is_rssi_valid:
                        self.log.warning(("Reported RSSI %d is out of the"
                                " acceptable range 0-200") % rssi)
                        out_of_range += 1
                        continue
        self.log.info(("Processed %d RTT events. %d aborted, %s failed. Among"
            " the %d responses in successful callbacks, %s are invalid, %s has"
            " RTT values that are out of range.") % (
            len(events), aborted, failure, total, invalid, out_of_range))
        asserts.assert_true(total > 0, "No RTT response received.")
        # Percentage of responses that are valid should be >= 90%.
        valid_total = float(total - invalid)
        valid_response_rate = valid_total / total
        self.log.info("%.2f%% of the responses are valid." % (
                      valid_response_rate * 100))
        asserts.assert_true(valid_response_rate >= 0.9,
                         "Valid response rate is below 90%%.")
        # Among the valid responses, the percentage of having an in-range RTT
        # value should be >= 67%.
        valid_value_rate = (total - invalid - out_of_range) / valid_total
        self.log.info("%.2f%% of valid responses have in-range RTT value" % (
                      valid_value_rate * 100))
        msg = "In-range response rate is below 67%%."
        asserts.assert_true(valid_value_rate >= 0.67, msg)

    def scan_then_rtt_ranging_stress_logic(self, scan_func):
        """Test logic to scan then do rtt ranging based on the scan results.

        Steps:
        1. Start scan and get scan results.
        2. Filter out the networks that support rtt in scan results.
        3. Start rtt ranging against those networks that support rtt.
        4. Repeat
        5. Process RTT events.

        Args:
            scan_func: A function that does a wifi scan and only returns the
                networks that support rtt in the scan results.

        Returns:
            True if rtt behaves as expected, False otherwise.
        """
        total = self.stress_num
        failed = 0
        all_results = []
        for i in range(total):
            self.log.info("Iteration %d" % i)
            rtt_networks = scan_func()
            if not rtt_networks:
                self.log.warning("Found no rtt network, skip this iteration.")
                failed += 1
                continue
            self.log.debug("Found rtt networks:%s" % rtt_networks)
            rtt_params = []
            for rn in rtt_networks:
                rtt_params.append(self.rtt_config_from_scan_result(rn))
            results = self.get_rtt_results(rtt_params)
            if results:
                self.log.debug(results)
                all_results += results
        self.process_rtt_events(all_results)

    def rtt_config_from_scan_result(self, scan_result):
        """Creates an Rtt configuration based on the scan result of a network.
        """
        scan_result_channel_width_to_rtt = {
            ScanResult.CHANNEL_WIDTH_20MHZ: RttBW.BW_20_SUPPORT,
            ScanResult.CHANNEL_WIDTH_40MHZ: RttBW.BW_40_SUPPORT,
            ScanResult.CHANNEL_WIDTH_80MHZ: RttBW.BW_80_SUPPORT,
            ScanResult.CHANNEL_WIDTH_160MHZ: RttBW.BW_160_SUPPORT,
            ScanResult.CHANNEL_WIDTH_80MHZ_PLUS_MHZ: RttBW.BW_160_SUPPORT
        }
        p = {}
        freq = scan_result[RttParam.frequency]
        p[RttParam.frequency] = freq
        p[RttParam.BSSID] = scan_result[WifiEnums.BSSID_KEY]
        if freq > 5000:
            p[RttParam.preamble] = RttPreamble.PREAMBLE_VHT
        else:
            p[RttParam.preamble] = RttPreamble.PREAMBLE_HT
        cf0 = scan_result[RttParam.center_freq0]
        if cf0 > 0:
            p[RttParam.center_freq0] = cf0
        cf1 = scan_result[RttParam.center_freq1]
        if cf1 > 0:
            p[RttParam.center_freq1] = cf1
        cw = scan_result["channelWidth"]
        p[RttParam.channel_width] = cw
        p[RttParam.bandwidth] = scan_result_channel_width_to_rtt[cw]
        if scan_result["is80211McRTTResponder"]:
            p[RttParam.request_type] = RttType.TYPE_TWO_SIDED
        else:
            p[RttParam.request_type] = RttType.TYPE_ONE_SIDED
        return p

    """Tests"""
    def test_invalid_params(self):
        """Tests the sanity check function in RttManager.
        """
        param_list = [
            {RttParam.device_type: 3},
            {RttParam.device_type: 1, RttParam.request_type:3},
            {
                RttParam.device_type: 1,
                RttParam.request_type:1,
                RttParam.BSSID: None
            },
            {RttParam.BSSID: "xxxxxxxx", RttParam.number_burst: 1},
            {RttParam.number_burst: 0, RttParam.num_samples_per_burst: -1},
            {RttParam.num_samples_per_burst:32},
            {
                RttParam.num_samples_per_burst:5,
                RttParam.num_retries_per_measurement_frame: -1
            },
            {RttParam.num_retries_per_measurement_frame: 4 },
            {
                RttParam.num_retries_per_measurement_frame: 2,
                RttParam.num_retries_per_FTMR: -1
            },
            {RttParam.num_retries_per_FTMR: 4}
        ]
        for param in param_list:
            self.invalid_params_logic(param)
        return True

    def test_support_check(self):
        """No device supports device-to-device RTT; only shamu and volantis
        devices support device-to-ap RTT.
        """
        model = acts.utils.trim_model_name(self.dut.model)
        asserts.assert_true(not self.dut.droid.wifiIsDeviceToDeviceRttSupported(),
            "Device to device is not supposed to be supported.")
        if any([model in m for m in self.support_models]):
            asserts.assert_true(self.dut.droid.wifiIsDeviceToApRttSupported(),
                "%s should support device-to-ap RTT." % model)
            self.log.info("%s supports device-to-ap RTT as expected." % model)
        else:
            asserts.assert_true(not self.dut.droid.wifiIsDeviceToApRttSupported(),
                "%s should not support device-to-ap RTT." % model)
            self.log.info(("%s does not support device-to-ap RTT as expected."
                ) % model)
            asserts.abort_class("Device %s does not support RTT, abort." % model)
        return True

    def test_capability_check(self):
        """Checks the capabilities params are reported as expected.
        """
        caps = self.dut.droid.wifiRttGetCapabilities()
        asserts.assert_true(caps, "Unable to get rtt capabilities.")
        self.log.debug("Got rtt capabilities %s" % caps)
        model = acts.utils.trim_model_name(self.dut.model)
        asserts.assert_true(model in self.rtt_cap_table, "Unknown model %s" % model)
        expected_caps = self.rtt_cap_table[model]
        for k, v in expected_caps.items():
            asserts.assert_true(k in caps, "%s missing in capabilities." % k)
            asserts.assert_true(v == caps[k],
                "Expected %s for %s, got %s." % (v, k, caps[k]))
        return True

    def test_discovery(self):
        """Make sure all the expected 11mc BSSIDs are discovered properly, and
        they are all reported as 802.11mc Rtt Responder.

        Procedures:
            1. Scan for wifi networks.

        Expect:
            All the RTT networks show up in scan results and their
            "is80211McRTTResponder" is True.
            All the non-RTT networks show up in scan results and their
            "is80211McRTTResponder" is False.
        """
        wutils.start_wifi_connection_scan(self.dut)
        scan_results = self.dut.droid.wifiGetScanResults()
        self.log.debug(scan_results)
        for n in visible_networks:
            asserts.assert_true(wutils.match_networks(n, scan_results),
                "Network %s was not discovered properly." % n)
        return True

    def test_missing_bssid(self):
        """Start Rtt ranging with a config that does not have BSSID set.
        Should not get onSuccess.
        """
        p = {}
        p[RttParam.request_type] = RttType.TYPE_TWO_SIDED
        p[RttParam.device_type]  = RttPeerType.PEER_TYPE_AP
        p[RttParam.preamble]     = RttPreamble.PREAMBLE_VHT
        p[RttParam.bandwidth]    = RttBW.BW_80_SUPPORT
        p[RttParam.frequency] = self.vht80_5g[WifiEnums.frequency_key]
        p[RttParam.center_freq0] = self.vht80_5g[RttParam.center_freq0]
        results = self.get_rtt_results([p])
        asserts.assert_true(results, "Did not get any result.")
        self.log.info(pprint.pformat(results))

    def test_rtt_ranging_single_AP_stress(self):
        """Stress test for Rtt against one AP.

        Steps:
            1. Do RTT ranging against the self.vht80_5g BSSID.
            2. Repeat self.stress_num times.
            3. Verify RTT results.
        """
        p = {}
        p[RttParam.request_type] = RttType.TYPE_TWO_SIDED
        p[RttParam.device_type]  = RttPeerType.PEER_TYPE_AP
        p[RttParam.preamble]     = RttPreamble.PREAMBLE_VHT
        p[RttParam.bandwidth]    = RttBW.BW_80_SUPPORT
        p[RttParam.BSSID] = self.vht80_5g[WifiEnums.BSSID_KEY]
        p[RttParam.frequency] = self.vht80_5g[WifiEnums.frequency_key]
        p[RttParam.center_freq0] = self.vht80_5g[RttParam.center_freq0]
        p[RttParam.channel_width] = ScanResult.CHANNEL_WIDTH_80MHZ
        all_results = []
        for i in range(self.stress_num):
            self.log.info("RTT Ranging iteration %d" % (i + 1))
            results = self.get_rtt_results([p])
            if results:
                all_results += results
            else:
                self.log.warning("Did not get result for iteration %d." % i)
        frate = self.process_rtt_events(all_results)

    def test_regular_scan_then_rtt_ranging_stress(self):
        """Stress test for regular scan then start rtt ranging against the RTT
        compatible networks found by the scan.

        Steps:
            1. Start a WiFi connection scan.
            2. Get scan results.
            3. Find all the 11mc capable BSSIDs and choose the ones to use
               (self.network_selector)
            4. Do RTT ranging against the selected BSSIDs, with the info from
               the scan results.
            5. Repeat self.stress_num times.
            6. Verify RTT results.
        """
        scan_func = self.regular_scan_for_rtt_networks
        self.scan_then_rtt_ranging_stress_logic(scan_func)

    def test_gscan_then_rtt_ranging_stress(self):
        """Stress test for gscan then start rtt ranging against the RTT
        compatible networks found by the scan.

        Steps:
            1. Start a WifiScanner single shot scan on all channels.
            2. Wait for full scan results of the expected 11mc capable BSSIDs.
            3. Wait for single shot scan to finish on all channels.
            4. Do RTT ranging against the selected BSSIDs, with the info from
               the scan results.
            5. Repeat self.stress_num times.
            6. Verify RTT results.
        """
        scan_func = self.gscan_for_rtt_networks
        self.scan_then_rtt_ranging_stress_logic(scan_func)

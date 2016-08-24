#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

import json
import os
import itertools

from queue import Empty
from acts import asserts
from acts.base_test import BaseTestClass
from acts.utils import load_config
from acts.test_utils.wifi.wifi_test_utils import start_wifi_track_bssid
from acts.test_utils.wifi.wifi_test_utils import start_wifi_background_scan
from acts.test_utils.wifi.wifi_test_utils import wifi_test_device_init
from acts.test_utils.wifi.wifi_test_utils import WifiChannelUS
from acts.test_utils.wifi.wifi_test_utils import WifiEnums
from acts.test_utils.wifi.wifi_test_utils import get_scan_time_and_channels


BSSID_EVENT_WAIT = 30

BSSID_EVENT_TAG = "WifiScannerBssid"
SCAN_EVENT_TAG = "WifiScannerScan"
SCANTIME = 10000 #framework support only 10s as minimum scan interval

class WifiScannerBssidError(Exception):
    pass

class WifiScannerBssidTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        # A list of all test cases to be executed in this class.
        self.tests = ("test_wifi_track_bssid_sanity",
                      "test_wifi_track_bssid_found",
                      "test_wifi_track_bssid_lost",
                      "test_wifi_track_bssid_for_2g_while_scanning_5g_channels",
                      "test_wifi_track_bssid_for_5g_while_scanning_2g_channels",)
        self.default_scan_setting = {
            "band": WifiEnums.WIFI_BAND_BOTH_WITH_DFS,
            "periodInMs": SCANTIME,
            "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
            'numBssidsPerScan': 32
        }
        self.leeway = 5
        self.stime_channel = 47 #dwell time plus 2ms

    def setup_class(self):
        self.dut = self.android_devices[0]
        wifi_test_device_init(self.dut)
        asserts.assert_true(self.dut.droid.wifiIsScannerSupported(),
            "Device %s doesn't support WifiScanner, abort." % self.dut.model)
        """It will setup the required dependencies and fetch the user params from
          config file"""
        self.attenuators[0].set_atten(0)
        self.attenuators[1].set_atten(0)
        req_params = ("bssid_2g", "bssid_5g", "bssid_dfs", "attenuator_id",
                      "max_bugreports")
        self.wifi_chs = WifiChannelUS(self.dut.model)
        self.unpack_userparams(req_params)

    def teardown_class(self):
        BaseTestClass.teardown_test(self)
        self.log.debug("Shut down all wifi scanner activities.")
        self.dut.droid.wifiScannerShutdown()

    def on_fail(self, test_name, begin_time):
        if self.max_bugreports > 0:
            self.dut.take_bug_report(test_name, begin_time)
            self.max_bugreports -= 1

    """ Helper Functions Begin """
    def fetch_scan_result(self, scan_idx, scan_setting):
        """Fetch the scan result for provider listener index.

        This function calculate the time required for scanning based on scan setting
        and wait for scan result event, on triggering of event process the scan result.

        Args:
          scan_idx: Index of the scan listener.
          scan_setting: Setting used for starting the scan.

        Returns:
          scan_results: if scan result available.
        """
        #generating event wait time from scan setting plus leeway
        self.log.debug(scan_setting)
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                              scan_setting,
                                                              self.stime_channel)
        scan_time += scan_setting['periodInMs'] #add scan period delay for next cycle
        if scan_setting["reportEvents"] == WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN:
            waittime = int(scan_time/1000) + self.leeway
        else:
            time_cache = scan_setting['periodInMs'] * 10 #default cache
            waittime = int((time_cache + scan_time )/1000) + self.leeway
        event_name = "{}{}onResults".format(SCAN_EVENT_TAG, scan_idx)
        self.log.info("Waiting for the scan result event {}".format(event_name))
        event = self.dut.ed.pop_event(event_name, waittime)
        results = event["data"]["Results"]
        if len(results) > 0 and "ScanResults" in results[0]:
            return results[0]["ScanResults"]

    def start_scan_and_validate_environment(self, scan_setting, bssid_settings):
        """Validate environment for test using current scan result for provided
           settings.

        This function start the scan for given setting and verify that interested
        Bssids are in scan result or not.

        Args:
            scan_setting: Setting used for starting the scan.
            bssid_settings: list of bssid settings.

        Returns:
            True, if bssid not found in scan result.
        """
        try:
            data = start_wifi_background_scan(self.dut, scan_setting)
            self.scan_idx = data["Index"]
            results = self.fetch_scan_result(self.scan_idx, scan_setting)
            self.log.debug("scan result {}".format(results))
            asserts.assert_true(results, "Device is not able to fetch the scan results")
            for result in results:
                for bssid_setting in bssid_settings:
                    if bssid_setting[WifiEnums.BSSID_KEY] == result[WifiEnums.BSSID_KEY]:
                        self.log.error("Bssid {} already exist in current scan results".
                                   format(result[WifiEnums.BSSID_KEY]))
                        return False
        except Empty as error:
            self.dut.droid.wifiScannerStopBackgroundScan(self.scan_idx)
            raise AssertionError("OnResult event did not triggered for scanner\n{}".
                                 format(error))
        return True

    def check_bssid_in_found_result(self, bssid_settings, found_results):
        """look for any tracked bssid in reported result of found bssids.

        Args:
            bssid_settings:Setting used for tracking bssids.
            found_results: Result reported in found event.

        Returns:
            True if bssid is present in result.
        """
        for bssid_setting in bssid_settings:
            for found_result in found_results:
                if found_result[WifiEnums.BSSID_KEY] == bssid_setting[WifiEnums.
                                                                      BSSID_KEY]:
                    return True
        return False

    def track_bssid_with_vaild_scan_for_found(self, track_setting):
        """Common logic for tracking a bssid for Found event.

         1. Starts Wifi Scanner bssid tracking for interested bssids in track_setting.
         2. Start Wifi Scanner scan with default scan settings.
         3. Validate the environment to check AP is not in range.
         4. Attenuate the signal to make AP in range.
         5. Verified that onFound event is triggered for interested bssids in
            track setting.

        Args:
            track_setting: Setting for bssid tracking.

        Returns:
            True if found event occur for interested BSSID.
        """
        self.attenuators[self.attenuator_id].set_atten(90)
        data = start_wifi_track_bssid(self.dut, track_setting)
        idx = data["Index"]
        valid_env = self.start_scan_and_validate_environment(
                           self.default_scan_setting, track_setting["bssidInfos"])
        try:
            asserts.assert_true(valid_env,
                             "Test environment is not valid, AP is in range")
            self.attenuators[self.attenuator_id].set_atten(0)
            event_name = "{}{}onFound".format(BSSID_EVENT_TAG, idx)
            self.log.info("Waiting for the BSSID event {}".format(event_name))
            event = self.dut.ed.pop_event(event_name, BSSID_EVENT_WAIT)
            self.log.debug(event)
            found = self.check_bssid_in_found_result(track_setting["bssidInfos"],
                                                      event["data"]["Results"])
            asserts.assert_true(found,
                             "Test fail because Bssid is not found in event results")
        except Empty as error:
            self.log.error("{}".format(error))
            # log scan result for debugging
            results = self.fetch_scan_result(self.scan_idx,
                                             self.default_scan_setting)
            self.log.debug("scan result {}".format(results))
            raise AssertionError("Event {} did not triggered for {}\n{}".
                                 format(event_name, track_setting["bssidInfos"],
                                        error))
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(self.scan_idx)
            self.dut.droid.wifiScannerStopTrackingBssids(idx)

    def track_bssid_with_vaild_scan_for_lost(self, track_setting):
        """Common logic for tracking a bssid for Lost event.

         1. Start Wifi Scanner scan with default scan settings.
         2. Validate the environment to check AP is not in range.
         3. Starts Wifi Scanner bssid tracking for interested bssids in track_setting.
         4. Attenuate the signal to make Bssids in range.
         5. Verified that onFound event is triggered for interested bssids in
            track setting.
         6. Attenuate the signal to make Bssids out of range.
         7. Verified that onLost event is triggered.

        Args:
            track_setting: Setting for bssid tracking.
            scan_setting: Setting used for starting the scan.

        Returns:
            True if Lost event occur for interested BSSID.
        """
        self.attenuators[self.attenuator_id].set_atten(90)
        valid_env = self.start_scan_and_validate_environment(
                          self.default_scan_setting, track_setting["bssidInfos"])
        idx = None
        found = False
        try:
            asserts.assert_true(valid_env,
                             "Test environment is not valid, AP is in range")
            data = start_wifi_track_bssid(self.dut, track_setting)
            idx = data["Index"]
            self.attenuators[self.attenuator_id].set_atten(0)
            #onFound event should be occurre before tracking for onLost event
            event_name = "{}{}onFound".format(BSSID_EVENT_TAG, idx)
            self.log.info("Waiting for the BSSID event {}".format(event_name))
            event = self.dut.ed.pop_event(event_name, BSSID_EVENT_WAIT)
            self.log.debug(event)
            found = self.check_bssid_in_found_result(track_setting["bssidInfos"],
                                                      event["data"]["Results"])
            asserts.assert_true(found,
                             "Test fail because Bssid is not found in event results")
            if found:
                self.attenuators[self.attenuator_id].set_atten(90)
                # log scan result for debugging
                for i in range(1, track_setting["apLostThreshold"]):
                    results = self.fetch_scan_result(self.scan_idx,
                                                     self.default_scan_setting)
                    self.log.debug("scan result {} {}".format(i, results))
                event_name = "{}{}onLost".format(BSSID_EVENT_TAG, idx)
                self.log.info("Waiting for the BSSID event {}".format(event_name))
                event = self.dut.ed.pop_event(event_name, BSSID_EVENT_WAIT)
                self.log.debug(event)
        except Empty as error:
            raise AssertionError("Event {} did not triggered for {}\n{}".
                                 format(event_name, track_setting["bssidInfos"], error))
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(self.scan_idx)
            if idx:
                self.dut.droid.wifiScannerStopTrackingBssids(idx)

    def wifi_generate_track_bssid_settings(self, isLost):
        """Generates all the combinations of different track setting parameters.

        Returns:
            A list of dictionaries each representing a set of track settings.
        """
        bssids = [[self.bssid_2g], [self.bssid_5g],
                  [self.bssid_2g, self.bssid_5g]]
        if self.dut.model != "hammerhead":
            bssids.append([self.bssid_dfs])
        if isLost:
            apthreshold = (3,5)
        else:
            apthreshold = (1,)
        # Create track setting strings based on the combinations
        setting_combinations = list(itertools.product(bssids, apthreshold))
        # Create scan setting strings based on the combinations
        track_settings = []
        for combo in setting_combinations:
            s = {}
            s["bssidInfos"] = combo[0]
            s["apLostThreshold"] = combo[1]
            track_settings.append(s)
        return track_settings

    def track_setting_to_string(self, track_setting):
        """Convert track setting to string for Bssids in that"""
        string = ""
        for bssid_setting in track_setting:
            string += bssid_setting[WifiEnums.BSSID_KEY]
            string += "_"
        return string

    def combineBssids(self, *track_settings):
        """Combine bssids in the track_settings to one list"""
        bssids = []
        for track_setting in track_settings:
            bssids.extend(track_setting["bssidInfos"])
        return bssids

    """ Helper Functions End """

    """ Tests Begin """
    def test_wifi_track_bssid_found(self):
        """Test bssid track for event found with a list of different settings.

         1. Starts Wifi Scanner bssid tracking for interested bssids in track_setting.
         2. Start Wifi Scanner scan with default scan settings.
         3. Validate the environment to check AP is not in range.
         4. Attenuate the signal to make AP in range.
         5. Verified that onFound event is triggered for interested bssids in
            track setting.
        """
        track_settings = self.wifi_generate_track_bssid_settings(False)
        name_func = (lambda track_setting :
                     "test_wifi_track_found_bssidInfos_{}apLostThreshold_{}".
                     format(self.track_setting_to_string(track_setting["bssidInfos"]),
                            track_setting["apLostThreshold"]))
        failed = self.run_generated_testcases( self.track_bssid_with_vaild_scan_for_found,
                                               track_settings, name_func = name_func)
        asserts.assert_true(not failed,
                         "Track bssid found failed with these bssids: {}".
                         format(failed))

    def test_wifi_track_bssid_lost(self):
        """Test bssid track for event lost with a list of different settings.

         1. Start Wifi Scanner scan with default scan settings.
         2. Validate the environment to check AP is not in range.
         3. Starts Wifi Scanner bssid tracking for interested bssids in track_setting.
         4. Attenuate the signal to make Bssids in range.
         5. Verified that onFound event is triggered for interested bssids in
            track setting.
         6. Attenuate the signal to make Bssids out of range.
         7. Verified that onLost event is triggered.
        """
        track_settings = self.wifi_generate_track_bssid_settings(True)
        name_func = (lambda track_setting :
                     "test_wifi_track_lost_bssidInfos_{}apLostThreshold_{}".
                     format(self.track_setting_to_string(track_setting["bssidInfos"]),
                            track_setting["apLostThreshold"]))
        failed = self.run_generated_testcases( self.track_bssid_with_vaild_scan_for_lost,
                                               track_settings, name_func = name_func)
        asserts.assert_true(not failed,
                         "Track bssid lost failed with these bssids: {}".format(failed))

    def test_wifi_track_bssid_sanity(self):
        """Test bssid track for event found and lost with default settings.

         1. Start WifiScanner scan for default scan settings.
         2. Start Bssid track for "bssid_2g" AP.
         3. Attenuate the signal to move in AP range.
         4. Verify that onFound event occur.
         5. Attenuate the signal to move out of range
         6. Verify that onLost event occur.
        """
        track_setting = {"bssidInfos":[self.bssid_2g], "apLostThreshold":3}
        self.track_bssid_with_vaild_scan_for_lost(track_setting)

    def test_wifi_track_bssid_for_2g_while_scanning_5g_channels(self):
      """Test bssid track for 2g bssids while scanning 5g channels.

         1. Starts Wifi Scanner bssid tracking for 2g bssids in track_setting.
         2. Start Wifi Scanner scan for 5G Band only.
         3. Validate the environment to check AP is not in range.
         4. Attenuate the signal to make AP in range.
         5. Verified that onFound event isn't triggered for 2g bssids.
      """
      self.attenuators[self.attenuator_id].set_atten(90)
      scan_setting = { "band": WifiEnums.WIFI_BAND_5_GHZ,
                       "periodInMs": SCANTIME,
                       "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                       "numBssidsPerScan": 32}
      track_setting = {"bssidInfos":[self.bssid_2g], "apLostThreshold":3}
      valid_env = self.start_scan_and_validate_environment(scan_setting,
                                                     track_setting["bssidInfos"])
      idx = None
      try:
          asserts.assert_true(valid_env,
                               "Test environment is not valid, AP is in range")
          data = start_wifi_track_bssid(self.dut, track_setting)
          idx = data["Index"]
          self.attenuators[self.attenuator_id].set_atten(0)
          event_name = "{}{}onFound".format(BSSID_EVENT_TAG, idx)
          self.log.info("Waiting for the BSSID event {}".format(event_name))
          #waiting for 2x time to make sure
          event = self.dut.ed.pop_event(event_name, BSSID_EVENT_WAIT * 2)
          self.log.debug(event)
          found = self.check_bssid_in_found_result(track_setting["bssidInfos"],
                                                    event["data"]["Results"])
          asserts.assert_true(not found,
                             "Test fail because Bssid onFound event is triggered")
      except Empty as error:
          self.log.info("As excepted event didn't occurred with different scan setting")
      finally:
          self.dut.droid.wifiScannerStopBackgroundScan(self.scan_idx)
          if idx:
              self.dut.droid.wifiScannerStopTrackingBssids(idx)

    def test_wifi_track_bssid_for_5g_while_scanning_2g_channels(self):
        """Test bssid track for 5g bssids while scanning 2g channels.

           1. Starts Wifi Scanner bssid tracking for 5g bssids in track_setting.
           2. Start Wifi Scanner scan for 2G Band only.
           3. Validate the environment to check AP is not in range.
           4. Attenuate the signal to make AP in range.
           5. Verified that onFound event isn't triggered for 5g bssids.
        """
        self.attenuators[self.attenuator_id].set_atten(90)
        scan_setting = { "band": WifiEnums.WIFI_BAND_24_GHZ,
                         "periodInMs": SCANTIME,
                         "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                         "numBssidsPerScan": 32}
        track_setting = {"bssidInfos":[self.bssid_5g], "apLostThreshold":3}
        data = start_wifi_track_bssid(self.dut, track_setting)
        idx = data["Index"]
        valid_env = self.start_scan_and_validate_environment(scan_setting,
                                                       track_setting["bssidInfos"])
        try:
            asserts.assert_true(valid_env,
                               "Test environment is not valid, AP is in range")
            self.attenuators[self.attenuator_id].set_atten(0)
            event_name = "{}{}onFound".format(BSSID_EVENT_TAG, idx)
            self.log.info("Waiting for the BSSID event {}".format(event_name))
            #waiting for 2x time to make sure
            event = self.dut.ed.pop_event(event_name, BSSID_EVENT_WAIT * 2)
            self.log.debug(event)
            found = self.check_bssid_in_found_result(track_setting["bssidInfos"],
                                                      event["data"]["Results"])
            asserts.assert_true(not found,
                             "Test fail because Bssid onFound event is triggered")
        except Empty as error:
            self.log.info("As excepted event didn't occurred with different scan setting")
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(self.scan_idx)
            if idx:
                self.dut.droid.wifiScannerStopTrackingBssids(idx)

    def test_wifi_tracking_bssid_multi_listeners_found(self):
        """Test bssid tracking for multiple listeners
            1. Start BSSID tracking for 5g bssids
            2. Start BSSID tracking for 2g bssids
            3. Start WifiScanner scan on both bands.
            4. Valid the environment and check the APs are not in range.
            5. Attenuate the signal to make the APs in range.
            6. Verify onFound event triggered on both APs.
        """
        # Attenuate the signal to make APs invisible.
        self.attenuators[self.attenuator_id].set_atten(90)
        scan_setting = { "band": WifiEnums.WIFI_BAND_BOTH_WITH_DFS,
                         "periodInMs": SCANTIME,
                         "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                         "numBssidsPerScan": 32}
        track_setting_5g = {"bssidInfos":[self.bssid_5g], "apLostThreshold":3}
        data_5g = start_wifi_track_bssid(self.dut, track_setting_5g)
        idx_5g = data_5g["Index"]

        track_setting_2g = {"bssidInfos":[self.bssid_2g], "apLostThreshold":3}
        data_2g = start_wifi_track_bssid(self.dut, track_setting_2g)
        idx_2g = data_2g["Index"]

        valid_env = self.start_scan_and_validate_environment(
            scan_setting, self.combineBssids(track_setting_5g, track_setting_2g))
        try:
            asserts.assert_true(valid_env,
                                "Test environment is not valid, AP is in range")
            self.attenuators[self.attenuator_id].set_atten(0)
            event_name = "{}{}{}{}onFound".format(BSSID_EVENT_TAG, idx_5g, BSSID_EVENT_TAG, idx_2g)
            self.log.info("Waiting for the BSSID event {}".format(event_name))
            #waiting for 2x time to make sure
            event = self.dut.ed.pop_event(event_name, BSSID_EVENT_WAIT * 2)
            self.log.debug(event)
            found = self.check_bssid_in_found_result(
                self.combineBssids(track_setting_5g, track_setting_2g),
                event["data"]["Results"])
            asserts.assert_true(found,
                                "Test failed because Bssid onFound event is not triggered")
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(self.scan_idx)
            if idx_5g:
                self.dut.droid.wifiScannerStopTrackingBssids(idx_5g)
            if idx_2g:
                self.dut.droid.wifiScannerStopTrackingBssids(idx_2g);

""" Tests End """

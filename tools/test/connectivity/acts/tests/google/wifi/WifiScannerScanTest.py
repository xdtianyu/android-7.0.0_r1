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

import itertools
from queue import Empty
import threading, time, traceback

from acts import asserts
from acts.base_test import BaseTestClass
from acts.utils import load_config
from acts.test_utils.wifi.wifi_test_utils import get_scan_time_and_channels
from acts.test_utils.wifi.wifi_test_utils import check_internet_connection
from acts.test_utils.wifi.wifi_test_utils import start_wifi_background_scan
from acts.test_utils.wifi.wifi_test_utils import start_wifi_single_scan
from acts.test_utils.wifi.wifi_test_utils import track_connection
from acts.test_utils.wifi.wifi_test_utils import wifi_test_device_init
from acts.test_utils.wifi.wifi_test_utils import WifiEnums
from acts.test_utils.wifi.wifi_test_utils import WifiChannelUS
from acts.test_utils.wifi.wifi_test_utils import wifi_forget_network
from acts.test_utils.wifi.wifi_test_utils import wifi_toggle_state

SCANTIME = 10000 #framework support only 10s as minimum scan interval
NUMBSSIDPERSCAN = 8
EVENT_TAG = "WifiScannerScan"
SCAN_TIME_PASSIVE = 47 # dwell time plus 2ms
SCAN_TIME_ACTIVE = 32 # dwell time plus 2ms
SHORT_TIMEOUT = 30
NETWORK_ID_ERROR = "Network don't have ID"
NETWORK_ERROR = "Device is not connected to reference network"
INVALID_RESULT = "Test fail because scan result reported are not valid"
EMPTY_RESULT = "Test fail because empty scan result reported"
KEY_RET = "ResultElapsedRealtime"

class WifiScannerScanError(Exception):
    pass

class WifiScannerScanTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        asserts.failed_scan_settings = None
        # A list of all test cases to be executed in this class.
        self.tests = (
            "test_available_channels_generated",
            "test_wifi_scanner_single_scan_channel_sanity",
            "test_wifi_scanner_with_wifi_off",
            "test_single_scan_report_each_scan_for_channels_with_enumerated_params",
            "test_single_scan_report_each_scan_for_band_with_enumerated_params",
            "test_wifi_scanner_batch_scan_channel_sanity",
            "test_wifi_scanner_batch_scan_period_too_short",
            "test_batch_scan_report_buffer_full_for_channels_with_enumerated_params",
            "test_batch_scan_report_buffer_full_for_band_with_enumerated_params",
            "test_batch_scan_report_each_scan_for_channels_with_enumerated_params",
            "test_batch_scan_report_each_scan_for_band_with_enumerated_params",
            "test_single_scan_report_full_scan_for_channels_with_enumerated_params",
            "test_single_scan_report_full_scan_for_band_with_enumerated_params",
            "test_batch_scan_report_full_scan_for_channels_with_enumerated_params",
            "test_batch_scan_report_full_scan_for_band_with_enumerated_params",
            "test_wifi_connection_while_single_scan",
            "test_single_scan_while_pno",
            "test_wifi_connection_and_pno_while_batch_scan",
            "test_wifi_scanner_single_scan_in_isolated",
            "test_wifi_scanner_with_invalid_numBssidsPerScan"
            )
        self.leeway = 10
        self.stime_channel = SCAN_TIME_PASSIVE
        self.default_scan_setting = {
                        "band": WifiEnums.WIFI_BAND_BOTH,
                        "periodInMs": SCANTIME,
                        "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN
                        }
        self.default_batch_scan_setting = {
                        "band": WifiEnums.WIFI_BAND_BOTH,
                        "periodInMs": SCANTIME,
                        "reportEvents": WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL
                        }

    def setup_class(self):
        self.dut = self.android_devices[0]
        wifi_test_device_init(self.dut)
        req_params = ("connect_network", "run_extended_test", "ping_addr",
                      "max_bugreports")
        self.unpack_userparams(req_params)
        self.log.debug("Run extended test: {}".format(self.run_extended_test))
        self.wifi_chs = WifiChannelUS(self.dut.model)
        asserts.assert_true(self.dut.droid.wifiIsScannerSupported(),
            "Device %s doesn't support WifiScanner, abort." % self.dut.model)
        self.attenuators[0].set_atten(0)
        self.attenuators[1].set_atten(0)

    def teardown_test(self):
        BaseTestClass.teardown_test(self)
        self.log.debug("Shut down all wifi scanner activities.")
        self.dut.droid.wifiScannerShutdown()

    def on_fail(self, test_name, begin_time):
        if self.max_bugreports > 0:
            self.dut.take_bug_report(test_name, begin_time)
            self.max_bugreports -= 1
        self.dut.cat_adb_log(test_name, begin_time)

    """ Helper Functions Begin """

    def wifi_generate_scanner_scan_settings(self, extended, scan_type, report_result):
        """Generates all the combinations of different scan setting parameters.

        Args:
          extended: True for extended setting
          scan_type: key for type of scan
          report_result: event type of report scan results

        Returns:
          A list of dictionaries each representing a set of scan settings.
        """
        base_scan_time = [SCANTIME*2]
        if scan_type == "band":
            scan_types_setting = [WifiEnums.WIFI_BAND_BOTH]
        else:
            scan_types_setting = [self.wifi_chs.MIX_CHANNEL_SCAN]
        num_of_bssid = [NUMBSSIDPERSCAN*4]
        max_scan_cache = [0]
        if extended:
            base_scan_time.append(SCANTIME)
            if scan_type == "band":
                scan_types_setting.extend([WifiEnums.WIFI_BAND_24_GHZ,
                                           WifiEnums.WIFI_BAND_5_GHZ_WITH_DFS,
                                           WifiEnums.WIFI_BAND_BOTH_WITH_DFS])
            else:
                scan_types_setting.extend([self.wifi_chs.NONE_DFS_5G_FREQUENCIES,
                                           self.wifi_chs.ALL_2G_FREQUENCIES,
                                           self.wifi_chs.DFS_5G_FREQUENCIES,
                                           self.wifi_chs.ALL_5G_FREQUENCIES])
            num_of_bssid.append(NUMBSSIDPERSCAN*3)
            max_scan_cache.append(5)
            # Generate all the combinations of report types and scan types
        if report_result == WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT:
            report_types = {"reportEvents" : report_result}
            setting_combinations = list(itertools.product(scan_types_setting,
                                                          base_scan_time))
            # Create scan setting strings based on the combinations
            scan_settings = []
            for combo in setting_combinations:
                s = dict(report_types)
                s[scan_type] = combo[0]
                s["periodInMs"] = combo[1]
                scan_settings.append(s)
        else:
            report_types = {"reportEvents" : report_result}
            setting_combinations = list(itertools.product(scan_types_setting,
                                                          base_scan_time, num_of_bssid,
                                                          max_scan_cache))
            # Create scan setting strings based on the combinations
            scan_settings = []
            for combo in setting_combinations:
                s = dict(report_types)
                s[scan_type] = combo[0]
                s["periodInMs"] = combo[1]
                s["numBssidsPerScan"] = combo[2]
                s["maxScansToCache"] = combo[3]
                scan_settings.append(s)
        return scan_settings

    def proces_and_valid_batch_scan_result(self, scan_resutls, scan_rt,
                                           result_rt, scan_setting):
        """This function process scan results and validate against settings used
        while starting the scan.

        There are two steps for the verification. First it checks that all the
        wifi networks in results are of the correct frequencies set by scan setting
        params. Then it checks that the delta between the batch of scan results less
        than the time required for scanning channel set by scan setting params.

        Args:
            scan_results: scan results reported.
            scan_rt: Elapsed real time on start scan.
            result_rt: Elapsed ral time on results reported.
            scan_setting: The params for the single scan.

        Returns:
            bssids: total number of bssids scan result have
            validity: True if the all scan result are valid.
        """
        bssids = 0
        validity = True
        scan_time_mic = 0
        scan_channels = []
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                              scan_setting,
                                                              self.stime_channel)
        scan_time_mic = scan_time * 1000
        for i, batch in enumerate(scan_resutls, start=1):
            max_scan_interval =  batch["ScanResults"][0]["timestamp"] + scan_time_mic
            self.log.debug("max_scan_interval: {}".format(max_scan_interval) )
            for result in batch["ScanResults"]:
              if (result["frequency"] not in scan_channels
                      or result["timestamp"] > max_scan_interval
                      or result["timestamp"] < scan_rt*1000
                      or result["timestamp"] > result_rt*1000) :
                  self.log.error("Result didn't match requirement: {}".
                                 format(result) )
                  validity = False
            self.log.info("Number of scan result in batch {} :: {}".format(i,
                                                     len(batch["ScanResults"])))
            bssids += len(batch["ScanResults"])
        return bssids, validity

    def pop_scan_result_events(self, event_name):
        """Function to pop all the scan result events.

        Args:
            event_name: event name.

        Returns:
            results: list  of scan result reported in events
        """
        results = []
        try:
            events = self.dut.ed.pop_all(event_name)
            for event in events:
                results.append(event["data"]["Results"])
        except Empty as error:
            self.log.debug("Number of Full scan results {}".format(len(results)))
        return results

    def wifi_scanner_single_scan(self, scan_setting):
        """Common logic for an enumerated wifi scanner single scan test case.

         1. Start WifiScanner single scan for scan_setting.
         2. Wait for the scan result event, wait time depend on scan settings
            parameter.
         3. Verify that scan results match with scan settings parameters.
         4. Also verify that only one scan result event trigger.

        Args:
            scan_setting: The params for the single scan.
        """
        data = start_wifi_single_scan(self.dut, scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi single shot scan started index: {} at real time: {}".
                      format(idx, scan_rt))
        results = []
        #generating event wait time from scan setting plus leeway
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                              scan_setting,
                                                              self.stime_channel)
        wait_time = int(scan_time/1000) + self.leeway
        validity = False
        #track number of result received
        result_received = 0
        try:
            for snumber in range(1,3):
                event_name = "{}{}onResults".format(EVENT_TAG, idx)
                self.log.debug("Waiting for event: {} for time {}".
                               format(event_name, wait_time))
                event = self.dut.ed.pop_event(event_name, wait_time)
                self.log.debug("Event received: {}".format(event ))
                results = event["data"]["Results"]
                result_received += 1
                bssids, validity = self.proces_and_valid_batch_scan_result(
                                                          results, scan_rt,
                                                          event["data"][KEY_RET],
                                                          scan_setting)
                asserts.assert_true(len(results) == 1,
                                 "Test fail because number of scan result {}"
                                 .format(len(results)))
                asserts.assert_true(bssids > 0, EMPTY_RESULT)
                asserts.assert_true(validity, INVALID_RESULT)
                self.log.info("Scan number Buckets: {}\nTotal BSSID: {}".
                              format(len(results), bssids))
        except Empty as error:
            asserts.assert_true(result_received >= 1,
                             "Event did not triggered for single shot {}".
                             format(error))
        finally:
            self.dut.droid.wifiScannerStopScan(idx)
            #For single shot number of result received and length of result should be one
            asserts.assert_true(result_received == 1,
                             "Test fail because received result {}".
                             format(result_received))

    def wifi_scanner_single_scan_full(self, scan_setting):
        """Common logic for single scan test case for full scan result.

        1. Start WifiScanner single scan with scan_setting for full scan result.
        2. Wait for the scan result event, wait time depend on scan settings
           parameter.
        3. Pop all full scan result events occurred earlier.
        4. Verify that full scan results match with normal scan results.

        Args:
            scan_setting: The parameters for the single scan.
        """
        self.dut.ed.clear_all_events()
        data = start_wifi_single_scan(self.dut, scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi single shot scan started with index: {}".format(idx))
        #generating event wait time from scan setting plus leeway
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                              scan_setting,
                                                              self.stime_channel)
        wait_time = int(scan_time/1000) + self.leeway
        results = []
        validity = False
        try:
            event_name = "{}{}onResults".format(EVENT_TAG, idx)
            self.log.debug("Waiting for event: {} for time {}".
                           format(event_name, wait_time))
            event = self.dut.ed.pop_event(event_name, wait_time)
            self.log.info("Event received: {}".format(event))
            bssids, validity = (self.proces_and_valid_batch_scan_result(
                                                event["data"]["Results"], scan_rt,
                                                event["data"][KEY_RET],
                                                scan_setting))
            asserts.assert_true(bssids > 0, EMPTY_RESULT)
            asserts.assert_true(validity, INVALID_RESULT)
            event_name = "{}{}onFullResult".format(EVENT_TAG, idx)
            results = self.pop_scan_result_events(event_name)
            asserts.assert_true(len(results) >= bssids,
                             "Full single shot result don't match {}".
                             format(len(results)))
        except Empty as error:
            raise AssertionError("Event did not triggered for single shot {}".
                                 format(error))
        finally:
            self.dut.droid.wifiScannerStopScan(idx)

    def wifi_scanner_batch_scan_full(self, scan_setting):
        """Common logic for batch scan test case for full scan result.

        1. Start WifiScanner batch scan with scan_setting for full scan result.
        2. Wait for the scan result event, wait time depend on scan settings
           parameter.
        3. Pop all full scan result events occurred earlier.
        4. Verify that full scan results match with scan results.

        Args:
            scan_setting: The params for the batch scan.
        """
        self.dut.ed.clear_all_events()
        data = start_wifi_background_scan(self.dut, scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi batch shot scan started with index: {}".format(idx))
        #generating event wait time from scan setting plus leeway
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                              scan_setting,
                                                              self.stime_channel)
        # multiply scan period by two to account for scheduler changing period
        scan_time += scan_setting['periodInMs'] * 2 #add scan period delay for next cycle
        wait_time = scan_time / 1000 + self.leeway
        validity = False
        try:
            for snumber in range(1,3):
                results = []
                event_name = "{}{}onResults".format(EVENT_TAG, idx)
                self.log.debug("Waiting for event: {} for time {}".
                               format(event_name, wait_time))
                event = self.dut.ed.pop_event(event_name, wait_time)
                self.log.debug("Event received: {}".format(event))
                bssids, validity = self.proces_and_valid_batch_scan_result(
                                                      event["data"]["Results"],
                                                      scan_rt,
                                                      event["data"][KEY_RET],
                                                      scan_setting)
                event_name = "{}{}onFullResult".format(EVENT_TAG, idx)
                results = self.pop_scan_result_events(event_name)
                asserts.assert_true(len(results) >= bssids,
                                 "Full single shot result don't match {}".
                                 format(len(results)))
                asserts.assert_true(bssids > 0, EMPTY_RESULT)
                asserts.assert_true(validity, INVALID_RESULT)
        except Empty as error:
             raise AssertionError("Event did not triggered for batch scan {}".
                                  format(error))
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(idx)
            self.dut.ed.clear_all_events()

    def wifi_scanner_batch_scan(self, scan_setting):
        """Common logic for an enumerated wifi scanner batch scan test case.

        1. Start WifiScanner batch scan for given scan_setting.
        2. Wait for the scan result event, wait time depend on scan settings
           parameter.
        3. Verify that scan results match with scan settings parameters.
        4. Also verify that multiple scan result events trigger.

        Args:
            scan_setting: The parameters for the batch scan.
        """
        data = start_wifi_background_scan(self.dut, scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi background scan started with index: {} real time {}"
                      .format(idx, scan_rt))
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                              scan_setting,
                                                              self.stime_channel)
        #generating event wait time from scan setting plus leeway
        time_cache = 0
        number_bucket = 1 #bucket for Report result on each scan
        check_get_result = False
        if scan_setting['reportEvents'] == WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL:
            check_get_result = True
            if ('maxScansToCache' in scan_setting and
                                    scan_setting['maxScansToCache'] != 0) :
                time_cache = (scan_setting['maxScansToCache'] *
                              scan_setting['periodInMs'])
                number_bucket = scan_setting['maxScansToCache']
            else:
                time_cache = 10 * scan_setting['periodInMs'] #10 as default max scan cache
                number_bucket = 10
        else:
            time_cache = scan_setting['periodInMs'] #need while waiting for seconds resutls
        # multiply cache time by two to account for scheduler changing period
        wait_time = (time_cache * 2 + scan_time) / 1000 + self.leeway
        validity = False
        try:
            for snumber in range(1,3):
                event_name = "{}{}onResults".format(EVENT_TAG, idx)
                self.log.info("Waiting for event: {} for time {}".
                              format(event_name,wait_time))
                event = self.dut.ed.pop_event(event_name, wait_time)
                self.log.debug("Event received: {}".format(event ))
                results = event["data"]["Results"]
                bssids, validity = (self.proces_and_valid_batch_scan_result(
                                                          results, scan_rt,
                                                          event["data"][KEY_RET],
                                                          scan_setting))
                self.log.info("Scan number: {}\n Buckets: {}\n  BSSID: {}".
                              format(snumber, len(results), bssids))
                asserts.assert_true(len(results) == number_bucket,
                                     "Test fail because number_bucket {}".
                                     format(len(results)))
                asserts.assert_true(bssids >= 1, EMPTY_RESULT)
                asserts.assert_true(validity, INVALID_RESULT)
                if snumber%2 == 1 and check_get_result :
                    self.log.info("Get Scan result using GetScanResult API")
                    time.sleep(wait_time/number_bucket)
                    if self.dut.droid.wifiScannerGetScanResults():
                        event = self.dut.ed.pop_event(event_name, 1)
                        self.log.debug("Event onResults: {}".format(event))
                        results = event["data"]["Results"]
                        bssids, validity = (self.proces_and_valid_batch_scan_result(
                                                          results, scan_rt,
                                                          event["data"][KEY_RET],
                                                          scan_setting))
                        self.log.info("Got Scan result number: {} BSSID: {}".
                                      format(snumber, bssids))
                        asserts.assert_true(bssids >= 1, EMPTY_RESULT)
                        asserts.assert_true(validity, INVALID_RESULT)
                    else:
                        self.log.error("Error while fetching the scan result")
        except Empty as error:
            raise AssertionError("Event did not triggered for batch scan {}".
                                 format(error))
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(idx)
            self.dut.ed.clear_all_events()

    def start_wifi_scanner_single_scan_expect_failure(self, scan_setting):
        """Common logic to test wif scanner single scan with invalid settings
           or environment

         1. Start WifiScanner batch scan for setting parameters.
         2. Verify that scan is not started.

         Args:
            scan_setting: The params for the single scan.
        """
        try:
            idx = self.dut.droid.wifiScannerStartScan(scan_setting)
            event = self.dut.ed.pop_event("{}{}onFailure".format(EVENT_TAG, idx),
                                      SHORT_TIMEOUT)
        except Empty as error:
            raise AssertionError("Did not get expected onFailure {}".format(error))

    def start_wifi_scanner_background_scan_expect_failure(self, scan_setting):
        """Common logic to test wif scanner batch scan with invalid settings
           or environment

         1. Start WifiScanner batch scan for setting parameters.
         2. Verify that scan is not started.

         Args:
          scan_setting: The params for the single scan.
        """
        try:
          idx = self.dut.droid.wifiScannerStartBackgroundScan(scan_setting)
          event = self.dut.ed.pop_event("{}{}onFailure".format(EVENT_TAG, idx),
                                    SHORT_TIMEOUT)
        except Empty as error:
          raise AssertionError("Did not get expected onFailure {}".format(error))

    def check_get_available_channels_with_one_band(self, band):
        """Common logic to check available channels for a band.

         1. Get available channels for band.
         2. Verify that channels match with supported channels for band.

         Args:
            band: wifi band."""

        r = self.dut.droid.wifiScannerGetAvailableChannels(band)
        self.log.debug(band)
        self.log.debug(r)
        expected = self.wifi_chs.band_to_freq(band)
        asserts.assert_true(set(r) == set(expected),
                         "Band {} failed. Expected {}, got {}".
                         format(band, expected, r))

    def connect_to_reference_network(self):
        """Connect to reference network and make sure that connection happen"""
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        try:
            self.dut.droid.wifiPriorityConnect(self.connect_network)
            connect_result = self.dut.ed.pop_event("WifiManagerPriorityConnectOnSuccess",
                                               SHORT_TIMEOUT)
            self.log.info(connect_result)
            return track_connection(self.dut, self.connect_network["ssid"], 1)
        except Exception as error:
            self.log.exception(traceback.format_exc())
            self.log.error("Connection to network fail because {}".format(error))
            return False
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    """ Helper Functions End """

    """ Tests Begin """
    def test_available_channels_generated(self):
        """Test available channels for different bands.

         1. Get available channels for different bands.
         2. Verify that channels match with supported channels for respective band.
        """
        bands = (1,2,3,4,6,7)
        name_func = lambda band : "test_get_channel_band_{}".format(band)
        failed = self.run_generated_testcases(
                                self.check_get_available_channels_with_one_band,
                                bands, name_func = name_func)
        asserts.assert_true(not failed,
                         "Number of test_get_channel_band failed {}".
                         format(len(failed)))

    def test_single_scan_report_each_scan_for_channels_with_enumerated_params(self):
        """Test WiFi scanner single scan for channels with enumerated settings.

         1. Start WifiScanner single scan for different channels with enumerated
            scan settings.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                          self.run_extended_test, "channels",
                                          WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN)
        self.log.debug("Scan settings:{}\n{}".format(len(scan_settings),
                                                     scan_settings))
        name_func = (lambda scan_setting :
                     ("test_single_scan_report_each_scan_for_channels_{}"
                      "_numBssidsPerScan_{}_maxScansToCache_{}_period_{}").
                      format(scan_setting["channels"],
                             scan_setting["numBssidsPerScan"],
                             scan_setting["maxScansToCache"],
                             scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_single_scan,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_single_scan_report_each_scan_for_channels"
                          " failed {}").format(len(failed)))

    def test_single_scan_report_each_scan_for_band_with_enumerated_params(self):
        """Test WiFi scanner single scan for bands with enumerated settings.

         1. Start WifiScanner single scan for different bands with enumerated
            scan settings.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                          self.run_extended_test,"band",
                                          WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN)
        self.log.debug("Scan settings:{}\n{}".format(len(scan_settings),
                                                     scan_settings))
        name_func = (lambda scan_setting :
                     ("test_single_scan_report_each_scan_for_band_{}"
                      "_numBssidsPerScan_{}_maxScansToCache_{}_period_{}").
                     format(scan_setting["band"],
                            scan_setting["numBssidsPerScan"],
                            scan_setting["maxScansToCache"],
                            scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_single_scan,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_single_scan_report_each_scan_for_band"
                          " failed {}").format(len(failed)))

    def test_batch_scan_report_buffer_full_for_channels_with_enumerated_params(self):
        """Test WiFi scanner batch scan using channels with enumerated settings
           to report buffer full scan results.

         1. Start WifiScanner batch scan using different channels with enumerated
            scan settings to report buffer full scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                      self.run_extended_test, "channels",
                                      WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL)
        self.log.debug("Scan settings:{}\n{}".format(len(scan_settings),
                                                     scan_settings))
        name_func = (lambda scan_setting :
                     ("test_batch_scan_report_buffer_full_for_channels_{}"
                      "_numBssidsPerScan_{}_maxScansToCache_{}_periodInMs_{}").
                     format(scan_setting["channels"],
                            scan_setting["numBssidsPerScan"],
                            scan_setting["maxScansToCache"],
                            scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_batch_scan,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_batch_scan_report_buffer_full_for_channels"
                          " failed {}").format(len(failed)))

    def test_batch_scan_report_buffer_full_for_band_with_enumerated_params(self):
        """Test WiFi scanner batch scan using band with enumerated settings
           to report buffer full scan results.

         1. Start WifiScanner batch scan using different bands with enumerated
            scan settings to report buffer full scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                       self.run_extended_test,"band",
                                       WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL)
        self.log.debug("Scan settings:{}\n{}".format(len(scan_settings),
                                                     scan_settings))
        name_func = (lambda scan_setting :
                     ("test_batch_scan_report_buffer_full_for_band_{}"
                      "_numBssidsPerScan_{}_maxScansToCache_{}_periodInMs_{}").
                     format(scan_setting["band"],
                            scan_setting["numBssidsPerScan"],
                            scan_setting["maxScansToCache"],
                            scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_batch_scan,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_batch_scan_report_buffer_full_for_band"
                          " failed {}").format(len(failed)))

    def test_batch_scan_report_each_scan_for_channels_with_enumerated_params(self):
        """Test WiFi scanner batch scan using channels with enumerated settings
           to report each scan results.

         1. Start WifiScanner batch scan using different channels with enumerated
            scan settings to report each scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                        self.run_extended_test, "channels",
                                        WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN)
        self.log.debug("Scan settings:{}\n{}".
                       format(len(scan_settings),scan_settings))
        name_func = (lambda scan_setting :
                     ("test_batch_scan_report_each_scan_for_channels_{}"
                      "_numBssidsPerScan_{}_maxScansToCache_{}_periodInMs_{}").
                     format(scan_setting["channels"],
                            scan_setting["numBssidsPerScan"],
                            scan_setting["maxScansToCache"],
                            scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_batch_scan,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_batch_scan_report_each_scan_for_channels"
                          " failed {}").format(len(failed)))

    def test_batch_scan_report_each_scan_for_band_with_enumerated_params(self):
        """Test WiFi scanner batch scan using band with enumerated settings
           to report each scan results.

         1. Start WifiScanner batch scan using different bands with enumerated
            scan settings to report each scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                        self.run_extended_test, "band",
                                        WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN)
        self.log.debug("Scan settings:{}\n{}".format(len(scan_settings),
                                                     scan_settings))
        name_func = (lambda scan_setting :
                     ("test_batch_scan_report_each_scan_for_band_{}"
                      "_numBssidsPerScan_{}_maxScansToCache_{}_periodInMs_{}").
                     format(scan_setting["band"],
                            scan_setting["numBssidsPerScan"],
                            scan_setting["maxScansToCache"],
                            scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_batch_scan,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_batch_scan_report_each_scan_for_band"
                          " failed {}").format(len(failed)))

    def test_single_scan_report_full_scan_for_channels_with_enumerated_params(self):
        """Test WiFi scanner single scan using channels with enumerated settings
           to report full scan results.

         1. Start WifiScanner single scan using different channels with enumerated
            scan settings to report full scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                        self.run_extended_test, "channels",
                                        WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT)
        self.log.debug("Full Scan settings:{}\n{}".format(len(scan_settings),
                                                          scan_settings))
        name_func = (lambda scan_setting :
                     "test_single_scan_report_full_scan_for_channels_{}_periodInMs_{}".
                     format(scan_setting["channels"],scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_single_scan_full,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_single_scan_report_full_scan_for_channels"
                          " failed {}").format(len(failed)))

    def test_single_scan_report_full_scan_for_band_with_enumerated_params(self):
        """Test WiFi scanner single scan using band with enumerated settings
           to report full scan results.

         1. Start WifiScanner single scan using different bands with enumerated
            scan settings to report full scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                        self.run_extended_test, "band",
                                        WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT)
        self.log.debug("Full Scan settings:{}\n{}".format(len(scan_settings),
                                                          scan_settings))
        name_func = (lambda scan_setting :
                     "test_single_scan_report_full_scan_for_band_{}_periodInMs_{}".
                     format(scan_setting["band"],scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_single_scan_full,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_single_scan_report_full_scan_for_band"
                          " failed {}").format(len(failed)))

    def test_batch_scan_report_full_scan_for_channels_with_enumerated_params(self):
        """Test WiFi scanner batch scan using channels with enumerated settings
           to report full scan results.

         1. Start WifiScanner batch scan using different channels with enumerated
            scan settings to report full scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                        self.run_extended_test, "channels",
                                        WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT)
        self.log.debug("Full Scan settings:{}\n{}".format(len(scan_settings),
                                                          scan_settings))
        name_func = (lambda scan_setting :
                     ("test_batch_scan_report_full_scan_for_channels"
                      "_{}_periodInMs_{}").format(scan_setting["channels"],
                                                  scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_batch_scan_full,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_batch_scan_report_full_scan_for_channels"
                          " failed {}").format(len(failed)))

    def test_batch_scan_report_full_scan_for_band_with_enumerated_params(self):
        """Test WiFi scanner batch scan using channels with enumerated settings
           to report full scan results.

         1. Start WifiScanner batch scan using different channels with enumerated
            scan settings to report full scan results.
         2. Verify that scan results match with respective scan settings.
        """
        scan_settings = self.wifi_generate_scanner_scan_settings(
                                        self.run_extended_test, "band",
                                        WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT)
        self.log.debug("Full Scan settings:{}\n{}".format(len(scan_settings),
                                                          scan_settings))
        name_func = (lambda scan_setting :
                     ("test_batch_scan_report_full_scan_for_band"
                      "_{}_periodInMs_{}").format(scan_setting["band"],
                                                  scan_setting["periodInMs"]))
        failed = self.run_generated_testcases(self.wifi_scanner_batch_scan_full,
                                              scan_settings,
                                              name_func = name_func)
        asserts.assert_true(not failed,
                         ("Number of test_batch_scan_report_full_scan_for_band"
                          " failed {}").format(len(failed)))

    def test_wifi_connection_while_single_scan(self):
        """Test configuring a connection parallel to wifi scanner single scan.

         1. Start WifiScanner single scan for both band with default scan settings.
         2. Configure a connection to reference network.
         3. Verify that connection to reference network occurred.
         2. Verify that scanner report single scan results.
        """
        self.attenuators[self.connect_network["attenuator"]].set_atten(0)
        data = start_wifi_single_scan(self.dut, self.default_scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi single shot scan started with index: {}".format(idx))
        asserts.assert_true(self.connect_to_reference_network(), NETWORK_ERROR)
        time.sleep(10) #wait for connection to be active
        asserts.assert_true(check_internet_connection(self.dut, self.ping_addr),
                         "Error, No internet connection for current network")
        #generating event wait time from scan setting plus leeway
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                      self.default_scan_setting,
                                                      self.stime_channel)
        wait_time = int(scan_time/1000) + self.leeway
        validity = False
        try:
            event_name = "{}{}onResults".format(EVENT_TAG, idx)
            self.log.debug("Waiting for event: {} for time {}".
                           format(event_name, wait_time))
            event = self.dut.ed.pop_event(event_name, wait_time)
            self.log.debug("Event received: {}".format(event ))
            results = event["data"]["Results"]
            bssids, validity = self.proces_and_valid_batch_scan_result(
                                                      results, scan_rt,
                                                      event["data"][KEY_RET],
                                                      self.default_scan_setting)
            self.log.info("Scan number Buckets: {}\nTotal BSSID: {}".
                                                    format(len(results), bssids))
            asserts.assert_true(len(results) == 1 and bssids >= 1, EMPTY_RESULT)
        except Empty as error:
            raise AssertionError("Event did not triggered for single scan {}".
                                 format(error))

    def test_single_scan_while_pno(self):
        """Test wifi scanner single scan parallel to PNO connection.

         1. Check device have a saved network.
         2. Trigger PNO by attenuate the signal to move out of range.
         3. Start WifiScanner single scan for both band with default scan settings.
         4. Verify that scanner report single scan results.
         5. Attenuate the signal to move in range.
         6. Verify connection occurred through PNO.
        """
        self.log.info("Check connection through PNO for reference network")
        current_network = self.dut.droid.wifiGetConnectionInfo()
        self.log.info("Current network: {}".format(current_network))
        asserts.assert_true('network_id' in current_network, NETWORK_ID_ERROR)
        asserts.assert_true(current_network['network_id'] >= 0, NETWORK_ERROR)
        self.log.info("Kicking PNO for reference network")
        self.attenuators[self.connect_network["attenuator"]].set_atten(90)
        time.sleep(10) #wait for PNO to be kicked
        self.log.info("Starting single scan while PNO")
        self.wifi_scanner_single_scan(self.default_scan_setting)
        self.attenuators[self.connect_network["attenuator"]].set_atten(0)
        self.log.info("Check connection through PNO for reference network")
        time.sleep(30) #wait for connection through PNO
        current_network = self.dut.droid.wifiGetConnectionInfo()
        self.log.info("Current network: {}".format(current_network))
        asserts.assert_true('network_id' in current_network, NETWORK_ID_ERROR)
        asserts.assert_true(current_network['network_id'] >= 0, NETWORK_ERROR)
        time.sleep(10) #wait for IP to be assigned
        asserts.assert_true(check_internet_connection(self.dut, self.ping_addr),
                         "Error, No internet connection for current network")
        wifi_forget_network(self.dut, self.connect_network["ssid"])

    def test_wifi_connection_and_pno_while_batch_scan(self):
        """Test configuring a connection and PNO connection parallel to wifi
           scanner batch scan.

         1. Start WifiScanner batch scan with default batch scan settings.
         2. Wait for scan result event for a time depend on scan settings.
         3. Verify reported batch scan results.
         4. Configure a connection to reference network.
         5. Verify that connection to reference network occurred.
         6. Wait for scan result event for a time depend on scan settings.
         7. Verify reported batch scan results.
         8. Trigger PNO by attenuate the signal to move out of range.
         9. Wait for scan result event for a time depend on scan settings.
         10. Verify reported batch scan results.
         11. Attenuate the signal to move in range.
         12. Verify connection occurred through PNO.
        """
        self.attenuators[self.connect_network["attenuator"]].set_atten(0)
        data = start_wifi_background_scan(self.dut, self.default_batch_scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi background scan started with index: {} rt {}".
                      format(idx, scan_rt))
        #generating event wait time from scan setting plus leeway
        scan_time, scan_channels = get_scan_time_and_channels(
                                                self.wifi_chs,
                                                self.default_batch_scan_setting,
                                                self.stime_channel)
        #default number buckets
        number_bucket = 10
        time_cache = self.default_batch_scan_setting['periodInMs'] * number_bucket #default cache
        #add 2 seconds extra time for switch between the channel for connection scan
        #multiply cache time by two to account for scheduler changing period
        wait_time = (time_cache * 2 + scan_time) / 1000 + self.leeway + 2
        result_flag = 0
        try:
          for snumber in range(1,7):
            event_name = "{}{}onResults".format(EVENT_TAG, idx)
            self.log.info("Waiting for event: {}".format(event_name ))
            event = self.dut.ed.pop_event(event_name, wait_time)
            self.log.debug("Event onResults: {}".format(event ))
            results = event["data"]["Results"]
            bssids, validity = self.proces_and_valid_batch_scan_result(
                                               results, scan_rt,
                                               event["data"][KEY_RET],
                                               self.default_batch_scan_setting)
            self.log.info("Scan number: {}\n Buckets: {}\n BSSID: {}".
                                         format(snumber, len(results), bssids))
            asserts.assert_true(bssids >= 1, "Not able to fetch scan result")
            if snumber == 1:
                self.log.info("Try to connect AP while waiting for event: {}".
                              format(event_name ))
                asserts.assert_true(self.connect_to_reference_network(), NETWORK_ERROR)
                time.sleep(10) #wait for connection to be active
                asserts.assert_true(check_internet_connection(self.dut, self.ping_addr),
                                 "Error, No internet connection for current network")
            elif snumber == 3:
                self.log.info("Kicking PNO for reference network")
                self.attenuators[self.connect_network["attenuator"]].set_atten(90)
            elif snumber == 4:
                self.log.info("Bring back device for PNO connection")
                current_network = self.dut.droid.wifiGetConnectionInfo()
                self.log.info("Current network: {}".format(current_network))
                asserts.assert_true('network_id' in current_network, NETWORK_ID_ERROR)
                asserts.assert_true(current_network['network_id'] == -1,
                                 "Device is still connected to network  {}".
                                 format(current_network[WifiEnums.SSID_KEY]))
                self.attenuators[self.connect_network["attenuator"]].set_atten(0)
                time.sleep(10) #wait for connection to take place before waiting for scan result
            elif snumber == 6:
                self.log.info("Check connection through PNO for reference network")
                current_network = self.dut.droid.wifiGetConnectionInfo()
                self.log.info("Current network: {}".format(current_network))
                asserts.assert_true('network_id' in current_network, NETWORK_ID_ERROR)
                asserts.assert_true(current_network['network_id'] >= 0, NETWORK_ERROR)
                time.sleep(10) #wait for connection to be active
                asserts.assert_true(check_internet_connection(self.dut, self.ping_addr),
                                 "Error, No internet connection for current network")
                wifi_forget_network(self.dut, self.connect_network["ssid"])
        except Empty as error:
            raise AssertionError("Event did not triggered for batch scan {}".
                                 format(error))
        finally:
            self.dut.droid.wifiScannerStopBackgroundScan(idx)
            self.dut.ed.clear_all_events()

    def test_wifi_scanner_single_scan_channel_sanity(self):
        """Test WiFi scanner single scan for mix channel with default setting
           parameters.

         1. Start WifiScanner single scan for mix channels with default setting
            parameters.
         2. Verify that scan results match with respective scan settings.
        """
        scan_setting = { "channels": self.wifi_chs.MIX_CHANNEL_SCAN,
                         "periodInMs": SCANTIME,
                         "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN }
        self.wifi_scanner_single_scan(scan_setting)

    def test_wifi_scanner_batch_scan_channel_sanity(self):
        """Test WiFi scanner batch scan for mix channel with default setting
           parameters to report the result on buffer full.

         1. Start WifiScanner batch scan for mix channels with default setting
            parameters.
         2. Verify that scan results match with respective scan settings.
        """
        scan_setting = { "channels": self.wifi_chs.MIX_CHANNEL_SCAN,
                         "periodInMs": SCANTIME,
                         "reportEvents": WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL}
        self.wifi_scanner_batch_scan(scan_setting)

    def test_wifi_scanner_batch_scan_period_too_short(self):
        """Test WiFi scanner batch scan for band with too short period time.

         1. Start WifiScanner batch scan for both band with interval period as 5s.
         2. Verify that scan is not started."""
        scan_setting = { "band": WifiEnums.WIFI_BAND_BOTH_WITH_DFS,
                         "periodInMs": 5000,
                         "reportEvents": WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL}
        self.start_wifi_scanner_background_scan_expect_failure(scan_setting);

    def test_wifi_scanner_single_scan_in_isolated(self):
        """Test WiFi scanner in isolated environment with default scan settings.

         1. Created isolated environment by attenuating the single by 90db
         2. Start WifiScanner single scan for mix channels with default setting
            parameters.
         3. Verify that empty scan results reported.
        """
        self.attenuators[0].set_atten(90)
        self.attenuators[1].set_atten(90)
        data = start_wifi_single_scan(self.dut, self.default_scan_setting)
        idx = data["Index"]
        scan_rt = data["ScanElapsedRealtime"]
        self.log.info("Wifi single shot scan started with index: {}".format(idx))
        results = []
        #generating event wait time from scan setting plus leeway
        scan_time, scan_channels = get_scan_time_and_channels(self.wifi_chs,
                                                      self.default_scan_setting,
                                                      self.stime_channel)
        wait_time = int(scan_time/1000) + self.leeway
        try:
            event_name = "{}{}onResults".format(EVENT_TAG, idx)
            self.log.debug("Waiting for event: {} for time {}".format(event_name,
                                                                      wait_time))
            event = self.dut.ed.pop_event(event_name, wait_time)
            self.log.debug("Event received: {}".format(event))
            results = event["data"]["Results"]
            bssids, validity = (self.proces_and_valid_batch_scan_result(
                                                      results, scan_rt,
                                                      event["data"][KEY_RET],
                                                      self.default_scan_setting))
            self.log.info("Scan number Buckets: {}\nTotal BSSID: {}".
                          format(len(results), bssids))
            asserts.assert_true(bssids == 0, ("Test fail because report scan "
                                              "results reported are not empty"))
        except Empty as error:
            raise AssertionError("Event did not triggered for in isolated environment {}".
                   format(error))
        finally:
            self.dut.ed.clear_all_events()
            self.attenuators[0].set_atten(0)
            self.attenuators[1].set_atten(0)

    def test_wifi_scanner_with_wifi_off(self):
        """Test WiFi scanner single scan when wifi is off.

         1. Toggle wifi state to off.
         2. Start WifiScanner single scan for both band with default scan settings.
         3. Verify that scan is not started.
        """
        self.log.debug("Make sure wifi is off.")
        wifi_toggle_state(self.dut, False)
        self.start_wifi_scanner_single_scan_expect_failure(self.default_scan_setting)
        self.log.debug("Turning wifi back on.")
        wifi_toggle_state(self.dut, True)

    def test_wifi_scanner_with_invalid_numBssidsPerScan(self):
        """Test WiFi scanner single scan with invalid number of bssids reported
           per scan.

         1. Start WifiScanner single scan with invalid number of bssids reported
            per scan.
         2. Verify that scan results triggered for default supported number of
            bssids per scan.
        """
        scan_setting = {
            "band": WifiEnums.WIFI_BAND_BOTH_WITH_DFS,
            "periodInMs": SCANTIME,
            "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
            'numBssidsPerScan': 33
        }
        self.wifi_scanner_single_scan(scan_setting)
    """ Tests End """

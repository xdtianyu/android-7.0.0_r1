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

import queue
import time

import acts.base_test
import acts.test_utils.wifi.wifi_test_utils as wutils

from acts import asserts

WifiChannelUS = wutils.WifiChannelUS
WifiEnums = wutils.WifiEnums

SCAN_EVENT_TAG = "WifiScannerScan"

class WifiScanResultEvents():
    """This class stores the setting of a scan, parameters generated
    from starting the scan, and events reported later from the scan
    for validation.

    Attributes:
        scan_setting: Setting used to perform the scan.
        scan_channels: Channels used for scanning.
        events: A list to store the scan result events.
    """

    def __init__(self, scan_setting, scan_channels):
        self.scan_setting = scan_setting
        self.scan_channels = scan_channels
        self.results_events = []

    def add_results_event(self, event):
        self.results_events.append(event)

    def check_interval(self, scan_result, scan_result_next):
        """Verifies that the time gap between two consecutive results is within
        expected range.

        Right now it is hard coded to be 20 percent of the interval specified
        by scan settings. This threshold can be imported from the configuration
        file in the future if necessary later.

        Note the scan result timestamps are in microseconds, but "periodInMs"
        in scan settings is in milliseconds.

        Args:
            scan_result: A dictionary representing a scan result for a BSSID.
            scan_result_next: A dictionary representing a scan result for a
                BSSID, whose scan happened after scan_result.
        """
        actual_interval = scan_result_next["timestamp"] - scan_result["timestamp"]
        expected_interval = self.scan_setting['periodInMs'] * 1000
        delta = abs(actual_interval - expected_interval)
        margin = expected_interval * 0.20 # 20% of the expected_interval
        assert delta < margin, ("The difference in time between scan %s and "
                "%s is %dms, which is out of the expected range %sms") % (
                    scan_result,
                    scan_result_next,
                    delta / 1000,
                    self.scan_setting['periodInMs']
                )

    def verify_one_scan_result(self, scan_result):
        """Verifies the scan result of a single BSSID.

        1. Verifies the frequency of the network is within the range requested
        in the scan.

        Args:
            scan_result: A dictionary representing the scan result of a single
                BSSID.
        """
        freq = scan_result["frequency"]
        assert freq in self.scan_channels, ("Frequency %d of "
            "result entry %s is out of the expected range %s.") % (
                freq, scan_result, self.scan_channels)
        # TODO(angli): add RSSI check.

    def verify_one_scan_result_group(self, batch):
        """Verifies a group of scan results obtained during one scan.

        1. Verifies the number of BSSIDs in the batch is less than the
        threshold set by scan settings.
        2. Verifies each scan result for individual BSSID.

        Args:
            batch: A list of dictionaries, each dictionary represents a scan
                result.
        """
        scan_results = batch["ScanResults"]
        actual_num_of_results = len(scan_results)
        expected_num_of_results = self.scan_setting['numBssidsPerScan']
        assert actual_num_of_results <= expected_num_of_results, (
            "Expected no more than %d BSSIDs, got %d.") % (
                expected_num_of_results,
                actual_num_of_results
            )
        for scan_result in scan_results:
            self.verify_one_scan_result(scan_result)

    def have_enough_events(self):
        """Check if there are enough events to properly validate the scan"""
        return len(self.results_events) >= 2

    def check_scan_results(self):
        """Validate the reported scan results against the scan settings.
        Assert if any error detected in the results.

        1. For each scan setting there should be no less than 2 events received.
        2. For batch scan, the number of buffered results in each event should
           be exactly what the scan setting specified.
        3. Each scan result should contain no more BBSIDs than what scan
           setting specified.
        4. The frequency reported by each scan result should comply with its
           scan setting.
        5. The time gap between two consecutive scan results should be
           approximately equal to the scan interval specified by the scan
           setting.
        """
        num_of_events = len(self.results_events)
        assert num_of_events >= 2, (
                "Expected more than one scan result events, got %d."
            ) % num_of_events
        for event_idx in range(num_of_events):
            batches = self.results_events[event_idx]["data"]["Results"]
            actual_num_of_batches = len(batches)
            # For batch scan results.
            report_type = self.scan_setting['reportEvents']
            if not (report_type & WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN):
                # Verifies that the number of buffered results matches the
                # number defined in scan settings.
                expected_num_of_batches = self.scan_setting['maxScansToCache']
                assert actual_num_of_batches <= expected_num_of_batches, (
                    "Expected to get at most %d batches in event No.%d, got %d.") % (
                        expected_num_of_batches,
                        event_idx,
                        actual_num_of_batches
                    )
                # Check the results within each event of batch scan
                for batch_idx in range(1, actual_num_of_batches):
                    self.check_interval(
                        batches[batch_idx-1]["ScanResults"][0],
                        batches[batch_idx]["ScanResults"][0]
                    )
            for batch in batches:
                self.verify_one_scan_result_group(batch)

            # Check the time gap between the first result of an event and
            # the last result of its previous event
            # Skip the very first event.
            if event_idx >= 1:
                previous_batches = self.results_events[event_idx-1]["data"]["Results"]
                self.check_interval(
                    previous_batches[-1]["ScanResults"][0],
                    batches[0]["ScanResults"][0]
                )

class WifiScannerMultiScanTest(acts.base_test.BaseTestClass):
    """This class is the WiFi Scanner Multi-Scan Test suite.
    It collects a number of test cases, sets up and executes
    the tests, and validates the scan results.

    Attributes:
        tests: A collection of tests to excute.
        leeway: Scan interval drift time (in seconds).
        stime_channels: Dwell time plus 2ms.
        dut: Android device(s).
        wifi_chs: WiFi channels according to the device model.
        max_bugreports: Max number of bug reports allowed.
    """

    def __init__(self, controllers):
        acts.base_test.BaseTestClass.__init__(self, controllers)
        # A list of all test cases to be executed in this class.
        self.tests = ("test_wifi_two_scans_at_same_interval",
                      "test_wifi_two_scans_at_different_interval",
                      "test_wifi_scans_24GHz_and_both",
                      "test_wifi_scans_5GHz_and_both",
                      "test_wifi_scans_24GHz_5GHz_and_DFS",
                      "test_wifi_scans_batch_and_24GHz",
                      "test_wifi_scans_batch_and_5GHz",
                      "test_wifi_scans_24GHz_5GHz_full_result",)
        self.leeway = 5 # seconds, for event wait time computation
        self.stime_channel = 47 #dwell time plus 2ms

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        asserts.assert_true(self.dut.droid.wifiIsScannerSupported(),
            "Device %s doesn't support WifiScanner, abort." % self.dut.model)

        """ Setup the required dependencies and fetch the user params from
        config file.
        """
        req_params = ("bssid_2g", "bssid_5g", "bssid_dfs", "max_bugreports")
        self.wifi_chs = WifiChannelUS(self.dut.model)
        self.unpack_userparams(req_params)

    def on_fail(self, test_name, begin_time):
        if self.max_bugreports > 0:
            self.dut.take_bug_report(test_name, begin_time)
            self.max_bugreports -= 1
        self.dut.cat_adb_log(test_name, begin_time)

    """ Helper Functions Begin """
    def start_scan(self, scan_setting):
        data = wutils.start_wifi_background_scan(self.dut, scan_setting)
        idx = data["Index"]
        # Calculate event wait time from scan setting plus leeway
        scan_time, scan_channels = wutils.get_scan_time_and_channels(
                                        self.wifi_chs,
                                        scan_setting,
                                        self.stime_channel
                                    )
        scan_period = scan_setting['periodInMs']
        report_type = scan_setting['reportEvents']
        if report_type & WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN:
            scan_time += scan_period
        else:
            max_scan = scan_setting['maxScansToCache']
            scan_time += max_scan * scan_period
        wait_time = scan_time / 1000 + self.leeway
        return idx, wait_time, scan_channels

    def validate_scan_results(self, scan_results_dict):
        # Sanity check to make sure the dict is not empty
        asserts.assert_true(scan_results_dict, "Scan result dict is empty.")
        for scan_result_obj in scan_results_dict.values():
            # Validate the results received for each scan setting
            scan_result_obj.check_scan_results()

    def wait_for_scan_events(self, wait_time_list, scan_results_dict):
        """Poll for WifiScanner events and record them"""

        # Compute the event wait time
        event_wait_time = min(wait_time_list)

        # Compute the maximum test time that guarantee that even the scan
        # which requires the most wait time will receive at least two
        # results.
        max_wait_time = max(wait_time_list)
        max_end_time = time.monotonic() + max_wait_time
        self.log.debug("Event wait time {} seconds".format(event_wait_time))

        try:
            # Wait for scan results on all the caller specified bands
            event_name = SCAN_EVENT_TAG
            while True:
                self.log.debug("Waiting for events '{}' for up to {} seconds".
                               format(event_name, event_wait_time))
                events = self.dut.ed.pop_events(event_name, event_wait_time)
                for event in events:
                    self.log.debug("Event received: {}".format(event))
                    # Event name is the key to the scan results dictionary
                    actual_event_name = event["name"]
                    asserts.assert_true(actual_event_name in scan_results_dict,
                        ("Expected one of these event names: %s, got '%s'."
                        ) % (scan_results_dict.keys(), actual_event_name))

                    # TODO validate full result callbacks also
                    if event["name"].endswith("onResults"):
                        # Append the event
                        scan_results_dict[actual_event_name].add_results_event(event)

                # If we time out then stop waiting for events.
                if time.monotonic() >= max_end_time:
                    break
                # If enough scan results have been returned to validate the
                # results then break early.
                have_enough_events = True
                for key in scan_results_dict:
                    if not scan_results_dict[key].have_enough_events():
                        have_enough_events = False
                if have_enough_events:
                  break
        except queue.Empty:
            asserts.fail("Event did not trigger for {} in {} seconds".
                      format(event_name, event_wait_time))



    def scan_and_validate_results(self, scan_settings):
        """Perform WifiScanner scans and check the scan results

        Procedures:
          * Start scans for each caller specified setting
          * Wait for at least two results for each scan
          * Check the results received for each scan
        """
        # Awlays get a clean start
        self.dut.ed.clear_all_events()

        # Start scanning with the caller specified settings and
        # compute parameters for receiving events
        idx_list = []
        wait_time_list = []
        scan_results_dict = {}

        try:
            for scan_setting in scan_settings:
                self.log.debug("Scan setting: band {}, interval {}, reportEvents "
                               "{}, numBssidsPerScan {}".format(
                                    scan_setting["band"],
                                    scan_setting["periodInMs"],
                                    scan_setting["reportEvents"],
                                    scan_setting["numBssidsPerScan"]
                                ))
                idx, wait_time, scan_chan = self.start_scan(scan_setting)
                self.log.debug(("Scan started for band {}: idx {}, wait_time {} s,"
                                " scan_channels {}").format(
                                scan_setting["band"], idx, wait_time, scan_chan))
                idx_list.append(idx)
                wait_time_list.append(wait_time)

                report_type = scan_setting['reportEvents']
                scan_results_events = WifiScanResultEvents(scan_setting, scan_chan)
                scan_results_dict["{}{}onResults".format(SCAN_EVENT_TAG, idx)] = scan_results_events
                if (scan_setting['reportEvents'] & WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT):
                    scan_results_dict["{}{}onFullResult".format(SCAN_EVENT_TAG, idx)] = scan_results_events

            self.wait_for_scan_events(wait_time_list, scan_results_dict)

            # Validate the scan results
            self.validate_scan_results(scan_results_dict)

        finally:
            # Tear down and clean up
            for idx in idx_list:
                self.dut.droid.wifiScannerStopBackgroundScan(idx)
            self.dut.ed.clear_all_events()
    """ Helper Functions End """


    """ Tests Begin """
    def test_wifi_two_scans_at_same_interval(self):
        """Perform two WifiScanner background scans, one at 2.4GHz and the other
        at 5GHz, the same interval and number of BSSIDs per scan.

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_24_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24},
                         { "band": WifiEnums.WIFI_BAND_5_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_two_scans_at_different_interval(self):
        """Perform two WifiScanner background scans, one at 2.4GHz and the other
        at 5GHz, different interval and number of BSSIDs per scan.

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_24_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 20},
                         { "band": WifiEnums.WIFI_BAND_5_GHZ,
                           "periodInMs": 20000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_scans_24GHz_and_both(self):
        """Perform two WifiScanner background scans, one at 2.4GHz and
           the other at both 2.4GHz and 5GHz

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_BOTH,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24},
                         { "band": WifiEnums.WIFI_BAND_24_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_scans_5GHz_and_both(self):
        """Perform two WifiScanner scans, one at 5GHz and the other at both
           2.4GHz and 5GHz

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_BOTH,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24},
                         { "band": WifiEnums.WIFI_BAND_5_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_scans_24GHz_5GHz_and_DFS(self):
        """Perform three WifiScanner scans, one at 5GHz, one at 2.4GHz and the
        other at just 5GHz DFS channels

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_5_GHZ_DFS_ONLY,
                           "periodInMs": 20000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24},
                         { "band": WifiEnums.WIFI_BAND_5_GHZ,
                           "periodInMs": 20000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24},
                         { "band": WifiEnums.WIFI_BAND_24_GHZ,
                           "periodInMs": 40000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_scans_batch_and_24GHz(self):
        """Perform two WifiScanner background scans, one in batch mode for both
        bands and the other in periodic mode at 2.4GHz

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of results in batch mode should match the setting
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_BOTH,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL,
                           "numBssidsPerScan": 24,
                           "maxScansToCache": 2},
                         { "band": WifiEnums.WIFI_BAND_24_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_scans_batch_and_5GHz(self):
        """Perform two WifiScanner background scans, one in batch mode for both
        bands and the other in periodic mode at 5GHz

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of results in batch mode should match the setting
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_BOTH,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_BUFFER_FULL,
                           "numBssidsPerScan": 24,
                           "maxScansToCache": 2},
                         { "band": WifiEnums.WIFI_BAND_5_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    def test_wifi_scans_24GHz_5GHz_full_result(self):
        """Perform two WifiScanner background scans, one at 2.4GHz and
           the other at 5GHz. Report full scan results.

        Initial Conditions:
          * Set multiple APs broadcasting 2.4GHz and 5GHz.

        Expected Results:
          * DUT reports success for starting both scans
          * Scan results for each callback contains only the results on the
            frequency scanned
          * Wait for at least two scan results and confirm that separation
            between them approximately equals to the expected interval
          * Number of BSSIDs doesn't exceed
        """
        scan_settings = [{ "band": WifiEnums.WIFI_BAND_24_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT
                           | WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24},
                         { "band": WifiEnums.WIFI_BAND_5_GHZ,
                           "periodInMs": 10000, # ms
                           "reportEvents": WifiEnums.REPORT_EVENT_FULL_SCAN_RESULT
                           | WifiEnums.REPORT_EVENT_AFTER_EACH_SCAN,
                           "numBssidsPerScan": 24}]

        self.scan_and_validate_results(scan_settings)

    """ Tests End """

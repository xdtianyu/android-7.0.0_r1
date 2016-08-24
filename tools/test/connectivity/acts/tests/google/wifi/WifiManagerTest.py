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
import pprint
import queue
import time

import acts.base_test
import acts.signals
import acts.test_utils.wifi.wifi_test_utils as wutils

from acts import asserts

WifiEnums = wutils.WifiEnums
WifiEventNames = wutils.WifiEventNames

class WifiManagerTest(acts.base_test.BaseTestClass):

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = (
            "iot_networks",
            "open_network",
            "iperf_server_address",
            "tdls_models",
            "energy_info_models"
            )
        self.unpack_userparams(req_params)
        asserts.assert_true(len(self.iot_networks) > 0,
            "Need at least one iot network with psk.")
        asserts.assert_true(wutils.wifi_toggle_state(self.dut, True),
            "Failed to turn on wifi before tests.")
        self.iot_networks = self.iot_networks + [self.open_network]
        self.iperf_server = self.iperf_servers[0]

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        self.iperf_server.start()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)
        self.iperf_server.stop()

    def on_fail(self, test_name, begin_time):
        self.dut.cat_adb_log(test_name, begin_time)

    """Helper Functions"""
    def connect_to_wifi_network_with_password(self, params):
        """Connection logic for open and psk wifi networks.

        Logic steps are
        1. Connect to the network.
        2. Run iperf traffic.

        Args:
            params: A tuple of network info and AndroidDevice object.

        Returns:
            True if successful, False otherwise.
        """
        result = False
        wait_time = 5
        network, ad = params
        droid = ad.droid
        ed = ad.ed
        SSID = network[WifiEnums.SSID_KEY]
        try:
            ed.clear_all_events()
            wutils.start_wifi_connection_scan(ad)
            droid.wifiStartTrackingStateChange()
            asserts.assert_true(droid.wifiConnect(network),
                "wifi connect returned false.")
            connect_result = ed.pop_event(WifiEventNames.WIFI_CONNECTED)
            self.log.debug(connect_result)
            result = connect_result['data'][WifiEnums.SSID_KEY] == SSID
            if result:
                self.log.info("Starting iperf traffic through {}".format(SSID))
                time.sleep(wait_time)
                port_arg = "-p {}".format(self.iperf_server.port)
                result, data = ad.run_iperf_client(self.iperf_server_address,
                                                   port_arg)
                self.log.debug(pprint.pformat(data))
        except queue.Empty:
            self.log.exception("Failed to connect to {}".format(SSID))
        finally:
            droid.wifiStopTrackingStateChange()
        return result

    def run_iperf(self, iperf_args):
        if "iperf_server_address" not in self.user_params:
            self.log.error(("Missing iperf_server_address. "
                "Provide one in config."))
        else:
            iperf_addr = self.user_params["iperf_server_address"]
            self.log.info("Running iperf client.")
            result, data = self.dut.run_iperf_client(iperf_addr,
                iperf_args)
            self.log.debug(data)

    def run_iperf_rx_tx(self, time, omit=10):
        args = "-p {} -t {} -O 10".format(self.iperf_server.port, time, omit)
        self.log.info("Running iperf client {}".format(args))
        self.run_iperf(args)
        args = "-p {} -t {} -O 10 -R".format(self.iperf_server.port, time, omit)
        self.log.info("Running iperf client {}".format(args))
        self.run_iperf(args)

    """Tests"""
    def test_toggle_state(self):
        """Test toggling wifi"""
        self.log.debug("Going from on to off.")
        asserts.assert_true(wutils.wifi_toggle_state(self.dut, False),
                         "Failed to turn wifi off.")
        self.log.debug("Going from off to on.")
        asserts.assert_true(wutils.wifi_toggle_state(self.dut, True),
                         "Failed to turn wifi on.")

    def test_toggle_with_screen(self):
        """Test toggling wifi with screen on/off"""
        wait_time = 5
        self.log.debug("Screen from off to on.")
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        time.sleep(wait_time)
        self.log.debug("Going from on to off.")
        try:
            asserts.assert_true(wutils.wifi_toggle_state(self.dut, False),
                             "Failed to turn wifi off.")
            time.sleep(wait_time)
            self.log.debug("Going from off to on.")
            asserts.assert_true(wutils.wifi_toggle_state(self.dut, True),
                             "Failed to turn wifi on.")
        finally:
            self.dut.droid.wakeLockRelease()
            time.sleep(wait_time)
            self.dut.droid.goToSleepNow()

    def test_scan(self):
        """Test wifi connection scan can start and find expected networks."""
        wutils.wifi_toggle_state(self.dut, True)
        self.log.debug("Start regular wifi scan.")
        wutils.start_wifi_connection_scan(self.dut)
        wifi_results = self.dut.droid.wifiGetScanResults()
        self.log.debug("Scan results: %s" % wifi_results)
        ssid = self.open_network[WifiEnums.SSID_KEY]
        condition = {WifiEnums.SSID_KEY: ssid}
        asserts.assert_true(wutils.match_networks(condition, wifi_results),
                         "Can not find expected network %s" % ssid)

    def test_add_network(self):
        """Test wifi connection scan."""
        ssid = self.open_network[WifiEnums.SSID_KEY]
        nId = self.dut.droid.wifiAddNetwork(self.open_network)
        asserts.assert_true(nId > -1, "Failed to add network.")
        configured_networks = self.dut.droid.wifiGetConfiguredNetworks()
        self.log.debug(("Configured networks after adding: %s" %
                        configured_networks))
        condition = {WifiEnums.SSID_KEY: ssid}
        asserts.assert_true(wutils.match_networks(condition, configured_networks),
                         ("Could not find expected network %s in configured "
                          "networks.") % ssid)

    def test_forget_network(self):
        self.test_add_network()
        ssid = self.open_network[WifiEnums.SSID_KEY]
        wutils.wifi_forget_network(self.dut, ssid)
        configured_networks = self.dut.droid.wifiGetConfiguredNetworks()
        for nw in configured_networks:
            asserts.assert_true(nw[WifiEnums.BSSID_KEY] != ssid,
                "Found forgotten network %s in configured networks." % ssid)

    @acts.signals.generated_test
    def test_iot_with_password(self):
        params = list(itertools.product(self.iot_networks, self.android_devices))
        name_gen = lambda p : "test_connection_to-%s" % p[0][WifiEnums.SSID_KEY]
        failed = self.run_generated_testcases(
            self.connect_to_wifi_network_with_password,
            params,
            name_func=name_gen)
        asserts.assert_true(not failed, "Failed ones: {}".format(failed))

    def test_tdls_supported(self):
        model = acts.utils.trim_model_name(self.dut.model)
        self.log.debug("Model is %s" % model)
        if model in self.tdls_models:
            asserts.assert_true(self.dut.droid.wifiIsTdlsSupported(),
                             ("TDLS should be supported on %s, but device is "
                              "reporting not supported.") % model)
        else:
            asserts.assert_true(not self.dut.droid.wifiIsTdlsSupported(),
                             ("TDLS should not be supported on %s, but device "
                              "is reporting supported.") % model)

    def test_energy_info(self):
        """Verify the WiFi energy info reporting feature.

        Steps:
            1. Check that the WiFi energy info reporting support on this device
               is as expected (support or not).
            2. If the device does not support energy info reporting as
               expected, skip the test.
            3. Call API to get WiFi energy info.
            4. Verify the values of "ControllerEnergyUsed" and
               "ControllerIdleTimeMillis" in energy info don't decrease.
            5. Repeat from Step 3 for 10 times.
        """
        # Check if dut supports energy info reporting.
        actual_support = self.dut.droid.wifiIsEnhancedPowerReportingSupported()
        model = self.dut.model
        expected_support = model in self.energy_info_models
        msg = "Expect energy info support to be %s on %s, got %s." % (
              expected_support, model, actual_support)
        asserts.assert_true(actual_support == expected_support, msg)
        if not actual_support:
            asserts.skip(("Device %s does not support energy info reporting as "
                       "expected.") % model)
        # Verify reported values don't decrease.
        self.log.info(("Device %s supports energy info reporting, verify that "
                       "the reported values don't decrease.") % model)
        energy = 0
        idle_time = 0
        for i in range(10):
            info = self.dut.droid.wifiGetControllerActivityEnergyInfo()
            self.log.debug("Iteration %d, got energy info: %s" % (i, info))
            new_energy = info["ControllerEnergyUsed"]
            new_idle_time = info["ControllerIdleTimeMillis"]
            asserts.assert_true(new_energy >= energy,
                "Energy value decreased: previous %d, now %d" % (energy,
                    new_energy))
            energy = new_energy
            asserts.assert_true(new_idle_time >= idle_time,
                "Idle time decreased: previous %d, now %d" % (idle_time,
                    new_idle_time))
            idle_time = new_idle_time
            wutils.start_wifi_connection_scan(self.dut)

    def test_energy_info_connected(self):
        """Verify the WiFi energy info reporting feature when connected.

        Connect to a wifi network, then the same as test_energy_info.
        """
        wutils.wifi_connect(self.dut, self.open_network)
        self.test_energy_info()

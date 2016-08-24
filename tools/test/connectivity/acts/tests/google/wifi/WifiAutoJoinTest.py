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

import time

from acts import asserts
from acts.base_test import BaseTestClass
from acts.test_utils.wifi.wifi_test_utils import wifi_forget_network
from acts.test_utils.wifi.wifi_test_utils import wifi_test_device_init
from acts.test_utils.wifi.wifi_test_utils import WifiEnums
from acts.test_utils.wifi.wifi_test_utils import start_wifi_connection_scan
from acts.test_utils.wifi.wifi_test_utils import check_internet_connection
from acts.test_utils.wifi.wifi_test_utils import track_connection

NETWORK_ID_ERROR = "Network don't have ID"
NETWORK_ERROR = "Device is not connected to reference network"

class WifiAutoJoinTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_autojoin_out_of_range",
            "test_autojoin_Ap1_2g",
            "test_autojoin_Ap1_2gto5g",
            "test_autojoin_in_AP1_5gto2g",
            "test_autojoin_swtich_AP1toAp2",
            "test_autojoin_Ap2_2gto5g",
            "test_autojoin_Ap2_5gto2g",
            "test_autojoin_out_of_range",
            "test_autojoin_Ap2_2g",
            "test_autojoin_Ap2_2gto5g",
            "test_autojoin_in_Ap2_5gto2g",
            "test_autojoin_swtich_AP2toAp1",
            "test_autojoin_Ap1_2gto5g",
            "test_autojoin_Ap1_5gto2g",
            "test_autojoin_swtich_to_blacklist_AP",
            "test_autojoin_in_blacklist_AP",
            "test_autojoin_back_from_blacklist_AP",
            )

    def setup_class(self):
        """It will setup the required dependencies from config file and configure
           the required networks for auto-join testing. Configured networks will
           not be removed. If networks are already configured it will skip
           configuring the networks

        Returns:
            True if successfully configured the requirements for testing.
        """
        self.dut = self.android_devices[0]
        wifi_test_device_init(self.dut)
        req_params = ("reference_networks", "other_network", "atten_val",
                      "ping_addr", "max_bugreports" )
        self.unpack_userparams(req_params)
        self.log.debug("Connect networks :: {}".format(self.other_network))
        configured_networks = self.dut.droid.wifiGetConfiguredNetworks()
        self.log.debug("Configured networks :: {}".format(configured_networks))
        count_confnet = 0
        result = False
        if self.reference_networks[0]['2g']['ssid'] == self.reference_networks[0]['5g']['ssid']:
            self.ref_ssid_count = 1
        else:
            self.ref_ssid_count = 2 # Different SSID for 2g and 5g
        for confnet in configured_networks:
            if confnet[WifiEnums.SSID_KEY] == self.reference_networks[0]['2g']['ssid']:
                count_confnet += 1
            elif confnet[WifiEnums.SSID_KEY] == self.reference_networks[0]['5g']['ssid']:
                count_confnet += 1
        self.log.info("count_confnet {}".format(count_confnet))
        if count_confnet == self.ref_ssid_count:
            return True
        else:
            self.log.info("Configured networks for testing")
            self.attenuators[0].set_atten(0)
            self.attenuators[1].set_atten(90)
            self.attenuators[2].set_atten(90)
            wait_time = 15
            self.dut.droid.wakeLockAcquireBright()
            self.dut.droid.wakeUpNow()
            try:
                self.dut.droid.wifiPriorityConnect(self.reference_networks[0]['2g'])
                connect_result = self.dut.ed.pop_event("WifiManagerPriorityConnectOnSuccess", 1)
                self.log.info(connect_result)
                time.sleep(wait_time)
                if self.ref_ssid_count == 2: #add 5g network as well
                    self.dut.droid.wifiPriorityConnect(self.reference_networks[0]['5g'])
                    connect_result = self.dut.ed.pop_event("WifiManagerPriorityConnectOnSuccess", 1)
                    self.log.info(connect_result)
                    time.sleep(wait_time)
                self.dut.droid.wifiPriorityConnect(self.other_network)
                connect_result = self.dut.ed.pop_event("WifiManagerPriorityConnectOnSuccess")
                self.log.info(connect_result)
                track_connection(self.dut, self.other_network["ssid"], 1)
                wifi_forget_network(self.dut, self.other_network["ssid"])
                time.sleep(wait_time)
                current_network = self.dut.droid.wifiGetConnectionInfo()
                self.log.info("Current network: {}".format(current_network))
                asserts.assert_true('network_id' in current_network, NETWORK_ID_ERROR)
                asserts.assert_true(current_network['network_id'] >= 0, NETWORK_ERROR)
                self.ip_address = self.dut.droid.wifiGetConfigFile();
                self.log.info("IP info: {}".format(self.ip_address))
            finally:
                self.dut.droid.wifiLockRelease()
                self.dut.droid.goToSleepNow()

    def check_connection(self, network_bssid):
        """Check current wifi connection networks.
        Args:
            network_bssid: Network bssid to which connection.
        Returns:
            True if connection to given network happen, else return False.
        """
        time.sleep(40) #time for connection state to be updated
        self.log.info("Check network for {}".format(network_bssid))
        current_network = self.dut.droid.wifiGetConnectionInfo()
        self.log.debug("Current network:  {}".format(current_network))
        if WifiEnums.BSSID_KEY in current_network:
            return current_network[WifiEnums.BSSID_KEY] == network_bssid
        return False

    def set_attn_and_validate_connection(self, attn_value, bssid):
        """Validate wifi connection status on different attenuation setting.

        Args:
            attn_value: Attenuation value for different APs signal.
            bssid: Bssid of excepted network.

        Returns:
            True if bssid of current network match, else false.
        """
        self.attenuators[0].set_atten(attn_value[0])
        self.attenuators[1].set_atten(attn_value[1])
        self.attenuators[2].set_atten(attn_value[2])
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        try:
            asserts.assert_true(self.check_connection(bssid),
                    "Device is not connected to required bssid {}".format(bssid))
            time.sleep(10) #wait for connection to be active
            asserts.assert_true(check_internet_connection(self.dut, self.ping_addr),
                             "Error, No Internet connection for current bssid {}".
                             format(bssid))
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    def on_fail(self, test_name, begin_time):
        if self.max_bugreports > 0:
            self.dut.take_bug_report(test_name, begin_time)
            self.max_bugreports -= 1
        self.dut.cat_adb_log(test_name, begin_time)

    """ Tests Begin """
    def test_autojoin_Ap1_2g(self):
        """Test wifi auto join functionality move in range of AP1.

         1. Attenuate the signal to low range of AP1 and Ap2 not visible at all.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Ap1_2g"]
        variance = 5
        attenuations = ([att0+variance*2, att1, att2], [att0+variance, att1, att2],
                        [att0, att1, att2], [att0-variance, att1, att2])
        name_func = lambda att_value, bssid : ("test_autojoin_Ap1_2g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1], att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[0]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_Ap1_2g failed {}".
                         format(len(failed)))

    def test_autojoin_Ap1_2gto5g(self):
        """Test wifi auto join functionality move to high range.

         1. Attenuate the signal to high range of AP1.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Ap1_2gto5g"]
        variance = 5
        attenuations = ([att0+variance*2, att1, att2], [att0+variance, att1, att2],
                        [att0, att1, att2])
        name_func = lambda att_value, bssid : ("test_autojoin_Ap1_2gto5g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[0]["5g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_Ap1_2gto5g failed {}".
                         format(len(failed)))


    def test_autojoin_in_AP1_5gto2g(self):
        """Test wifi auto join functionality move to low range toward AP2.

         1. Attenuate the signal to medium range of AP1 and low range of AP2.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["In_AP1_5gto2g"]
        variance = 5
        attenuations = ([att0-variance, att1+variance, att2], [att0, att1, att2],
                        [att0+variance, att1-variance, att2])
        name_func = lambda att_value, bssid : ("test_autojoin_in_AP1_5gto2g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[0]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_in_AP1_5gto2g failed {}".
                         format(len(failed)))

    def test_autojoin_swtich_AP1toAp2(self):
        """Test wifi auto join functionality move from low range of AP1 to better
           range of AP2.

         1. Attenuate the signal to low range of AP1 and medium range of AP2.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Swtich_AP1toAp2"]
        variance = 5
        attenuations = ([att0-variance, att1+variance, att2], [att0, att1, att2],
                        [att0+variance, att1-variance, att2])
        name_func = lambda att_value, bssid : ("test_autojoin_swtich_AP1toAp2_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[1]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_swtich_AP1toAp2 failed {}".
                         format(len(failed)))

    def test_autojoin_Ap2_2gto5g(self):
        """Test wifi auto join functionality move to high range of AP2.

         1. Attenuate the signal to out range of AP1 and high range of AP2.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Ap2_2gto5g"]
        variance = 5
        attenuations = ([att0-variance, att1+variance*2, att2],
                        [att0, att1+variance, att2], [att0, att1, att2])
        name_func = lambda att_value, bssid : ("test_autojoin_Ap2_2gto5g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1], att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[1]["5g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_Ap2_2gto5g failed {}".
                         format(len(failed)))

    def test_autojoin_Ap2_5gto2g(self):
        """Test wifi auto join functionality move to low range of AP2.

         1. Attenuate the signal to low range of AP2.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable.
        """
        att0,att1,att2 =  self.atten_val["Ap2_5gto2g"]
        variance = 5
        attenuations = ([att0, att1-variance, att2], [att0, att1, att2],
                        [att0, att1+variance, att2])
        name_func = lambda att_value, bssid : ("test_autojoin_Ap2_5gto2g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1], att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[1]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_Ap2_5gto2g failed {}".
                         format(len(failed)))

    def test_autojoin_out_of_range(self):
        """Test wifi auto join functionality move to low range.

         1. Attenuate the signal to out of range.
         2. Wake up the device.
         3. Start the scan.
         4. Check that device is not connected to any network.
        """
        self.attenuators[0].set_atten(90)
        self.attenuators[1].set_atten(90)
        self.attenuators[2].set_atten(90)
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        try:
            start_wifi_connection_scan(self.dut)
            wifi_results = self.dut.droid.wifiGetScanResults()
            self.log.debug("Scan result {}".format(wifi_results))
            time.sleep(20)
            current_network = self.dut.droid.wifiGetConnectionInfo()
            self.log.info("Current network: {}".format(current_network))
            asserts.assert_true(('network_id' in current_network and
                              current_network['network_id'] == -1),
                             "Device is connected to network {}".format(current_network))
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    def test_autojoin_Ap2_2g(self):
        """Test wifi auto join functionality move in low range of AP2.

         1. Attenuate the signal to move in range of AP2 and Ap1 not visible at all.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Ap2_2g"]
        variance = 5
        attenuations = ([att0,att1+variance*2,att2],
                        [att0,att1+variance,att2],[att0,att1,att2],
                        [att0,att1-variance,att2])
        name_func = lambda att_value, bssid : ("test_autojoin_Ap2_2g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[1]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_Ap2_2g failed {}".
                         format(len(failed)))

    def test_autojoin_in_Ap2_5gto2g(self):
        """Test wifi auto join functionality move to medium range of Ap2 and
           low range of AP1.

         1. Attenuate the signal to move in medium range of AP2 and low range of AP1.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["In_Ap2_5gto2g"]
        variance = 5
        attenuations = ([att0,att1-variance,att2],[att0,att1,att2],
                        [att0,att1+variance,att2])
        name_func = lambda att_value, bssid : ("test_autojoin_in_Ap2_5gto2g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[1]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_in_Ap2_5gto2g failed {}".
                         format(len(failed)))

    def test_autojoin_swtich_AP2toAp1(self):
        """Test wifi auto join functionality move from low range of AP2 to better
           range of AP1.

         1. Attenuate the signal to low range of AP2 and medium range of AP1.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Swtich_AP2toAp1"]
        variance = 5
        attenuations = ([att0+variance,att1-variance,att2],[att0,att1,att2],
                        [att0-variance,att1+variance,att2])
        name_func = lambda att_value, bssid : ("test_autojoin_swtich_AP2toAp1_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[0]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_swtich_AP2toAp1 failed {}".
                         format(len(failed)))

    def test_autojoin_Ap1_5gto2g(self):
        """Test wifi auto join functionality move to medium range of AP1.

         1. Attenuate the signal to medium range of AP1.
         2. Wake up the device.
         3. Check that device is connected to right BSSID and maintain stable
            connection to BSSID in range.
        """
        att0,att1,att2 =  self.atten_val["Ap1_5gto2g"]
        variance = 5
        attenuations = ([att0,att1,att2], [att0+variance,att1,att2],
                        [att0+variance*2,att1,att2])
        name_func = lambda att_value, bssid : ("test_autojoin_Ap1_5gto2g_AP1_{}_AP2"
                     "_{}_AP3_{}").format(att_value[0], att_value[1],att_value[2])
        failed = self.run_generated_testcases(
                        self.set_attn_and_validate_connection,
                        attenuations,
                        args = (self.reference_networks[0]["2g"]['bssid'],),
                        name_func = name_func)
        asserts.assert_true(not failed, "Number of test_autojoin_Ap1_5gto2g failed {}".
                         format(len(failed)))

    def test_autojoin_swtich_to_blacklist_AP(self):
        """Test wifi auto join functionality in medium range of blacklist BSSID.

         1. Attenuate the signal to low range of AP1 and medium range of AP3.
         2. Wake up the device.
         3. Check that device is connected to AP1 BSSID and maintain stable
            connection to BSSID.
        """
        self.set_attn_and_validate_connection(self.atten_val["Swtich_to_blacklist"],
                                              self.reference_networks[0]["2g"]['bssid'])

    def test_autojoin_in_blacklist_AP(self):
        """Test wifi auto join functionality in high range of blacklist BSSID.

         1. Attenuate the signal to out of range of AP1 and full range of AP3.
         2. Wake up the device.
         3. Check that device is disconnected form all AP.
        """
        attn0, attn1, attn2 = self.atten_val["In_blacklist"]
        self.attenuators[0].set_atten(attn0)
        self.attenuators[1].set_atten(attn1)
        self.attenuators[2].set_atten(attn2)
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        try:
            start_wifi_connection_scan(self.dut)
            wifi_results = self.dut.droid.wifiGetScanResults()
            self.log.debug("Scan result {}".format(wifi_results))
            time.sleep(20)
            current_network = self.dut.droid.wifiGetConnectionInfo()
            self.log.info("Current network: {}".format(current_network))
            asserts.assert_true(('network_id' in current_network and
                              current_network['network_id'] == -1),
                             "Device is still connected to blacklisted network {}".
                             format(current_network))
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    def test_autojoin_back_from_blacklist_AP(self):
        """Test wifi auto join functionality in medium range of blacklist BSSID.

         1. Attenuate the signal to medium of range of AP1 and low range of AP3.
         2. Wake up the device.
         3. Check that device is disconnected form all AP.
        """
        self.set_attn_and_validate_connection(self.atten_val["Back_from_blacklist"],
                                              self.reference_networks[0]["2g"]['bssid'])
    """ Tests End """
if __name__ == "__main__":
    pass

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

import pprint
import random
import time

import acts.base_test
import acts.signals
import acts.test_utils.wifi.wifi_test_utils as wutils

from acts import asserts

WifiEnums = wutils.WifiEnums

# EAP Macros
EAP = WifiEnums.Eap
EapPhase2 = WifiEnums.EapPhase2

# Enterprise Config Macros
Ent = WifiEnums.Enterprise

class WifiEnterpriseRoamingTest(acts.base_test.BaseTestClass):

    def __init__(self, controllers):
        acts.base_test.BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_roaming_with_different_auth_method",
        )

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = (
            "ent_roaming_ssid",
            "bssid_a",
            "bssid_b",
            "attn_vals",
            # Expected time within which roaming should finish, in seconds.
            "roam_interval",
            "ca_cert",
            "client_cert",
            "client_key",
            "eap_identity",
            "eap_password",
            "device_password"
        )
        self.unpack_userparams(req_params)
        self.config_peap = {
            Ent.EAP: EAP.PEAP,
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: EapPhase2.MSCHAPV2,
            WifiEnums.SSID_KEY: self.ent_roaming_ssid
        }
        self.config_tls = {
            Ent.EAP: EAP.TLS,
            Ent.CA_CERT: self.ca_cert,
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            Ent.CLIENT_CERT: self.client_cert,
            Ent.PRIVATE_KEY_ID: self.client_key,
            Ent.IDENTITY: self.eap_identity,
        }
        self.config_ttls = {
            Ent.EAP: EAP.TTLS,
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: EapPhase2.MSCHAPV2,
            WifiEnums.SSID_KEY: self.ent_roaming_ssid
        }
        self.config_sim = {
            Ent.EAP: EAP.SIM,
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
        }
        self.attn_a = self.attenuators[0]
        self.attn_b = self.attenuators[1]
        # Set screen lock password so ConfigStore is unlocked.
        self.dut.droid.setDevicePassword(self.device_password)
        self.set_attns("default")

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        self.dut.droid.disableDevicePassword()
        self.dut.ed.clear_all_events()
        self.set_attns("default")

    def setup_test(self):
        self.dut.droid.wifiStartTrackingStateChange()
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()
        return True

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        self.dut.droid.wifiStopTrackingStateChange()
        self.set_attns("default")

    def on_fail(self, test_name, begin_time):
        self.dut.cat_adb_log(test_name, begin_time)

    def set_attns(self, attn_val_name):
        """Sets attenuation values on attenuators used in this test.

        Args:
            attn_val_name: Name of the attenuation value pair to use.
        """
        msg = "Set attenuation values to %s" % self.attn_vals[attn_val_name]
        self.log.info(msg)
        try:
            self.attn_a.set_atten(self.attn_vals[attn_val_name][0])
            self.attn_b.set_atten(self.attn_vals[attn_val_name][1])
        except:
            msg = "Failed to set attenuation values %s." % attn_val_name
            self.log.error(msg)
            raise

    def gen_eap_configs(self):
        """Generates configurations for different EAP authentication types.

        Returns:
            A list of dicts each representing an EAP configuration.
        """
        configs = [self.config_tls]
                   # self.config_sim
        configs += wutils.expand_enterprise_config_by_phase2(self.config_ttls)
        configs += wutils.expand_enterprise_config_by_phase2(self.config_peap)
        return configs

    def gen_eap_roaming_test_name(self, config):
        """Generates a test case name based on an EAP configuration.

        Args:
            config: A dict representing an EAP credential.

        Returns:
            A string representing the name of a generated EAP test case.
        """
        name = "test_roaming-%s" % config[Ent.EAP].name
        if Ent.PHASE2 in config:
            name += "-{}".format(config[Ent.PHASE2].name)
        return name

    def trigger_roaming_and_validate(self, attn_val_name, expected_con):
        """Sets attenuators to trigger roaming and validate the DUT connected
        to the BSSID expected.

        Args:
            attn_val_name: Name of the attenuation value pair to use.
            expected_con: The expected info of the network to we expect the DUT
                to roam to.
        """
        self.set_attns(attn_val_name)
        self.log.info("Wait %ss for roaming to finish." % self.roam_interval)
        time.sleep(self.roam_interval)
        try:
            self.dut.droid.wakeLockAcquireBright()
            self.dut.droid.wakeUpNow()
            wutils.verify_wifi_connection_info(self.dut, expected_con)
            expected_bssid = expected_con[WifiEnums.BSSID_KEY]
            self.log.info("Roamed to %s successfully" % expected_bssid)
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    def roaming_between_a_and_b_logic(self, config):
        """Test roaming between two enterprise APs.

        Steps:
        1. Make bssid_a visible, bssid_b not visible.
        2. Connect to ent_roaming_ssid. Expect DUT to connect to bssid_a.
        3. Make bssid_a not visible, bssid_b visible.
        4. Expect DUT to roam to bssid_b.
        5. Make bssid_a visible, bssid_b not visible.
        6. Expect DUT to roam back to bssid_a.
        """
        expected_con_to_a = {
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            WifiEnums.BSSID_KEY: self.bssid_a,
        }
        expected_con_to_b = {
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            WifiEnums.BSSID_KEY: self.bssid_b,
        }
        self.set_attns("a_on_b_off")
        asserts.assert_true(
            wutils.eap_connect(config, self.dut, validate_con=False),
            "Failed to connect to %s" % config
            )
        wutils.verify_wifi_connection_info(self.dut, expected_con_to_a)
        self.log.info("Roaming from %s to %s" % (self.bssid_a, self.bssid_b))
        self.trigger_roaming_and_validate("b_on_a_off", expected_con_to_b)
        self.log.info("Roaming from %s to %s" % (self.bssid_b, self.bssid_a))
        self.trigger_roaming_and_validate("a_on_b_off", expected_con_to_a)
        return True

    """ Tests Begin """
    @acts.signals.generated_test
    def test_roaming_with_different_auth_method(self):
        eap_configs = self.gen_eap_configs()
        self.log.info("Testing %d different configs." % len(eap_configs))
        random.shuffle(eap_configs)
        failed = self.run_generated_testcases(
            self.roaming_between_a_and_b_logic,
            eap_configs,
            name_func=self.gen_eap_roaming_test_name)
        msg = ("The following configs failed enterprise roaming test: %s" %
               pprint.pformat(failed))
        asserts.assert_true(len(failed) == 0, msg)
    """ Tests End """

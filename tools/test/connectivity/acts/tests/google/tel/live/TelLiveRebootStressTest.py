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
"""
    Test Script for Telephony Pre Check In Sanity
"""

import time
from acts.utils import load_config
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_data_utils import wifi_tethering_setup_teardown
from acts.test_utils.tel.tel_defines import AOSP_PREFIX
from acts.test_utils.tel.tel_defines import CAPABILITY_VOLTE
from acts.test_utils.tel.tel_defines import CAPABILITY_WFC
from acts.test_utils.tel.tel_defines import CAPABILITY_OMADM
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_PROVISIONING
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK
from acts.test_utils.tel.tel_defines import TETHERING_MODE_WIFI
from acts.test_utils.tel.tel_defines import WAIT_TIME_AFTER_REBOOT
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts.test_utils.tel.tel_defines import WFC_MODE_DISABLED
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_lookup_tables import device_capabilities
from acts.test_utils.tel.tel_lookup_tables import operator_capabilities
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import get_model_name
from acts.test_utils.tel.tel_test_utils import get_operator_name
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import sms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import \
    phone_setup_iwlan_cellular_preferred
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_csfb
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte

from acts.utils import rand_ascii_str


class TelLiveRebootStressTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            "test_reboot_stress",
            "test_reboot_stress_without_clear_provisioning"
            )

        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.stress_test_number = int(self.user_params["stress_test_number"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

        self.dut = self.android_devices[0]
        self.ad_reference = self.android_devices[1]
        self.dut_model = get_model_name(self.dut)
        self.dut_operator = get_operator_name(self.log, self.dut)

    def _check_provisioning(self, ad):
        if (CAPABILITY_OMADM in device_capabilities[self.dut_model] and
                CAPABILITY_OMADM in operator_capabilities[self.dut_operator]):
            self.log.info("Check Provisioning bit")
            if not ad.droid.imsIsVolteProvisionedOnDevice():
                self.log.error("{}: VoLTE Not Provisioned on the Platform".format(
                    ad.serial))
                return False
        return True

    def _clear_provisioning(self, ad):
        if (CAPABILITY_OMADM in device_capabilities[self.dut_model] and
                CAPABILITY_OMADM in operator_capabilities[self.dut_operator]):
            self.log.info("Clear Provisioning bit")
            ad.droid.imsSetVolteProvisioning(False)
        return True

    def _check_lte_data(self, ad):
        self.log.info("Check LTE data.")
        if not phone_setup_csfb(self.log, ad):
            self.log.error("Failed to setup LTE data.")
            return False
        if not verify_http_connection(self.log, ad):
            self.log.error("Data not available on cell.")
            return False
        return True

    def _check_volte(self, ad, ad_reference):
        if (CAPABILITY_VOLTE in device_capabilities[self.dut_model] and
                CAPABILITY_VOLTE in operator_capabilities[self.dut_operator]):
            self.log.info("Check VoLTE")
            if not phone_setup_volte(self.log, ad):
                self.log.error("Failed to setup VoLTE.")
                return False
            if not call_setup_teardown(self.log, ad, ad_reference, ad,
                                       is_phone_in_call_volte):
                self.log.error("VoLTE Call Failed.")
                return False
            if not sms_send_receive_verify(self.log, ad, ad_reference,
                                           [rand_ascii_str(50)]):
                self.log.error("SMS failed")
                return False
        return True

    def _check_wfc(self, ad, ad_reference):
        if (CAPABILITY_WFC in device_capabilities[self.dut_model] and
                CAPABILITY_WFC in operator_capabilities[self.dut_operator]):
            self.log.info("Check WFC")
            if not phone_setup_iwlan(self.log, ad, True, WFC_MODE_WIFI_PREFERRED,
                self.wifi_network_ssid, self.wifi_network_pass):
                self.log.error("Failed to setup WFC.")
                return False
            if not call_setup_teardown(self.log, ad, ad_reference, ad,
                                       is_phone_in_call_iwlan):
                self.log.error("WFC Call Failed.")
                return False
            if not sms_send_receive_verify(self.log, ad, ad_reference,
                                           [rand_ascii_str(50)]):
                self.log.error("SMS failed")
                return False
        return True

    def _check_3g(self, ad, ad_reference):
        self.log.info("Check 3G data and CS call")
        if not phone_setup_voice_3g(self.log, ad):
            self.log.error("Failed to setup 3G")
            return False
        if not verify_http_connection(self.log, ad):
            self.log.error("Data not available on cell.")
            return False
        if not call_setup_teardown(self.log, ad, ad_reference, ad,
                                   is_phone_in_call_3g):
            self.log.error("WFC Call Failed.")
            return False
        if not sms_send_receive_verify(self.log, ad, ad_reference,
                                       [rand_ascii_str(50)]):
            self.log.error("SMS failed")
            return False
        return True

    def _check_tethering(self, ad, ad_reference):
        self.log.info("Check tethering")
        if not ad.droid.carrierConfigIsTetheringModeAllowed(
            TETHERING_MODE_WIFI, MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK):
            self.log.error("Tethering Entitlement check failed.")
            return False
        if not wifi_tethering_setup_teardown(self.log, ad, [ad_reference],
            check_interval = 5, check_iteration = 1):
            self.log.error("Tethering Failed.")
            return False
        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_reboot_stress(self):
        """Reboot Reliability Test

        Steps:
            1. Reboot DUT.
            2. Check Provisioning bit (if support provisioning)
            3. Wait for DUT to camp on LTE, Verify Data.
            4. Enable VoLTE, check IMS registration. Wait for DUT report VoLTE
                enabled, make VoLTE call. And verify VoLTE SMS.
                (if support VoLTE)
            5. Connect WiFi, enable WiFi Calling, wait for DUT report WiFi
                Calling enabled and make a WFC call and verify SMS.
                Disconnect WiFi. (if support WFC)
            6. Wait for DUT to camp on 3G, Verify Data.
            7. Make CS call and verify SMS.
            8. Verify Tethering Entitlement Check and Verify WiFi Tethering.
            9. Check crashes.
            10. Repeat Step 1~9 for N times. (before reboot, clear Provisioning
                bit if provisioning is supported)

        Expected Results:
            No crash happens in stress test.

        Returns:
            True is pass, False if fail.
        """
        CHECK_INTERVAL = 10

        toggle_airplane_mode(self.log, self.dut, False)
        phone_setup_voice_general(self.log, self.ad_reference)

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Reboot Stress Test Iteration: <{}> / <{}>".
                format(i, self.stress_test_number))

            self.log.info("{} reboot!".format(self.dut.serial))
            self.dut.reboot()
            self.log.info("{} wait {}s for radio up.".format(
                self.dut.serial, WAIT_TIME_AFTER_REBOOT))
            time.sleep(WAIT_TIME_AFTER_REBOOT)

            elapsed_time = 0
            provisioned = False
            while(elapsed_time < MAX_WAIT_TIME_PROVISIONING):
                if self._check_provisioning(self.dut):
                    provisioned = True
                    break
                else:
                    time.sleep(CHECK_INTERVAL)
                    elapsed_time += CHECK_INTERVAL
            if not provisioned:
                self.log.error("Provisioning fail.")
                return False

            if not self._check_lte_data(self.dut):
                self.log.error("LTE Data fail.")
                return False

            if not self._check_volte(self.dut, self.ad_reference):
                self.log.error("VoLTE fail.")
                return False

            if not self._check_wfc(self.dut, self.ad_reference):
                self.log.error("WFC fail.")
                return False

            if not self._check_3g(self.dut, self.ad_reference):
                self.log.error("3G fail.")
                return False

            if not self._check_tethering(self.dut, self.ad_reference):
                self.log.error("Tethering fail.")
                return False

            self._clear_provisioning(self.dut)

            # TODO: Check if crash happens.

            self.log.info("Iteration: <{}> / <{}> Pass".
                format(i, self.stress_test_number))

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_reboot_stress_without_clear_provisioning(self):
        """Reboot Reliability Test without Clear Provisioning

        Steps:
            1. Reboot DUT.
            2. Check Provisioning bit (if support provisioning)
            3. Wait for DUT to camp on LTE, Verify Data.
            4. Enable VoLTE, check IMS registration. Wait for DUT report VoLTE
                enabled, make VoLTE call. And verify VoLTE SMS.
                (if support VoLTE)
            5. Connect WiFi, enable WiFi Calling, wait for DUT report WiFi
                Calling enabled and make a WFC call and verify SMS.
                Disconnect WiFi. (if support WFC)
            6. Wait for DUT to camp on 3G, Verify Data.
            7. Make CS call and verify SMS.
            8. Verify Tethering Entitlement Check and Verify WiFi Tethering.
            9. Check crashes.
            10. Repeat Step 1~9 for N times.

        Expected Results:
            No crash happens in stress test.

        Returns:
            True is pass, False if fail.
        """

        toggle_airplane_mode(self.log, self.dut, False)
        phone_setup_voice_general(self.log, self.ad_reference)

        for i in range(1, self.stress_test_number + 1):
            self.log.info("Reboot Stress Test Iteration: <{}> / <{}>".
                format(i, self.stress_test_number))

            self.log.info("{} reboot!".format(self.dut.serial))
            self.dut.reboot()
            self.log.info("{} wait {}s for radio up.".format(
                self.dut.serial, WAIT_TIME_AFTER_REBOOT))
            time.sleep(WAIT_TIME_AFTER_REBOOT)

            if not self._check_provisioning(self.dut):
                self.log.error("Provisioning fail.")
                return False

            if not self._check_lte_data(self.dut):
                self.log.error("LTE Data fail.")
                return False

            if not self._check_volte(self.dut, self.ad_reference):
                self.log.error("VoLTE fail.")
                return False

            if not self._check_wfc(self.dut, self.ad_reference):
                self.log.error("WFC fail.")
                return False

            if not self._check_3g(self.dut, self.ad_reference):
                self.log.error("3G fail.")
                return False

            if not self._check_tethering(self.dut, self.ad_reference):
                self.log.error("Tethering fail.")
                return False

            # TODO: Check if crash happens.

            self.log.info("Iteration: <{}> / <{}> Pass".
                format(i, self.stress_test_number))

        return True

""" Tests End """

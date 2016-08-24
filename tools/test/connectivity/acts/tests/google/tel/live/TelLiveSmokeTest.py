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
    Test Script for Telephony Smoke Test
"""

import time
from acts.keys import Config
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_data_utils import airplane_mode_test
from acts.test_utils.tel.tel_data_utils import wifi_cell_switching
from acts.test_utils.tel.tel_data_utils import wifi_tethering_setup_teardown
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK
from acts.test_utils.tel.tel_defines import TETHERING_MODE_WIFI
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_lookup_tables import is_rat_svd_capable
from acts.test_utils.tel.tel_test_utils import WifiUtils
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts.test_utils.tel.tel_test_utils import get_network_rat
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import sms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.utils import load_config
from acts.utils import rand_ascii_str

SKIP = 'Skip'

class TelLiveSmokeTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            "test_smoke_volte_call_data_sms",
            "test_smoke_csfb_3g_call_data_sms",
            "test_smoke_3g_call_data_sms",
            "test_smoke_wfc_call_sms",
            "test_smoke_data_airplane_mode_network_switch_tethering"
            )

        self.simconf = load_config(self.user_params["sim_conf_file"])

        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]
        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

    """ Tests Begin """
    @TelephonyBaseTest.tel_test_wrap
    def test_smoke_volte_call_data_sms(self):
        try:
            ads = self.android_devices
            sms_idle_result = False
            sms_incall_result = False
            data_idle_result = False
            data_incall_result = False
            call_result = False

            self.log.info("--------start test_smoke_volte_call_data_sms--------")
            ensure_phones_default_state(self.log, ads)
            tasks = [(phone_setup_volte, (self.log, ads[0])),
                     (phone_setup_volte, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up VoLTE.")
                return False

            # This is to reduce call fail in VoLTE mode.
            # TODO: b/26338170 remove sleep, use proper API to check DUT status.
            time.sleep(10)

            self.log.info("1. SMS in LTE idle.")
            sms_idle_result = sms_send_receive_verify(self.log, ads[0], ads[1],
                                                      [rand_ascii_str(50)])

            self.log.info("2. Data in LTE idle.")
            if (wait_for_cell_data_connection(self.log, ads[0], True) and
                    verify_http_connection(self.log, ads[0])):
                data_idle_result = True

            self.log.info("3. Setup VoLTE Call.")
            if not call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[1],
                    ad_hangup=None,
                    verify_caller_func=is_phone_in_call_volte,
                    verify_callee_func=is_phone_in_call_volte,
                    wait_time_in_call=WAIT_TIME_IN_CALL_FOR_IMS):
                self.log.error("Setup VoLTE Call Failed.")
                return False

            self.log.info("4. Verify SMS in call.")
            sms_incall_result = sms_send_receive_verify(self.log, ads[0], ads[1],
                                                        [rand_ascii_str(51)])

            self.log.info("5. Verify Data in call.")
            if (wait_for_cell_data_connection(self.log, ads[0], True) and
                    verify_http_connection(self.log, ads[0])):
                data_incall_result = True

            self.log.info("6. Verify Call not drop and hangup.")
            if (is_phone_in_call_volte(self.log, ads[0]) and
                    is_phone_in_call_volte(self.log, ads[1]) and
                    hangup_call(self.log, ads[0])):
                call_result = True

            return (sms_idle_result and data_idle_result and call_result and
                    sms_incall_result and data_incall_result)
        finally:
            self.log.info(
                "Summary for test run. Testbed:<{}>. <VoLTE> SMS idle: {}, "
                "Data idle: {}, SMS in call: {}, Data in call: {}, "
                "Voice Call: {}".format(
                    getattr(self, Config.ikey_testbed_name.value),
                    sms_idle_result, data_idle_result,
                    sms_incall_result, data_incall_result,
                    call_result))

    @TelephonyBaseTest.tel_test_wrap
    def test_smoke_csfb_3g_call_data_sms(self):
        try:
            ads = self.android_devices
            sms_idle_result = False
            sms_incall_result = False
            data_idle_result = False
            data_incall_result = False
            call_result = False

            self.log.info("--------start test_smoke_csfb_3g_call_data_sms--------")
            ensure_phones_default_state(self.log, ads)
            tasks = [(phone_setup_csfb, (self.log, ads[0])),
                     (phone_setup_csfb, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up CSFB_3G.")
                return False

            # This is to reduce SMS send failure in CSFB mode.
            # TODO: b/26338170 remove sleep, use proper API to check DUT status.
            time.sleep(10)

            self.log.info("1. SMS in LTE idle (no IMS).")
            sms_idle_result = sms_send_receive_verify(self.log, ads[0], ads[1],
                                                      [rand_ascii_str(50)])

            self.log.info("2. Data in LTE idle (no IMS).")
            if (wait_for_cell_data_connection(self.log, ads[0], True) and
                    verify_http_connection(self.log, ads[0])):
                data_idle_result = True

            self.log.info("3. Setup CSFB_3G Call.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[1],
                                       ad_hangup=None,
                                       verify_caller_func=is_phone_in_call_csfb,
                                       verify_callee_func=is_phone_in_call_csfb):
                self.log.error("Setup CSFB_3G Call Failed.")
                return False

            self.log.info("4. Verify SMS in call.")
            sms_incall_result = sms_send_receive_verify(self.log, ads[0], ads[1],
                                                        [rand_ascii_str(51)])

            self.log.info("5. Verify Data in call.")
            if is_rat_svd_capable(get_network_rat(self.log, ads[0],
                                                  NETWORK_SERVICE_VOICE)):
                if (wait_for_cell_data_connection(self.log, ads[0], True) and
                        verify_http_connection(self.log, ads[0])):
                    data_incall_result = True
            else:
                self.log.info("Data in call not supported on current RAT."
                              "Skip Data verification.")
                data_incall_result = SKIP

            self.log.info("6. Verify Call not drop and hangup.")
            if (is_phone_in_call_csfb(self.log, ads[0]) and
                    is_phone_in_call_csfb(self.log, ads[1]) and
                    hangup_call(self.log, ads[0])):
                call_result = True

            return (sms_idle_result and data_idle_result and call_result and
                    sms_incall_result and ((data_incall_result is True) or
                                           (data_incall_result == SKIP)))
        finally:
            self.log.info(
                "Summary for test run. Testbed:<{}>. <3G+CSFB> SMS idle: {}, "
                "Data idle: {}, SMS in call: {}, Data in call: {}, "
                "Voice Call: {}".format(
                    getattr(self, Config.ikey_testbed_name.value),
                    sms_idle_result, data_idle_result,
                    sms_incall_result, data_incall_result,
                    call_result))

    @TelephonyBaseTest.tel_test_wrap
    def test_smoke_3g_call_data_sms(self):
        try:
            ads = self.android_devices
            sms_idle_result = False
            sms_incall_result = False
            data_idle_result = False
            data_incall_result = False
            call_result = False

            self.log.info("--------start test_smoke_3g_call_data_sms--------")
            ensure_phones_default_state(self.log, ads)
            tasks = [(phone_setup_voice_3g, (self.log, ads[0])),
                     (phone_setup_voice_3g, (self.log, ads[1]))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up 3G.")
                return False
            self.log.info("1. SMS in 3G idle.")
            sms_idle_result = sms_send_receive_verify(self.log, ads[0], ads[1],
                                                      [rand_ascii_str(50)])

            self.log.info("2. Data in 3G idle.")
            if (wait_for_cell_data_connection(self.log, ads[0], True) and
                    verify_http_connection(self.log, ads[0])):
                data_idle_result = True

            self.log.info("3. Setup 3G Call.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[1],
                                       ad_hangup=None,
                                       verify_caller_func=is_phone_in_call_3g,
                                       verify_callee_func=is_phone_in_call_3g):
                self.log.error("Setup 3G Call Failed.")
                return False

            self.log.info("4. Verify SMS in call.")
            sms_incall_result = sms_send_receive_verify(self.log, ads[0], ads[1],
                                                        [rand_ascii_str(51)])

            self.log.info("5. Verify Data in call.")
            if is_rat_svd_capable(get_network_rat(self.log, ads[0],
                                                  NETWORK_SERVICE_VOICE)):
                if (wait_for_cell_data_connection(self.log, ads[0], True) and
                        verify_http_connection(self.log, ads[0])):
                    data_incall_result = True
            else:
                self.log.info("Data in call not supported on current RAT."
                              "Skip Data verification.")
                data_incall_result = SKIP

            self.log.info("6. Verify Call not drop and hangup.")
            if (is_phone_in_call_3g(self.log, ads[0]) and
                    is_phone_in_call_3g(self.log, ads[1]) and
                    hangup_call(self.log, ads[0])):
                call_result = True

            return (sms_idle_result and data_idle_result and call_result and
                    sms_incall_result and ((data_incall_result is True) or
                                           (data_incall_result == SKIP)))
        finally:
            self.log.info(
                "Summary for test run. Testbed:<{}>. <3G> SMS idle: {}, "
                "Data idle: {}, SMS in call: {}, Data in call: {}, "
                "Voice Call: {}".format(
                    getattr(self, Config.ikey_testbed_name.value),
                    sms_idle_result, data_idle_result,
                    sms_incall_result, data_incall_result,
                    call_result))

    @TelephonyBaseTest.tel_test_wrap
    def test_smoke_wfc_call_sms(self):
        ads = self.android_devices
        sms_idle_result = False
        sms_incall_result = False
        call_result = False

        self.log.info("--------start test_smoke_wfc_call_sms--------")
        for ad in [ads[0], ads[1]]:
            if not ad.droid.imsIsWfcEnabledByPlatform():
                self.log.info("WFC not supported by platform.")
                return True
        try:
            ensure_phones_default_state(self.log, ads)
            tasks = [(phone_setup_iwlan,
                      (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass)),
                     (phone_setup_iwlan,
                      (self.log, ads[1], True, WFC_MODE_WIFI_PREFERRED,
                       self.wifi_network_ssid, self.wifi_network_pass))]
            if not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up WiFI Calling.")
                return False

            self.log.info("1. Verify SMS in idle.")
            if sms_send_receive_verify(self.log, ads[0], ads[1],
                                       [rand_ascii_str(50)]):
                sms_idle_result = True

            self.log.info("2. Setup WiFi Call.")
            if not call_setup_teardown(self.log,
                                       ads[0],
                                       ads[1],
                                       ad_hangup=None,
                                       verify_caller_func=is_phone_in_call_iwlan,
                                       verify_callee_func=is_phone_in_call_iwlan):
                self.log.error("Setup WiFi Call Failed.")
                self.log.info("sms_idle_result:{}".format(sms_idle_result))
                return False

            self.log.info("3. Verify SMS in call.")
            if sms_send_receive_verify(self.log, ads[0], ads[1],
                                       [rand_ascii_str(51)]):
                sms_incall_result = True

            self.log.info("4. Verify Call not drop and hangup.")
            if (is_phone_in_call_iwlan(self.log, ads[0]) and
                    is_phone_in_call_iwlan(self.log, ads[1]) and
                    hangup_call(self.log, ads[0])):
                call_result = True

            return (call_result and sms_idle_result and sms_incall_result)
        finally:
            self.log.info(
                "Summary for test run. Testbed:<{}>. <WFC> SMS idle: {}, "
                "SMS in call: {}, Voice Call: {}".format(
                    getattr(self, Config.ikey_testbed_name.value),
                    sms_idle_result, sms_incall_result,
                    call_result))

    @TelephonyBaseTest.tel_test_wrap
    def test_smoke_data_airplane_mode_network_switch_tethering(self):
        try:
            ads = self.android_devices
            apm_result = False
            nw_switch_result = False
            tethering_result = False

            self.log.info("--------start test_smoke_data_airplane_mode_network"
                "_switch_tethering--------")
            ensure_phones_default_state(self.log, ads)
            self.log.info("1. Verify toggle airplane mode.")
            apm_result = airplane_mode_test(self.log, ads[0])
            self.log.info("2. Verify LTE-WiFi network switch.")
            nw_switch_result = wifi_cell_switching(self.log, ads[0],
                                                   self.wifi_network_ssid,
                                                   self.wifi_network_pass, GEN_4G)
            if ads[0].droid.carrierConfigIsTetheringModeAllowed(
                    TETHERING_MODE_WIFI, MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK):
                self.log.info("3. Verify WiFi Tethering.")
                if ads[0].droid.wifiIsApEnabled():
                    WifiUtils.stop_wifi_tethering(self.log, ads[0])
                tethering_result = wifi_tethering_setup_teardown(
                    self.log,
                    ads[0],
                    [ads[1]],
                    ap_band=WifiUtils.WIFI_CONFIG_APBAND_2G,
                    check_interval=10,
                    check_iteration=4)
                # check_interval=10, check_iteration=4: in this Smoke test,
                # check tethering connection for 4 times, each time delay 10s,
                # to provide a reasonable check_time (10*4=40s) and also reduce test
                # execution time. In regular test, check_iteration is set to 10.
            else:
                self.log.info("3. Skip WiFi Tethering."
                              "Tethering not allowed on SIM.")
                tethering_result = SKIP

            return (apm_result and nw_switch_result and
                    ((tethering_result is True) or (tethering_result == SKIP)))
        finally:
            self.log.info(
                "Summary for test run. Testbed:<{}>. <Data> Airplane Mode: {}, "
                "WiFi-Cell Network Switch: {}, Tethering: {}".format(
                    getattr(self, Config.ikey_testbed_name.value),
                    apm_result, nw_switch_result, tethering_result))

    """ Tests End """

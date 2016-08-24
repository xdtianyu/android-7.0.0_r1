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

import math
import os
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import ensure_phone_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts.test_utils.tel.tel_test_utils import is_wfc_enabled
from acts.test_utils.tel.tel_test_utils import set_phone_screen_on
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import verify_incall_state
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_idle_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.utils import create_dir
from acts.utils import disable_doze
from acts.utils import get_current_human_time
from acts.utils import set_adaptive_brightness
from acts.utils import set_ambient_display
from acts.utils import set_auto_rotate
from acts.utils import set_location_service
from acts.utils import set_mobile_data_always_on

# Monsoon output Voltage in V
MONSOON_OUTPUT_VOLTAGE = 4.2
# Monsoon output max current in A
MONSOON_MAX_CURRENT = 7.8

# Default power test pass criteria
DEFAULT_POWER_PASS_CRITERIA = 999

# Sampling rate in Hz
ACTIVE_CALL_TEST_SAMPLING_RATE = 100
# Sample duration in seconds
ACTIVE_CALL_TEST_SAMPLE_TIME = 300
# Offset time in seconds
ACTIVE_CALL_TEST_OFFSET_TIME = 180

# Sampling rate in Hz
IDLE_TEST_SAMPLING_RATE = 100
# Sample duration in seconds
IDLE_TEST_SAMPLE_TIME = 2400
# Offset time in seconds
IDLE_TEST_OFFSET_TIME = 360

# For wakeup ping test, the frequency to wakeup. In unit of second.
WAKEUP_PING_TEST_WAKEUP_FREQ = 60

WAKEUP_PING_TEST_NUMBER_OF_ALARM = math.ceil(
    (IDLE_TEST_SAMPLE_TIME * 60 + IDLE_TEST_OFFSET_TIME) /
    WAKEUP_PING_TEST_WAKEUP_FREQ)


class TelPowerTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            # Note: For all these power tests, please do environment calibration
            # and baseline for pass criteria.
            # All pass criteria information should be included in test config file.
            # The test result will be meaning less if pass criteria is not correct.
            "test_power_active_call_3g",
            "test_power_active_call_volte",
            "test_power_active_call_wfc_2g_apm",
            "test_power_active_call_wfc_2g_lte_volte_on",
            "test_power_active_call_wfc_5g_apm",
            "test_power_active_call_wfc_5g_lte_volte_on",
            "test_power_idle_baseline",
            "test_power_idle_baseline_wifi_connected",
            "test_power_idle_wfc_2g_apm",
            "test_power_idle_wfc_2g_lte",
            "test_power_idle_lte_volte_enabled",
            "test_power_idle_lte_volte_disabled",
            "test_power_idle_3g",
            "test_power_idle_lte_volte_enabled_wakeup_ping",
            "test_power_idle_lte_volte_disabled_wakeup_ping",
            "test_power_idle_3g_wakeup_ping",

            # Mobile Data Always On
            "test_power_mobile_data_always_on_lte",
            "test_power_mobile_data_always_on_wcdma",
            "test_power_mobile_data_always_on_gsm",
            "test_power_mobile_data_always_on_1x",
            "test_power_mobile_data_always_on_lte_wifi_on",
            "test_power_mobile_data_always_on_wcdma_wifi_on",
            "test_power_mobile_data_always_on_gsm_wifi_on",
            "test_power_mobile_data_always_on_1x_wifi_on"
            )

    def setup_class(self):
        super().setup_class()
        self.mon = self.monsoons[0]
        self.mon.set_voltage(MONSOON_OUTPUT_VOLTAGE)
        self.mon.set_max_current(MONSOON_MAX_CURRENT)
        # Monsoon phone
        self.mon.dut = self.ad = self.android_devices[0]
        set_adaptive_brightness(self.ad, False)
        set_ambient_display(self.ad, False)
        set_auto_rotate(self.ad, False)
        set_location_service(self.ad, False)
        # This is not needed for AOSP build
        disable_doze(self.ad)
        set_phone_screen_on(self.log, self.ad, 15)

        self.wifi_network_ssid_2g = self.user_params["wifi_network_ssid_2g"]
        self.wifi_network_pass_2g = self.user_params["wifi_network_pass_2g"]
        self.wifi_network_ssid_5g = self.user_params["wifi_network_ssid_5g"]
        self.wifi_network_pass_5g = self.user_params["wifi_network_pass_5g"]

        self.monsoon_log_path = os.path.join(self.log_path, "MonsoonLog")
        create_dir(self.monsoon_log_path)
        return True

    def _save_logs_for_power_test(self, test_name, monsoon_result):
        current_time = get_current_human_time()
        file_name = "{}_{}".format(test_name, current_time)
        if "monsoon_log_for_power_test" in self.user_params:
            monsoon_result.save_to_text_file(
                [monsoon_result],
                os.path.join(self.monsoon_log_path, file_name))
        if "bug_report_for_power_test" in self.user_params:
            self.android_devices[0].take_bug_report(test_name, current_time)

    def _test_power_active_call(self,
                                test_name,
                                test_setup_func,
                                pass_criteria=DEFAULT_POWER_PASS_CRITERIA,
                                phone_check_func_after_power_test=None,
                                *args,
                                **kwargs):
        average_current = 0
        try:
            ensure_phone_default_state(self.log, self.android_devices[0])
            ensure_phone_default_state(self.log, self.android_devices[1])
            if not phone_setup_voice_general(self.log,
                                             self.android_devices[1]):
                self.log.error("PhoneB Failed to Set Up Properly.")
                return False
            if not test_setup_func(self.android_devices[0], *args, **kwargs):
                self.log.error("DUT Failed to Set Up Properly.")
                return False
            result = self.mon.measure_power(
                ACTIVE_CALL_TEST_SAMPLING_RATE,
                ACTIVE_CALL_TEST_SAMPLE_TIME,
                test_name,
                ACTIVE_CALL_TEST_OFFSET_TIME
                )
            self._save_logs_for_power_test(test_name, result)
            average_current = result.average_current
            if not verify_incall_state(self.log, [self.android_devices[0],
                                                  self.android_devices[1]],
                                       True):
                self.log.error("Call drop during power test.")
                return False
            if ((phone_check_func_after_power_test is not None) and
                (not phone_check_func_after_power_test(
                    self.log, self.android_devices[0]))):
                self.log.error(
                    "Phone is not in correct status after power test.")
                return False
            return (average_current <= pass_criteria)
        finally:
            self.android_devices[1].droid.telecomEndCall()
            self.log.info("Result: {} mA, pass criteria: {} mA".format(
                average_current, pass_criteria))

    def _test_power_idle(self,
                         test_name,
                         test_setup_func,
                         pass_criteria=DEFAULT_POWER_PASS_CRITERIA,
                         phone_check_func_after_power_test=None,
                         *args,
                         **kwargs):
        average_current = 0
        try:
            ensure_phone_default_state(self.log, self.android_devices[0])
            if not test_setup_func(self.android_devices[0], *args, **kwargs):
                self.log.error("DUT Failed to Set Up Properly.")
                return False
            result = self.mon.measure_power(
                IDLE_TEST_SAMPLING_RATE,
                IDLE_TEST_SAMPLE_TIME,
                test_name,
                IDLE_TEST_OFFSET_TIME
                )
            self._save_logs_for_power_test(test_name, result)
            average_current = result.average_current
            if ((phone_check_func_after_power_test is not None) and
                (not phone_check_func_after_power_test(
                    self.log, self.android_devices[0]))):
                self.log.error(
                    "Phone is not in correct status after power test.")
                return False
            return (average_current <= pass_criteria)
        finally:
            self.log.info("Result: {} mA, pass criteria: {} mA".format(
                average_current, pass_criteria))

    def _start_alarm(self):
        alarm_id = self.ad.droid.phoneStartRecurringAlarm(
            WAKEUP_PING_TEST_NUMBER_OF_ALARM,
            1000 * WAKEUP_PING_TEST_WAKEUP_FREQ, "PING_GOOGLE", None)
        if alarm_id is None:
            self.log.error("Start alarm failed.")
            return False
        return True

    def _setup_phone_idle_and_wakeup_ping(self, ad, phone_setup_func):
        if not phone_setup_func(self.log, ad):
            self.log.error("Phone failed to setup {}.".format(
                phone_setup_func.__name__))
            return False
        if not self._start_alarm():
            return False
        ad.droid.goToSleepNow()
        return True

    def _setup_phone_mobile_data_always_on(self, ad, phone_setup_func,
                                           connect_wifi,
                                           wifi_ssid=None,
                                           wifi_password=None,
                                           mobile_data_always_on=True):
        set_mobile_data_always_on(ad, mobile_data_always_on)
        if not phone_setup_func(self.log, ad):
            self.log.error("Phone failed to setup {}.".format(
                phone_setup_func.__name__))
            return False
        if (connect_wifi and
            not ensure_wifi_connected(self.log, ad, wifi_ssid, wifi_password)):
            self.log.error("WiFi connect failed")
            return False
        # simulate normal user behavior -- wake up every 1 minutes and do ping
        # (transmit data)
        if not self._start_alarm():
            return False
        ad.droid.goToSleepNow()
        return True

    def _setup_phone_active_call(self, ad, phone_setup_func,
                                 phone_idle_check_func,
                                 phone_in_call_check_func):
        if not phone_setup_func(self.log, ad):
            self.log.error("DUT Failed to Set Up Properly: {}".format(
                phone_setup_func.__name__))
            return False
        ensure_phones_idle(self.log, [ad, self.android_devices[1]])
        if not phone_idle_check_func(self.log, ad):
            self.log.error("DUT not in correct idle state: {}".format(
                phone_idle_check_func.__name__))
            return False
        if not call_setup_teardown(
                self.log,
                ad,
                self.android_devices[1],
                ad_hangup=None,
                verify_caller_func=phone_in_call_check_func):
            self.log.error("Setup Call failed.")
            return False
        ad.droid.goToSleepNow()
        return True

    def _setup_phone_active_call_wfc(self,
                                     ad,
                                     ssid,
                                     password,
                                     airplane_mode,
                                     wfc_mode,
                                     setup_volte=False):
        if setup_volte and (not phone_setup_volte(self.log, ad)):
            self.log.error("Phone failed to setup VoLTE.")
            return False
        if not phone_setup_iwlan(self.log, ad, airplane_mode, wfc_mode, ssid,
                                 password):
            self.log.error("DUT Failed to Set Up WiFi Calling")
            return False
        ensure_phones_idle(self.log, [ad, self.android_devices[1]])
        if not phone_idle_iwlan(self.log, ad):
            self.log.error("DUT not in WFC enabled state.")
            return False
        if not call_setup_teardown(self.log,
                                   ad,
                                   self.android_devices[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan):
            self.log.error("Setup Call failed.")
            return False
        ad.droid.goToSleepNow()
        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_3g(self):
        """Power measurement test for active CS(3G) call.

        Steps:
        1. DUT idle, in 3G mode.
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in CS(3G) call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params["pass_criteria_call_3g"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        return self._test_power_active_call(
            "test_power_active_call_3g",
            self._setup_phone_active_call,
            PASS_CRITERIA,
            phone_check_func_after_power_test=is_phone_in_call_3g,
            phone_setup_func=phone_setup_voice_3g,
            phone_idle_check_func=phone_idle_3g,
            phone_in_call_check_func=is_phone_in_call_3g)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_volte(self):
        """Power measurement test for active VoLTE call.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled.
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in VoLTE call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params["pass_criteria_call_volte"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        return self._test_power_active_call(
            "test_power_active_call_volte",
            self._setup_phone_active_call,
            PASS_CRITERIA,
            phone_check_func_after_power_test=is_phone_in_call_volte,
            phone_setup_func=phone_setup_volte,
            phone_idle_check_func=phone_idle_volte,
            phone_in_call_check_func=is_phone_in_call_volte)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_2g_apm(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, in Airplane mode, connect to 2G WiFi,
            WiFi Calling enabled (WiFi-preferred mode).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_call_wfc_2g_apm"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        return self._test_power_active_call(
            "test_power_active_call_wfc_2g_apm",
            self._setup_phone_active_call_wfc,
            PASS_CRITERIA,
            phone_check_func_after_power_test=is_phone_in_call_iwlan,
            ssid=self.wifi_network_ssid_2g,
            password=self.wifi_network_pass_2g,
            airplane_mode=True,
            wfc_mode=WFC_MODE_WIFI_PREFERRED,
            setup_volte=False)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_2g_lte_volte_on(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, LTE cellular data network, VoLTE is On, connect to 2G WiFi,
            WiFi Calling enabled (WiFi-preferred).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_call_wfc_2g_lte_volte_on"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        return self._test_power_active_call(
            "test_power_active_call_wfc_2g_lte_volte_on",
            self._setup_phone_active_call_wfc,
            PASS_CRITERIA,
            phone_check_func_after_power_test=is_phone_in_call_iwlan,
            ssid=self.wifi_network_ssid_2g,
            password=self.wifi_network_pass_2g,
            airplane_mode=False,
            wfc_mode=WFC_MODE_WIFI_PREFERRED,
            setup_volte=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_5g_apm(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, in Airplane mode, connect to 5G WiFi,
            WiFi Calling enabled (WiFi-preferred mode).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_call_wfc_5g_apm"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        return self._test_power_active_call(
            "test_power_active_call_wfc_5g_apm",
            self._setup_phone_active_call_wfc,
            PASS_CRITERIA,
            phone_check_func_after_power_test=is_phone_in_call_iwlan,
            ssid=self.wifi_network_ssid_5g,
            password=self.wifi_network_pass_5g,
            airplane_mode=True,
            wfc_mode=WFC_MODE_WIFI_PREFERRED,
            setup_volte=False)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_active_call_wfc_5g_lte_volte_on(self):
        """Power measurement test for active WiFi call.

        Steps:
        1. DUT idle, LTE cellular data network, VoLTE is On, connect to 5G WiFi,
            WiFi Calling enabled (WiFi-preferred).
        2. Make a phone Call from DUT to PhoneB. Answer on PhoneB.
            Make sure DUT is in WFC call.
        3. Turn off screen and wait for 3 minutes.
            Then measure power consumption for 5 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """

        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_call_wfc_5g_lte_volte_on"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        return self._test_power_active_call(
            "test_power_active_call_wfc_5g_lte_volte_on",
            self._setup_phone_active_call_wfc,
            PASS_CRITERIA,
            phone_check_func_after_power_test=is_phone_in_call_iwlan,
            ssid=self.wifi_network_ssid_5g,
            password=self.wifi_network_pass_5g,
            airplane_mode=False,
            wfc_mode=WFC_MODE_WIFI_PREFERRED,
            setup_volte=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_baseline(self):
        """Power measurement test for phone idle baseline.

        Steps:
        1. DUT idle, in Airplane mode. WiFi disabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_baseline"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_baseline(ad):
            if not toggle_airplane_mode(self.log, ad, True):
                self.log.error("Phone failed to turn on airplane mode.")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_baseline",
                                     _idle_baseline, PASS_CRITERIA)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_baseline_wifi_connected(self):
        """Power measurement test for phone idle baseline (WiFi connected).

        Steps:
        1. DUT idle, in Airplane mode. WiFi connected to 2.4G WiFi,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_baseline_wifi_connected"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_baseline_wifi_connected(ad):
            if not toggle_airplane_mode(self.log, ad, True):
                self.log.error("Phone failed to turn on airplane mode.")
                return False
            if not ensure_wifi_connected(self.log, ad,
                self.wifi_network_ssid_2g, self.wifi_network_pass_2g):
                self.log.error("WiFi connect failed")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_baseline_wifi_connected",
                                     _idle_baseline_wifi_connected,
                                     PASS_CRITERIA)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_wfc_2g_apm(self):
        """Power measurement test for phone idle WiFi Calling Airplane Mode.

        Steps:
        1. DUT idle, in Airplane mode. Connected to 2G WiFi,
            WiFi Calling enabled (WiFi preferred).
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_wfc_2g_apm"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_wfc_2g_apm(ad):
            if not phone_setup_iwlan(
                    self.log, ad, True, WFC_MODE_WIFI_PREFERRED,
                    self.wifi_network_ssid_2g, self.wifi_network_pass_2g):
                self.log.error("Phone failed to setup WFC.")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_wfc_2g_apm",
                                     _idle_wfc_2g_apm, PASS_CRITERIA,
                                     is_wfc_enabled)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_wfc_2g_lte(self):
        """Power measurement test for phone idle WiFi Calling LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. Connected to 2G WiFi,
            WiFi Calling enabled (WiFi preferred).
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_wfc_2g_lte"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_wfc_2g_lte(ad):
            if not phone_setup_volte(self.log, ad):
                self.log.error("Phone failed to setup VoLTE.")
                return False
            if not phone_setup_iwlan(
                    self.log, ad, False, WFC_MODE_WIFI_PREFERRED,
                    self.wifi_network_ssid_2g, self.wifi_network_pass_2g):
                self.log.error("Phone failed to setup WFC.")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_wfc_2g_lte",
                                     _idle_wfc_2g_lte, PASS_CRITERIA,
                                     is_wfc_enabled)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled(self):
        """Power measurement test for phone idle LTE VoLTE enabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi disabled,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_lte_volte_enabled"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_lte_volte_enabled(ad):
            if not phone_setup_volte(self.log, ad):
                self.log.error("Phone failed to setup VoLTE.")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_lte_volte_enabled",
                                     _idle_lte_volte_enabled, PASS_CRITERIA)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_disabled(self):
        """Power measurement test for phone idle LTE VoLTE disabled.

        Steps:
        1. DUT idle, in LTE mode, VoLTE disabled. WiFi disabled,
            WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_lte_volte_disabled"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_lte_volte_disabled(ad):
            if not phone_setup_csfb(self.log, ad):
                self.log.error("Phone failed to setup CSFB.")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_lte_volte_disabled",
                                     _idle_lte_volte_disabled, PASS_CRITERIA)

    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g(self):
        """Power measurement test for phone idle 3G.

        Steps:
        1. DUT idle, in 3G mode. WiFi disabled, WiFi Calling disabled.
        2. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params["pass_criteria_idle_3g"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA

        def _idle_3g(ad):
            if not phone_setup_voice_3g(self.log, ad):
                self.log.error("Phone failed to setup 3g.")
                return False
            ad.droid.goToSleepNow()
            return True

        return self._test_power_idle("test_power_idle_3g", _idle_3g,
                                     PASS_CRITERIA)

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_enabled_wakeup_ping(self):
        """Power measurement test for phone LTE VoLTE enabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE enabled. WiFi disabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_lte_volte_enabled_wakeup_ping"][
                    "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        # TODO: b/26338146 need a SL4A API to clear all existing alarms.

        result = self._test_power_idle(
            "test_power_idle_lte_volte_enabled_wakeup_ping",
            self._setup_phone_idle_and_wakeup_ping,
            PASS_CRITERIA,
            phone_setup_func=phone_setup_volte)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_lte_volte_disabled_wakeup_ping(self):
        """Power measurement test for phone LTE VoLTE disabled Wakeup Ping every
        1 minute.

        Steps:
        1. DUT idle, in LTE mode, VoLTE disabled. WiFi disabled,
            WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_lte_volte_disabled_wakeup_ping"][
                    "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        # TODO: b/26338146 need a SL4A API to clear all existing alarms.

        result = self._test_power_idle(
            "test_power_idle_lte_volte_disabled_wakeup_ping",
            self._setup_phone_idle_and_wakeup_ping,
            PASS_CRITERIA,
            phone_setup_func=phone_setup_csfb)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_idle_3g_wakeup_ping(self):
        """Power measurement test for phone 3G Wakeup Ping every 1 minute.

        Steps:
        1. DUT idle, in 3G mode. WiFi disabled, WiFi Calling disabled.
        2. Start script to wake up AP every 1 minute, after wakeup,
            DUT send http Request to Google.com then go to sleep.
        3. Turn off screen and wait for 6 minutes. Then measure power
            consumption for 40 minutes and get average.

        Expected Results:
        Average power consumption should be within pre-defined limit.

        Returns:
        True if Pass, False if Fail.

        Note: Please calibrate your test environment and baseline pass criteria.
        Pass criteria info should be in test config file.
        """
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_idle_3g_wakeup_ping"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        # TODO: b/26338146 need a SL4A API to clear all existing alarms.

        result = self._test_power_idle("test_power_idle_3g_wakeup_ping",
                                       self._setup_phone_idle_and_wakeup_ping,
                                       PASS_CRITERIA,
                                       phone_setup_func=phone_setup_voice_3g)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_lte(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_lte"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle("test_power_mobile_data_always_on_lte",
                                       self._setup_phone_mobile_data_always_on,
                                       PASS_CRITERIA,
                                       phone_setup_func=phone_setup_csfb,
                                       connect_wifi=False)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_wcdma(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_wcdma"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle("test_power_mobile_data_always_on_wcdma",
                                       self._setup_phone_mobile_data_always_on,
                                       PASS_CRITERIA,
                                       phone_setup_func=phone_setup_voice_3g,
                                       connect_wifi=False)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_1xevdo(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_1xevdo"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle("test_power_mobile_data_always_on_1xevdo",
                                       self._setup_phone_mobile_data_always_on,
                                       PASS_CRITERIA,
                                       phone_setup_func=phone_setup_voice_3g,
                                       connect_wifi=False)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_gsm(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_gsm"]["pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle("test_power_mobile_data_always_on_gsm",
                                       self._setup_phone_mobile_data_always_on,
                                       PASS_CRITERIA,
                                       phone_setup_func=phone_setup_voice_2g,
                                       connect_wifi=False)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_lte_wifi_on(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_lte_wifi_on"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle(
            "test_power_mobile_data_always_on_lte_wifi_on",
            self._setup_phone_mobile_data_always_on,
            PASS_CRITERIA,
            phone_setup_func=phone_setup_csfb,
            connect_wifi=True,
            wifi_ssid=self.wifi_network_ssid_2g,
            wifi_password=self.wifi_network_pass_2g)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_wcdma_wifi_on(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_wcdma_wifi_on"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle(
            "test_power_mobile_data_always_on_wcdma_wifi_on",
            self._setup_phone_mobile_data_always_on,
            PASS_CRITERIA,
            phone_setup_func=phone_setup_voice_3g,
            connect_wifi=True,
            wifi_ssid=self.wifi_network_ssid_2g,
            wifi_password=self.wifi_network_pass_2g)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_1xevdo_wifi_on(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_1xevdo_wifi_on"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle(
            "test_power_mobile_data_always_on_1xevdo_wifi_on",
            self._setup_phone_mobile_data_always_on,
            PASS_CRITERIA,
            phone_setup_func=phone_setup_voice_3g,
            connect_wifi=True,
            wifi_ssid=self.wifi_network_ssid_2g,
            wifi_password=self.wifi_network_pass_2g)
        set_mobile_data_always_on(self.ad, False)
        return result

    # TODO: This one is not working right now. Requires SL4A API to start alarm.
    @TelephonyBaseTest.tel_test_wrap
    def test_power_mobile_data_always_on_gsm_wifi_on(self):
        try:
            PASS_CRITERIA = int(self.user_params[
                "pass_criteria_mobile_data_always_on_gsm_wifi_on"][
                "pass_criteria"])
        except KeyError:
            PASS_CRITERIA = DEFAULT_POWER_PASS_CRITERIA
        result = self._test_power_idle(
            "test_power_mobile_data_always_on_gsm_wifi_on",
            self._setup_phone_mobile_data_always_on,
            PASS_CRITERIA,
            phone_setup_func=phone_setup_voice_2g,
            connect_wifi=True,
            wifi_ssid=self.wifi_network_ssid_2g,
            wifi_password=self.wifi_network_pass_2g)
        set_mobile_data_always_on(self.ad, False)
        return result

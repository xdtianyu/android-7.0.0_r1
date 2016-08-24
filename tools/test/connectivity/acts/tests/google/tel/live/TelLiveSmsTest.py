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
from queue import Empty
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import GEN_3G
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_defines import PHONE_TYPE_CDMA
from acts.test_utils.tel.tel_defines import PHONE_TYPE_GSM
from acts.test_utils.tel.tel_defines import RAT_3G
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL
from acts.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts.test_utils.tel.tel_test_utils import call_setup_teardown
from acts.test_utils.tel.tel_test_utils import \
    ensure_network_generation_for_subscription
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import mms_send_receive_verify
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import set_call_state_listen_level
from acts.test_utils.tel.tel_test_utils import setup_sim
from acts.test_utils.tel.tel_test_utils import sms_send_receive_verify
from acts.test_utils.tel.tel_video_utils import phone_setup_video
from acts.test_utils.tel.tel_video_utils import is_phone_in_call_video_bidirectional
from acts.test_utils.tel.tel_video_utils import video_call_setup_teardown
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_1x
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_2g
from acts.test_utils.tel.tel_voice_utils import phone_setup_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.utils import load_config
from acts.utils import rand_ascii_str


class TelLiveSmsTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = ("test_sms_mo_4g",
                      "test_sms_mt_4g",
                      "test_sms_mo_in_call_volte",
                      "test_sms_mt_in_call_volte",
                      "test_sms_mo_in_call_csfb",
                      "test_sms_mt_in_call_csfb",
                      "test_sms_mo_in_call_csfb_1x",
                      "test_sms_mt_in_call_csfb_1x",
                      "test_sms_mo_3g",
                      "test_sms_mt_3g",
                      "test_sms_mo_in_call_wcdma",
                      "test_sms_mt_in_call_wcdma",
                      "test_sms_mo_in_call_1x",
                      "test_sms_mt_in_call_1x",
                      "test_sms_mo_2g",
                      "test_sms_mt_2g",
                      "test_sms_mo_in_call_gsm",
                      "test_sms_mt_in_call_gsm",
                      "test_sms_mo_iwlan",
                      "test_sms_mt_iwlan",
                      "test_sms_mo_in_call_iwlan",
                      "test_sms_mt_in_call_iwlan",
                      "test_sms_mo_in_call_vt",
                      "test_sms_mt_in_call_vt",
                    )
        # The path for "sim config file" should be set
        # in "testbed.config" entry "sim_conf_file".
        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

    def _sms_test(self, ads):
        """Test SMS between two phones.

        Returns:
            True if success.
            False if failed.
        """

        sms_params = [(ads[0], ads[1])]
        message_arrays = [[rand_ascii_str(50)], [rand_ascii_str(160)],
                          [rand_ascii_str(180)]]

        for outer_param in sms_params:
            outer_param = (self.log, ) + outer_param
            for message_array in message_arrays:
                inner_param = outer_param + (message_array, )
                if not sms_send_receive_verify(*inner_param):
                    return False

        return True

    def _sms_test_mo(self, ads):
        return self._sms_test([ads[0], ads[1]])

    def _sms_test_mt(self, ads):
        return self._sms_test([ads[1], ads[0]])

    def _mo_sms_in_3g_call(self, ads):
        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_3g,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    def _mt_sms_in_3g_call(self, ads):
        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_3g,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    def _mo_sms_in_2g_call(self, ads):
        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_2g,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    def _mt_sms_in_2g_call(self, ads):
        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_2g,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_general(self):
        """Test SMS basic function between two phone. Phones in any network.

        Airplane mode is off.
        Send SMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_general(self):
        """Test SMS basic function between two phone. Phones in any network.

        Airplane mode is off.
        Send SMS from PhoneB to PhoneA.
        Verify received message on PhoneA is correct.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_general, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_2g(self):
        """Test SMS basic function between two phone. Phones in 3g network.

        Airplane mode is off.
        Send SMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_2g(self):
        """Test SMS basic function between two phone. Phones in 3g network.

        Airplane mode is off.
        Send SMS from PhoneB to PhoneA.
        Verify received message on PhoneA is correct.

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_3g(self):
        """Test SMS basic function between two phone. Phones in 3g network.

        Airplane mode is off.
        Send SMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_3g(self):
        """Test SMS basic function between two phone. Phones in 3g network.

        Airplane mode is off.
        Send SMS from PhoneB to PhoneA.
        Verify received message on PhoneA is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_4g(self):
        """Test SMS basic function between two phone. Phones in LTE network.

        Airplane mode is off.
        Send SMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices
        # TODO: this is a temporary fix for this test case.
        # A better fix will be introduced once pag/539845 is merged.
        if not phone_setup_voice_general(self.log, ads[1]):
            self.log.error("Failed to setup PhoneB.")
            return False
        if not ensure_network_generation(self.log, ads[0], GEN_4G):
            self.log.error("DUT Failed to Set Up Properly.")
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_4g(self):
        """Test SMS basic function between two phone. Phones in LTE network.

        Airplane mode is off.
        Send SMS from PhoneB to PhoneA.
        Verify received message on PhoneA is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices

        # TODO: this is a temporary fix for this test case.
        # A better fix will be introduced once pag/539845 is merged.
        if not phone_setup_voice_general(self.log, ads[1]):
            self.log.error("Failed to setup PhoneB.")
            return False
        if not ensure_network_generation(self.log, ads[0], GEN_4G):
            self.log.error("DUT Failed to Set Up Properly.")
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_volte(self):
        """ Test MO SMS during a MO VoLTE call.

        Make Sure PhoneA is in LTE mode (with VoLTE).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_volte,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_volte(self):
        """ Test MT SMS during a MO VoLTE call.

        Make Sure PhoneA is in LTE mode (with VoLTE).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_volte, (self.log, ads[0])), (phone_setup_volte,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_volte,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_wcdma(self):
        """ Test MO SMS during a MO wcdma call.

        Make Sure PhoneA is in wcdma mode.
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma SMS test.")
            return False

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._mo_sms_in_3g_call(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_wcdma(self):
        """ Test MT SMS during a MO wcdma call.

        Make Sure PhoneA is in wcdma mode.
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this wcdma SMS test.")
            return False

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._mt_sms_in_3g_call(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_csfb(self):
        """ Test MO SMS during a MO csfb wcdma/gsm call.

        Make Sure PhoneA is in LTE mode (no VoLTE).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this csfb wcdma SMS test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_csfb,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_csfb(self):
        """ Test MT SMS during a MO csfb wcdma/gsm call.

        Make Sure PhoneA is in LTE mode (no VoLTE).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, receive receive on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this csfb wcdma SMS test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_csfb,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_1x(self):
        """ Test MO SMS during a MO 1x call.

        Make Sure PhoneA is in 1x mode.
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("Not CDMA phone, abort this 1x SMS test.")
            return False

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_1x,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_1x(self):
        """ Test MT SMS during a MO 1x call.

        Make Sure PhoneA is in 1x mode.
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("Not CDMA phone, abort this 1x SMS test.")
            return False

        tasks = [(phone_setup_3g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_1x,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_csfb_1x(self):
        """ Test MO SMS during a MO csfb 1x call.

        Make Sure PhoneA is in LTE mode (no VoLTE).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("Not CDMA phone, abort this csfb 1x SMS test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_1x,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_csfb_1x(self):
        """ Test MT SMS during a MO csfb 1x call.

        Make Sure PhoneA is in LTE mode (no VoLTE).
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is CDMA phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_CDMA):
            self.log.error("Not CDMA phone, abort this csfb 1x SMS test.")
            return False

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_1x,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_iwlan(self):
        """ Test MO SMS, Phone in APM, WiFi connected, WFC WiFi Preferred mode.

        Make Sure PhoneA APM, WiFi connected, WFC WiFi preferred mode.
        Make sure PhoneA report iwlan as data rat.
        Make Sure PhoneB is able to make/receive call/sms.
        Send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_iwlan(self):
        """ Test MT SMS, Phone in APM, WiFi connected, WFC WiFi Preferred mode.

        Make Sure PhoneA APM, WiFi connected, WFC WiFi preferred mode.
        Make sure PhoneA report iwlan as data rat.
        Make Sure PhoneB is able to make/receive call/sms.
        Receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_iwlan(self):
        """ Test MO SMS, Phone in APM, WiFi connected, WFC WiFi Preferred mode.

        Make Sure PhoneA APM, WiFi connected, WFC WiFi preferred mode.
        Make sure PhoneA report iwlan as data rat.
        Make Sure PhoneB is able to make/receive call/sms.
        Call from PhoneA to PhoneB, accept on PhoneB.
        Send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan,
                                   verify_callee_func=None):
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_iwlan(self):
        """ Test MT SMS, Phone in APM, WiFi connected, WFC WiFi Preferred mode.

        Make Sure PhoneA APM, WiFi connected, WFC WiFi preferred mode.
        Make sure PhoneA report iwlan as data rat.
        Make Sure PhoneB is able to make/receive call/sms.
        Call from PhoneA to PhoneB, accept on PhoneB.
        Receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        tasks = [(phone_setup_iwlan,
                  (self.log, ads[0], True, WFC_MODE_WIFI_PREFERRED,
                   self.wifi_network_ssid, self.wifi_network_pass)),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_iwlan,
                                   verify_callee_func=None):
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_vt(self):
        """ Test MO SMS, Phone in ongoing VT call.

        Make Sure PhoneA and PhoneB in LTE and can make VT call.
        Make Video Call from PhoneA to PhoneB, accept on PhoneB as Video Call.
        Send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        return self._sms_test_mo(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_vt(self):
        """ Test MT SMS, Phone in ongoing VT call.

        Make Sure PhoneA and PhoneB in LTE and can make VT call.
        Make Video Call from PhoneA to PhoneB, accept on PhoneB as Video Call.
        Receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup a call")
            return False

        return self._sms_test_mt(ads)

    @TelephonyBaseTest.tel_test_wrap
    def test_mms_mo_4g(self):
        """Test MMS text function between two phone. Phones in LTE network.

        Airplane mode is off.
        Send SMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """

        self.log.error("Test Case is non-functional: b/21569494")
        return False

        ads = self.android_devices

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return mms_send_receive_verify(
            self.log, ads[0], ads[1],
            [("Test Message", "Basic Message Body", None)])

    @TelephonyBaseTest.tel_test_wrap
    def test_mms_mt_4g(self):
        """Test MMS text function between two phone. Phones in LTE network.

        Airplane mode is off.
        Send SMS from PhoneB to PhoneA.
        Verify received message on PhoneA is correct.

        Returns:
            True if success.
            False if failed.
        """

        self.log.error("Test Case is non-functional: b/21569494")
        return False

        ads = self.android_devices

        tasks = [(phone_setup_csfb, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        return mms_send_receive_verify(
            self.log, ads[1], ads[0],
            [("Test Message", "Basic Message Body", None)])

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mo_in_call_gsm(self):
        """ Test MO SMS during a MO gsm call.

        Make Sure PhoneA is in gsm mode.
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this gsm SMS test.")
            return False

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_2g,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mo(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    @TelephonyBaseTest.tel_test_wrap
    def test_sms_mt_in_call_gsm(self):
        """ Test MT SMS during a MO gsm call.

        Make Sure PhoneA is in gsm mode.
        Make Sure PhoneB is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, receive SMS on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        # make sure PhoneA is GSM phone before proceed.
        if (ads[0].droid.telephonyGetPhoneType() != PHONE_TYPE_GSM):
            self.log.error("Not GSM phone, abort this gsm SMS test.")
            return False

        tasks = [(phone_setup_voice_2g, (self.log, ads[0])),
                 (phone_setup_voice_general, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Begin In Call SMS Test.")
        if not call_setup_teardown(self.log,
                                   ads[0],
                                   ads[1],
                                   ad_hangup=None,
                                   verify_caller_func=is_phone_in_call_2g,
                                   verify_callee_func=None):
            return False

        if not self._sms_test_mt(ads):
            self.log.error("SMS test fail.")
            return False

        return True

    """ Tests End """

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
    Test Script for VT Data test
"""
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_defines import VT_STATE_BIDIRECTIONAL
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import multithread_func
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_video_utils import \
    is_phone_in_call_video_bidirectional
from acts.test_utils.tel.tel_video_utils import phone_setup_video
from acts.test_utils.tel.tel_video_utils import video_call_setup_teardown
from acts.utils import load_config


class TelLiveVideoDataTest(TelephonyBaseTest):
    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)
        self.tests = (
            # Data during VT call
            "test_internet_access_during_video_call", )

        self.simconf = load_config(self.user_params["sim_conf_file"])
        self.stress_test_number = int(self.user_params["stress_test_number"])
        self.wifi_network_ssid = self.user_params["wifi_network_ssid"]

        try:
            self.wifi_network_pass = self.user_params["wifi_network_pass"]
        except KeyError:
            self.wifi_network_pass = None

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_internet_access_during_video_call(self):
        """ Test Internet access during VT<->VT call.

        Make Sure PhoneA is in LTE mode (with Video Calling).
        Make Sure PhoneB is in LTE mode (with Video Calling).
        Call from PhoneA to PhoneB as Bi-Directional Video,
        Accept on PhoneB as video call.
        Verify PhoneA have Internet access.
        Hang up on PhoneA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        tasks = [(phone_setup_video, (self.log, ads[0])), (phone_setup_video,
                                                           (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        self.log.info("Step1: Make MO VT call.")
        if not video_call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                None,
                video_state=VT_STATE_BIDIRECTIONAL,
                verify_caller_func=is_phone_in_call_video_bidirectional,
                verify_callee_func=is_phone_in_call_video_bidirectional):
            self.log.error("Failed to setup+teardown a call")
            return False

        self.log.info("Step2: Verify Internet on PhoneA.")
        if not verify_http_connection(self.log, ads[0]):
            self.log.error("Verify Internet on PhoneA failed.")
            return False

        return hangup_call(self.log, ads[0])


""" Tests End """

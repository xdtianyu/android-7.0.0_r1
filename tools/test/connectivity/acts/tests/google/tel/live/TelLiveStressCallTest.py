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
    Test Script for Telephony integration with TF
"""

import time
from acts.base_test import BaseTestClass
from queue import Empty
from acts.test_utils.tel import tel_defines
from acts.test_utils.tel.tel_test_utils import initiate_call
from acts.test_utils.tel.tel_test_utils import hangup_call
from acts.test_utils.tel.tel_test_utils import ensure_phone_default_state
from acts.test_utils.tel.tel_test_utils import ensure_phone_idle
from acts.test_utils.tel.tel_test_utils import verify_active_call_number
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_3g
from acts.test_utils.tel.tel_voice_utils import phone_idle_3g
from acts.test_utils.tel.tel_voice_utils import is_phone_in_call_3g

MAX_NUMBER_REDIALS = 20
INCORRECT_STATE_MSG = "Caller not in correct state!"


class TelLiveStressCallTest(BaseTestClass):
    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = ("test_call_3g_stress", )

    def setup_class(self):
        self.ad_caller = self.android_devices[0]
        self.stress_test_callee_number = self.user_params["call_server_number"]
        self.phone_call_iteration = self.user_params["phone_call_iteration"]
        return True

    def setup_test(self):
        # try removing lock
        self.android_devices[0].droid.wakeLockAcquireBright()
        self.android_devices[0].droid.wakeUpNow()
        self.assert_true(
            ensure_phone_default_state(self.log, self.ad_caller),
            "Make sure phone is in default state")
        return True

    def teardown_test(self):
        self.android_devices[0].droid.wakeLockRelease()
        self.android_devices[0].droid.goToSleepNow()
        self.assert_true(
            ensure_phone_default_state(self.log, self.ad_caller),
            "Make sure phone returns to default state")

    """ Tests Begin """

    def test_call_3g_stress(self):
        """ 3G to 800 call test

        Steps:
        1. Make Sure PhoneA is in 3G mode.
        2. Call from PhoneA to a 800 number, hang up on PhoneA.
        3, Repeat 2 around 100 times based on the config setup

        Expected Results:
        1, Verify phone is at IDLE state
        2, Verify the phone is at ACTIVE, if it is in dialing, then we retry
        3, Verify the phone is IDLE after hung up

        Returns:
            True if pass; False if fail.
        """
        ad_caller = self.ad_caller
        callee_number = self.stress_test_callee_number
        self.assert_true(
            phone_setup_voice_3g(self.log,
                                 ad_caller), "Phone Failed to Set Up Properly.")

        # Make sure phone is idle.
        ensure_phone_idle(self.log, ad_caller)
        self.assert_true(
            phone_idle_3g(self.log, ad_caller), "DUT Failed to Reselect")

        self.log.info("Call test:{} to {}".format(ad_caller.serial,
                                                  callee_number))
        subid_caller = ad_caller.droid.subscriptionGetDefaultVoiceSubId()

        total_iteration = self.phone_call_iteration
        current_iteration = 0
        redial_time = 0
        while current_iteration < total_iteration:
            self.log.info("---> Call test: iteration {} redial {}<---"
                          .format(current_iteration, redial_time))
            self.log.info("Checking Telephony Manager Call State")
            self.assert_true(
                self._check_phone_call_status(
                    ad_caller, tel_defines.TELEPHONY_STATE_IDLE),
                INCORRECT_STATE_MSG)

            self.log.info("Making a phone call")
            self.assert_true(
                initiate_call(self.log, ad_caller, callee_number),
                "Initiate call failed.")

            self.log.info("Ensure that all internal states are updated")
            time.sleep(tel_defines.WAIT_TIME_ANDROID_STATE_SETTLING)
            self.assert_true(
                is_phone_in_call_3g(self.log, ad_caller), INCORRECT_STATE_MSG)
            self.assert_true(
                self._check_phone_call_status(
                    ad_caller, tel_defines.TELEPHONY_STATE_OFFHOOK,
                    tel_defines.CALL_STATE_DIALING), INCORRECT_STATE_MSG)

            time.sleep(tel_defines.WAIT_TIME_IN_CALL)
            self.log.info(
                "Checking Telephony Manager Call State after waiting for a while")
            if (self._check_phone_call_status(
                    ad_caller, tel_defines.TELEPHONY_STATE_OFFHOOK,
                    tel_defines.CALL_STATE_ACTIVE)):
                current_iteration += 1
                redial_time = 0
            elif (self._check_phone_call_status(
                    ad_caller, tel_defines.TELEPHONY_STATE_OFFHOOK,
                    tel_defines.CALL_STATE_DIALING)):
                self.log.info("The line is busy, try again")
                redial_time += 1
                if redial_time > MAX_NUMBER_REDIALS:
                    self.assert_true(
                        False, "Re-dial {} times and still having busy signal"
                        .format(redial_time))
            else:
                self.assert_true(False, INCORRECT_STATE_MSG)
                current_iteration += 1

            self.log.info("Hang up phone for this iteration")
            self.assert_true(
                hangup_call(self.log, ad_caller), "Error in Hanging-Up Call")
            time.sleep(tel_defines.WAIT_TIME_ANDROID_STATE_SETTLING)
            self.log.info(
                "Checking Telephony Manager Call State after hang up")
            self.assert_true(
                self._check_phone_call_status(
                    ad_caller, tel_defines.TELEPHONY_STATE_IDLE),
                INCORRECT_STATE_MSG)

            ensure_phone_idle(self.log, ad_caller)

    """ Tests End """

    def _check_phone_call_status(self, ad, telecom_status, call_status=None):
        """Check existing event until we get either "ACTIVE" or "DIALING" event
        Args:
            ad: Android object
            telecome_status: expected telecom call state
            call_status: expcted telecomcall call state

        Return:
            True if all the status are matching, False otherwise
         """
        # Checking phone call status
        if ad.droid.telecomGetCallState() != telecom_status:
            return False
        if call_status:
            call_list = ad.droid.telecomCallGetCallIds()
            if not call_list:
                return False
            if not verify_active_call_number(self.log, ad, 1):
                return False
            call_id = call_list[0]
            self.log.info("TelecomCall Call State {}"
                          .format(ad.droid.telecomCallGetCallState(call_id)))
            if ad.droid.telecomCallGetCallState(call_id) != call_status:
                return False
        return True


if __name__ == "__main__":
    pass

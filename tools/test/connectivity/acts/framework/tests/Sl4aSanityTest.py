#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

from queue import Empty
from acts.base_test import BaseTestClass
from acts.test_utils.wifi.wifi_test_utils import wifi_toggle_state
from acts.test_utils.wifi.wifi_test_utils import WifiEnums

class Sl4aSanityTest(BaseTestClass):
    """Tests for sl4a basic sanity.

    Run these tests individually with option -r 100.
    """

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_bring_up_and_shutdown",
            "test_message_then_shutdown_stress"
        )

    def test_bring_up_and_shutdown(self):
        """Constantly start and terminate sl4a sessions.

        Verify in log that the "manager map key" is always empty before a
        session starts.
        Verify in log by looking at timestamps that after the test finishes, no
        more message regarding sl4a happens.
        """
        ad = self.android_devices[0]
        for i in range(100):
            self.log.info("Iteration %d, terminating." % i)
            ad.terminate_all_sessions()
            self.log.info("Iteration %d, starting." % i)
            droid, ed = ad.get_droid()
        return True

    def test_message_then_shutdown_stress(self):
        ad = self.android_devices[0]
        for i in range(10):
            assert wifi_toggle_state(ad.droid, ad.ed, False)
            assert wifi_toggle_state(ad.droid, ad.ed, True)
        return True

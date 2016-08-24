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

from acts import base_test
from acts.controllers.sniffer import Sniffer

class SnifferSanityTest(base_test.BaseTestClass):

    def setup_class(self):
        self._channels = [6, 44]

        # capture (sniff) for 30 seconds or 10 packets - whichever comes first
        self._capture_sec = 30
        self._packet_count = 10

        self._filter = {"tcpdump": "type mgt subtype beacon",
                        "tshark": "type mgt subtype beacon"}

    def test_sniffer_validation_using_with(self):
        """Validate sniffer configuration & capture API using the 'with' clause.

        This is the standard example - this syntax should typically be used.
        """
        index = 0
        for sniffer in self.sniffers:
            for channel in self._channels:
                with sniffer.start_capture(
                         override_configs={Sniffer.CONFIG_KEY_CHANNEL:channel},
                                           duration=self._capture_sec,
                                           packet_count=self._packet_count):
                    self.log.info("Capture: %s", sniffer.get_capture_file())

    def test_sniffer_validation_manual(self):
        """Validate sniffer configuration & capture API using a manual/raw
        API mechanism.

        The standard process should use a with clause. This demonstrates the
        manual process which uses an explicit wait_for_capture() call.
        Alternatively, could also use a sleep() + stop_capture() process
        (though that mechanism won't terminate early if the capture is done).
        """
        index = 0
        for sniffer in self.sniffers:
            for channel in self._channels:
                sniffer.start_capture(
                          override_configs={Sniffer.CONFIG_KEY_CHANNEL:channel},
                                      packet_count=self._packet_count)
                self.log.info("Capture: %s", sniffer.get_capture_file())
                sniffer.wait_for_capture(timeout=self._capture_sec)

    def test_sniffer_validation_capture_3_beacons(self):
        """Demonstrate the use of additional configuration.
        """
        index = 0
        for sniffer in self.sniffers:
            for channel in self._channels:
                with sniffer.start_capture(
                         override_configs={Sniffer.CONFIG_KEY_CHANNEL:channel},
                                           duration=self._capture_sec,
                                           packet_count=3,
                                           additional_args=self._filter[
                                                       sniffer.get_subtype()]):
                    self.log.info("Capture: %s", sniffer.get_capture_file())

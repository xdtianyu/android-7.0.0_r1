#/usr/bin/env python3.4
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

"""
Test script to test connect and disconnect sequence between two devices which can run
SL4A. The script does the following:
  Setup:
    Clear up the bonded devices on both bluetooth adapters and bond the DUTs to each other.
  Test (NUM_TEST_RUNS times):
    1. Connect A2dpSink and HeadsetClient
      1.1. Check that devices are connected.
    2. Disconnect A2dpSink and HeadsetClient
      2.1 Check that devices are disconnected.
"""

import time

from acts.base_test import BaseTestClass
from acts.test_utils.bt import bt_test_utils
from acts import asserts

class BtCarPairedConnectDisconnectTest(BaseTestClass):
    def setup_class(self):
        self.droid_ad = self.android_devices[0]
        self.droid1_ad = self.android_devices[1]

    def setup_test(self):
        # Reset the devices in a clean state.
        bt_test_utils.setup_multiple_devices_for_bt_test(self.android_devices)
        bt_test_utils.reset_bluetooth(self.android_devices)
        for a in self.android_devices:
            a.ed.clear_all_events()

        # Pair the devices.
        # This call may block until some specified timeout in bt_test_utils.py.
        result = bt_test_utils.pair_pri_to_sec(self.droid_ad.droid, self.droid1_ad.droid)

        asserts.assert_true(result, "pair_pri_to_sec returned false.");

        # Check for successful setup of test.
        devices = self.droid_ad.droid.bluetoothGetBondedDevices()
        asserts.assert_equal(len(devices), 1, "pair_pri_to_sec succeeded but no bonded devices.")

    def on_fail(self, test_name, begin_time):
        bt_test_utils.take_btsnoop_logs(self.android_devices, self, test_name)

    def test_connect_disconnect_paired(self):
        NUM_TEST_RUNS = 2
        failure = 0
        for i in range(NUM_TEST_RUNS):
            self.log.info("Running test [" + str(i) + "/" + str(NUM_TEST_RUNS) + "]")
            # Connect the device.
            devices = self.droid_ad.droid.bluetoothGetBondedDevices()
            if (len(devices) == 0):
                self.log.info("No bonded devices.")
                failure = failure + 1
                continue

            self.log.info("Attempting to connect.")
            self.droid_ad.droid.bluetoothConnectBonded(devices[0]['address'])
            end_time = time.time() + 20
            expected_address = self.droid1_ad.droid.bluetoothGetLocalAddress()
            connected = False
            a2dp_sink_connected = False
            pbap_client_connected = False
            hfp_client_connected = False

            # Busy loop to check if we found a matching device.
            while time.time() < end_time:
                connected_devices = self.droid_ad.droid.bluetoothGetConnectedDevices()
                for d in connected_devices:
                    if d['address'] == expected_address:
                        connected = True
                        break
                a2dp_sink_connected_devices = (self.droid_ad.droid
                                               .bluetoothGetConnectedDevicesOnProfile(11))
                for d in a2dp_sink_connected_devices:
                    if d['address'] == expected_address:
                        a2dp_sink_connected = True
                        break

                hfp_client_connected_devices = (self.droid_ad.droid.
                                                bluetoothGetConnectedDevicesOnProfile(16))
                for d in hfp_client_connected_devices:
                    if d['address'] == expected_address:
                        hfp_client_connected = True
                        break

                pbap_client_connected_devices = (self.droid_ad.droid.
                                                 bluetoothGetConnectedDevicesOnProfile(17))
                for d in hfp_client_connected_devices:
                    if d['address'] == expected_address:
                        pbap_client_connected = True
                        break
                time.sleep(5)

                self.log.info("Connected " + str(connected))
                self.log.info("A2DP Sink Connected " + str(a2dp_sink_connected))
                self.log.info("HFP client connected " + str(hfp_client_connected))
                self.log.info("PBAP Client connected " + str(pbap_client_connected))

                if (all([connected, a2dp_sink_connected,
                         hfp_client_connected, pbap_client_connected])):
                    break

                # Try again to overcome occasional throw away by bluetooth
                self.droid_ad.droid.bluetoothConnectBonded(devices[0]['address'])

            # Check if we got connected.
            if (not all([connected, a2dp_sink_connected, pbap_client_connected,
                         hfp_client_connected])):
                self.log.info("Not all profiles connected.")
                failure = failure + 1
                continue

            # Disconnect the devices.
            self.log.info("Attempt to disconnect.")
            self.droid_ad.droid.bluetoothDisconnectConnected(expected_address)

            end_time = time.time() + 10
            disconnected = False
            # Busy loop to check if we have successfully disconnected from the
            # device
            while time.time() < end_time:
                connectedDevices = self.droid_ad.droid.bluetoothGetConnectedDevices()
                exists = False
                connected_devices = self.droid_ad.droid.bluetoothGetConnectedDevices()
                for d in connected_devices:
                  if d['address'] == expected_address:
                      exists = True
                      break
                if exists is False:
                    disconnected = True
                    break
                time.sleep(1)

            if disconnected is False:
                self.log.info("Still connected devices.")
                failure = failure + 1
                continue
        self.log.info("Failure {} total tests {}".format(failure, NUM_TEST_RUNS))
        asserts.assert_equal(failure, 0, "")

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
Test script to execute Bluetooth basic functionality test cases.
This test was designed to be run in a shield box.
"""

import threading
import time
from contextlib import suppress

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import get_bt_mac_address
from acts.test_utils.bt.bt_test_utils import rfcomm_accept
from acts.test_utils.bt.bt_test_utils import rfcomm_connect
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs


class RfcommStressTest(BluetoothBaseTest):
    default_timeout = 10
    scan_discovery_time = 5
    thread_list = []
    message = (
        "Space: the final frontier. These are the voyages of "
        "the starship Enterprise. Its continuing mission: to explore "
        "strange new worlds, to seek out new life and new civilizations,"
        " to boldly go where no man has gone before.")

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.client_ad = self.android_devices[0]
        self.server_ad = self.android_devices[1]
        self.tests = (
            "test_rfcomm_connection_stress",
            "test_rfcomm_read_write_stress",
        )

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    def teardown_test(self):
        with suppress(Exception):
            for thread in self.thread_list:
                thread.join()

    def orchestrate_rfcomm_connect(self, server_mac):
        accept_thread = threading.Thread(target=rfcomm_accept,
                                         args=(self.server_ad.droid, ))
        self.thread_list.append(accept_thread)
        accept_thread.start()
        connect_thread = threading.Thread(
            target=rfcomm_connect,
            args=(self.client_ad, server_mac))
        self.thread_list.append(connect_thread)
        connect_thread.start()

    def test_rfcomm_connection_stress(self):
        """Stress test an RFCOMM connection

        Test the integrity of RFCOMM. Verify that file descriptors are cleared
        out properly.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Stop the RFCOMM connection.
        5. Repeat steps 2-4 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Stress, RFCOMM
        Priority: 1
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        for n in range(1000):
            self.orchestrate_rfcomm_connect(server_mac)
            self.log.info("Write message.")
            self.client_ad.droid.bluetoothRfcommWrite(self.message)
            self.log.info("Read message.")
            read_msg = self.server_ad.droid.bluetoothRfcommRead()
            self.log.info("Verify message.")
            assert self.message == read_msg, "Mismatch! Read {}".format(
                read_msg)
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
            for t in self.thread_list:
                t.join()
            self.thread_list.clear()
            self.log.info("Iteration {} completed".format(n))
        return True

    def test_rfcomm_read_write_stress(self):
        """Stress test an RFCOMM connection's read and write capabilities

        Test the integrity of RFCOMM. Verify that file descriptors are cleared
        out properly.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Repeat steps 2-3 10000 times.
        5. Stop the RFCOMM connection.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Stress, RFCOMM
        Priority: 1
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        reset_bluetooth([self.server_ad])
        self.orchestrate_rfcomm_connect(server_mac)
        for n in range(10000):
            self.log.info("Write message.")
            self.client_ad.droid.bluetoothRfcommWrite(self.message)
            self.log.info("Read message.")
            read_msg = self.server_ad.droid.bluetoothRfcommRead()
            self.log.info("Verify message.")
            assert self.message == read_msg, "Mismatch! Read {}".format(
                read_msg)
            self.log.info("Iteration {} completed".format(n))
        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

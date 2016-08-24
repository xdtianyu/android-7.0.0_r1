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

from contextlib import suppress
import threading
import time

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import log_energy_info
from acts.test_utils.bt.bt_test_utils import kill_bluetooth_process
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.bt_test_utils import rfcomm_accept
from acts.test_utils.bt.bt_test_utils import rfcomm_connect
from acts.test_utils.bt.bt_test_utils import setup_multiple_devices_for_bt_test
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import write_read_verify_data


class RfcommTest(BluetoothBaseTest):
    default_timeout = 10
    rf_client_th = 0
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

    def _clear_bonded_devices(self):
        for a in self.android_devices:
            bonded_device_list = a.droid.bluetoothGetBondedDevices()
            for device in bonded_device_list:
                a.droid.bluetoothUnbond(device['address'])

    def setup_class(self):
        return setup_multiple_devices_for_bt_test(self.android_devices)

    def setup_test(self):
        self._clear_bonded_devices()
        self.log.debug(log_energy_info(self.android_devices, "Start"))
        for a in self.android_devices:
            a.ed.clear_all_events()
        return True

    def teardown_test(self):
        self.log.debug(log_energy_info(self.android_devices, "End"))
        return True

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        reset_bluetooth(self.android_devices)

    def teardown_test(self):
        with suppress(Exception):
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
            self.client_ad.droid.bluetoothRfcommCloseSocket()
            self.server_ad.droid.bluetoothRfcommCloseSocket()
        for thread in self.thread_list:
            thread.join()
        self.thread_list.clear()

    def orchestrate_rfcomm_connect(self, server_mac):
        accept_thread = threading.Thread(target=rfcomm_accept,
                                         args=(self.server_ad, ))
        self.thread_list.append(accept_thread)
        accept_thread.start()
        connect_thread = threading.Thread(target=rfcomm_connect,
                                          args=(self.client_ad, server_mac))
        self.rf_client_th = connect_thread
        self.thread_list.append(connect_thread)
        connect_thread.start()
        for thread in self.thread_list:
            thread.join()
        end_time = time.time() + self.default_timeout
        result = False
        while time.time() < end_time:
            if len(self.client_ad.droid.bluetoothRfcommActiveConnections(
            )) > 0:
                self.log.info("RFCOMM Connection Active")
                return True
        self.log.error("Failed to establish an RFCOMM connection")
        return False

    @BluetoothBaseTest.bt_test_wrap
    def test_rfcomm_connection_write_ascii(self):
        """Test bluetooth RFCOMM writing and reading ascii data

        Test RFCOMM though establishing a connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.
        4. Write data from the client and read received data from the server.
        5. Verify data matches from client and server
        6. Disconnect the RFCOMM connection.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 1
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        if not self.orchestrate_rfcomm_connect(server_mac):
            return False
        if not write_read_verify_data(self.client_ad, self.server_ad,
                                      self.message, False):
            return False
        if len(self.server_ad.droid.bluetoothRfcommActiveConnections()) == 0:
            self.log.info("No rfcomm connections found on server.")
            return False
        if len(self.client_ad.droid.bluetoothRfcommActiveConnections()) == 0:
            self.log.info("no rfcomm connections found on client.")
            return False

        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        return True

    @BluetoothBaseTest.bt_test_wrap
    def test_rfcomm_write_binary(self):
        """Test bluetooth RFCOMM writing and reading binary data

        Test profile though establishing an RFCOMM connection.

        Steps:
        1. Get the mac address of the server device.
        2. Establish an RFCOMM connection from the client to the server AD.
        3. Verify that the RFCOMM connection is active from both the client and
        server.
        4. Write data from the client and read received data from the server.
        5. Verify data matches from client and server
        6. Disconnect the RFCOMM connection.

        Expected Result:
        RFCOMM connection is established then disconnected succcessfully.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, RFCOMM
        Priority: 1
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        if not self.orchestrate_rfcomm_connect(server_mac):
            return False
        binary_message = "11010101"
        if not write_read_verify_data(self.client_ad, self.server_ad,
                                      binary_message, True):
            return False
        if len(self.server_ad.droid.bluetoothRfcommActiveConnections()) == 0:
            self.log.info("No rfcomm connections found on server.")
            return False
        if len(self.client_ad.droid.bluetoothRfcommActiveConnections()) == 0:
            self.log.info("no rfcomm connections found on client.")
            return False

        self.client_ad.droid.bluetoothRfcommStop()
        self.server_ad.droid.bluetoothRfcommStop()
        return True

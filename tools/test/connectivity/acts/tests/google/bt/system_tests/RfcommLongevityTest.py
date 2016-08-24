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
from random import randint

from queue import Empty
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.bt_test_utils import rfcomm_accept
from acts.test_utils.bt.bt_test_utils import rfcomm_connect
from acts.test_utils.bt.bt_test_utils import take_btsnoop_logs
from acts.test_utils.bt.bt_test_utils import write_read_verify_data

class RfcommLongevityTest(BluetoothBaseTest):
    default_timeout = 10
    longev_iterations = 200
    thread_list = []
    generic_message = (
        "Space: the final frontier. These are the voyages of "
        "the starship Enterprise. Its continuing mission: to explore "
        "strange new worlds, to seek out new life and new civilizations,"
        " to boldly go where no man has gone before.")

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.client_ad = self.android_devices[0]
        self.server_ad = self.android_devices[1]

    def on_fail(self, test_name, begin_time):
        take_btsnoop_logs(self.android_devices, self, test_name)
        for _ in range(5):
            if reset_bluetooth(self.android_devices):
                break
            else:
                self.log.info("Failed to reset bluetooth state, retrying...")

    def teardown_test(self):
        with suppress(Exception):
            for thread in self.thread_list:
                thread.join()
        for _ in range(5):
            if reset_bluetooth(self.android_devices):
                break
            else:
                self.log.info("Failed to reset bluetooth state, retrying...")
        time.sleep(20) #safeguard in case of connId errors

    def orchestrate_rfcomm_connect(self, server_mac):
        accept_thread = threading.Thread(target=rfcomm_accept,
                                         args=(self.server_ad, ))
        self.thread_list.append(accept_thread)
        accept_thread.start()
        connect_thread = threading.Thread(
            target=rfcomm_connect,
            args=(self.client_ad, server_mac))
        self.thread_list.append(connect_thread)
        connect_thread.start()

    def test_rfcomm_longev_read_write_message(self):
        """Longevity test an RFCOMM connection's I/O with a generic message

        Test the longevity of RFCOMM with a basic read/write
        connect/disconnect sequence.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Verify data written matches data read.
        5. Repeat steps 2-4 5000 times.
        6. Disconnect RFCOMM connection.
        7. Repeat steps 1-6 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully. Each connect and disconnect should be successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Longevity, RFCOMM
        Priority: 2
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        write_iterations = 5000
        for i in range(self.longev_iterations):
            self.log.info("iteration {} connection".format(i))
            self.orchestrate_rfcomm_connect(server_mac)
            for n in range(write_iterations):
                self.log.info("iteration {} data".format(((n + 1) + (
                    i * write_iterations))))
                if not write_read_verify_data(self.client_ad, self.server_ad,
                    self.generic_message, False):
                    return False
                self.log.info("Iteration {} completed".format(n))
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

    def test_rfcomm_longev_read_write_small_message(self):
        """Longevity test an RFCOMM connection's I/O with a small message

        Test the longevity of RFCOMM with a basic read/write
        connect/disconnect sequence. The data being transfered is only
        one character in size.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Verify data written matches data read.
        5. Repeat steps 2-4 5000 times.
        6. Disconnect RFCOMM connection.
        7. Repeat steps 1-6 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully. Each connect and disconnect should be successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Longevity, RFCOMM
        Priority: 2
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        message = "x"
        write_iterations = 5000
        for i in range(self.longev_iterations):
            self.log.info("iteration {} connection".format(i))
            self.orchestrate_rfcomm_connect(server_mac)
            for n in range(write_iterations):
                self.log.info("iteration {} data".format(((n + 1) + (
                    i * write_iterations))))
                if not write_read_verify_data(self.client_ad, self.server_ad,
                    message, False):
                    return False
                self.log.info("Iteration {} completed".format(n))
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

    def test_rfcomm_longev_read_write_binary_message(self):
        """Longevity test an RFCOMM connection's I/O with a binary message

        Test the longevity of RFCOMM with a basic read/write
        connect/disconnect sequence. The data being transfered is in a
        binary format.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Verify data written matches data read.
        5. Repeat steps 2-4 5000 times.
        6. Disconnect RFCOMM connection.
        7. Repeat steps 1-6 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully. Each connect and disconnect should be successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Longevity, RFCOMM
        Priority: 2
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        binary_message = "11010101"
        write_iterations = 5000
        for i in range(self.longev_iterations):
            self.log.info("iteration {} connection".format(i))
            self.orchestrate_rfcomm_connect(server_mac)
            for n in range(write_iterations):
                self.log.info("iteration {} data".format(((n + 1) + (
                    i * write_iterations))))
                if not write_read_verify_data(self.client_ad, self.server_ad,
                    binary_message, True):
                    return False
                self.log.info("Iteration {} completed".format(n))
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

    def test_rfcomm_longev_read_write_large_message(self):
        """Longevity test an RFCOMM connection's I/O with a large message

        Test the longevity of RFCOMM with a basic read/write
        connect/disconnect sequence. The data being transfered is 990 chars
        in size.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Verify data written matches data read.
        5. Repeat steps 2-4 5000 times.
        6. Disconnect RFCOMM connection.
        7. Repeat steps 1-6 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully. Each connect and disconnect should be successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Longevity, RFCOMM
        Priority: 2
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        message = "x" * 990  #largest message size till sl4a fixed
        write_iterations = 5000
        for i in range(self.longev_iterations):
            self.log.info("iteration {} connection".format(i))
            self.orchestrate_rfcomm_connect(server_mac)
            for n in range(write_iterations):
                self.log.info("iteration {} data".format(((n + 1) + (
                    i * write_iterations))))
                if not write_read_verify_data(self.client_ad, self.server_ad,
                    message, False):
                    return False
                self.log.info("Iteration {} completed".format(n))
            self.client_ad.droid.bluetoothRfcommStop()
            self.server_ad.droid.bluetoothRfcommStop()
        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

    def test_rfcomm_longev_connection_interuption(self):
        """Longevity test an RFCOMM connection's with socket interuptions

        Test the longevity of RFCOMM with a basic read/write
        connect/disconnect sequence. Randomly in the sequence of reads and
        writes the socket on the client side will close. There should be
        an exception thrown for writing the next set of data and the
        test should start up a new connection and continue.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Verify data written matches data read.
        5. Repeat steps 2-4 5000 times or until the random interupt occurs.
        6. Re-establish an RFCOMM connection.
        7. Repeat steps 1-6 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully. Each connect and disconnect should be successful.
        Devices should recover a new connection after each interruption.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Longevity, RFCOMM
        Priority: 2
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        write_iterations = 5000
        for i in range(self.longev_iterations):
            try:
                self.log.info("iteration {} connection".format(i))
                self.orchestrate_rfcomm_connect(server_mac)
                random_interup_iteration = randint(0, write_iterations)
                for n in range(write_iterations):
                    self.log.info("iteration {} data".format(((n + 1) + (
                        i * write_iterations))))
                    if not write_read_verify_data(self.client_ad, self.server_ad,
                        self.generic_message, False):
                        return False
                    self.log.info("Iteration {} completed".format(n))
                    if n > random_interup_iteration:
                        self.client_ad.droid.bluetoothRfcommCloseSocket()
                self.client_ad.droid.bluetoothRfcommStop()
                self.server_ad.droid.bluetoothRfcommStop()
            except Exception:
                self.log.info("Exception found as expected. Continuing...")
                try:
                    self.client_ad.droid.bluetoothRfcommStop()
                except Exception as err:
                    self.log.error(
                        "Error closing client connection: {}".format(err))
                    return False
                try:
                    self.server_ad.droid.bluetoothRfcommStop()
                except Exception as err:
                    self.log.error(
                        "Error closing server connection: {}".format(err))
                    return False
                for t in self.thread_list:
                    t.join()
                self.thread_list.clear()
        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

    def test_rfcomm_longev_data_elasticity(self):
        """Longevity test an RFCOMM connection's I/O with changing data size

        Test the longevity of RFCOMM with a basic read/write
        connect/disconnect sequence. The data being transfered changes
        in size after each write/read sequence to increase up to 990
        chars in size and decrease down to 1 in size. This repeats through
        the entire test in order to exercise different size values being
        written.

        Steps:
        1. Establish a bonding between two Android devices.
        2. Write data to RFCOMM from the client droid.
        3. Read data from RFCOMM from the server droid.
        4. Verify data written matches data read.
        5. Change data size according to above description.
        6. Repeat steps 2-5 5000 times.
        7. Disconnect RFCOMM connection.
        8. Repeat steps 1-6 1000 times.

        Expected Result:
        Each iteration should read and write to the RFCOMM connection
        successfully. Each connect and disconnect should be successful.

        Returns:
          Pass if True
          Fail if False

        TAGS: Classic, Longevity, RFCOMM
        Priority: 2
        """
        server_mac = self.server_ad.droid.bluetoothGetLocalAddress()
        message = "x"
        resize_toggle = 1
        write_iterations = 5000
        for i in range(self.longev_iterations):
            try:
                self.log.info("iteration {} connection".format(i))
                self.orchestrate_rfcomm_connect(server_mac)
                for n in range(write_iterations):
                    self.log.info("iteration {} data".format(((n + 1) + (
                        i * write_iterations))))
                    if not write_read_verify_data(self.client_ad, self.server_ad,
                        message, False):
                        return False
                    self.log.info("Iteration {} completed".format(n))
                    size_of_message = len(message)
                    #max size is 990 due to a bug in sl4a.
                    if size_of_message >= 990:
                        resize_toggle = 0
                    elif size_of_message <= 1:
                        resize_toggle = 1
                    if resize_toggle == 0:
                        message = "x" * (size_of_message - 1)
                    else:
                        message = "x" * (size_of_message + 1)
                self.client_ad.droid.bluetoothRfcommStop()
                self.server_ad.droid.bluetoothRfcommStop()
            except Exception as err:
                self.log.info("Error in longevity test: {}".format(err))
                for t in self.thread_list:
                    t.join()
                self.thread_list.clear()
                return False

        for t in self.thread_list:
            t.join()
        self.thread_list.clear()
        return True

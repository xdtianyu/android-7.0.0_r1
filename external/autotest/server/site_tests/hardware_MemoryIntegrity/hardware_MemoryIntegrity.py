# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test

class hardware_MemoryIntegrity(test.test):
    """
    Integrity test for memory device
    """
    version = 1
    suspend_fail = False

    # Define default value for the test case

    def _determine_usable_memory(self):
        """
        Determine size of memory to test.
        """
        self._client_at.run_test('hardware_RamFio', size=0, dry_run=True)

        keyval_path = os.path.join(self.outputdir,
                                   'hardware_RamFio',
                                   'results',
                                   'keyval')

        return utils.read_keyval(keyval_path, type_tag='perf')['Size']


    def _create_ramfs(self):
        """
        Create ramfs mount directory on client
        """
        self._client.run('mkdir -p /tmp/ramdisk')
        self._client.run('mount -t ramfs ramfs /tmp/ramdisk')


    def _write_test_data(self, size):
        """
        Write test data using hardware_StorageFio.
        """
        self._client_at.run_test('hardware_StorageFio',
                                 tag='_write_test_data',
                                 dev='/tmp/ramdisk/test_file',
                                 size=size,
                                 requirements=[('8k_async_randwrite', [])])


    def _wait(self, seconds, suspend):
        """
        Wait for specifed time. Also suspend if specified.
        """
        if suspend:
            self._client.suspend(suspend_time=seconds)
        else:
            time.sleep(seconds)


    def _verify_test_data(self, size):
        """
        Verify integrity of written data using hardware_StorageFio.
        """
        # 'v' option means verify only
        self._client_at.run_test('hardware_StorageFio',
                                 tag='_verify_test_data',
                                 dev='/tmp/ramdisk/test_file',
                                 size=size,
                                 wait=0,
                                 requirements=[('8k_async_randwrite', ['v'])])

    def _check_alive(self):
        """
        Check that client is still alived. Raise error if not.
        """
        if not self._client.ping_wait_up(30):
            raise error.TestFail("Test fail: Client died")

    def _clean_up(self):
        """
        Cleanup the system. Unmount the ram disk.
        """
        self._client.run('umount /tmp/ramdisk')


    def run_once(self, client_ip=None, seconds=3600, size=0, suspend=True):
        """
        Run the test
        Use ram drive and hardware_Storage fio verify feature to verify the
        integrity of memory system. The test will write data to memory and then
        idle or suspend for specify time and verify the integrity of that data.

        @param client_ip: string of client's ip address (required)
        @param seconds:   seconds to idle / suspend. default = 3600 (1 hour)
        @param size:      size to used. 0 means use all tesable memory (default)
        @param suspend:   set to suspend between write and verify phase.
        """

        if not client_ip:
            error.TestError("Must provide client's IP address to test")

        self._client = hosts.create_host(client_ip)
        self._client_at = autotest.Autotest(self._client)

        if size == 0:
            size = self._determine_usable_memory()
        logging.info('size: %d', size)

        self._create_ramfs()

        self._write_test_data(size)

        self._check_alive()
        self._wait(seconds, suspend)
        self._check_alive()

        self._verify_test_data(size)

        self._check_alive()
        self._clean_up()

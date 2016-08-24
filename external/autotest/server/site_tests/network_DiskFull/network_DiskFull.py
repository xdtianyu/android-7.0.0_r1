# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test
from autotest_lib.server.cros import remote_command

class network_DiskFull(test.test):
    """Test networking daemons when /var is full."""

    version = 1
    CLIENT_TEST_LIST = [
        ('network_DhcpNegotiationSuccess', {}),
        ('network_DhcpRenew', {}),
        ('network_RestartShill', {
                'tag': 'profile_exists',
                'remove_profile': False}),
        ('network_RestartShill', {
                'tag': 'profile_missing',
                'remove_profile': True}),
        ]
    CLIENT_TMP_DIR = '/tmp'
    DISK_FILL_SCRIPT = 'hog_disk.sh'
    FILL_TIMEOUT_SECONDS = 5
    MAX_FREE_KB = 1024
    STATEFUL_PATH = '/var'
    TEST_TIMEOUT_SECONDS = 180

    def get_free_kilobytes(self, mount_point):
        """
        Get the size of free space on the filesystem mounted at |mount_point|,
        in kilobytes.

        @return Kilobytes free, as an integer.
        """
        # Filesystem              1024-blocks  Used Available Capacity Mount...
        # /dev/mapper/encstateful      290968 47492    243476      17% /var
        output = self._client.run('df -P %s' % mount_point).stdout
        lines = output.splitlines()
        if len(lines) != 2:
            raise error.TestFail('Unexpected df output: %s' % lines)
        _, _, _, free_kb, _, df_mount_point = lines[1].split(None, 5)
        if df_mount_point != mount_point:
            raise error.TestFail('Failed to find %s, got %s instead.' %
                                 (mount_point, df_mount_point))
        return int(free_kb)


    def wait_until_full(self, mount_point, max_free_kilobytes):
        """
        Wait until |mount_point| has no more than |max_free_kilobytes| free.

        @param mount_point The path at which the filesystem is mounted.
        @param max_free_kilobytes Maximum free space permitted, in kilobytes.
        @return True if the disk is full, else False
        """
        start_time = time.time()
        while time.time() - start_time < self.FILL_TIMEOUT_SECONDS:
            if (self.get_free_kilobytes(mount_point) <= max_free_kilobytes):
                return True
            else:
                time.sleep(1)
        return False


    def run_once(self, client_addr):
        """
        Test main loop.

        @param client_addr DUT hostname or IP address.
        """
        self._client = hosts.create_host(client_addr)
        client_autotest = autotest.Autotest(self._client)

        disk_filler_src = os.path.join(self.bindir, self.DISK_FILL_SCRIPT)
        disk_filler_dst = os.path.join(self.CLIENT_TMP_DIR,
                                       os.path.basename(self.DISK_FILL_SCRIPT))
        self._client.send_file(disk_filler_src, disk_filler_dst)

        disk_filler_command = '%s %s %d' % (
            disk_filler_dst, self.STATEFUL_PATH, self.TEST_TIMEOUT_SECONDS)

        with remote_command.Command(self._client, disk_filler_command) \
                as disk_filler_process:
            if not self.wait_until_full(self.STATEFUL_PATH, self.MAX_FREE_KB):
                logging.debug(disk_filler_process.result)
                raise error.TestFail(
                    'did not fill %s within %d seconds' % (
                        self.STATEFUL_PATH, self.FILL_TIMEOUT_SECONDS))

            client_autotest.run_test('network_CheckCriticalProcesses',
                                     tag='before_client_tests')
            passed_with_failsafe = []

            for name, kwargs in self.CLIENT_TEST_LIST:
                # Autotest goes to /mnt/stateful_partition/dev_image,
                # while /var is on /mnt/stateful_partition/encrypted.
                #
                # These are separate partitions, so we can copy
                # the tests onto the DUT even when /var is full.
                client_autotest.run_test(name, **kwargs)

                if 'tag' in kwargs:
                    full_test_name = '%s.%s' % (name, kwargs['tag'])
                else:
                    full_test_name = name

                # To avoid leaving the system in a bad state, the disk
                # filler times out eventually. This means a test can
                # "pass" due to the failsafe. Check if the failsafe
                # kicked in, by checking if the disk is still full.
                if (self.get_free_kilobytes(self.STATEFUL_PATH) >
                    self.MAX_FREE_KB):
                    passed_with_failsafe.append(full_test_name)

                client_autotest.run_test('network_CheckCriticalProcesses',
                                         tag='after_%s' % full_test_name)

            if len(passed_with_failsafe):
                raise error.TestFail(
                    '%d test(s) triggered the fail-safe: %s. '
                    'They may be incorrectly listed as passing.' % (
                        len(passed_with_failsafe),
                        ', '.join(passed_with_failsafe)))

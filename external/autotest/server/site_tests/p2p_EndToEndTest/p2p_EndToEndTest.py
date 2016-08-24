# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import random
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros import queue_barrier


# P2P_PATH is the path where the p2p server expects the sharing files.
P2P_PATH = '/var/cache/p2p'

# Prefix all the test files with P2P_TEST_PREFIX.
P2P_TEST_PREFIX = 'p2p-test'

# File size of the shared file in KB.
P2P_FILE_SIZE_KB = 80 * 1000

# After a peer finishes the download we need it to keep serving the file for
# other peers. This peer will then wait up to P2P_SERVING_TIMEOUT_SECS seconds
# for the test to conclude.
P2P_SERVING_TIMEOUT_SECS = 600

# The file is initialy shared by the master in two parts. The first part is
# available at the beginning of the test, while the second part of the file
# becomes ready in the master after P2P_SHARING_GAP_SECS seconds.
P2P_SHARING_GAP_SECS = 90

# The master and clients have to initialize the p2p service and, in the case
# of the master, generate the first part of the file on disk.
P2P_INITIALIZE_TIMEOUT_SECS = 90

class p2p_EndToEndTest(test.test):
    """Test to check that p2p works."""
    version = 1


    def run_once(self, dut, file_id, is_master, peers, barrier):
        self._dut = dut

        file_id = '%s-%s' % (P2P_TEST_PREFIX, file_id)
        file_temp_name = os.path.join(P2P_PATH, file_id + '.tmp')
        file_shared_name = os.path.join(P2P_PATH, file_id + '.p2p')

        # Ensure that p2p is running.
        dut.run('start p2p || true')
        dut.run('status p2p | grep running')

        # Prepare the file - this includes specifying its final size.
        dut.run('touch %s' % file_temp_name)
        dut.run('setfattr -n user.cros-p2p-filesize -v %d %s'
                % (P2P_FILE_SIZE_KB * 1000, file_temp_name))
        dut.run('mv %s %s' % (file_temp_name, file_shared_name))

        if is_master:
            # The master generates a file and shares a part of it but announces
            # the total size via the "user.cros-p2p-filesize" attribute.
            # To ensure that the clients are retrieving this first shared part
            # and hopefully blocking until the rest of the file is available,
            # a sleep is included in the master side.

            logging.info('Master process running.')

            first_part_size_kb = P2P_FILE_SIZE_KB / 3
            dut.run('dd if=/dev/urandom of=%s bs=1000 count=%d'
                    % (file_shared_name, first_part_size_kb))

            # This small sleep is to ensure that the new file size is updated
            # by avahi daemon.
            time.sleep(5)

            # At this point, the master is sharing a non-empty file, signal all
            # the clients that they can start the test. The clients should not
            # take more and a few seconds to launch.
            barrier.master_barrier(timeout=P2P_INITIALIZE_TIMEOUT_SECS)

            # Wait some time to allow clients download a partial file.
            time.sleep(P2P_SHARING_GAP_SECS)
            dut.run('dd if=/dev/urandom of=%s bs=1000 count=%d'
                    ' conv=notrunc oflag=append'
                    % (file_shared_name, P2P_FILE_SIZE_KB - first_part_size_kb))
        else:
            # On the client side, first wait until the master is sharing
            # a non-empty file, otherwise p2p-client will ignore the file.
            # The master should not take more than a few seconds to generate
            # the file.
            barrier.slave_barrier(timeout=P2P_INITIALIZE_TIMEOUT_SECS)

            # Wait a random time in order to not launch all the downloads
            # at the same time, otherwise all devices would be seeing
            # num-connections < $THRESHOLD .
            r = random.Random()
            secs_to_sleep = r.randint(1, 10)
            logging.debug('Sleeping %d seconds', secs_to_sleep)
            time.sleep(secs_to_sleep)

            # Attempt the file download and start sharing it while
            # downloading it.
            ret = dut.run('p2p-client --get-url=%s' % file_id)
            url = ret.stdout.strip()

            if not url:
                raise error.TestFail('p2p-client returned an empty URL.')
            else:
                logging.info('Using URL %s', url)
                dut.run('curl %s -o %s' % (url, file_shared_name))

        # Calculate the SHA1 (160 bits -> 40 characters when
        # hexencoded) of the file and report this back so the
        # server-side test can check they're all the same.
        ret = dut.run('sha1sum %s' % file_shared_name)
        sha1 = ret.stdout.strip()[0:40]
        logging.info('SHA1 is %s', sha1)

        # Wait for all the clients to finish and check the received SHA1.
        if is_master:
            try:
                client_sha1s = barrier.master_barrier(
                        timeout=P2P_SERVING_TIMEOUT_SECS)
            except queue_barrier.QueueBarrierTimeout:
                raise error.TestFail("Test failed to complete in %d seconds."
                                     % P2P_SERVING_TIMEOUT_SECS)

            for client_sha1 in client_sha1s:
                if client_sha1 != sha1:
                    # Wrong SHA1 received.
                    raise error.TestFail("Received SHA1 (%s) doesn't match "
                            "master's SHA1 (%s)." % (client_sha1, sha1))
        else:
            try:
                barrier.slave_barrier(sha1, timeout=P2P_SERVING_TIMEOUT_SECS)
            except queue_barrier.QueueBarrierTimeout:
                raise error.TestFail("Test failed to complete in %d seconds."
                                     % P2P_SERVING_TIMEOUT_SECS)


    def cleanup(self):
        # Clean the test environment and stop sharing this file.
        self._dut.run('rm -f %s/%s-*.p2p' % (P2P_PATH, P2P_TEST_PREFIX))

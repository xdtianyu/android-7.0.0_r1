# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

import logging


POWERWASH_COUNT = '/mnt/stateful_partition/unencrypted/preserve/powerwash_count'

POWERWASH_MARKER_FILE = '/mnt/stateful_partition/factory_install_reset'

POWERWASH_COMMAND = 'safe fast keepimg'

STATEFUL_MARKER_FILE = '/mnt/stateful_partition/platform_Powerwash_flag'


class platform_Powerwash(test.test):
    """Powerwash a device."""
    version = 1

    def run_once(self, host):
        self._host = host

        count_before = self._powerwash_count()

        # We create a file on the stateful partition to test if it is deleted
        # during the powerwash.
        self._host.run('echo car > %s' % STATEFUL_MARKER_FILE)

        logging.debug('Signaling powerwash on the device.')
        self._mark_powerwash()
        self._host.reboot()

        # Check if the marker file still exists on the stateful partition.
        # The powerwash cycle should remove it.
        marker = self._host.run('[ -e %s ]' % STATEFUL_MARKER_FILE,
                                ignore_status=True, ignore_timeout=True)

        # If "[ -e file ]" finishes with status 0, the file is present.
        if marker is None or marker.exit_status == 0:
            raise error.TestFail("Powerwash cycle didn't remove the marker "
                                 "file on the stateful partition.")

        # Check the powerwash counter before and after the powerwash to verify
        # it was incremented. This file should be preserved by the powerwash.
        count_after = self._powerwash_count()
        if count_after != count_before + 1:
            raise error.TestFail("Powerwash count didn't increase after "
                                 "powerwash cycle.")


    def _mark_powerwash(self, command=None):
        """Creates the Powerwash marker file on the host with the given command.

        @param command: The text to include on the marker file, *not* including
                        the '\n' at the end.
        """
        if command is None:
            command = POWERWASH_COMMAND
        self._host.run("echo '%s' > %s" % (command, POWERWASH_MARKER_FILE))


    def _powerwash_count(self):
        """Return the powerwash count from the DUT."""
        count = self._host.run('cat %s' % POWERWASH_COUNT,
                               ignore_status=True).stdout.strip()
        logging.debug('Powerwash count is: %r', count)
        if count:
            return int(count)
        return 0

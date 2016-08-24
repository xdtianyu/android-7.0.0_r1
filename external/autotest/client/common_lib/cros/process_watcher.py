# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

# Use this with ProcessWatcher to start your process in a minijail.  This
# is useful for instance if you would like to drop autotest's default root
# priviledges.  Both fields must be set to valid users/groups.
MinijailConfig = collections.namedtuple('MinijailConfig', ['user', 'group'])


class ProcessWatcher(object):
    """Start a process, and terminate it later."""

    def __init__(self, command, args=tuple(), minijail_config=None, host=None):
        """Construst a ProcessWatcher without starting the process.

        @param command: string command to use to start the process.
        @param args: list of strings to pass to the command.
        @param minijail_config: MinijailConfig tuple defined above.
        @param host: host object if the server should be started on a remote
                host.

        """
        self._command = ' '.join([command] + list(args))
        if '"' in self._command:
            raise error.TestError('Please implement shell escaping in '
                                  'ProcessWatcher.')
        self._minijail_config = minijail_config
        self._run = utils.run if host is None else host.run


    def start(self):
        """Start a (potentially remote) instance of the process."""
        command = self._command
        prefix = ''
        if self._minijail_config is not None:
            prefix = 'minijail0 -i -g %s -u %s ' % (self._minijail_config.group,
                                                    self._minijail_config.user)
        # Redirect output streams to avoid odd interactions between autotest's
        # shell environment and the command's runtime environment.
        self._run('%s%s >/dev/null 2>&1 &' % (prefix, self._command))


    def close(self, timeout_seconds=40):
        """Close the (potentially remote) instance of the process.

        @param timeout_seconds: int number of seconds to wait for shutdown.

        """
        self._run('pkill -f --signal TERM "%s"' % self._command,
                  ignore_status=True)
        start_time = time.time()
        while time.time() - start_time < timeout_seconds:
            result = self._run('pgrep -f -l "%s"' % self._command,
                               ignore_status=True)
            if result.exit_status != 0:
                return
            time.sleep(0.3)
        raise error.TestError('Timed out waiting for %s to die.' %
                              self._command)

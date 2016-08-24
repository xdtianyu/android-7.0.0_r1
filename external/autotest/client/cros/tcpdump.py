# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess

from autotest_lib.client.bin import utils

class Tcpdump(object):
    """tcpdump capture process wrapper."""

    def __init__(self, iface, dumpfilename):
        """Launches a tcpdump process on the background.

        @param iface: The name of the interface to listen on.
        @param dumpfilename: The filename of the destination dump file.
        @raise utils.TimeoutError if tcpdump fails to start after 10 seconds.
        """
        logging.debug('Recording %s traffic to %s.', iface, dumpfilename)
        # Force to run tcpdump as root, since the dump file is created *after*
        # the process drops to a unprivileged user, meaning that it can't create
        # the passed dumpfilename file.
        self._tcpdump_proc = subprocess.Popen(
                ['tcpdump', '-i', iface, '-w', dumpfilename, '-Z', 'root'],
                stdout=open('/dev/null', 'w'),
                stderr=subprocess.STDOUT)
        # Wait for tcpdump to initialize and create the dump file.
        utils.poll_for_condition(
                lambda: os.path.exists(dumpfilename),
                desc='tcpdump creates the dump file.',
                sleep_interval=1,
                timeout=10.)


    def stop(self, timeout=10.):
        """Stop the dump process and wait for it to return.

        This method stops the tcpdump process running in background and waits
        for it to finish for a given timeout.
        @param timeout: The time to wait for the tcpdump to finish in seconds.
                        None means no timeout.
        @return whether the tcpdump is not running.
        """
        if not self._tcpdump_proc:
            return True

        # Send SIGTERM to tcpdump.
        try:
            self._tcpdump_proc.terminate()
        except OSError, e:
            # If the process exits before we can send it a SIGTERM, an
            # OSError exception is raised here which we can ignore since the
            # process already finished.
            logging.error('Trying to kill tcpdump (%d): %s',
                          self._tcpdump_proc.pid, e.strerror)

        logging.debug('Waiting for pid %d to finish.', self._tcpdump_proc.pid)
        if timeout is None:
            self._tcpdump_proc.wait()
        else:
            try:
                utils.poll_for_condition(
                        lambda: not self._tcpdump_proc.poll() is None,
                        sleep_interval=1,
                        timeout=timeout)
            except utils.TimeoutError:
                logging.error('tcpdump failed to finish after %f seconds. Dump '
                              'file can be truncated.', timeout)
                return False

        self._tcpdump_proc = None
        return True


    def __del__(self):
        self.stop()

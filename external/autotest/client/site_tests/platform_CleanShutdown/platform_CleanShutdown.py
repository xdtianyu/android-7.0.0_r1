# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


PROCESS_WHITELIST = (
    # TODO(dalecurtis): Remove once http://crosbug.com/15697 is fixed.
    'cryptohom',
    'chapsd',
)

SHUTDOWN_CRYPTOHOME_UMOUNT_FAIL = '/var/log/shutdown_cryptohome_umount_failure'
SHUTDOWN_STATEFUL_UMOUNT_FAIL = '/var/log/shutdown_stateful_umount_failure'
SHUTDOWN_KILLED_PROCESSES_LOG = '/var/log/shutdown_force_kill_processes'


class platform_CleanShutdown(test.test):
    version = 1


    def _log_remove_if_exists(self, filename, message):
        if not os.path.exists(filename):
            return

        contents = utils.read_file(filename).strip()
        os.remove(filename)

        if filename == SHUTDOWN_KILLED_PROCESSES_LOG:
            # Remove all killed processes listed in the white list. An example
            # log is included below:
            #
            #    COMMAND     PID    USER  FD   TYPE DEVICE SIZE/OFF   NODE NAME
            #    cryptohom  [........]
            #
            filtered_contents = filter(
                lambda line: not line.startswith(PROCESS_WHITELIST),
                contents.splitlines())

            # If there are no lines left but the header, return nothing.
            if len(filtered_contents) <= 1:
                return
            else:
                contents = '\n'.join(filtered_contents)

        logging.error('Last shutdown problem: %s. Detailed output was:\n%s' %
                      (message, contents))
        self._errors.append(message)


    def run_once(self):
        self._errors = []
        # Problems during shutdown are brought out in /var/log files
        # which we show here.
        self._log_remove_if_exists(SHUTDOWN_CRYPTOHOME_UMOUNT_FAIL,
                                   'cryptohome unmount failed')
        self._log_remove_if_exists(SHUTDOWN_STATEFUL_UMOUNT_FAIL,
                                   'stateful unmount failed')
        self._log_remove_if_exists(SHUTDOWN_KILLED_PROCESSES_LOG,
                                   'force killed processes')
        if self._errors:
            raise error.TestFail(
                'Last shutdown problems: %s' % ' and '.join(self._errors))

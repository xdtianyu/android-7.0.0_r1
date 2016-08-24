# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.common_lib import error
from autotest_lib.server import test, autotest


class platform_TrackpadStressServer(test.test):
    """
    Make sure the trackpad continues to operate after a kernel panic.
    """
    version = 1

    def _run_client_test(self, client_at, verify_only=False):
        self.job.set_state('client_passed', None)
        client_at.run_test(self.client_test)
        return self.job.get_state('client_passed')

    def run_once(self, host=None):
        self.client = host
        self.client_test = 'platform_TrackpadStress'

        logging.info('TrackpadStressServer: start client test')
        client_at = autotest.Autotest(self.client)
        if not self._run_client_test(client_at):
            raise error.TestFail('Client test failed precheck state')

        # Configure the client to reboot on a kernel panic
        self.client.run('sysctl kernel.panic|grep "kernel.panic = -1"')
        self.client.run('sysctl kernel.panic_on_oops|'
                        'grep "kernel.panic_on_oops = 1"')

        boot_id = self.client.get_boot_id()

        # Make it rain
        command  = 'echo BUG > /sys/kernel/debug/provoke-crash/DIRECT'
        command += '|| echo bug > /proc/breakme'
        logging.info('TrackpadStressServer: executing "%s" on %s',
                     command, self.client.hostname)
        try:
            # Simply writing to the crash interface resets the target
            # immediately, leaving files unsaved to disk and the master ssh
            # connection wedged for a long time. The sequence below borrowed
            # from logging_KernelCrashServer.py makes sure that the test
            # proceeds smoothly.
            self.client.run(
                'sh -c "sync; sleep 1; %s" >/dev/null 2>&1 &' % command)
        except error.AutoservRunError, e:
            # It is expected that this will cause a non-zero exit status.
            pass

        self.client.wait_for_restart(
                    down_timeout=60,
                    down_warning=60,
                    old_boot_id=boot_id,
                    # Extend the default reboot timeout as some targets take
                    # longer than normal before ssh is available again.
                    timeout=self.client.DEFAULT_REBOOT_TIMEOUT * 4)

        # Check that the trackpad is running
        # Todo, need an additional client test.
        if not self._run_client_test(client_at, verify_only=True):
            raise error.TestFail('Client test failed final state verification.'
                                 'The trackpad is not running after kernel '
                                 'panic.')



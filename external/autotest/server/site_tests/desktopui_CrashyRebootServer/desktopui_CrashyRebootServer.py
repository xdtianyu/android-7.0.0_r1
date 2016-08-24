# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test, autotest

class desktopui_CrashyRebootServer(test.test):
    """Validate logic for mitigating too-crashy UI.

    If the UI crashes too much too fast, the device will eventually
    reboot to attempt to mitigate the problem. If the device
    determines that it's already tried that once, it will shut down
    the UI and remain up.

    This test deploys the client test desktopui_CrashyReboot in order
    to drive the device into the desired states.
    """
    version = 1

    CRASHY_DEVICE_TIMEOUT_SECONDS = 120
    CLIENT_TEST = 'desktopui_CrashyReboot'

    def run_once(self, host=None):
        host.run('rm -f /var/lib/ui/reboot-timestamps')
        boot_id = host.get_boot_id()

        # Run a client-side test that crashes the UI a bunch, and
        # expect a reboot.  We need to run this test in the background in
        # order to prevent the reboot from causing autotest to auto-fail
        # the entire test. This means we also need to handle collecting
        # and parsing results manually if it doesn't work.
        logging.info('CrashyRebootServer: start client test')
        tag = 'reboot'
        client_at = autotest.Autotest(host)
        client_at.run_test(self.CLIENT_TEST, expect_reboot=True, tag='reboot',
                           background=True)

        logging.info('Client test now running in background.')
        # Prepare for result gathering.
        collector = autotest.log_collector(host, None, '.')
        host.job.add_client_log(host.hostname,
                                collector.client_results_dir,
                                collector.server_results_dir)
        job_record_context = host.job.get_record_context()

        logging.info('Waiting for host to go down.')
        if not host.wait_down(timeout=self.CRASHY_DEVICE_TIMEOUT_SECONDS,
                              old_boot_id=boot_id):
            # Gather results to determine why device didn't reboot.
            collector.collect_client_job_results()
            collector.remove_redundant_client_logs()
            host.job.remove_client_log(host.hostname,
                                       collector.client_results_dir,
                                       collector.server_results_dir)
            job_record_context.restore()
            raise error.TestError('Host should have rebooted!')

        logging.info('Waiting for host to come back up.')
        try:
            # wait_up() issues an ssh connection attempt and then spends
            # the entire given timeout waiting for it to succeed. If it
            # does this before the device is ready to accept ssh
            # connections, it will decide that the device never came up,
            # even if it is ready and waiting. To combat this, loop with
            # a short timeout.
            utils.poll_for_condition(lambda: host.wait_up(5),
                                     timeout=self.CRASHY_DEVICE_TIMEOUT_SECONDS)
        except utils.TimeoutError:
            raise error.TestError('Host never came back!')

        # NB: If we change the reboot-attempt threshold in
        # /etc/init/ui-respawn.conf to be >1, this will start failing
        # and need to be updated.
        client_at.run_test(self.CLIENT_TEST, expect_reboot=False)

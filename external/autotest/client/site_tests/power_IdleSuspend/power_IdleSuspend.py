# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, tempfile, threading
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

POWER_MANAGER_SETTINGS = {
    'plugged_dim_ms': 1000,
    'plugged_off_ms': 5000,
    'plugged_suspend_ms': 10000,
    'unplugged_dim_ms': 1000,
    'unplugged_off_ms': 5000,
    'unplugged_suspend_ms': 10000,
    'disable_idle_suspend': 0,
    'ignore_external_policy': 1,
}

SUSPEND_TIMEOUT_MS = 30000


class power_IdleSuspend(test.test):
    """
    Verify power manager tries to suspend while idle.

    This test does not actually allow the system to suspend. Instead,
    it replaces /sys/power/state with a pipe and waits until "mem" is
    written to it. Such a write would normally cause suspend.
    """
    version = 1
    mounts = ()

    def initialize(self):
        super(power_IdleSuspend, self).initialize()
        self.mounts = []


    def setup_power_manager(self):
        # create directory for temporary settings
        self.tempdir = tempfile.mkdtemp(prefix='IdleSuspend.')
        logging.info('using temporary directory %s', self.tempdir)

        # override power manager settings
        for key, val in POWER_MANAGER_SETTINGS.iteritems():
            logging.info('overriding %s to %s', key, val)
            tmp_path = '%s/%s' % (self.tempdir, key)
            mount_path = '/usr/share/power_manager/%s' % key
            utils.write_one_line(tmp_path, str(val))
            utils.run('mount --bind %s %s' % (tmp_path, mount_path))
            self.mounts.append(mount_path)

        # override /sys/power/state with fifo
        fifo_path = '%s/sys_power_state' % self.tempdir
        os.mkfifo(fifo_path)
        utils.run('mount --bind %s /sys/power/state' % fifo_path)
        self.mounts.append('/sys/power/state')


    def wait_for_suspend(self):
        # block reading new power state from /sys/power/state
        sys_power_state = open('/sys/power/state')
        self.new_power_state = sys_power_state.read()
        logging.info('new power state: %s', self.new_power_state)


    def run_once(self):
        with chrome.Chrome():
            # stop power manager before reconfiguring
            logging.info('stopping powerd')
            utils.run('stop powerd')

            # override power manager settings
            self.setup_power_manager()

            # start thread to wait for suspend
            self.new_power_state = None
            thread = threading.Thread(target=self.wait_for_suspend)
            thread.start()

            # restart powerd to pick up new settings
            logging.info('restarting powerd')
            utils.run('start powerd')

            # wait for idle suspend
            thread.join(SUSPEND_TIMEOUT_MS / 1000.)

            if thread.is_alive():
                # join timed out - powerd didn't write to /sys/power/state
                raise error.TestFail('timed out waiting for suspend')

            if self.new_power_state is None:
                # probably an exception in the thread, check the log
                raise error.TestError('reader thread crashed')

            if self.new_power_state.strip() != 'mem':
                # oops, power manager wrote something other than "mem"
                err_str = 'bad power state written to /sys/power/state'
                raise error.TestFail(err_str)


    def cleanup(self):
        # restore original power manager settings
        for mount in self.mounts:
            logging.info('restoring %s', mount)
            utils.run('umount -l %s' % mount)

        # restart powerd to pick up original settings
        logging.info('restarting powerd')
        utils.run('restart powerd')

        super(power_IdleSuspend, self).cleanup()

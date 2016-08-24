# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig

BOOT_WAIT_SECONDS = 100
DARK_RESUME_SOURCE_PREF = '/sys/class/rtc/rtc0/device'
POWER_DIR = '/var/lib/power_manager'
SHUTDOWN_WAIT_SECONDS = 30
SUSPEND_DURATION = 20
SUSPEND_DURATION_PREF = '0.0'
SUSPEND_WAIT_SECONDS = 10
TMP_POWER_DIR = '/tmp/power_manager'


class power_DarkResumeShutdownServer(test.test):
    """Test power manager shut down from dark resume action."""
    version = 1


    def initialize(self, host):
        # save original boot id
        self.orig_boot_id = host.get_boot_id()

        host.run('mkdir -p %s' % TMP_POWER_DIR)
        # override suspend durations preference for dark resume
        logging.info('setting dark_resume_suspend_durations to %s %d',
                      SUSPEND_DURATION_PREF, SUSPEND_DURATION)
        host.run('echo %s %d > %s/dark_resume_suspend_durations' %
                 (SUSPEND_DURATION_PREF, SUSPEND_DURATION, TMP_POWER_DIR))

        # override sources preference for dark resume
        logging.info('setting dark_resume_sources to %s',
                     DARK_RESUME_SOURCE_PREF)
        host.run('echo %s > %s/dark_resume_sources' %
                 (DARK_RESUME_SOURCE_PREF, TMP_POWER_DIR))

        # override disabling of dark resume
        logging.info('enabling dark resume')
        host.run('echo 0 > %s/disable_dark_resume' % TMP_POWER_DIR)

        # bind the tmp directory to the power preference directory
        host.run('mount --bind %s %s' % (TMP_POWER_DIR, POWER_DIR))

        # restart powerd to pick up new dark resume settings
        logging.info('restarting powerd')
        host.run('restart powerd')


    def platform_supports_dark_resume(self, platform_name):
        """Check if the test works on the given platform

        @param platform_name: the name of the given platform
        """
        client_attr = FAFTConfig(platform_name)
        return client_attr.dark_resume_capable


    def run_once(self, host=None):
        """Run the test.

           Setup preferences so that a dark resume will happen shortly after
           suspending the machine.

           suspend the machine
           wait
           turn off AC power
           wait for shutdown
           reboot
           turn on AC power

        @param host: The machine to run the tests on
        """
        platform = host.run_output('mosys platform name')
        logging.info('Checking platform %s for compatibility with dark resume',
                     platform)
        if not self.platform_supports_dark_resume(platform):
            return

        host.power_on()
        # The IO redirection is to make the command return right away. For now,
        # don't go through sys_power for suspending since those code paths use
        # the RTC.
        # TODO(dbasehore): rework sys_power to make the RTC alarm optional
        host.run('/usr/bin/powerd_dbus_suspend --delay=1 '
                 '> /dev/null 2>&1 < /dev/null &')
        time.sleep(SUSPEND_WAIT_SECONDS)
        host.power_off()

        # wait for power manager to give up and shut down
        logging.info('waiting for power off')
        host.wait_down(timeout=SHUTDOWN_WAIT_SECONDS,
                       old_boot_id=self.orig_boot_id)

        # ensure host is now off
        if host.is_up():
            raise error.TestFail('DUT still up. Machine did not shut down from'
                                 ' dark resume')
        else:
            logging.info('good, host is now off')

        # restart host
        host.power_on()
        host.servo.power_normal_press()
        if not host.wait_up(timeout=BOOT_WAIT_SECONDS):
            raise error.TestFail('DUT did not turn back on after shutting down')


    def cleanup(self, host):
        # make sure that the machine is not suspended and that the power is on
        # when exiting the test
        host.power_on()
        host.servo.ctrl_key()

        # try to clean up the mess we've made if shutdown failed
        if host.is_up() and host.get_boot_id() == self.orig_boot_id:
            # clean up mounts
            logging.info('cleaning up bind mounts')
            host.run('umount %s' % POWER_DIR,
                     ignore_status=True)

            # restart powerd to pick up old retry settings
            host.run('restart powerd')

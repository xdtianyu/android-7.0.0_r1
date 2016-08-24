# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json, logging, threading, time, traceback

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, test
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig

_RETRY_SUSPEND_ATTEMPTS = 1
_RETRY_SUSPEND_MS = 10000
_SUSPEND_WAIT_SECONDS = 30
_BOOT_WAIT_SECONDS = 100


class power_SuspendShutdown(test.test):
    """Test power manager fallback to power-off if suspend fails."""
    version = 1

    def initialize(self, host):
        """
        Initial settings before running test.

        @param host: Host/DUT object to run test on.

        """
        # save original boot id
        self.orig_boot_id = host.get_boot_id()
        self.host = host

        # override /sys/power/state via bind mount
        logging.info('binding /dev/full to /sys/power/state')
        host.run('mount --bind /dev/full /sys/power/state')

        # override suspend retry attempts via bind mount
        logging.info('settings retry_suspend_attempts to %s',
                     _RETRY_SUSPEND_ATTEMPTS)
        host.run('echo %s > /tmp/retry_suspend_attempts;'
                 ' mount --bind /tmp/retry_suspend_attempts'
                 ' /usr/share/power_manager/retry_suspend_attempts'
                 % _RETRY_SUSPEND_ATTEMPTS)

        # override suspend retry interval via bind mount
        logging.info('settings retry_suspend_ms to %s',
                     _RETRY_SUSPEND_MS)
        host.run('echo %s > /tmp/retry_suspend_ms;'
                 ' mount --bind /tmp/retry_suspend_ms'
                 ' /usr/share/power_manager/retry_suspend_ms'
                 % _RETRY_SUSPEND_MS)

        # restart powerd to pick up new retry settings
        logging.info('restarting powerd')
        host.run('restart powerd')
        time.sleep(2)


    def platform_check(self, platform_name):
        """
        Raises error if device does not have a lid.

        @param platform_name: Name of the platform

        """
        client_attr = FAFTConfig(platform_name)

        if not client_attr.has_lid:
            raise error.TestError(
                    'This test does nothing on devices without a lid.')

        if client_attr.chrome_ec and not 'lid' in client_attr.ec_capability:
            raise error.TestNAError("TEST IT MANUALLY! Chrome EC can't control "
                    "lid on the device %s" % client_attr.platform)


    def login_into_dut(self, client_autotest, thread_started_evt,
                       exit_without_logout=True):
        """
        Runs the Desktopui_Simple login client test in a seperate thread. The
        Desktopui_Simple client test will exit without logout.

        @param client_autotest: Client autotest name to login into DUT

        @param thread_started_evt: Thread attribute to start the thread

        @param exit_without_logout: if flag is set thread exists without logout.
                                    if not set, thread will wait fot logout
                                    event.

        """
        logging.info('Login into client started')
        thread_started_evt.set()
        try:
            self.autotest_client.run_test(client_autotest,
                                          exit_without_logout=
                                          exit_without_logout)
        except:
            logging.info('DUT login process failed')


    def create_thread(self, client_autotest, exit_without_logout):
        """
        Created seperate thread for client test

        @param client_autotest: Client autotest name to login into DUT

        @param exit_without_logout: if flag is set thread exists without logout.
                                    if not set, thread will wait fot logout
                                    event.
        @return t: thread object

        """
        thread_started_evt = threading.Event()
        logging.info('Launching Desktopui_simplelogin thread')
        try:
            t = threading.Thread(target=self.login_into_dut,
                                 args=(client_autotest,
                                 thread_started_evt, exit_without_logout))
        except:
            raise error.TestError('Thread creation failed')
        t.start()
        thread_started_evt.wait()
        logging.info('Login thread started')
        return t


    def logged_in(self):
        """
        Checks if the host has a logged in user.

        @param host: Host/DUT object

        @return True if a user is logged in on the device.

        """
        host = self.host
        try:
            out = host.run('cryptohome --action=status').stdout.strip()
        except:
            return False
        try:
            status = json.loads(out)
        except ValueError:
            logging.info('Cryptohome did not return a value, retrying.')
            return False

        return any((mount['mounted'] for mount in status['mounts']))


    def run_once(self, client_autotest):
        """
        Run the acutal test on device.

        @param client_autotest: Client autotest name to login into DUT

        @param host: Host/DUT object

        """
        # check platform is capable of running the test
        host = self.host
        platform = host.run_output('mosys platform name')
        self.platform_check(platform)
        self.autotest_client = autotest.Autotest(host)
        logging.info('platform is %s', platform)
        exit_without_logout = True
        t = self.create_thread(client_autotest, exit_without_logout)
        t.join()

        # Waiting for the login thread to finish
        max_wait_time = 15
        for check_count in range(int(max_wait_time)):
            if check_count == max_wait_time:
                raise error.TestError('Login thread is still'
                                      'alive after %s seconds' % max_wait_time)
            if t.is_alive():
                time.sleep(1)
            else:
                logging.info('Login thread successfully finished')
                break

        # close the lid while logged_in to initiate suspend
        logging.info('closing lid')
        host.servo.lid_close()

        # wait for power manager to give up and shut down
        logging.info('waiting for power off')
        host.wait_down(timeout=_SUSPEND_WAIT_SECONDS,
                       old_boot_id=self.orig_boot_id)

        # ensure host is now off
        if host.is_up():
            raise error.TestFail('DUT still up with lid closed')
        else:
            logging.info('good, host is now off')

        # restart host
        host.servo.lid_open()
        host.wait_up(timeout=_BOOT_WAIT_SECONDS)


    def cleanup(self):
        """Clean up the mounts and restore the settings."""
        # reopen lid - might still be closed due to failure
        host = self.host
        logging.info('reopening lid')
        host.servo.lid_open()

        # try to clean up the mess we've made if shutdown failed
        if host.get_boot_id() == self.orig_boot_id:
            # clean up mounts
            logging.info('cleaning up bind mounts')
            host.run('umount /sys/power/state'
                     ' /usr/share/power_manager/retry_suspend_attempts'
                     ' /usr/share/power_manager/retry_suspend_ms',
                     ignore_status=True)

            # restart powerd to pick up old retry settings
            host.run('restart powerd')

        # Reboot Device to logout and cleanup
        logging.info('Server: reboot client')
        try:
            self.host.reboot()
        except error.AutoservRebootError as e:
            raise error.TestFail('%s.\nTest failed with error %s' % (
                    traceback.format_exc(), str(e)))

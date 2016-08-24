# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


# Timeout for commands run on the host.
_COMMAND_TIMEOUT = 60

# Lock file created by flashrom to tell powerd not to suspend or shut down the
# system.
_LOCK_FILE = '/var/lock/flashrom_powerd.lock'

# Time in seconds to perform a flashrom write and to wait for the system to
# suspend and resume.
_SUSPEND_FLASHROM_SEC = 15
_SUSPEND_DOWN_SEC = 60
_SUSPEND_UP_SEC = 60

# Time in seconds to perform a flashrom write and to wait for the system to
# power down and reboot.
_REBOOT_FLASHROM_SEC = 15
_REBOOT_DOWN_SEC = 60
_REBOOT_UP_SEC = 60


class power_DeferForFlashrom(test.test):
    """Test that powerd defers suspend and shutdown for flashrom."""
    version = 1

    def initialize(self, host):
        """
        Initial settings before running test.

        @param host: Host/DUT object to use for test.
        """
        self.host = host


    def create_temp_file(self, base_name, source_path, size):
        """
        Create a temporary file on the host and returns its path.

        @param base_name: String containing the base name for the temp file.
        @param source_path: String containing the path to the device from
                which file contents should be read.
        @param size: Number of bytes to write to the file.
        """
        logging.info('Creating %d-byte temp file from %s', size, source_path)
        temp_file = self.host.run(
            'mktemp --tmpdir %s.XXXXXXXXXX' % base_name,
            timeout=_COMMAND_TIMEOUT).stdout.strip()
        self.host.run('dd if=%s of=%s bs=%d count=1 2>&1' %
            (source_path, temp_file, size))
        logging.info('Created %s', temp_file)
        return temp_file


    def run_in_background(self, cmd):
        """
        Asynchronously run a command on the host.

        @param cmd: Command to run (as a string).
        """
        bg_cmd = '(%s) </dev/null >/dev/null 2>&1 &' % (cmd)
        logging.info("Running %s", bg_cmd)
        self.host.run(bg_cmd, timeout=_COMMAND_TIMEOUT)


    def start_fake_flashrom_write(self, duration_sec):
        """
        Start a fake flashrom write.

        @param duration_sec: Duration for the write in seconds.
        """
        # flashrom simulates a 4096-byte block size, so the file size needs to
        # be a multiple of that.
        BLOCK_SIZE = 4096

        # flashrom will write one bit per cycle. Convert the block size to bits
        # (yielding the frequency for a one-second write) and then scale it as
        # needed.
        frequency_hz = int(BLOCK_SIZE * 8 / float(duration_sec))

        # To avoid flashrom needing to read (slowly) from the dummy device, pass
        # a custom diff file filled with zeroes.
        zero_file = self.create_temp_file(
            'power_DeferForFlashrom.zero', '/dev/zero', BLOCK_SIZE)
        rand_file = self.create_temp_file(
            'power_DeferForFlashrom.rand', '/dev/urandom', BLOCK_SIZE)

        # Start flashrom in the background and wait for it to create its lock
        # file.
        self.run_in_background(
            ('flashrom -w %s --diff %s --noverify '
             '-p dummy:freq=%d,emulate=VARIABLE_SIZE,size=%d,'
             'erase_to_zero=yes') %
            (rand_file, zero_file, frequency_hz, BLOCK_SIZE))

        logging.info("Waiting for flashrom to create %s...", _LOCK_FILE)
        self.host.run(
            'while [ ! -e %s ]; do sleep 0.1; done' % (_LOCK_FILE),
            timeout=_COMMAND_TIMEOUT)


    def send_suspend_request(self, wake_sec):
        """
        Asynchronously ask powerd to suspend the system immediately.

        @param wake_sec: Integer delay in seconds to use for setting a wake
                alarm. Note that the alarm starts when the request is sent to
                powerd, not when the system actually suspends.
        """
        self.run_in_background(
            'powerd_dbus_suspend --delay=0 --wakeup_timeout=%d' % (wake_sec))


    def send_reboot_request(self):
        """Ask powerd to reboot the system immediately."""
        logging.info('Calling powerd\'s RequestRestart method')
        self.host.run(
            ('dbus-send --type=method_call --system '
             '--dest=org.chromium.PowerManager /org/chromium/PowerManager '
             'org.chromium.PowerManager.RequestRestart'),
            timeout=_COMMAND_TIMEOUT)


    def wait_for_system_to_cycle(self, down_sec, up_sec):
        """
        Wait for the system to stop and then start responding to pings.

        @param down_sec: Maximum delay for the system to go down.
        @param up_sec: Maximum delay for the system to come back up.

        @return: Floating-point time when system went down.
        """
        logging.info("Waiting for host to go down...")
        if not self.host.ping_wait_down(timeout=down_sec):
            raise error.TestError(
                'System hasn\'t gone down after %d seconds' % (down_sec))
        down_timestamp = time.time()
        logging.info("System went down at %.2f", down_timestamp)

        logging.info("Waiting for host to come back up...")
        if not self.host.ping_wait_up(timeout=up_sec) or \
            not self.host.wait_up(timeout=up_sec):
            raise error.TestError('System didn\'t come back up')

        return down_timestamp


    def run_once(self):
        # Start flashrom and then request that the system be suspended. The
        # suspend should be deferred until flashrom finishes writing but should
        # happen eventually.
        flashrom_time = time.time()
        self.start_fake_flashrom_write(_SUSPEND_FLASHROM_SEC)
        self.send_suspend_request(_SUSPEND_DOWN_SEC)
        delay_sec = self.wait_for_system_to_cycle(
            _SUSPEND_DOWN_SEC, _SUSPEND_UP_SEC) - flashrom_time

        # Check that powerd waited for flashrom to finish.
        if delay_sec < _SUSPEND_FLASHROM_SEC:
            raise error.TestError(
                ('Suspend was blocked for %.2f sec; expected it to be blocked '
                 'for at least %d sec') % (delay_sec, _SUSPEND_FLASHROM_SEC))

        # Now do the same thing, but with a reboot request.
        flashrom_time = time.time()
        self.start_fake_flashrom_write(_REBOOT_FLASHROM_SEC)
        self.send_reboot_request()
        delay_sec = self.wait_for_system_to_cycle(
            _REBOOT_DOWN_SEC, _REBOOT_UP_SEC) - flashrom_time
        if delay_sec < _REBOOT_FLASHROM_SEC:
            raise error.TestError(
                ('Reboot was blocked for %.2f sec; expected it to be blocked '
                 'for at least %d sec') % (delay_sec, _REBOOT_FLASHROM_SEC))

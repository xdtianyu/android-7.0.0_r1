# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import tempfile
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import avahi_utils

class platform_DBusMachineIdRotation(test.test):
    """Verify that /var/lib/dbus/machine-id is properly rotated.

    To avoid interference with existing rotation scripts on the DUT,
    we actually don't use /var/lib/dbus/machine-id for
    testing. Instead we allocate a file on test start.
    """
    version = 1

    def initialize(self):
        """Allocates the machine-id file to use and initialize it."""
        fd, self._machine_id_file = tempfile.mkstemp(prefix='machine-id-rot-')
        os.write(fd, '0123456789abcdef0123456789abcdef\n')
        os.close(fd)

    def cleanup(self):
        """Cleans up the allocated machine-id file."""
        os.unlink(self._machine_id_file)

    def _get_machine_id(self):
        """Helper function to read the machine-id file."""
        with open(self._machine_id_file, 'r') as f:
            return f.read().strip()

    def _test_forced_rotation(self):
        """Check that forced regeneration work."""
        machine_id_before = self._get_machine_id()
        subprocess.check_call(['cros-machine-id-regen', '-r', 'network',
                               '-p', self._machine_id_file])
        machine_id_after = self._get_machine_id()
        if machine_id_before == machine_id_after:
            raise error.TestFail('Forced rotation failed.')

    def _test_time_limit(self):
        """Check that the machine-id is not regenerated unless a given amount
        of time has passed."""
        machine_id_before = self._get_machine_id()
        subprocess.check_call(['cros-machine-id-regen', '-r', 'network',
                               '-p', self._machine_id_file])
        machine_id_after = self._get_machine_id()
        if machine_id_before == machine_id_after:
            raise error.TestFail('Forced rotation failed.')

        # Now request a very long time limit (1000 seconds) and check
        # that the machine-id hasn't been regenerated.
        machine_id_before = self._get_machine_id()
        subprocess.check_call(['cros-machine-id-regen', '-r', 'periodic',
                               '-t', '1000', '-p', self._machine_id_file])
        machine_id_after = self._get_machine_id()
        if machine_id_before != machine_id_after:
            raise error.TestFail('Rotated despite timeout not reached.')

        # Sleep ten seconds and request regeneration if ten seconds
        # have passed. This should always result in regeneration.
        machine_id_before = self._get_machine_id()
        time.sleep(10)
        subprocess.check_call(['cros-machine-id-regen', '-r', 'periodic',
                               '-t', '10', '-p', self._machine_id_file])
        machine_id_after = self._get_machine_id()
        if machine_id_after == machine_id_before:
            raise error.TestFail('Not rotated despite timeout reached.')

    def _test_avahi_host_name(self):
        """Check that the Avahi host name is set to the machine-id when
        cros-machine-id-regen runs."""
        # Right now this throws if Avahi is running so manually
        # catch and ignore any error.
        try:
            avahi_utils.avahi_start()
        except:
            pass
        subprocess.check_call(['cros-machine-id-regen', '-r', 'network',
                               '-p', self._machine_id_file])
        machine_id = self._get_machine_id()
        host_name = avahi_utils.avahi_get_hostname()
        if host_name != machine_id:
            raise error.TestFail('Avahi host name not updated as expected.')

    def run_once(self):
        """Run tests related to /var/lib/dbus/machine-id rotation."""
        self._test_forced_rotation()
        self._test_time_limit()
        self._test_avahi_host_name()

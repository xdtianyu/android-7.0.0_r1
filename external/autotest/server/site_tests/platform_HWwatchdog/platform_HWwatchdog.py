# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This test isn't really HW specific. It's testing /dev/watchdog API
# to make sure it works as expected. The most robust implementations is
# based on real HW but it doesn't have to be.

import logging, re

# http://docs.python.org/2/library/errno.html
import errno

from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class platform_HWwatchdog(test.test):
    """Test to make sure that /dev/watchdog will reboot the system."""

    version = 1

    def _stop_watchdog(self, wd_dev):
        # HW watchdog is open and closed "properly".
        try:
            self.client.run('echo "V" > %s' % wd_dev);
        except error.AutoservRunError, e:
            raise error.TestError('write to %s failed (%s)' %
                                  (wd_dev, errno.errorcode[e.errno]))

    def _trig_watchdog(self, wd_dev):
        # Test the machine will reboot if HW watchdog is open but NOT pet.
        try:
            self.client.run('echo "z" > %s' % wd_dev);
        except error.AutoservRunError, e:
            raise error.TestError('write to %s failed (%s)' %
                                  (wd_dev, errno.errorcode[e.errno]))

        logging.info("KernelHWpath: tickled watchdog on %s (%ds to reboot)",
                     self.client.hostname, self._hw_interval)

        # machine should became unpingable after lockup
        # ...give 5 seconds slack...
        wait_down = self._hw_interval + 5
        if not self.client.wait_down(timeout=wait_down):
            raise error.TestError('machine should be unpingable '
                                  'within %d seconds' % wait_down)

        # make sure the machine comes back,
        # DHCP can take up to 45 seconds in odd cases.
        if not self.client.wait_up(timeout=60):
            raise error.TestError('machine did not reboot/ping within '
                                  '60 seconds of HW reset')

    def _exists_on_client(self, wd_dev):
        return self.client.run('test -c "%s"' % wd_dev,
                               ignore_status=True).exit_status == 0

    # If daisydog is running, stop it so we can use /dev/watchdog
    def _stop_daemon(self):
        """If running, stop daisydog so we can use /dev/watchdog."""
        self.client.run('stop daisydog', ignore_status=True)

    def _start_daemon(self):
        self.client.run('start daisydog', ignore_status=True)

    def _query_hw_interval(self):
        """Check how long the hardware interval is."""
        output = self.client.run('daisydog -c').stdout
        secs = re.findall(r'HW watchdog interval is (\d*) seconds', output)[0]
        return int(secs)

    def run_once(self, host=None):
        self.client = host
        wd_dev = '/dev/watchdog'

        # If watchdog not present, just skip this test
        if not self._exists_on_client(wd_dev):
            logging.info("INFO: %s not present. Skipping test.", wd_dev)
            return

        self._stop_daemon()
        self._hw_interval = self._query_hw_interval()
        self._stop_watchdog(wd_dev)
        self._trig_watchdog(wd_dev)
        self._start_daemon()

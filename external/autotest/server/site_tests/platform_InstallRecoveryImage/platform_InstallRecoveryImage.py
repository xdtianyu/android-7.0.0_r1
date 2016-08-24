# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.server import test
from autotest_lib.client.common_lib import error

class platform_InstallRecoveryImage(test.test):
    """Installs a specified recovery image onto a servo-connected DUT."""
    version = 1

    _RECOVERY_INSTALL_DELAY = 540

    def run_once(self, host, image):
        host.servo.install_recovery_image(image,
                                          make_image_noninteractive=True)
        logging.info('Running the recovery process on the DUT. '
                     'Will wait up to %d seconds for recovery to '
                     'complete.', self._RECOVERY_INSTALL_DELAY)
        start_time = time.time()
        # Wait for the host to come up.
        if host.ping_wait_up(timeout=self._RECOVERY_INSTALL_DELAY):
            logging.info('Recovery process completed successfully in '
                         '%d seconds.', time.time() - start_time)
        else:
            raise error.TestFail('Host failed to come back up after '
                                 '%d seconds.' % self._RECOVERY_INSTALL_DELAY)
        logging.info('Removing the usb key from the DUT.')
        host.servo.switch_usbkey('host')

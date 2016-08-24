# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server import test
from autotest_lib.server.cros import autoupdate_utils


class autoupdate_Rollback(test.test):
    """Test that updates the machine and performs rollback."""
    version = 1


    def run_once(self, host, job_repo_url=None):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param job_repo_url: URL to get the image.

        @raise error.TestError if anything went wrong with setting up the test;
               error.TestFail if any part of the test has failed.

        """
        updater = autoupdate_utils.get_updater_from_repo_url(host, job_repo_url)

        initial_kernel, updated_kernel = updater.get_kernel_state()
        logging.info('Initial device state: active kernel %s, '
                     'inactive kernel %s.', initial_kernel, updated_kernel)

        logging.info('Performing an update.')
        updater.update_rootfs()
        host.reboot()

        # We should be booting from the new partition.
        error_message = 'Failed to set up test by updating DUT.'
        updater.verify_boot_expectations(expected_kernel_state=updated_kernel,
                                         rollback_message=error_message)
        logging.info('Update verified, initiating rollback.')

        # Powerwash is tested separately from rollback.
        updater.rollback_rootfs(powerwash=False)
        host.reboot()

        # We should be back on our initial partition.
        error_message = ('Autoupdate reported that rollback succeeded but we '
                         'did not boot into the correct partition.')
        updater.verify_boot_expectations(expected_kernel_state=initial_kernel,
                                         rollback_message=error_message)
        logging.info('We successfully rolled back to initial kernel.')

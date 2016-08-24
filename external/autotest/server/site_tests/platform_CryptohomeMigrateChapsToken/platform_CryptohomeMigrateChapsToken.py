# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import afe_utils
from autotest_lib.server import autotest
from autotest_lib.server import test

class platform_CryptohomeMigrateChapsToken(test.test):
    """ This test checks to see if Chaps generated keys are
        available after a ChromeOS autoupdate.
    """
    version = 1

    CLIENT_TEST = 'platform_CryptohomeMigrateChapsTokenClient'


    def run_once(self, host, baseline_version=None):
        # Save the build on the DUT, because we want to provision it after
        # the test.
        final_version = afe_utils.get_build(host)
        if baseline_version:
            version = baseline_version
        else:
            board_name = host.get_board().split(':')[1]
            version = "%s-release/R37-3773.0.0" % board_name

        # Downgrade to baseline version and run client side test.
        self.job.run_test('provision_AutoUpdate', host=host,
                          value=version)
        client_at = autotest.Autotest(host)
        client_at.run_test(self.CLIENT_TEST, generate_key=True)
        # Upgrade back to latest version and see if the key migration
        # succeeded.
        self.job.run_test('provision_AutoUpdate', host=host,
                          value=final_version)
        client_at.run_test(self.CLIENT_TEST, generate_key=False)

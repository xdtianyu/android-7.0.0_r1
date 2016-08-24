# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test, autotest


class firmware_TouchMTBSetup(test.test):
    version = 1
    client_test_name = 'firmware_TouchMTB'

    def _get_client_test_path(self):
        test_rel_dir = os.path.join(os.path.dirname(__file__),
                                    '..', '..', '..', 'client', 'site_tests',
                                    self.client_test_name)
        return os.path.realpath(test_rel_dir)

    def run_once(self, host=None):
        """Run the test."""
        # Run the client test for installing the test.
        self.client_at = autotest.Autotest(host)
        self.client_at.run_test(self.client_test_name)

        # Copy the version info to the test machine.
        version_script = os.path.join(self._get_client_test_path(),
                                      'version.sh')
        cmd = '%s -r %s' % (version_script, host.ip)
        try:
            utils.system(cmd)
        except:
            raise error.TestError('executing "%s"' % cmd)

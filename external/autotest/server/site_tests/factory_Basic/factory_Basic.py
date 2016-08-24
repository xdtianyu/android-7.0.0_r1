# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.server import test
from autotest_lib.server.cros import goofy_client


class factory_Basic(test.test):
    """Basic factory wrapper."""
    version = 1
    REMOTE_TEST_LIST_DIR = '/usr/local/factory/test_lists'

    def initialize(self, host, test_list_path, test_list_name):
        """Initialize a goofy proxy and copy over the test lists.

        @param host: The host to run this test on.
        @param test_list_path: The local path of the test_list to copy
                               over to the DUT.
        @param test_list_name: The name of the test list.
        """
        self._goofy_client = goofy_client.GoofyProxy(host)
        if test_list_path:
            host.send_file(test_list_path,
                           os.path.join(self.REMOTE_TEST_LIST_DIR,
                                        'test_list.%s' % test_list_name))

            # For goofy to load any new tests lists we need a factory restart.
            host.run('factory_restart -a')


    def run_once(self, host, test_list_name):
        """Wait on all the tests in a test_list to finish.

        @param test_list_name: The name of the tests list to wait on.
        """
        self._goofy_client.monitor_tests(test_list_name)
        self._goofy_client.get_results(self.resultsdir)

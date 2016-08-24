# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server.cros import moblab_test
from autotest_lib.server.hosts import moblab_host


FAILURE_FOLDERS = ['/usr/local/autotest/results', '/usr/local/autotest/logs']


class moblab_RunSuite(moblab_test.MoblabTest):
    """
    Moblab run suite test. Ensures that a Moblab can run a suite from start
    to finish by kicking off a suite which will have the Moblab stage an
    image, provision its DUTs and run the tests.
    """
    version = 1


    def run_once(self, host, suite_name):
        """Runs a suite on a Moblab Host against its test DUTS.

        @param host: Moblab Host that will run the suite.
        @param suite_name: Name of the suite to run.

        @raises AutoservRunError if the suite does not complete successfully.
        """
        # Fetch the board of the DUT's assigned to this Moblab. There should
        # only be one type.
        board = host.afe.get_hosts()[0].platform
        # TODO (crbug.com/399132) sbasi - Replace repair version with actual
        # stable_version for the given board.
        stable_version = host.afe.run('get_stable_version', board=board)
        build_pattern = global_config.global_config.get_config_value(
                'CROS', 'stable_build_pattern')
        build = build_pattern % (board, stable_version)

        logging.debug('Running suite: %s.', suite_name)
        try:
            result = host.run_as_moblab(
                    "%s/site_utils/run_suite.py --pool='' "
                    "--board=%s --build=%s --suite_name=%s" %
                    (moblab_host.AUTOTEST_INSTALL_DIR, board, build,
                     suite_name), timeout=10800)
        except error.AutoservRunError as e:
            # Collect the results and logs from the moblab device.
            moblab_logs_dir = os.path.join(self.resultsdir, 'moblab_logs')
            for folder in FAILURE_FOLDERS:
                try:
                    host.get_file(folder, moblab_logs_dir)
                except error.AutoservRunError as e2:
                    logging.error(e2)
                    pass
            raise e
        logging.debug('Suite Run Output:\n%s', result.stdout)

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Mock out test results for puppylab.
"""


import logging
import os
import time

import common
from autotest_lib.client.common_lib import time_utils
from autotest_lib.puppylab import templates


class ResultsMocker(object):
    """Class to mock out the results of a test."""

    def _make_dirs(self):
        """Create essential directories needed for faking test results.

        @raises ValueError: If the directories crucial to reporting
            test status already exist.
        @raises OSError: If we cannot make one of the directories for
            an os related reason (eg: permissions).
        @raises AssertionError: If one of the directories silently failed
            creation.
        """
        logging.info("creating dir %s, %s, %s",
                self.results_dir, self.test_results, self.host_keyval_dir)
        if not os.path.exists(self.results_dir):
            os.makedirs(self.results_dir)
        if not os.path.exists(self.test_results):
            os.makedirs(self.test_results)
        if not os.path.exists(self.host_keyval_dir):
            os.makedirs(self.host_keyval_dir)
        assert(os.path.exists(self.test_results) and
               os.path.exists(self.results_dir) and
               os.path.exists(self.host_keyval_dir))


    def __init__(self, test_name, results_dir, machine_name):
        """Initialize a results mocker.

        @param test_name: The name of the test, eg: dummy_Pass.
        @param results_dir: The results directory this test will use.
        @param machine_name: A string representing the hostname the test will
            run on.
        """
        self.results_dir = results_dir
        self.test_results = os.path.join(results_dir, test_name)
        self.host_keyval_dir = os.path.join(self.results_dir, 'host_keyvals')
        self.machine_name = machine_name
        self.test_name = test_name

        self._make_dirs()

        # Status logs are used by the parser to declare a test as pass/fail.
        self.job_status = os.path.join(self.results_dir, 'status')
        self.job_status_log = os.path.join(self.results_dir, 'status.log')
        self.test_status = os.path.join(self.test_results, 'status')

        # keyvals are used by the parser to figure out fine grained information
        # about a test. Only job_keyvals are crucial to parsing.
        self.test_keyvals = os.path.join(self.test_results, 'keyval')
        self.job_keyvals = os.path.join(self.results_dir, 'keyval')
        self.host_keyvals = os.path.join(self.results_dir, machine_name)


    def _write(self, results_path, results):
        """Write the content in results to the file in results_path.

        @param results_path: The path to the results file.
        @param results: The content to write to the file.
        """
        logging.info('Writing results to %s', results_path)
        with open(results_path, 'w') as results_file:
            results_file.write(results)


    def generate_keyvals(self):
        """Apply templates to keyval files.

        There are 3 important keyvals files, only one of which is actually
        crucial to results parsing:
            host_keyvals - information about the DUT
            job_keyvals - information about the server_job
            test_keyvals - information about the test

        Parsing cannot complete without the job_keyvals. Everything else is
        optional. Keyvals are parsed into tko tables.
        """
        #TODO(beeps): Include other keyvals.
        self._write(
                self.job_keyvals,
                templates.job_keyvals_template %
                        {'hostname': self.machine_name})


    def generate_status(self):
        """Generate status logs.

        3 important status logs are required for successful parsing:
            test_name/status - core test status
            results_dir/status - server job status (has test status in it)
            status.log - compiled final status log
        """
        current_timestamp = int(time.time())
        test_info = {
            'test_name': self.test_name,
            'timestamp': current_timestamp,
            'date': time_utils.epoch_time_to_date_string(
                            current_timestamp, fmt_string='%b %d %H:%M:%S'),
        }
        self._write(
                self.job_status,
                templates.success_job_template % test_info)
        self._write(
                self.job_status_log,
                templates.success_job_template % test_info)
        self._write(
                self.test_status,
                templates.success_test_template % test_info)


    def mock_results(self):
        """Create mock results in the directories used to init the instance."""
        self.generate_status()
        self.generate_keyvals()



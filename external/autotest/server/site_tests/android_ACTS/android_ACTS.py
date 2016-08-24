# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import json
import os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server import test


CONFIG_FOLDER_LOCATION = global_config.global_config.get_config_value(
        'ACTS', 'acts_config_folder', default='')

TEST_CONFIG_FILE_FOLDER = 'autotest_config'
TEST_CAMPAIGN_FILE_FOLDER = 'autotest_campaign'

class android_ACTS(test.test):
    '''Run an Android CTS test case.'''
    version = 1
    acts_result_to_autotest = {
        'PASS': 'GOOD',
        'FAIL': 'FAIL',
        'UNKNOWN': 'WARN',
        'SKIP': 'ABORT'
    }

    def fetch_file(self, input_path, sub_dir_name):
        """Ensures the file specified by a path exists locally. If the file
        specified by input_path does not exist, attempt to locate it in ACTS
        dirctory.

        @param input_path: A string that's the path to a file.
        @param sub_dir_name: A string that's the subdirectory name of where the
                             file exists.
        """
        if os.path.exists(input_path):
            self.test_station.send_file(input_path, self.ts_tempfolder)
            return
        actual_path = os.path.join(CONFIG_FOLDER_LOCATION,
                                   sub_dir_name,
                                   input_path)
        actual_path = os.path.realpath(actual_path)
        if not os.path.exists(actual_path):
            raise error.TestFail('File: %s does not exist' % actual_path)
        self.test_station.send_file(actual_path, self.ts_tempfolder)


    def run_once(self, testbed=None, config_file=None, testbed_name=None,
                 test_case=None, test_file=None):
        """Run ACTS on the DUT.

        Exactly one of test_case and test_file should be provided.

        @param testbed: Testbed representing the testbed under test. Required.
        @param config_file: Path to config file locally. Required.
        @param testbed_name: A string that's passed to act.py's -tb option.
                             Required.
        @param test_case: A string that's passed to act.py's -tc option.
        @param test_file: A string that's passed to act.py's -tf option.
        """
        self.test_station = testbed.get_test_station()
        # Get a tempfolder on the device.
        self.ts_tempfolder = self.test_station.get_tmp_dir()
        if not config_file:
            raise error.TestFail('A config file must be specified.')
        self.fetch_file(config_file, TEST_CONFIG_FILE_FOLDER)

        if test_file:
            self.fetch_file(test_file, TEST_CAMPAIGN_FILE_FOLDER)
        act_base_cmd = 'act.py -c %s -tb %s ' % (
                    os.path.join(self.ts_tempfolder, os.path.basename(config_file)),
                    testbed_name)
        # Run the acts script.
        if test_case:
            act_cmd = '%s -tc %s' % (act_base_cmd, test_case)
        elif test_file:
            act_cmd = '%s -tf %s' % (act_base_cmd,
                    os.path.join(self.ts_tempfolder, os.path.basename(test_file)))
        else:
            raise error.TestFail('No test was specified,  abort!')
        logging.debug('Running: %s', act_cmd)
        # TODO: Change below to be test_bed.teststation_host.run
        act_result = self.test_station.run(act_cmd)
        logging.debug('ACTS Output:\n%s', act_result.stdout)

        # Transport all the logs to local.
        with open(config_file, 'r') as f:
            configs = json.load(f)
        log_path = os.path.join(configs['logpath'], testbed_name, 'latest')
        self.test_station.get_file(log_path, self.resultsdir)
        # Load summary json file.
        summary_path = os.path.join(self.resultsdir,
                                    'latest',
                                    'test_run_summary.json')
        with open(summary_path, 'r') as f:
            results = json.load(f)['Results']
        # Report results to Autotest.
        for result in results:
            verdict = self.acts_result_to_autotest[result['Result']]
            details = result['Details']
            self.job.record(verdict, None, test_case, status=(details or ''))

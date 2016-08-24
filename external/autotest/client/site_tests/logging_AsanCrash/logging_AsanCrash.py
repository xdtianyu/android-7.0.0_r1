# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cros_logging


class logging_AsanCrash(test.test):
    """Verify Address Sanitizer does catch memory errors."""

    version = 1


    def run_once(self):
        if not 'asan' in utils.read_file('/etc/ui_use_flags.txt'):
            raise error.TestFail('Current image not built with ASAN')

        extension_path = os.path.join(os.path.dirname(__file__),
                                      'asan_crash_ext')

        with chrome.Chrome(extension_paths=[extension_path]) as cr:
            pid = utils.get_oldest_pid_by_name('chrome')
            asan_log_name = '/var/log/chrome/asan_log.%d' % pid
            logging.info('Browser PID under telemetry control is %d. '
                         'So ASAN log is expected at %s.', pid, asan_log_name)

            logging.info('Initiate simulating memory bug to be caught by ASAN.')
            extension = cr.get_extension(extension_path)
            if not extension:
                raise error.TestFail('Failed to find extension %s'
                                     % extension_path)

            # Catch the exception raised when the browser crashes.
            cr.did_browser_crash(lambda: extension.ExecuteJavaScript(
                    'chrome.autotestPrivate.simulateAsanMemoryBug();'))

            utils.poll_for_condition(
                    lambda: os.path.isfile(asan_log_name),
                    timeout=10,
                    exception=error.TestFail(
                            'Found no asan log file %s during 10s'
                            % asan_log_name))
            ui_log = cros_logging.LogReader(asan_log_name)
            ui_log.read_all_logs()

            # We must wait some time until memory bug is simulated (happens
            # immediately after the return on the call) and caught by ASAN.
            try:
                utils.poll_for_condition(
                        lambda: ui_log.can_find('ERROR: AddressSanitizer'),
                        timeout=10,
                        exception=error.TestFail(
                                'Found no asan log message about '
                                'Address Sanitizer catch'))

                utils.poll_for_condition(
                        lambda: ui_log.can_find("'testarray'"),
                        timeout=10,
                        exception=error.TestFail(
                                'ASAN caught bug but did not mention '
                                'the cause in the log'))

            except:
                logging.debug('ASAN log content: ' + ui_log.get_logs())
                raise

            # The cbuildbot logic will look for asan logs and process them.
            # Remove the simulated log file to avoid that.
            os.remove(asan_log_name)

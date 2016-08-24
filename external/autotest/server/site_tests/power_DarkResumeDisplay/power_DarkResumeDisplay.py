# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re, time

from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros.dark_resume_utils import DarkResumeUtils
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig

SUSPEND_DURATION = 15
NUM_DARK_RESUMES = 10
ERROR_FILE = '/sys/kernel/debug/dri/0/i915_crtc_errors'


class power_DarkResumeDisplay(test.test):
    """ Ensure we don't have display errors after dark resume """
    version = 1
    dark_resume_utils = None


    def initialize(self, host):
        self.dark_resume_utils = DarkResumeUtils(host,
                                                 duration=SUSPEND_DURATION)


    def verify_host_supports_test(self, host):
        """Check if the test works on the given host

        @param host: reference to the host object
        """
        platform = host.run_output('mosys platform name')
        logging.info('Checking platform %s for compatibility with display test',
                     platform)

        client_attr = FAFTConfig(platform)
        if client_attr.dark_resume_capable == False:
            raise error.TestError('platform is not capable of dark resume')

        cmd = host.run('test -r %s' % ERROR_FILE, ignore_status=True)
        logging.info("node_exists=%s", str(cmd.exit_status))
        if cmd.exit_status != 0:
            raise error.TestError('%s file not found.' % ERROR_FILE)


    def get_crtc_error_count(self, host):
        """Get the current crtc error count for the dut

        @returns: A dict whose key is the crtc id, and whose value is a
                  dict of {pipe, errors}
        """
        output = host.run_output('cat %s' % ERROR_FILE)
        pattern = 'Crtc ([0-9]+) Pipe ([A-Za-z]+) errors:\t\t([0-9a-fA-F]{8})'
        regex = re.compile(pattern)
        counts = {}
        for line in output.splitlines():
            match = regex.match(line)
            if match == None:
                raise error.TestError('Unexpected error file string: %s' % line)

            counts[int(match.group(1))] = {
                'pipe': match.group(2),
                'errors': int(match.group(3), 16),
            }
        return counts


    def run_once(self, host=None):
        """Run the test.

           Setup preferences so that a dark resume will happen shortly after
           suspending the machine.

           store the current crtc error count
           suspend the machine
           wait for dark resume
           wake the machine
           retrieve the current crtc error count after suspend
           ensure the error counts did not increase while suspended

        @param host: The machine to run the tests on
        """
        self.verify_host_supports_test(host)

        pre_err_count = self.get_crtc_error_count(host)

        """The DUT will perform a dark resume every SUSPEND_DURATION seconds
           while it is suspended. Suspend the device and wait for the amount
           of time to have performed NUM_DARK_RESUMES, plus half the
           SUSPEND_DURATION to ensure the last dark resume has a chance to
           complete.
        """
        wait_time = SUSPEND_DURATION * NUM_DARK_RESUMES + SUSPEND_DURATION / 2
        logging.info('suspending host, and waiting %ds', wait_time)
        with self.dark_resume_utils.suspend() as suspend_ctx:
            time.sleep(wait_time)

        dark_resume_count = self.dark_resume_utils.count_dark_resumes()
        logging.info('dark resume count = %d', dark_resume_count)
        if dark_resume_count == 0:
            raise error.TestError('Device did not enter dark resume!')

        logging.info('retrieving post-suspend error counts')
        post_err_count = self.get_crtc_error_count(host)
        for k in pre_err_count:
            pre = pre_err_count[k]['errors']
            post = post_err_count[k]['errors']
            if pre != post:
                raise error.TestError('Crtc %d Pipe %s err count changed %d/%d'
                                      % (k, pre_err_count[k]['pipe'], pre,
                                         post))
            logging.info('error counts for Crtc %d Pipe %s constant at %d', k,
                         pre_err_count[k]['pipe'], pre)


    def cleanup(self, host):
        self.dark_resume_utils.teardown()


# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Sequence extensions to server_job.
Adds ability to schedule jobs on given machines.
"""

import logging
import os

import common
from autotest_lib.client.common_lib import control_data
from autotest_lib.server import utils
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.site_utils import job_directories

MINUTE_IN_SECS = 60
HOUR_IN_MINUTES = 60
HOUR_IN_SECS = HOUR_IN_MINUTES * MINUTE_IN_SECS
DAY_IN_HOURS = 24
DAY_IN_SECS = DAY_IN_HOURS*HOUR_IN_SECS

DEFAULT_JOB_TIMEOUT_IN_MINS = 4 * HOUR_IN_MINUTES

class SequenceJob(object):
    """Define part of a sequence that will be scheduled by the sequence test."""

    CONTROL_FILE = """
def run(machine):
    job.run_test('%s', host=hosts.create_host(machine), client_ip=machine%s)

parallel_simple(run, machines)
"""

    def __init__(self, name, args=None, iteration=1, duration=None,
                 fetch_control_file=False):
        """
        Constructor

        @param name: name of the server test to run.
        @param args: arguments needed by the server test.
        @param iteration: number of copy of this test to sechudle
        @param duration: expected duration of the test (in seconds).
        @param fetch_control_file: If True, fetch the control file contents
                                   from disk. Otherwise uses the template
                                   control file.
        """
        self._name = name
        self._args = args or {}
        self._iteration = iteration
        self._duration = duration
        self._fetch_control_file = fetch_control_file


    def child_job_name(self, machine, iteration_number):
        """
        Return a name for a child job.

        @param machine: machine name on which the test will run.
        @param iteration_number: number with 0 and self._iteration - 1.

        @returns a unique name based on the machine, the name and the iteration.
        """
        name_parts = [machine, self._name]
        tag = self._args.get('tag')
        if tag:
            name_parts.append(tag)
        if self._iteration > 1:
            name_parts.append(str(iteration_number))
        return '_'.join(name_parts)


    def child_job_timeout(self):
        """
        Get the child job timeout in minutes.

        @param args: arguments sent to the test.

        @returns a timeout value for the test, 4h by default.
        """
        if self._duration:
            return 2 * int(self._duration) / MINUTE_IN_SECS
        # default value:
        return DEFAULT_JOB_TIMEOUT_IN_MINS


    def child_control_file(self):
        """
        Generate the child job's control file.

        If not fetching the contents, use the template control file and
        populate the template control file with the test name and expand the
        arguments list.

        @param test: name of the test to run
        @param args: dictionary of argument for this test.

        @returns a fully built control file to be use for the child job.
        """
        if self._fetch_control_file:
            # TODO (sbasi): Add arg support.
            cntl_file_getter = control_file_getter.FileSystemGetter(
                    [os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                  '..')])
            return cntl_file_getter.get_control_file_contents_by_name(
                    self._name)
        child_args = ['',]
        for arg, value in self._args.iteritems():
            child_args.append('%s=%s' % (arg, repr(value)))
        if self._duration:
            child_args.append('duration=%d' % self._duration)
        return self.CONTROL_FILE % (self._name, ', '.join(child_args))


    def schedule(self, job, timeout_mins, machine):
        """
        Sequence a job on the running AFE.

        Will schedule a given test on the job machine(s).
        Support a subset of tests:
        - server job
        - no hostless.
        - no cleanup around tests.

        @param job: server_job object that will server as parent.
        @param timeout_mins: timeout to set up: if the test last more than
           timeout_mins, the test will fail.
        @param machine: machine to run the test on.

        @returns a maximal time in minutes that the sequence can take.
        """
        afe = frontend_wrappers.RetryingAFE(timeout_min=30, delay_sec=10,
                                            user=job.user, debug=False)
        current_job_id = job_directories.get_job_id_or_task_id(job.resultdir)
        logging.debug('Current job id: %s', current_job_id)
        runtime_mins = self.child_job_timeout()
        hostname = utils.get_hostname_from_machine(machine)

        for i in xrange(0, self._iteration):
            child_job_name = self.child_job_name(hostname, i)
            logging.debug('Creating job: %s', child_job_name)
            afe.create_job(
                    self.child_control_file(),
                    name=child_job_name,
                    priority='Medium',
                    control_type=control_data.CONTROL_TYPE.SERVER,
                    hosts=[hostname], meta_hosts=(), one_time_hosts=(),
                    atomic_group_name=None, synch_count=None, is_template=False,
                    timeout_mins=timeout_mins + (i + 1) * runtime_mins,
                    max_runtime_mins=runtime_mins,
                    run_verify=False, email_list='', dependencies=(),
                    reboot_before=None, reboot_after=None,
                    parse_failed_repair=None,
                    hostless=False, keyvals=None,
                    drone_set=None, image=None,
                    parent_job_id=current_job_id, test_retry=0, run_reset=False,
                    require_ssp=utils.is_in_container())
        return runtime_mins * self._iteration


def sequence_schedule(job, machines, server_tests):
    """
    Schedule the tests to run

    Launch all the tests in the sequence on all machines.
    Returns as soon as the jobs are launched.

    @param job: Job running.
    @param machines: machine to run on.
    @param server_tests: Array of sequence_test objects.
    """
    for machine in machines:
        timeout_mins = 0
        for test in server_tests:
            timeout_mins += test.schedule(job, timeout_mins, machine)

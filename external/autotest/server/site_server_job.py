# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Site extensions to server_job.  Adds distribute_across_machines()."""

import os, logging, multiprocessing
from autotest_lib.server import site_gtest_runner, site_server_job_utils
from autotest_lib.server import subcommand
from autotest_lib.server.server_job import base_server_job
import utils


def get_site_job_data(job):
    """Add custom data to the job keyval info.

    When multiple machines are used in a job, change the hostname to
    the platform of the first machine instead of machine1,machine2,...  This
    makes the job reports easier to read and keeps the tko_machines table from
    growing too large.

    Args:
        job: instance of server_job.

    Returns:
        keyval dictionary with new hostname value, or empty dictionary.
    """
    site_job_data = {}
    # Only modify hostname on multimachine jobs. Assume all host have the same
    # platform.
    if len(job.machines) > 1:
        # Search through machines for first machine with a platform.
        for host in job.machines:
            keyval_path = os.path.join(job.resultdir, 'host_keyvals', host)
            keyvals = utils.read_keyval(keyval_path)
            host_plat = keyvals.get('platform', None)
            if not host_plat:
                continue
            site_job_data['hostname'] = host_plat
            break
    return site_job_data


class site_server_job(base_server_job):
    """Extend server_job adding distribute_across_machines."""

    def __init__(self, *args, **dargs):
        super(site_server_job, self).__init__(*args, **dargs)


    def run(self, *args, **dargs):
        """Extend server_job.run adding gtest_runner to the namespace."""

        gtest_run = {'gtest_runner': site_gtest_runner.gtest_runner()}

        # Namespace is the 5th parameter to run().  If args has 5 or more
        # entries in it then we need to fix-up this namespace entry.
        if len(args) >= 5:
            args[4].update(gtest_run)
        # Else, if present, namespace must be in dargs.
        else:
            dargs.setdefault('namespace', gtest_run).update(gtest_run)
        # Now call the original run() with the modified namespace containing a
        # gtest_runner
        super(site_server_job, self).run(*args, **dargs)


    def distribute_across_machines(self, tests, machines,
                                   continuous_parsing=False):
        """Run each test in tests once using machines.

        Instead of running each test on each machine like parallel_on_machines,
        run each test once across all machines. Put another way, the total
        number of tests run by parallel_on_machines is len(tests) *
        len(machines). The number of tests run by distribute_across_machines is
        len(tests).

        Args:
            tests: List of tests to run.
            machines: List of machines to use.
            continuous_parsing: Bool, if true parse job while running.
        """
        # The Queue is thread safe, but since a machine may have to search
        # through the queue to find a valid test the lock provides exclusive
        # queue access for more than just the get call.
        test_queue = multiprocessing.JoinableQueue()
        test_queue_lock = multiprocessing.Lock()

        unique_machine_attributes = []
        sub_commands = []
        work_dir = self.resultdir

        for machine in machines:
            if 'group' in self.resultdir:
                work_dir = os.path.join(self.resultdir, machine)

            mw = site_server_job_utils.machine_worker(self,
                                                      machine,
                                                      work_dir,
                                                      test_queue,
                                                      test_queue_lock,
                                                      continuous_parsing)

            # Create the subcommand instance to run this machine worker.
            sub_commands.append(subcommand.subcommand(mw.run,
                                                      [],
                                                      work_dir))

            # To (potentially) speed up searching for valid tests create a list
            # of unique attribute sets present in the machines for this job. If
            # sets were hashable we could just use a dictionary for fast
            # verification. This at least reduces the search space from the
            # number of machines to the number of unique machines.
            if not mw.attribute_set in unique_machine_attributes:
                unique_machine_attributes.append(mw.attribute_set)

        # Only queue tests which are valid on at least one machine.  Record
        # skipped tests in the status.log file using record_skipped_test().
        for test_entry in tests:
            # Check if it's an old style test entry.
            if len(test_entry) > 2 and not isinstance(test_entry[2], dict):
                test_attribs = {'include': test_entry[2]}
                if len(test_entry) > 3:
                    test_attribs['exclude'] = test_entry[3]
                if len(test_entry) > 4:
                    test_attribs['attributes'] = test_entry[4]

                test_entry = list(test_entry[:2])
                test_entry.append(test_attribs)

            ti = site_server_job_utils.test_item(*test_entry)
            machine_found = False
            for ma in unique_machine_attributes:
                if ti.validate(ma):
                    test_queue.put(ti)
                    machine_found = True
                    break
            if not machine_found:
                self.record_skipped_test(ti)

        # Run valid tests and wait for completion.
        subcommand.parallel(sub_commands)


    def record_skipped_test(self, skipped_test, message=None):
        """Insert a failure record into status.log for this test."""
        msg = message
        if msg is None:
            msg = 'No valid machines found for test %s.' % skipped_test
        logging.info(msg)
        self.record('START', None, skipped_test.test_name)
        self.record('INFO', None, skipped_test.test_name, msg)
        self.record('END TEST_NA', None, skipped_test.test_name, msg)

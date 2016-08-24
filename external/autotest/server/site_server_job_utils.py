# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility classes used by site_server_job.distribute_across_machines().

test_item: extends the basic test tuple to add include/exclude attributes and
    pre/post actions.

machine_worker: is a thread that manages running tests on a host.  It
    verifies test are valid for a host using the test attributes from test_item
    and the host attributes from host_attributes.
"""


import logging, os, Queue
from autotest_lib.client.common_lib import error, utils
from autotest_lib.server import autotest, hosts, host_attributes


class test_item(object):
    """Adds machine verification logic to the basic test tuple.

    Tests can either be tuples of the existing form ('testName', {args}) or the
    extended form ('testname', {args}, {'include': [], 'exclude': [],
    'attributes': []}) where include and exclude are lists of host attribute
    labels and attributes is a list of strings. A machine must have all the
    labels in include and must not have any of the labels in exclude to be valid
    for the test. Attributes strings can include reboot_before, reboot_after,
    and server_job.
    """

    def __init__(self, test_name, test_args, test_attribs=None):
        """Creates an instance of test_item.

        Args:
            test_name: string, name of test to execute.
            test_args: dictionary, arguments to pass into test.
            test_attribs: Dictionary of test attributes. Valid keys are:
              include - labels a machine must have to run a test.
              exclude - labels preventing a machine from running a test.
              attributes - reboot before/after test, run test as server job.
        """
        self.test_name = test_name
        self.test_args = test_args
        self.tagged_test_name = test_name
        if test_args.get('tag'):
            self.tagged_test_name = test_name + '.' + test_args.get('tag')

        if test_attribs is None:
            test_attribs = {}
        self.inc_set = set(test_attribs.get('include', []))
        self.exc_set = set(test_attribs.get('exclude', []))
        self.attributes = test_attribs.get('attributes', [])

    def __str__(self):
        """Return an info string of this test."""
        params = ['%s=%s' % (k, v) for k, v in self.test_args.items()]
        msg = '%s(%s)' % (self.test_name, params)
        if self.inc_set:
            msg += ' include=%s' % [s for s in self.inc_set]
        if self.exc_set:
            msg += ' exclude=%s' % [s for s in self.exc_set]
        if self.attributes:
            msg += ' attributes=%s' % self.attributes
        return msg

    def validate(self, machine_attributes):
        """Check if this test can run on machine with machine_attributes.

        If the test has include attributes, a candidate machine must have all
        the attributes to be valid.

        If the test has exclude attributes, a candidate machine cannot have any
        of the attributes to be valid.

        Args:
            machine_attributes: set, True attributes of candidate machine.

        Returns:
            True/False if the machine is valid for this test.
        """
        if self.inc_set is not None:
            if not self.inc_set <= machine_attributes:
                return False
        if self.exc_set is not None:
            if self.exc_set & machine_attributes:
                return False
        return True

    def run_test(self, client_at, work_dir='.', server_job=None):
        """Runs the test on the client using autotest.

        Args:
            client_at: Autotest instance for this host.
            work_dir: Directory to use for results and log files.
            server_job: Server_Job instance to use to runs server tests.
        """
        if 'reboot_before' in self.attributes:
            client_at.host.reboot()

        try:
            if 'server_job' in self.attributes:
                if 'host' in self.test_args:
                    self.test_args['host'] = client_at.host
                if server_job is not None:
                    logging.info('Running Server_Job=%s', self.test_name)
                    server_job.run_test(self.test_name, **self.test_args)
                else:
                    logging.error('No Server_Job instance provided for test '
                                  '%s.', self.test_name)
            else:
                client_at.run_test(self.test_name, results_dir=work_dir,
                                   **self.test_args)
        finally:
            if 'reboot_after' in self.attributes:
                client_at.host.reboot()


class machine_worker(object):
    """Worker that runs tests on a remote host machine."""

    def __init__(self, server_job, machine, work_dir, test_queue, queue_lock,
                 continuous_parsing=False):
        """Creates an instance of machine_worker to run tests on a remote host.

        Retrieves that host attributes for this machine and creates the set of
        True attributes to validate against test include/exclude attributes.

        Creates a directory to hold the log files for tests run and writes the
        hostname and tko parser version into keyvals file.

        Args:
            server_job: run tests for this server_job.
            machine: name of remote host.
            work_dir: directory server job is using.
            test_queue: queue of tests.
            queue_lock: lock protecting test_queue.
            continuous_parsing: bool, enable continuous parsing.
        """
        self._server_job = server_job
        self._test_queue = test_queue
        self._test_queue_lock = queue_lock
        self._continuous_parsing = continuous_parsing
        self._tests_run = 0
        self._machine = machine
        self._host = hosts.create_host(self._machine)
        self._client_at = autotest.Autotest(self._host)
        client_attributes = host_attributes.host_attributes(machine)
        self.attribute_set = set(client_attributes.get_attributes())
        self._results_dir = work_dir
        if not os.path.exists(self._results_dir):
            os.makedirs(self._results_dir)
        machine_data = {'hostname': self._machine,
                        'status_version': str(1)}
        utils.write_keyval(self._results_dir, machine_data)

    def __str__(self):
        attributes = [a for a in self.attribute_set]
        return '%s attributes=%s' % (self._machine, attributes)

    def get_test(self):
        """Return a test from the queue to run on this host.

        The test queue can be non-empty, but still not contain a test that is
        valid for this machine. This function will take exclusive access to
        the queue via _test_queue_lock and repeatedly pop tests off the queue
        until finding a valid test or depleting the queue.  In either case if
        invalid tests have been popped from the queue, they are pushed back
        onto the queue before returning.

        Returns:
            test_item, or None if no more tests exist for this machine.
        """
        good_test = None
        skipped_tests = []

        with self._test_queue_lock:
            while True:
                try:
                    canidate_test = self._test_queue.get_nowait()
                    # Check if test is valid for this machine.
                    if canidate_test.validate(self.attribute_set):
                        good_test = canidate_test
                        break
                    skipped_tests.append(canidate_test)

                except Queue.Empty:
                    break

            # Return any skipped tests to the queue.
            for st in skipped_tests:
                self._test_queue.put(st)

        return good_test

    def run(self):
        """Executes tests on the host machine.

        If continuous parsing was requested, start the parser before running
        tests.
        """
        # Modify job.resultdir so that it points to the results directory for
        # the machine we're working on. Required so that server jobs will write
        # to the proper location.
        self._server_job.machines = [self._machine]
        self._server_job.push_execution_context(self._machine['hostname'])
        os.chdir(self._server_job.resultdir)
        if self._continuous_parsing:
            self._server_job._parse_job += "/" + self._machine['hostname']
            self._server_job._using_parser = True
            self._server_job.init_parser()

        while True:
            active_test = self.get_test()
            if active_test is None:
                break

            logging.info('%s running %s', self._machine, active_test)
            try:
                active_test.run_test(self._client_at, self._results_dir,
                                     self._server_job)
            except error.AutoservError:
                logging.exception('Autoserv error running "%s".', active_test)
            except error.AutotestError:
                logging.exception('Autotest error running  "%s".', active_test)
            except Exception:
                logging.exception('Exception running test "%s".', active_test)
                raise
            finally:
                self._test_queue.task_done()
                self._tests_run += 1

        if self._continuous_parsing:
            self._server_job.cleanup_parser()
        logging.info('%s completed %d tests.', self._machine, self._tests_run)

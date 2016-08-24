# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import httplib
import logging
import os
import re
import socket
import time

import common

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.server import utils


GOOFY_JSONRPC_SERVER_PORT = 0x0FAC
GOOFY_RUNNING = 'RUNNING'


class GoofyProxyException(Exception):
    """Exception raised when a goofy rpc fails."""
    pass


class GoofyRuntimeException(Exception):
    """Exception raised when something goes wrong while a test is running."""
    pass


def retry_goofy_rpc(exception_tuple, timeout_min=30):
    """A decorator to use with goofy rpcs.

    This decorator tries to recreate the goofy client proxy on
    socket error. It will continue trying to do so until it
    executes the method without any socket errors or till the
    retry.retry decorator hits it's timeout.

    Usage:
        If you just want to recreate the proxy:
        1. @retry_goofy_rpc(exception_tuple=(<exception>, socket.error),
                            timeout_min=<timeout>)
        2. @retry_goofy_rpc(socket.error, timeout_min=<timeout>)
            Note: you need to specify the socket.error exception because we
            want to retry the call after recreating the proxy.

    @param exception_tuple: A tuple of exceptions to pass to
        the retry decorator. Any of these exceptions will result
        in retries for the duration of timeout_min.
    @param timeout_min: The timeout, in minutes, for which we should
        retry the method ignoring any exception in exception_tuple.
    """
    def inner_decorator(method):
        """Inner retry decorator applied to the method.

        @param method: The method that needs to be wrapped in the decorator.

        @return A wrapper function that implements the retry.
        """

        @retry.retry(exception_tuple, timeout_min=timeout_min)
        def wrapper(*args, **kwargs):
            """This wrapper handles socket errors.

            If the method in question:
            1. Throws an exception in exception_tuple and it is not a
               socket.error, retry for timeout_min through retry.retry.
            2. Throws a socket.error, recreate the client proxy, and
               retry for timeout_min through retry.retry.
            3. Throws an exception not in exception_tuple, fail.
            """
            try:
                return method(*args, **kwargs)
            except socket.error as e:
                goofy_proxy = args[0]
                if type(goofy_proxy) is GoofyProxy:
                    logging.warning('Socket error while running factory tests '
                                    '%s, recreating goofy proxy.', e)
                    goofy_proxy._create_client_proxy(timeout_min=timeout_min)
                else:
                    logging.warning('Connectivity was lost and the retry '
                                    'decorator was unable to recreate a goofy '
                                    'client proxy, args: %s.', args)
                raise

        return wrapper

    return inner_decorator


class GoofyProxy(object):
    """Client capable of making rpc calls to goofy.

    Methods of this class that can cause goofy to change state
    usually need a retry decorator. Methods that have a retry decorator
    need to be 'pure', i.e return the same results when called multiple
    times with the same argument.

    There are 2 known exceptions this class can deal with, a socket.error
    which happens when we try to execute an rpc when the DUT is, say, suspended
    and a BadStatusLine, which we get when we try to execute an rpc while the
    DUT is going through a factory_restart. Ideally we would like to handle
    socket timeouts different from BadStatusLines as we can get connection
    errors even when a device reboots and BadStatusLines ususally only when
    factory restarts. crbug.com/281714.
    """

    # This timeout was arbitrarily chosen as many tests in the factory test
    # suite run for days. Ideally we would like to split this into at least 2
    # timeouts, one which we use for rpcs that run while no other test is,
    # running and is smaller than the second that is designed for use with rpcs
    # that might execute simultaneously with a test. The latter needs a longer
    # timeout since tests could suspend,resume for a long time, and a call like
    # GetGoofyStatus should be tolerant to these suspend/resumes. In designing
    # the base timeout one needs to allocate time to component methods of this
    # class (such as _set_test_list) as a multiple of the number of rpcs it
    # executes.
    BASE_RPC_TIMEOUT = 1440
    POLLING_INTERVAL = 5
    FACTORY_BUG_RE = r'.*(/tmp/factory_bug.*tar.bz2).*'
    UNTAR_COMMAND = 'tar jxf %s -C %s'


    def __init__(self, host):
        """
        @param host: The host object representing the DUT running goofy.
        """
        self._host = host
        self._raw_stop_running_tests()
        self._create_client_proxy(timeout_min=self.BASE_RPC_TIMEOUT)


    def _create_client_proxy(self, timeout_min=30):
        """Create a goofy client proxy.

        Ping the host till it's up, then proceed to create a goofy proxy. We
        don't wrap this method with a retry because it's used in the retry
        decorator itself.
        """

        # We don't ssh ping here as there is a potential dealy in O(minutes)
        # with our ssh command against a sleeping DUT, once it wakes up, and
        # that will lead to significant overhead incurred over many reboots.
        self._host.ping_wait_up(timeout_min)
        logging.info('Host is pingable, creating goofy client proxy')
        self._client = self._host.rpc_server_tracker.jsonrpc_connect(
                GOOFY_JSONRPC_SERVER_PORT)


    @retry.retry((httplib.BadStatusLine, socket.error),
                 timeout_min=BASE_RPC_TIMEOUT)
    def _raw_stop_running_tests(self):
        """Stop running tests by shelling out to the DUT.

        Use this method only before we have actually created the client
        proxy, as shelling out has several pitfalls. We need to stop all
        tests in a retry loop because tests will start executing as soon
        as we have reimaged a DUT and trying to create the proxy while
        the DUT is rebooting will lead to a spurious failure.

        Note that we use the plain retry decorator for this method since
        we don't need to recreate the client proxy on failure.
        """
        logging.info('Stopping all tests and clearing factory state')
        self._host.run('factory clear')


    @retry_goofy_rpc((httplib.BadStatusLine, socket.error),
                     timeout_min=BASE_RPC_TIMEOUT)
    def _get_goofy_status(self):
        """Return status of goofy, ignoring socket timeouts and http exceptions.
        """
        status = self._client.GetGoofyStatus().get('status')
        return status


    def _wait_for_goofy(self, timeout_min=BASE_RPC_TIMEOUT*2):
        """Wait till goofy is running or a timeout occurs.

        @param timeout_min: Minutes to wait before timing this call out.
        """
        current_time = time.time()
        timeout_secs = timeout_min * 60
        logging.info('Waiting on goofy')
        while self._get_goofy_status() != GOOFY_RUNNING:
            if time.time() - current_time > timeout_secs:
                break
        return


    @retry_goofy_rpc(socket.error, timeout_min=BASE_RPC_TIMEOUT*2)
    def _set_test_list(self, next_list):
        """Set the given test list for execution and turn on test automation.

        Confirm that the given test list is a test that has been baked into
        the image, then run it. Some test lists are configured to start
        execution automatically when we call SetTestList, while others wait
        for a corresponding RunTest.

        @param next_list: The name of the test list.

        @raise jsonrpclib.ProtocolError: If the test list we're trying to switch
                                         to isn't on the DUT.
        """

        # We can get a BadStatus line on 2 occassions:
        # 1. As part of SwitchTestList goofy performs a factory restart, which
        # will throw a BadStatusLine because the rpc can't exit cleanly. We
        # don't want to retry on this exception, since we've already set the
        # right test list.
        # 2. If we try to set a test list while goofy is already down
        # (from a previous factory restart). In this case we wouldn't have
        # set the new test list, because we coulnd't connect to goofy.
        # To properly set a new test list it's important to wait till goofy is
        # up before attempting to set the test list, while being aware that the
        # preceding httplib error is from the rpc we just executed leading to
        # a factory restart. Also note that if the test list is not already on
        # the DUT this method will fail, emitting the possible test lists one
        # can switch to.
        self._wait_for_goofy()
        logging.info('Switching to test list %s', next_list)
        try:
            # Enable full factory test automation. Full test automation mode
            # skips all manual tests and test barriers, which is what we want in
            # the test lab. There are other automation modes: partial and none.
            # In partial automation mode manual tests and barrier are enabled
            # and user intervention is required; none disables automation.
            self._client.SwitchTestList(next_list, 'full')
        except httplib.BadStatusLine:
            logging.info('Switched to list %s, goofy restarting', next_list)


    @retry_goofy_rpc((httplib.BadStatusLine, socket.error),
                     timeout_min=BASE_RPC_TIMEOUT*2)
    def _stop_running_tests(self):
        """Stop all running tests.

        Wrap the StopTest rpc so we can attempt to stop tests even while a DUT
        is suspended or rebooting.
        """
        logging.info('Stopping tests.')
        self._client.StopTest()


    def _get_test_map(self):
        """Get a mapping of test suites -> tests.

        Ignore entries for tests that don't have a path.

        @return: A dictionary of the form
                 {'suite_name': ['suite_name.path_to_test', ...]}.
        """
        test_all = set([test['path'] for test in self._client.GetTests()
                        if test.get('path')])

        test_map = collections.defaultdict(list)
        for names in test_all:
            test_map[names.split('.')[0]].append(names)
        return test_map


    def _log_test_results(self, test_status, current_suite):
        """Format test status results and write them to status.log.

        @param test_status: The status dictionary of a single test.
        @param current_suite: The current suite name.
        """
        try:
            self._host.job.record('INFO', None, None,
                                  'suite %s, test %s, status: %s' %
                                  (current_suite, test_status.get('path'),
                                   test_status.get('status')))
        except AttributeError as e:
            logging.error('Could not gather results for current test: %s', e)


    @retry_goofy_rpc((httplib.BadStatusLine, socket.error),
                     timeout_min=BASE_RPC_TIMEOUT*2)
    def _get_test_info(self, test_name):
        """Get the status of one test.

        @param test_name: The name of the test we need the status of.

        @return: The entry for the test in the status dictionary.
        """
        for test in self._client.GetTests():
            if test['path'] == test_name:
                return test
        raise ValueError('Could not find test_name %s in _get_test_info.' %
                          test_name)


    @retry_goofy_rpc((httplib.BadStatusLine, socket.error),
                     timeout_min=BASE_RPC_TIMEOUT*2)
    def _get_test_run_info(self, run_id):
        """Get the information about the given test run.

        @param run_id: The ID of the test run.

        @return: A dict of test run status.
        """
        return self._client.GetTestRunStatus(run_id)


    def _wait_on_run(self, run_id):
        """Wait until the given test run to end.

        @param run_id: The ID of the test run.

        @raises GoofyRuntimeException: If the test run does not finish
            gracefully.
        """
        finished_tests = set()
        run_info = self._get_test_run_info(run_id)
        while run_info['status'] == 'RUNNING':
            finished = [(t['path'], t['status']) for t in
                        run_info['scheduled_tests']
                        if t['status'] in ('PASSED', 'FAILED')]
            for t in finished:
                if t not in finished_tests:
                    logging.info('[%s] %s', t[1], t[0])
                    finished_tests.add(t)
            time.sleep(self.POLLING_INTERVAL)
            run_info = self._get_test_run_info(run_id)
        if run_info['status'] != 'FINISHED':
            raise GoofyRuntimeException(
                    'The requested test run was interrupted.')


    def _synchronous_run_suite(self, suite_name):
        """Run one suite and wait for it to finish.

        Will start a test run for the specified suite_name and wait until it
        ends.

        @param suite_name: The name of the suite to wait for.

        @raises GoofyProxyException: If the status of the suite
            doesn't switch to active after we call RunTest.

        @return: The result of the suite.
        """
        logging.info('Starting suite: %s', suite_name)
        run_id = self._client.RunTest(suite_name)
        self._wait_on_run(run_id)
        return self._get_test_run_info(run_id)


    def monitor_tests(self, test_list):
        """Run a test list.

        Will run each suite in the given list in sequence, starting each one
        by name and waiting on its results. This method makes the following
        assumptions:
            - A test list is made up of self contained suites.
            - These suites trigger several things in parallel.
            - After a suite finishes it leaves goofy in an idle state.

        It is not safe to pull results for individual tests during the suite
        as the device could be rebooting, or goofy could be under stress.
        Instead, this method synchronously waits on an entire suite, then
        asks goofy for the status of each test in the suite. Since certain
        test lists automatically start and others don't, this method stops
        test list execution regardless, and sequentially triggers each suite.

        @param test_list: The test list to run.
        """
        self._set_test_list(test_list)
        self._wait_for_goofy()
        self._stop_running_tests()

        test_map = self._get_test_map()
        if test_map:
            logging.info('About to execute tests: %s', test_map)
        else:
            raise GoofyRuntimeException('Test map is empty, you might have an '
                                        'error in your test_list.')


        for current_suite in test_map.keys():
            logging.info('Processing suite %s', current_suite)

            result = self._synchronous_run_suite(current_suite)
            logging.info(result)

            for test_names in test_map.get(current_suite):
                self._log_test_results(self._get_test_info(test_names),
                                       current_suite)


    @retry_goofy_rpc((httplib.BadStatusLine, socket.error),
                     timeout_min=BASE_RPC_TIMEOUT*2)
    def get_results(self, resultsdir):
        """Copies results from the DUT to a local results directory.

        Copy the tarball over to the results folder, untar, and delete the
        tarball if everything was successful. This will effectively place
        all the logs relevant to factory testing in the job's results folder.

        @param resultsdir: The directory in which to untar the contents of the
                           tarball factory_bug generates.
        """
        logging.info('Getting results logs for test_list.')

        try:
            factory_bug_log = self._host.run('factory_bug').stderr
        except error.CmdError as e:
            logging.error('Could not execute factory_bug: %s', e)
            return

        try:
            factory_bug_tar = re.match(self.FACTORY_BUG_RE,
                                       factory_bug_log).groups(1)[0]
        except (IndexError, AttributeError):
            logging.error('could not collect logs for factory results, '
                          'factory bug returned %s', factory_bug_log)
            return

        factory_bug_tar_file = os.path.basename(factory_bug_tar)
        local_factory_bug_tar = os.path.join(resultsdir, factory_bug_tar_file)

        try:
            self._host.get_file(factory_bug_tar, local_factory_bug_tar)
        except error.AutoservRunError as e:
            logging.error('Failed to pull back the results tarball: %s', e)
            return

        try:
            utils.run(self.UNTAR_COMMAND % (local_factory_bug_tar, resultsdir))
        except error.CmdError as e:
            logging.error('Failed to untar the results tarball: %s', e)
            return
        finally:
            if os.path.exists(local_factory_bug_tar):
                os.remove(local_factory_bug_tar)

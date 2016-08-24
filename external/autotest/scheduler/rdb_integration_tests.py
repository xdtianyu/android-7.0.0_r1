#!/usr/bin/python
#pylint: disable-msg=C0111

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import collections

import common

from autotest_lib.client.common_lib import host_queue_entry_states
from autotest_lib.client.common_lib.test_utils import unittest
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.afe import rdb_model_extensions
from autotest_lib.scheduler import rdb
from autotest_lib.scheduler import rdb_hosts
from autotest_lib.scheduler import rdb_lib
from autotest_lib.scheduler import rdb_requests
from autotest_lib.scheduler import rdb_testing_utils
from autotest_lib.server.cros import provision


class AssignmentValidator(object):
    """Utility class to check that priority inversion doesn't happen. """


    @staticmethod
    def check_acls_deps(host, request):
        """Check if a host and request match by comparing acls and deps.

        @param host: A dictionary representing attributes of the host.
        @param request: A request, as defined in rdb_requests.

        @return True if the deps/acls of the request match the host.
        """
        # Unfortunately the hosts labels are labelnames, not ids.
        request_deps = set([l.name for l in
                models.Label.objects.filter(id__in=request.deps)])
        return (set(host['labels']).intersection(request_deps) == request_deps
                and set(host['acls']).intersection(request.acls))


    @staticmethod
    def find_matching_host_for_request(hosts, request):
        """Find a host from the given list of hosts, matching the request.

        @param hosts: A list of dictionaries representing host attributes.
        @param requetst: The unsatisfied request.

        @return: A host, if a matching host is found from the input list.
        """
        if not hosts or not request:
            return None
        for host in hosts:
            if AssignmentValidator.check_acls_deps(host, request):
                return host


    @staticmethod
    def sort_requests(requests):
        """Sort the requests by priority.

        @param requests: Unordered requests.

        @return: A list of requests ordered by priority.
        """
        return sorted(collections.Counter(requests).items(),
                key=lambda request: request[0].priority, reverse=True)


    @staticmethod
    def verify_priority(request_queue, result):
        requests = AssignmentValidator.sort_requests(request_queue)
        for request, count in requests:
            hosts = result.get(request)
            # The request was completely satisfied.
            if hosts and len(hosts) == count:
                continue
            # Go through all hosts given to lower priority requests and
            # make sure we couldn't have allocated one of them for this
            # unsatisfied higher priority request.
            lower_requests = requests[requests.index((request,count))+1:]
            for lower_request, count in lower_requests:
                if (lower_request.priority < request.priority and
                    AssignmentValidator.find_matching_host_for_request(
                            result.get(lower_request), request)):
                    raise ValueError('Priority inversion occured between '
                            'priorities %s and %s' %
                            (request.priority, lower_request.priority))


    @staticmethod
    def priority_checking_response_handler(request_manager):
        """Fake response handler wrapper for any request_manager.

        Check that higher priority requests get a response over lower priority
        requests, by re-validating all the hosts assigned to a lower priority
        request against the unsatisfied higher priority ones.

        @param request_manager: A request_manager as defined in rdb_lib.

        @raises ValueError: If priority inversion is detected.
        """
        # Fist call the rdb to make its decisions, then sort the requests
        # by priority and make sure unsatisfied requests higher up in the list
        # could not have been satisfied by hosts assigned to requests lower
        # down in the list.
        result = request_manager.api_call(request_manager.request_queue)
        if not result:
            raise ValueError('Expected results but got none.')
        AssignmentValidator.verify_priority(
                request_manager.request_queue, result)
        for hosts in result.values():
            for host in hosts:
                yield host


class BaseRDBTest(rdb_testing_utils.AbstractBaseRDBTester, unittest.TestCase):
    _config_section = 'AUTOTEST_WEB'


    def testAcquireLeasedHostBasic(self):
        """Test that acquisition of a leased host doesn't happen.

        @raises AssertionError: If the one host that satisfies the request
            is acquired.
        """
        job = self.create_job(deps=set(['a']))
        host = self.db_helper.create_host('h1', deps=set(['a']))
        host.leased = 1
        host.save()
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        hosts = list(rdb_lib.acquire_hosts(queue_entries))
        self.assertTrue(len(hosts) == 1 and hosts[0] is None)


    def testAcquireLeasedHostRace(self):
        """Test behaviour when hosts are leased just before acquisition.

        If a fraction of the hosts somehow get leased between finding and
        acquisition, the rdb should just return the remaining hosts for the
        request to use.

        @raises AssertionError: If both the requests get a host successfully,
            since one host gets leased before the final attempt to lease both.
        """
        j1 = self.create_job(deps=set(['a']))
        j2 = self.create_job(deps=set(['a']))
        hosts = [self.db_helper.create_host('h1', deps=set(['a'])),
                 self.db_helper.create_host('h2', deps=set(['a']))]

        @rdb_hosts.return_rdb_host
        def local_find_hosts(host_query_manger, deps, acls):
            """Return a predetermined list of hosts, one of which is leased."""
            h1 = models.Host.objects.get(hostname='h1')
            h1.leased = 1
            h1.save()
            h2 = models.Host.objects.get(hostname='h2')
            return [h1, h2]

        self.god.stub_with(rdb.AvailableHostQueryManager, 'find_hosts',
                           local_find_hosts)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        hosts = list(rdb_lib.acquire_hosts(queue_entries))
        self.assertTrue(len(hosts) == 2 and None in hosts)
        self.check_hosts(iter(hosts))


    def testHostReleaseStates(self):
        """Test that we will only release an unused host if it is in Ready.

        @raises AssertionError: If the host gets released in any other state.
        """
        host = self.db_helper.create_host('h1', deps=set(['x']))
        for state in rdb_model_extensions.AbstractHostModel.Status.names:
            host.status = state
            host.leased = 1
            host.save()
            self._release_unused_hosts()
            host = models.Host.objects.get(hostname='h1')
            self.assertTrue(host.leased == (state != 'Ready'))


    def testHostReleseHQE(self):
        """Test that we will not release a ready host if it's being used.

        @raises AssertionError: If the host is released even though it has
            been assigned to an active hqe.
        """
        # Create a host and lease it out in Ready.
        host = self.db_helper.create_host('h1', deps=set(['x']))
        host.status = 'Ready'
        host.leased = 1
        host.save()

        # Create a job and give its hqe the leased host.
        job = self.create_job(deps=set(['x']))
        self.db_helper.add_host_to_job(host, job.id)
        hqe = models.HostQueueEntry.objects.get(job_id=job.id)

        # Activate the hqe by setting its state.
        hqe.status = host_queue_entry_states.ACTIVE_STATUSES[0]
        hqe.save()

        # Make sure the hqes host isn't released, even if its in ready.
        self._release_unused_hosts()
        host = models.Host.objects.get(hostname='h1')
        self.assertTrue(host.leased == 1)


    def testBasicDepsAcls(self):
        """Test a basic deps/acls request.

        Make sure that a basic request with deps and acls, finds a host from
        the ready pool that has matching labels and is in a matching aclgroups.

        @raises AssertionError: If the request doesn't find a host, since the
            we insert a matching host in the ready pool.
        """
        deps = set(['a', 'b'])
        acls = set(['a', 'b'])
        self.db_helper.create_host('h1', deps=deps, acls=acls)
        job = self.create_job(user='autotest_system', deps=deps, acls=acls)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        matching_host  = rdb_lib.acquire_hosts(queue_entries).next()
        self.check_host_assignment(job.id, matching_host.id)
        self.assertTrue(matching_host.leased == 1)


    def testPreferredDeps(self):
        """Test that perferred deps is respected.

        If multiple hosts satisfied a job's deps, the one with preferred
        label will be assigned to the job.

        @raises AssertionError: If a host without a preferred label is
                                assigned to the job instead of one with
                                a preferred label.
        """
        lumpy_deps = set(['board:lumpy'])
        stumpy_deps = set(['board:stumpy'])
        stumpy_deps_with_crosversion = set(
                ['board:stumpy', 'cros-version:lumpy-release/R41-6323.0.0'])

        acls = set(['a', 'b'])
        # Hosts lumpy1 and lumpy2 are created as a control group,
        # which ensures that if no preferred label is used, the host
        # with a smaller id will be chosen first. We need to make sure
        # stumpy2 was chosen because it has a cros-version label, but not
        # because of other randomness.
        self.db_helper.create_host('lumpy1', deps=lumpy_deps, acls=acls)
        self.db_helper.create_host('lumpy2', deps=lumpy_deps, acls=acls)
        self.db_helper.create_host('stumpy1', deps=stumpy_deps, acls=acls)
        self.db_helper.create_host(
                    'stumpy2', deps=stumpy_deps_with_crosversion , acls=acls)
        job_1 = self.create_job(user='autotest_system',
                              deps=lumpy_deps, acls=acls)
        job_2 = self.create_job(user='autotest_system',
                              deps=stumpy_deps_with_crosversion, acls=acls)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        matching_hosts  = list(rdb_lib.acquire_hosts(queue_entries))
        assignment = {}
        import logging
        for job, host in zip(queue_entries, matching_hosts):
            self.check_host_assignment(job.id, host.id)
            assignment[job.id] = host.hostname
        self.assertEqual(assignment[job_1.id], 'lumpy1')
        self.assertEqual(assignment[job_2.id], 'stumpy2')


    def testBadDeps(self):
        """Test that we find no hosts when only acls match.

        @raises AssertionError: If the request finds a host, since the only
            host in the ready pool will not have matching deps.
        """
        host_labels = set(['a'])
        job_deps = set(['b'])
        acls = set(['a', 'b'])
        self.db_helper.create_host('h1', deps=host_labels, acls=acls)
        job = self.create_job(user='autotest_system', deps=job_deps, acls=acls)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        matching_host  = rdb_lib.acquire_hosts(queue_entries).next()
        self.assert_(not matching_host)


    def testBadAcls(self):
        """Test that we find no hosts when only deps match.

        @raises AssertionError: If the request finds a host, since the only
            host in the ready pool will not have matching acls.
        """
        deps = set(['a'])
        host_acls = set(['a'])
        job_acls = set(['b'])
        self.db_helper.create_host('h1', deps=deps, acls=host_acls)

        # Create the job as a new user who is only in the 'b' and 'Everyone'
        # aclgroups. Though there are several hosts in the Everyone group, the
        # 1 host that has the 'a' dep isn't.
        job = self.create_job(user='new_user', deps=deps, acls=job_acls)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        matching_host  = rdb_lib.acquire_hosts(queue_entries).next()
        self.assert_(not matching_host)


    def testBasicPriority(self):
        """Test that priority inversion doesn't happen.

        Schedule 2 jobs with the same deps, acls and user, but different
        priorities, and confirm that the higher priority request gets the host.
        This confirmation happens through the AssignmentValidator.

        @raises AssertionError: If the un important request gets host h1 instead
            of the important request.
        """
        deps = set(['a', 'b'])
        acls = set(['a', 'b'])
        self.db_helper.create_host('h1', deps=deps, acls=acls)
        important_job = self.create_job(user='autotest_system',
                deps=deps, acls=acls, priority=2)
        un_important_job = self.create_job(user='autotest_system',
                deps=deps, acls=acls, priority=0)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        self.god.stub_with(rdb_requests.BaseHostRequestManager, 'response',
                AssignmentValidator.priority_checking_response_handler)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))


    def testPriorityLevels(self):
        """Test that priority inversion doesn't happen.

        Increases a job's priority and makes several requests for hosts,
        checking that priority inversion doesn't happen.

        @raises AssertionError: If the unimportant job gets h1 while it is
            still unimportant, or doesn't get h1 while after it becomes the
            most important job.
        """
        deps = set(['a', 'b'])
        acls = set(['a', 'b'])
        self.db_helper.create_host('h1', deps=deps, acls=acls)

        # Create jobs that will bucket differently and confirm that jobs in an
        # earlier bucket get a host.
        first_job = self.create_job(user='autotest_system', deps=deps, acls=acls)
        important_job = self.create_job(user='autotest_system', deps=deps,
                acls=acls, priority=2)
        deps.pop()
        unimportant_job = self.create_job(user='someother_system', deps=deps,
                acls=acls, priority=1)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        self.god.stub_with(rdb_requests.BaseHostRequestManager, 'response',
                AssignmentValidator.priority_checking_response_handler)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))

        # Elevate the priority of the unimportant job, so we now have
        # 2 jobs at the same priority.
        self.db_helper.increment_priority(job_id=unimportant_job.id)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        self._release_unused_hosts()
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))

        # Prioritize the first job, and confirm that it gets the host over the
        # jobs that got it the last time.
        self.db_helper.increment_priority(job_id=unimportant_job.id)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        self._release_unused_hosts()
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))


    def testFrontendJobScheduling(self):
        """Test that basic frontend job scheduling.

        @raises AssertionError: If the received and requested host don't match,
            or the mis-matching host is returned instead.
        """
        deps = set(['x', 'y'])
        acls = set(['a', 'b'])

        # Create 2 frontend jobs and only one matching host.
        matching_job = self.create_job(acls=acls, deps=deps)
        matching_host = self.db_helper.create_host('h1', acls=acls, deps=deps)
        mis_matching_job = self.create_job(acls=acls, deps=deps)
        mis_matching_host = self.db_helper.create_host(
                'h2', acls=acls, deps=deps.pop())
        self.db_helper.add_host_to_job(matching_host, matching_job.id)
        self.db_helper.add_host_to_job(mis_matching_host, mis_matching_job.id)

        # Check that only the matching host is returned, and that we get 'None'
        # for the second request.
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        hosts = list(rdb_lib.acquire_hosts(queue_entries))
        self.assertTrue(len(hosts) == 2 and None in hosts)
        returned_host = [host for host in hosts if host].pop()
        self.assertTrue(matching_host.id == returned_host.id)


    def testFrontendJobPriority(self):
        """Test that frontend job scheduling doesn't ignore priorities.

        @raises ValueError: If the priorities of frontend jobs are ignored.
        """
        board = 'x'
        high_priority = self.create_job(priority=2, deps=set([board]))
        low_priority = self.create_job(priority=1, deps=set([board]))
        host = self.db_helper.create_host('h1', deps=set([board]))
        self.db_helper.add_host_to_job(host, low_priority.id)
        self.db_helper.add_host_to_job(host, high_priority.id)

        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        def local_response_handler(request_manager):
            """Confirms that a higher priority frontend job gets a host.

            @raises ValueError: If priority inversion happens and the job
                with priority 1 gets the host instead.
            """
            result = request_manager.api_call(request_manager.request_queue)
            if not result:
                raise ValueError('Excepted the high priority request to '
                                 'get a host, but the result is empty.')
            for request, hosts in result.iteritems():
                if request.priority == 1:
                    raise ValueError('Priority of frontend job ignored.')
                if len(hosts) > 1:
                    raise ValueError('Multiple hosts returned against one '
                                     'frontend job scheduling request.')
                yield hosts[0]

        self.god.stub_with(rdb_requests.BaseHostRequestManager, 'response',
                           local_response_handler)
        self.check_hosts(rdb_lib.acquire_hosts(queue_entries))


    def testSuiteOrderedHostAcquisition(self):
        """Test that older suite jobs acquire hosts first.

        Make sure older suite jobs get hosts first, but not at the expense of
        higher priority jobs.

        @raises ValueError: If unexpected acquisitions occur, eg:
            suite_job_2 acquires the last 2 hosts instead of suite_job_1.
            isolated_important_job doesn't get any hosts.
            Any job acquires more hosts than necessary.
        """
        board = 'x'

        # Create 2 suites such that the later suite has an ordering of deps
        # that places it ahead of the earlier suite, if parent_job_id is
        # ignored.
        suite_without_dep = self.create_suite(num=2, priority=0, board=board)

        suite_with_dep = self.create_suite(num=1, priority=0, board=board)
        self.db_helper.add_deps_to_job(suite_with_dep[0], dep_names=list('y'))

        # Create an important job that should be ahead of the first suite,
        # because priority trumps parent_job_id and time of creation.
        isolated_important_job = self.create_job(priority=3, deps=set([board]))

        # Create 3 hosts, all with the deps to satisfy the last suite.
        for i in range(0, 3):
            self.db_helper.create_host('h%s' % i, deps=set([board, 'y']))

        queue_entries = self._dispatcher._refresh_pending_queue_entries()

        def local_response_handler(request_manager):
            """Reorder requests and check host acquisition.

            @raises ValueError: If unexpected/no acquisitions occur.
            """
            if any([request for request in request_manager.request_queue
                    if request.parent_job_id is None]):
                raise ValueError('Parent_job_id can never be None.')

            # This will result in the ordering:
            # [suite_2_1, suite_1_*, suite_1_*, isolated_important_job]
            # The priority scheduling order should be:
            # [isolated_important_job, suite_1_*, suite_1_*, suite_2_1]
            # Since:
            #   a. the isolated_important_job is the most important.
            #   b. suite_1 was created before suite_2, regardless of deps
            disorderly_queue = sorted(request_manager.request_queue,
                    key=lambda r: -r.parent_job_id)
            request_manager.request_queue = disorderly_queue
            result = request_manager.api_call(request_manager.request_queue)
            if not result:
                raise ValueError('Expected results but got none.')

            # Verify that the isolated_important_job got a host, and that the
            # first suite got both remaining free hosts.
            for request, hosts in result.iteritems():
                if request.parent_job_id == 0:
                    if len(hosts) > 1:
                        raise ValueError('First job acquired more hosts than '
                                'necessary. Response map: %s' % result)
                    continue
                if request.parent_job_id == 1:
                    if len(hosts) < 2:
                        raise ValueError('First suite job requests were not '
                                'satisfied. Response_map: %s' % result)
                    continue
                # The second suite job got hosts instead of one of
                # the others. Eitherway this is a failure.
                raise ValueError('Unexpected host acquisition '
                        'Response map: %s' % result)
            yield None

        self.god.stub_with(rdb_requests.BaseHostRequestManager, 'response',
                           local_response_handler)
        list(rdb_lib.acquire_hosts(queue_entries))


    def testConfigurations(self):
        """Test that configurations don't matter.
        @raises AssertionError: If the request doesn't find a host,
                 this will happen if configurations are not stripped out.
        """
        self.god.stub_with(provision.Cleanup,
                           '_actions',
                           {'action': 'fakeTest'})
        job_labels = set(['action', 'a'])
        host_deps = set(['a'])
        db_host = self.db_helper.create_host('h1', deps=host_deps)
        self.create_job(user='autotest_system', deps=job_labels)
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        matching_host = rdb_lib.acquire_hosts(queue_entries).next()
        self.assert_(matching_host.id == db_host.id)


class RDBMinDutTest(
        rdb_testing_utils.AbstractBaseRDBTester, unittest.TestCase):
    """Test AvailableHostRequestHandler"""

    _config_section = 'AUTOTEST_WEB'


    def min_dut_test_helper(self, num_hosts, suite_settings):
        """A helper function to test min_dut logic.

        @param num_hosts: Total number of hosts to create.
        @param suite_settings: A dictionary specify how suites would be created
                               and verified.
                E.g.  {'priority': 10, 'num_jobs': 3,
                       'min_duts':2, 'expected_aquired': 1}
                       With this setting, will create a suite that has 3
                       child jobs, with priority 10 and min_duts 2.
                       The suite is expected to get 1 dut.
        """
        acls = set(['fake_acl'])
        hosts = []
        for i in range (0, num_hosts):
            hosts.append(self.db_helper.create_host(
                'h%d' % i, deps=set(['board:lumpy']), acls=acls))
        suites = {}
        suite_min_duts = {}
        for setting in suite_settings:
            s = self.create_suite(num=setting['num_jobs'],
                                  priority=setting['priority'],
                                  board='board:lumpy', acls=acls)
            # Empty list will be used to store acquired hosts.
            suites[s['parent_job'].id] = (setting, [])
            suite_min_duts[s['parent_job'].id] = setting['min_duts']
        queue_entries = self._dispatcher._refresh_pending_queue_entries()
        matching_hosts = rdb_lib.acquire_hosts(queue_entries, suite_min_duts)
        for host, queue_entry in zip(matching_hosts, queue_entries):
            if host:
                suites[queue_entry.job.parent_job_id][1].append(host)

        for setting, hosts in suites.itervalues():
            self.assertEqual(len(hosts),setting['expected_aquired'])


    def testHighPriorityTakeAll(self):
        """Min duts not satisfied."""
        num_hosts = 1
        suite1 = {'priority':20, 'num_jobs': 3, 'min_duts': 2,
                  'expected_aquired': 1}
        suite2 = {'priority':10, 'num_jobs': 7, 'min_duts': 5,
                  'expected_aquired': 0}
        self.min_dut_test_helper(num_hosts, [suite1, suite2])


    def testHighPriorityMinSatisfied(self):
        """High priority min duts satisfied."""
        num_hosts = 4
        suite1 = {'priority':20, 'num_jobs': 4, 'min_duts': 2,
                  'expected_aquired': 2}
        suite2 = {'priority':10, 'num_jobs': 7, 'min_duts': 5,
                  'expected_aquired': 2}
        self.min_dut_test_helper(num_hosts, [suite1, suite2])


    def testAllPrioritiesMinSatisfied(self):
        """Min duts satisfied."""
        num_hosts = 7
        suite1 = {'priority':20, 'num_jobs': 4, 'min_duts': 2,
                  'expected_aquired': 2}
        suite2 = {'priority':10, 'num_jobs': 7, 'min_duts': 5,
                  'expected_aquired': 5}
        self.min_dut_test_helper(num_hosts, [suite1, suite2])


    def testHighPrioritySatisfied(self):
        """Min duts satisfied, high priority suite satisfied."""
        num_hosts = 10
        suite1 = {'priority':20, 'num_jobs': 4, 'min_duts': 2,
                  'expected_aquired': 4}
        suite2 = {'priority':10, 'num_jobs': 7, 'min_duts': 5,
                  'expected_aquired': 6}
        self.min_dut_test_helper(num_hosts, [suite1, suite2])


    def testEqualPriorityFirstSuiteMinSatisfied(self):
        """Equal priority, earlier suite got min duts."""
        num_hosts = 4
        suite1 = {'priority':20, 'num_jobs': 4, 'min_duts': 2,
                  'expected_aquired': 2}
        suite2 = {'priority':20, 'num_jobs': 7, 'min_duts': 5,
                  'expected_aquired': 2}
        self.min_dut_test_helper(num_hosts, [suite1, suite2])


    def testEqualPriorityAllSuitesMinSatisfied(self):
        """Equal priority, all suites got min duts."""
        num_hosts = 7
        suite1 = {'priority':20, 'num_jobs': 4, 'min_duts': 2,
                  'expected_aquired': 2}
        suite2 = {'priority':20, 'num_jobs': 7, 'min_duts': 5,
                  'expected_aquired': 5}
        self.min_dut_test_helper(num_hosts, [suite1, suite2])


if __name__ == '__main__':
    unittest.main()

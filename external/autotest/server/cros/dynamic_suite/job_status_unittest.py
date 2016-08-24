#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable-msg=C0111

"""Unit tests for server/cros/dynamic_suite/job_status.py."""

import mox
import shutil
import tempfile
import time
import unittest
import os
import common

from autotest_lib.server import frontend
from autotest_lib.server.cros import host_lock_manager
from autotest_lib.server.cros.dynamic_suite import host_spec
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite.fakes import FakeHost, FakeJob
from autotest_lib.server.cros.dynamic_suite.fakes import FakeStatus


DEFAULT_WAITTIMEOUT_MINS = 60 * 4


class StatusTest(mox.MoxTestBase):
    """Unit tests for job_status.Status.
    """


    def setUp(self):
        super(StatusTest, self).setUp()
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.tko = self.mox.CreateMock(frontend.TKO)

        self.tmpdir = tempfile.mkdtemp(suffix=type(self).__name__)


    def tearDown(self):
        super(StatusTest, self).tearDown()
        shutil.rmtree(self.tmpdir, ignore_errors=True)


    def testGatherJobHostnamesAllRan(self):
        """All entries for the job were assigned hosts."""
        job = FakeJob(0, [])
        expected_hosts = ['host2', 'host1']
        entries = [{'status': 'Running',
                    'host': {'hostname': h}} for h in expected_hosts]
        self.afe.run('get_host_queue_entries', job=job.id).AndReturn(entries)
        self.mox.ReplayAll()

        self.assertEquals(sorted(expected_hosts),
                          sorted(job_status.gather_job_hostnames(self.afe,
                                                                 job)))


    def testGatherJobHostnamesSomeRan(self):
        """Not all entries for the job were assigned hosts."""
        job = FakeJob(0, [])
        expected_hosts = ['host2', 'host1']
        entries = [{'status': 'Running',
                    'host': {'hostname': h}} for h in expected_hosts]
        entries.append({'status': 'Running', 'host': None})
        self.afe.run('get_host_queue_entries', job=job.id).AndReturn(entries)
        self.mox.ReplayAll()

        self.assertEquals(sorted(expected_hosts + [None]),
                          sorted(job_status.gather_job_hostnames(self.afe,
                                                                 job)))


    def testGatherJobHostnamesSomeStillQueued(self):
        """Not all entries for the job were Running, though all had hosts."""
        job = FakeJob(0, [])
        expected_hosts = ['host2', 'host1']
        entries = [{'status': 'Running',
                    'host': {'hostname': h}} for h in expected_hosts]
        entries[-1]['status'] = 'Queued'
        self.afe.run('get_host_queue_entries', job=job.id).AndReturn(entries)
        self.mox.ReplayAll()

        self.assertTrue(expected_hosts[-1] not in
                        job_status.gather_job_hostnames(self.afe, job))


    def testWaitForJobToStart(self):
        """Ensure we detect when a job has started running."""
        self.mox.StubOutWithMock(time, 'sleep')

        job = FakeJob(0, [])
        self.afe.get_jobs(id=job.id, not_yet_run=True).AndReturn([job])
        self.afe.get_jobs(id=job.id, not_yet_run=True).AndReturn([])
        time.sleep(mox.IgnoreArg()).MultipleTimes()
        self.mox.ReplayAll()

        job_status.wait_for_jobs_to_start(self.afe, [job])


    def testWaitForMultipleJobsToStart(self):
        """Ensure we detect when all jobs have started running."""
        self.mox.StubOutWithMock(time, 'sleep')

        job0 = FakeJob(0, [])
        job1 = FakeJob(1, [])
        self.afe.get_jobs(id=job0.id, not_yet_run=True).AndReturn([job0])
        self.afe.get_jobs(id=job1.id, not_yet_run=True).AndReturn([job1])
        self.afe.get_jobs(id=job0.id, not_yet_run=True).AndReturn([])
        self.afe.get_jobs(id=job1.id, not_yet_run=True).AndReturn([job1])
        self.afe.get_jobs(id=job1.id, not_yet_run=True).AndReturn([])
        time.sleep(mox.IgnoreArg()).MultipleTimes()
        self.mox.ReplayAll()

        job_status.wait_for_jobs_to_start(self.afe, [job0, job1])


    def testWaitForJobToStartAlreadyStarted(self):
        """Ensure we don't wait forever if a job already started."""
        job = FakeJob(0, [])
        self.afe.get_jobs(id=job.id, not_yet_run=True).AndReturn([])
        self.mox.ReplayAll()
        job_status.wait_for_jobs_to_start(self.afe, [job])


    def testWaitForJobToFinish(self):
        """Ensure we detect when a job has finished."""
        self.mox.StubOutWithMock(time, 'sleep')

        job = FakeJob(0, [])
        self.afe.get_jobs(id=job.id, finished=True).AndReturn([])
        self.afe.get_jobs(id=job.id, finished=True).AndReturn([job])
        time.sleep(mox.IgnoreArg()).MultipleTimes()
        self.mox.ReplayAll()

        job_status.wait_for_jobs_to_finish(self.afe, [job])


    def testWaitForMultipleJobsToFinish(self):
        """Ensure we detect when all jobs have stopped running."""
        self.mox.StubOutWithMock(time, 'sleep')

        job0 = FakeJob(0, [])
        job1 = FakeJob(1, [])
        self.afe.get_jobs(id=job0.id, finished=True).AndReturn([])
        self.afe.get_jobs(id=job1.id, finished=True).AndReturn([])
        self.afe.get_jobs(id=job0.id, finished=True).AndReturn([])
        self.afe.get_jobs(id=job1.id, finished=True).AndReturn([job1])
        self.afe.get_jobs(id=job0.id, finished=True).AndReturn([job0])
        time.sleep(mox.IgnoreArg()).MultipleTimes()
        self.mox.ReplayAll()

        job_status.wait_for_jobs_to_finish(self.afe, [job0, job1])


    def testWaitForJobToFinishAlreadyFinished(self):
        """Ensure we don't wait forever if a job already finished."""
        job = FakeJob(0, [])
        self.afe.get_jobs(id=job.id, finished=True).AndReturn([job])
        self.mox.ReplayAll()
        job_status.wait_for_jobs_to_finish(self.afe, [job])


    def expect_hosts_query_and_lock(self, jobs, manager, running_hosts,
                                    do_lock=True):
        """Expect asking for a job's hosts and, potentially, lock them.

        job_status.gather_job_hostnames() should be mocked out prior to call.

        @param jobs: a lists of FakeJobs with a valid ID.
        @param manager: mocked out HostLockManager
        @param running_hosts: list of FakeHosts that should be listed as
                              'Running'.
        @param do_lock: If |manager| should expect |running_hosts| to get
                        added and locked.
        @return nothing, but self.afe, job_status.gather_job_hostnames, and
                manager will have expectations set.
        """
        used_hostnames = []
        for job in jobs:
            job_status.gather_job_hostnames(
                    mox.IgnoreArg(), job).InAnyOrder().AndReturn(job.hostnames)
            used_hostnames.extend([h for h in job.hostnames if h])

        if used_hostnames:
            self.afe.get_hosts(mox.SameElementsAs(used_hostnames),
                               status='Running').AndReturn(running_hosts)
        if do_lock:
            manager.lock([h.hostname for h in running_hosts])


    def testWaitForSingleJobHostsToRunAndGetLocked(self):
        """Ensure we lock all running hosts as they're discovered."""
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(job_status, 'gather_job_hostnames')

        manager = self.mox.CreateMock(host_lock_manager.HostLockManager)
        expected_hostnames=['host1', 'host0']
        expected_hosts = [FakeHost(h) for h in expected_hostnames]
        job = FakeJob(7, hostnames=[None, None])

        time.sleep(mox.IgnoreArg()).MultipleTimes()
        self.expect_hosts_query_and_lock([job], manager, [], False)
        # First, only one test in the job has had a host assigned at all.
        # Since no hosts are running, expect no locking.
        job.hostnames = [None] + expected_hostnames[1:]
        self.expect_hosts_query_and_lock([job], manager, [], False)

        # Then, that host starts running, but no other tests have hosts.
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[1:])

        # The second test gets a host assigned, but it's not yet running.
        # Since no new running hosts are found, no locking should happen.
        job.hostnames = expected_hostnames
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[1:],
                                         False)
        # The second test's host starts running as well.
        self.expect_hosts_query_and_lock([job], manager, expected_hosts)

        # The last loop update; doesn't impact behavior.
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job).AndReturn(expected_hostnames)
        self.mox.ReplayAll()
        self.assertEquals(
            sorted(expected_hostnames),
            sorted(job_status.wait_for_and_lock_job_hosts(self.afe,
                                                          [job],
                                                          manager)))


    def testWaitForAndLockWithTimeOutInStartJobs(self):
        """If we experience a timeout, no locked hosts are returned"""
        self.mox.StubOutWithMock(job_status, 'gather_job_hostnames')
        self.mox.StubOutWithMock(job_status, '_abort_jobs_if_timedout')

        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(True)
        manager = self.mox.CreateMock(host_lock_manager.HostLockManager)
        expected_hostnames=['host1', 'host0']
        expected_hosts = [FakeHost(h) for h in expected_hostnames]
        job = FakeJob(7, hostnames=[None, None])
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job).AndReturn(expected_hostnames)
        self.mox.ReplayAll()
        self.assertFalse(job_status.wait_for_and_lock_job_hosts(self.afe,
                [job], manager, wait_timeout_mins=DEFAULT_WAITTIMEOUT_MINS))


    def testWaitForAndLockWithTimedOutSubJobs(self):
        """If we experience a timeout, no locked hosts are returned"""
        self.mox.StubOutWithMock(job_status, 'gather_job_hostnames')
        self.mox.StubOutWithMock(job_status, '_abort_jobs_if_timedout')

        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(True)
        manager = self.mox.CreateMock(host_lock_manager.HostLockManager)
        expected_hostnames=['host1', 'host0']
        expected_hosts = [FakeHost(h) for h in expected_hostnames]
        job = FakeJob(7, hostnames=[None, None])
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job).AndReturn(expected_hostnames)
        self.mox.ReplayAll()
        self.assertEquals(set(),
                job_status.wait_for_and_lock_job_hosts(self.afe, [job],
                manager, wait_timeout_mins=DEFAULT_WAITTIMEOUT_MINS))


    def testWaitForSingleJobHostsWithTimeout(self):
        """Discover a single host for this job then timeout."""
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(job_status, 'gather_job_hostnames')
        self.mox.StubOutWithMock(job_status, '_abort_jobs_if_timedout')

        manager = self.mox.CreateMock(host_lock_manager.HostLockManager)
        expected_hostnames=['host1', 'host0']
        expected_hosts = [FakeHost(h) for h in expected_hostnames]
        job = FakeJob(7, hostnames=[None, None])

        time.sleep(mox.IgnoreArg()).MultipleTimes()
        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(False)
        self.expect_hosts_query_and_lock([job], manager, [], False)

        # First, only one test in the job has had a host assigned at all.
        # Since no hosts are running, expect no locking.
        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(False)
        job.hostnames = [None] + expected_hostnames[1:]
        self.expect_hosts_query_and_lock([job], manager, [], False)

        # Then, that host starts running, but no other tests have hosts.
        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(False)
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[1:])

        # The second test gets a host assigned, but it's not yet running.
        # Since no new running hosts are found, no locking should happen.
        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(False)
        job.hostnames = expected_hostnames
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[1:],
                                         False)

        # A timeout occurs, and only the locked hosts should be returned.
        job_status._abort_jobs_if_timedout(mox.IgnoreArg(), mox.IgnoreArg(),
                mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(True)

        # The last loop update; doesn't impact behavior.
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job).AndReturn(expected_hostnames)
        self.mox.ReplayAll()

        # Because of the timeout only one host is returned.
        expect_timeout_hostnames = ['host0']
        self.assertEquals(sorted(expect_timeout_hostnames),sorted(
                job_status.wait_for_and_lock_job_hosts(self.afe,
                [job],manager, wait_timeout_mins=DEFAULT_WAITTIMEOUT_MINS)))


    def testWaitForSingleJobHostsToRunAndGetLockedSerially(self):
        """Lock running hosts as discovered, serially."""
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(job_status, 'gather_job_hostnames')

        manager = self.mox.CreateMock(host_lock_manager.HostLockManager)
        expected_hostnames=['host1', 'host0']
        expected_hosts = [FakeHost(h) for h in expected_hostnames]
        job = FakeJob(7, hostnames=[None, None])

        time.sleep(mox.IgnoreArg()).MultipleTimes()
        self.expect_hosts_query_and_lock([job], manager, [], False)
        # First, only one test in the job has had a host assigned at all.
        # Since no hosts are running, expect no locking.
        job.hostnames = [None] + expected_hostnames[1:]
        self.expect_hosts_query_and_lock([job], manager, [], False)

        # Then, that host starts running, but no other tests have hosts.
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[1:])

        # The second test gets a host assigned, but it's not yet running.
        # Since no new running hosts are found, no locking should happen.
        job.hostnames = expected_hostnames
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[1:],
                                         False)
        # The second test's host starts running as well, and the first stops.
        self.expect_hosts_query_and_lock([job], manager, expected_hosts[:1])

        # The last loop update; doesn't impact behavior.
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job).AndReturn(expected_hostnames)
        self.mox.ReplayAll()
        self.assertEquals(
            sorted(expected_hostnames),
            sorted(job_status.wait_for_and_lock_job_hosts(self.afe,
                                                          [job],
                                                          manager)))


    def testWaitForMultiJobHostsToRunAndGetLocked(self):
        """Ensure we lock all running hosts for all jobs as discovered."""
        self.mox.StubOutWithMock(time, 'sleep')
        self.mox.StubOutWithMock(job_status, 'gather_job_hostnames')

        manager = self.mox.CreateMock(host_lock_manager.HostLockManager)
        expected_hostnames = ['host1', 'host0', 'host2']
        expected_hosts = [FakeHost(h) for h in expected_hostnames]
        job0 = FakeJob(0, hostnames=[])
        job1 = FakeJob(1, hostnames=[])

        time.sleep(mox.IgnoreArg()).MultipleTimes()
        # First, only one test in either job has had a host assigned at all.
        # Since no hosts are running, expect no locking.
        job0.hostnames = [None, expected_hostnames[2]]
        job1.hostnames = [None]
        self.expect_hosts_query_and_lock([job0, job1], manager, [], False)

        # Then, that host starts running, but no other tests have hosts.
        self.expect_hosts_query_and_lock([job0, job1], manager,
                                         expected_hosts[2:])

        # The test in the second job gets a host assigned, but it's not yet
        # running.
        # Since no new running hosts are found, no locking should happen.
        job1.hostnames = expected_hostnames[1:2]
        self.expect_hosts_query_and_lock([job0, job1], manager,
                                         expected_hosts[2:], False)

        # The second job's test's host starts running as well.
        self.expect_hosts_query_and_lock([job0, job1], manager,
                                         expected_hosts[1:])

        # All three hosts across both jobs are now running.
        job0.hostnames = [expected_hostnames[0], expected_hostnames[2]]
        self.expect_hosts_query_and_lock([job0, job1], manager, expected_hosts)

        # The last loop update; doesn't impact behavior.
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job0).AndReturn(job0.hostnames)
        job_status.gather_job_hostnames(mox.IgnoreArg(),
                                        job1).AndReturn(job1.hostnames)

        self.mox.ReplayAll()
        self.assertEquals(
            sorted(expected_hostnames),
            sorted(job_status.wait_for_and_lock_job_hosts(self.afe,
                                                          [job0, job1],
                                                          manager)))


    def expect_result_gathering(self, job):
        self.afe.get_jobs(id=job.id, finished=True).AndReturn(job)
        self.expect_yield_job_entries(job)


    def expect_yield_job_entries(self, job):
        entries = [s.entry for s in job.statuses]
        self.afe.run('get_host_queue_entries',
                     job=job.id).AndReturn(entries)
        if True not in map(lambda e: 'aborted' in e and e['aborted'], entries):
            self.tko.get_job_test_statuses_from_db(job.id).AndReturn(
                    job.statuses)


    def testWaitForResults(self):
        """Should gather status and return records for job summaries."""
        jobs = [FakeJob(0, [FakeStatus('GOOD', 'T0', ''),
                            FakeStatus('GOOD', 'T1', '')]),
                FakeJob(1, [FakeStatus('ERROR', 'T0', 'err', False),
                            FakeStatus('GOOD', 'T1', '')]),
                FakeJob(2, [FakeStatus('TEST_NA', 'T0', 'no')]),
                FakeJob(3, [FakeStatus('FAIL', 'T0', 'broken')]),
                FakeJob(4, [FakeStatus('ERROR', 'SERVER_JOB', 'server error'),
                            FakeStatus('GOOD', 'T0', '')]),]

                # TODO: Write a better test for the case where we yield
                # results for aborts vs cannot yield results because of
                # a premature abort. Currently almost all client aborts
                # have been converted to failures, and when aborts do happen
                # they result in server job failures for which we always
                # want results.
                # FakeJob(5, [FakeStatus('ERROR', 'T0', 'gah', True)]),
                # The next job shouldn't be recorded in the results.
                # FakeJob(6, [FakeStatus('GOOD', 'SERVER_JOB', '')])]

        for status in jobs[4].statuses:
            status.entry['job'] = {'name': 'broken_infra_job'}

        # To simulate a job that isn't ready the first time we check.
        self.afe.get_jobs(id=jobs[0].id, finished=True).AndReturn([])
        # Expect all the rest of the jobs to be good to go the first time.
        for job in jobs[1:]:
            self.expect_result_gathering(job)
        # Then, expect job[0] to be ready.
        self.expect_result_gathering(jobs[0])
        # Expect us to poll twice.
        self.mox.StubOutWithMock(time, 'sleep')
        time.sleep(5)
        time.sleep(5)
        self.mox.ReplayAll()

        results = [result for result in job_status.wait_for_results(self.afe,
                                                                    self.tko,
                                                                    jobs)]
        for job in jobs[:6]:  # the 'GOOD' SERVER_JOB shouldn't be there.
            for status in job.statuses:
                self.assertTrue(True in map(status.equals_record, results))


    def testWaitForChildResults(self):
        """Should gather status and return records for job summaries."""
        parent_job_id = 54321
        jobs = [FakeJob(0, [FakeStatus('GOOD', 'T0', ''),
                            FakeStatus('GOOD', 'T1', '')],
                        parent_job_id=parent_job_id),
                FakeJob(1, [FakeStatus('ERROR', 'T0', 'err', False),
                            FakeStatus('GOOD', 'T1', '')],
                        parent_job_id=parent_job_id),
                FakeJob(2, [FakeStatus('TEST_NA', 'T0', 'no')],
                        parent_job_id=parent_job_id),
                FakeJob(3, [FakeStatus('FAIL', 'T0', 'broken')],
                        parent_job_id=parent_job_id),
                FakeJob(4, [FakeStatus('ERROR', 'SERVER_JOB', 'server error'),
                            FakeStatus('GOOD', 'T0', '')],
                        parent_job_id=parent_job_id),]

                # TODO: Write a better test for the case where we yield
                # results for aborts vs cannot yield results because of
                # a premature abort. Currently almost all client aborts
                # have been converted to failures and when aborts do happen
                # they result in server job failures for which we always
                # want results.
                #FakeJob(5, [FakeStatus('ERROR', 'T0', 'gah', True)],
                #        parent_job_id=parent_job_id),
                # The next job shouldn't be recorded in the results.
                #FakeJob(6, [FakeStatus('GOOD', 'SERVER_JOB', '')],
                #        parent_job_id=12345)]
        for status in jobs[4].statuses:
            status.entry['job'] = {'name': 'broken_infra_job'}

        # Expect one call to get a list of all child jobs.
        self.afe.get_jobs(parent_job_id=parent_job_id).AndReturn(jobs[:6])

        # Have the first two jobs be finished by the first polling,
        # and the remaining ones (not including #6) for the second polling.
        self.afe.get_jobs(parent_job_id=parent_job_id,
                          finished=True).AndReturn([jobs[1]])
        self.expect_yield_job_entries(jobs[1])

        self.afe.get_jobs(parent_job_id=parent_job_id,
                          finished=True).AndReturn(jobs[:2])
        self.expect_yield_job_entries(jobs[0])

        self.afe.get_jobs(parent_job_id=parent_job_id,
                          finished=True).AndReturn(jobs[:6])
        for job in jobs[2:6]:
            self.expect_yield_job_entries(job)
        # Then, expect job[0] to be ready.

        # Expect us to poll thrice
        self.mox.StubOutWithMock(time, 'sleep')
        time.sleep(5)
        time.sleep(5)
        time.sleep(5)
        self.mox.ReplayAll()

        results = [result for result in job_status.wait_for_child_results(
                                                self.afe,
                                                self.tko,
                                                parent_job_id)]
        for job in jobs[:6]:  # the 'GOOD' SERVER_JOB shouldn't be there.
            for status in job.statuses:
                self.assertTrue(True in map(status.equals_record, results))


    def testYieldSubdir(self):
        """Make sure subdir are properly set for test and non-test status."""
        job_tag = '0-owner/172.33.44.55'
        job_name = 'broken_infra_job'
        job = FakeJob(0, [FakeStatus('ERROR', 'SERVER_JOB', 'server error',
                                     subdir='---', job_tag=job_tag),
                          FakeStatus('GOOD', 'T0', '',
                                     subdir='T0.subdir', job_tag=job_tag)],
                      parent_job_id=54321)
        for status in job.statuses:
            status.entry['job'] = {'name': job_name}
        self.expect_yield_job_entries(job)
        self.mox.ReplayAll()
        results = list(job_status._yield_job_results(self.afe, self.tko, job))
        for i in range(len(results)):
            result = results[i]
            if result.test_name.endswith('SERVER_JOB'):
                expected_name = '%s_%s' % (job_name, job.statuses[i].test_name)
                expected_subdir = job_tag
            else:
                expected_name = job.statuses[i].test_name
                expected_subdir = os.path.join(job_tag, job.statuses[i].subdir)
            self.assertEqual(results[i].test_name, expected_name)
            self.assertEqual(results[i].subdir, expected_subdir)


    def testGatherPerHostResults(self):
        """Should gather per host results."""
        # For the 0th job, the 1st entry is more bad/specific.
        # For all the others, it's the 0th that we expect.
        jobs = [FakeJob(0, [FakeStatus('FAIL', 'T0', '', hostname='h0'),
                            FakeStatus('FAIL', 'T1', 'bad', hostname='h0')]),
                FakeJob(1, [FakeStatus('ERROR', 'T0', 'err', False, 'h1'),
                            FakeStatus('GOOD', 'T1', '', hostname='h1')]),
                FakeJob(2, [FakeStatus('TEST_NA', 'T0', 'no', hostname='h2')]),
                FakeJob(3, [FakeStatus('FAIL', 'T0', 'broken', hostname='h3')]),
                FakeJob(4, [FakeStatus('ERROR', 'T0', 'gah', True, 'h4')]),
                FakeJob(5, [FakeStatus('GOOD', 'T0', 'Yay', hostname='h5')])]
        # Method under test returns status available right now.
        for job in jobs:
            entries = map(lambda s: s.entry, job.statuses)
            self.afe.run('get_host_queue_entries',
                         job=job.id).AndReturn(entries)
            self.tko.get_job_test_statuses_from_db(job.id).AndReturn(
                    job.statuses)
        self.mox.ReplayAll()

        results = job_status.gather_per_host_results(self.afe,
                                                     self.tko,
                                                     jobs).values()
        for status in [jobs[0].statuses[1]] + [j.statuses[0] for j in jobs[1:]]:
            self.assertTrue(True in map(status.equals_hostname_record, results))


    def _prepareForReporting(self, results):
        def callable(x):
            pass

        record_entity = self.mox.CreateMock(callable)
        group = self.mox.CreateMock(host_spec.HostGroup)

        statuses = {}
        all_bad = True not in results.itervalues()
        for hostname, result in results.iteritems():
            status = self.mox.CreateMock(job_status.Status)
            status.record_all(record_entity).InAnyOrder('recording')
            status.is_good().InAnyOrder('recording').AndReturn(result)
            if not result:
                status.test_name = 'test'
                if not all_bad:
                    status.override_status('WARN').InAnyOrder('recording')
            else:
                group.mark_host_success(hostname).InAnyOrder('recording')
            statuses[hostname] = status

        return (statuses, group, record_entity)


    def testRecordAndReportGoodResults(self):
        """Record and report success across the board."""
        results = {'h1': True, 'h2': True}
        (statuses, group, record_entity) = self._prepareForReporting(results)
        group.enough_hosts_succeeded().AndReturn(True)
        self.mox.ReplayAll()

        success = job_status.check_and_record_reimage_results(statuses,
                                                              group,
                                                              record_entity)
        self.assertTrue(success)


    def testRecordAndReportOkayResults(self):
        """Record and report success of at least one host."""
        results = {'h1': False, 'h2': True}
        (statuses, group, record_entity) = self._prepareForReporting(results)
        group.enough_hosts_succeeded().AndReturn(True)
        self.mox.ReplayAll()

        success = job_status.check_and_record_reimage_results(statuses,
                                                              group,
                                                              record_entity)
        self.assertTrue(success)


    def testRecordAndReportBadResults(self):
        """Record and report failure across the board."""
        results = {'h1': False, 'h2': False}
        (statuses, group, record_entity) = self._prepareForReporting(results)
        group.enough_hosts_succeeded().AndReturn(False)
        self.mox.ReplayAll()

        success = job_status.check_and_record_reimage_results(statuses,
                                                              group,
                                                              record_entity)
        self.assertFalse(success)


if __name__ == '__main__':
    unittest.main()

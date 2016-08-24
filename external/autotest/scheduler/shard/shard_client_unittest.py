# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import mox
import unittest

import common

from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.scheduler.shard import shard_client


class ShardClientTest(mox.MoxTestBase,
                      frontend_test_utils.FrontendTestMixin):
    """Unit tests for functions in shard_client.py"""


    GLOBAL_AFE_HOSTNAME = 'foo_autotest'


    def setUp(self):
        super(ShardClientTest, self).setUp()

        global_config.global_config.override_config_value(
                'SHARD', 'global_afe_hostname', self.GLOBAL_AFE_HOSTNAME)

        self._frontend_common_setup(fill_data=False)


    def setup_mocks(self):
        self.mox.StubOutClassWithMocks(frontend_wrappers, 'RetryingAFE')
        self.afe = frontend_wrappers.RetryingAFE(server=mox.IgnoreArg(),
                                                 delay_sec=5,
                                                 timeout_min=5)


    def setup_global_config(self):
        global_config.global_config.override_config_value(
                'SHARD', 'is_slave_shard', 'True')
        global_config.global_config.override_config_value(
                'SHARD', 'shard_hostname', 'host1')


    def expect_heartbeat(self, shard_hostname='host1',
                         known_job_ids=[], known_host_ids=[],
                         known_host_statuses=[], hqes=[], jobs=[],
                         side_effect=None, return_hosts=[], return_jobs=[],
                         return_suite_keyvals=[]):
        call = self.afe.run(
            'shard_heartbeat', shard_hostname=shard_hostname,
            hqes=hqes, jobs=jobs,
            known_job_ids=known_job_ids, known_host_ids=known_host_ids,
            known_host_statuses=known_host_statuses,
            )

        if side_effect:
            call = call.WithSideEffects(side_effect)

        call.AndReturn({
                'hosts': return_hosts,
                'jobs': return_jobs,
                'suite_keyvals': return_suite_keyvals,
            })


    def tearDown(self):
        self._frontend_common_teardown()

        # Without this global_config will keep state over test cases
        global_config.global_config.reset_config_values()


    def _get_sample_serialized_host(self):
        return {'aclgroup_set': [],
                'dirty': True,
                'hostattribute_set': [],
                'hostname': u'host1',
                u'id': 2,
                'invalid': False,
                'labels': [],
                'leased': True,
                'lock_time': None,
                'locked': False,
                'protection': 0,
                'shard': None,
                'status': u'Ready',
                'synch_id': None}


    def _get_sample_serialized_job(self):
        return {'control_file': u'foo',
                'control_type': 2,
                'created_on': datetime.datetime(2014, 9, 23, 15, 56, 10, 0),
                'dependency_labels': [{u'id': 1,
                                       'invalid': False,
                                       'kernel_config': u'',
                                       'name': u'board:lumpy',
                                       'only_if_needed': False,
                                       'platform': False}],
                'email_list': u'',
                'hostqueueentry_set': [{'aborted': False,
                                        'active': False,
                                        'complete': False,
                                        'deleted': False,
                                        'execution_subdir': u'',
                                        'finished_on': None,
                                        u'id': 1,
                                        'meta_host': {u'id': 1,
                                                      'invalid': False,
                                                      'kernel_config': u'',
                                                      'name': u'board:lumpy',
                                                      'only_if_needed': False,
                                                      'platform': False},
                                        'started_on': None,
                                        'status': u'Queued'}],
                u'id': 1,
                'jobkeyval_set': [],
                'max_runtime_hrs': 72,
                'max_runtime_mins': 1440,
                'name': u'dummy',
                'owner': u'autotest_system',
                'parse_failed_repair': True,
                'priority': 40,
                'parent_job_id': 0,
                'reboot_after': 0,
                'reboot_before': 1,
                'run_reset': True,
                'run_verify': False,
                'shard': {'hostname': u'shard1', u'id': 1},
                'synch_count': 0,
                'test_retry': 0,
                'timeout': 24,
                'timeout_mins': 1440}


    def _get_sample_serialized_suite_keyvals(self):
        return {'id': 1,
                'job_id': 0,
                'key': 'test_key',
                'value': 'test_value'}


    def testHeartbeat(self):
        """Trigger heartbeat, verify RPCs and persisting of the responses."""
        self.setup_mocks()

        global_config.global_config.override_config_value(
                'SHARD', 'shard_hostname', 'host1')

        self.expect_heartbeat(
                return_hosts=[self._get_sample_serialized_host()],
                return_jobs=[self._get_sample_serialized_job()],
                return_suite_keyvals=[
                        self._get_sample_serialized_suite_keyvals()])

        modified_sample_host = self._get_sample_serialized_host()
        modified_sample_host['hostname'] = 'host2'

        self.expect_heartbeat(
                return_hosts=[modified_sample_host],
                known_host_ids=[modified_sample_host['id']],
                known_host_statuses=[modified_sample_host['status']],
                known_job_ids=[1])


        def verify_upload_jobs_and_hqes(name, shard_hostname, jobs, hqes,
                                        known_host_ids, known_host_statuses,
                                        known_job_ids):
            self.assertEqual(len(jobs), 1)
            self.assertEqual(len(hqes), 1)
            job, hqe = jobs[0], hqes[0]
            self.assertEqual(hqe['status'], 'Completed')


        self.expect_heartbeat(
                jobs=mox.IgnoreArg(), hqes=mox.IgnoreArg(),
                known_host_ids=[modified_sample_host['id']],
                known_host_statuses=[modified_sample_host['status']],
                known_job_ids=[], side_effect=verify_upload_jobs_and_hqes)

        self.mox.ReplayAll()
        sut = shard_client.get_shard_client()

        sut.do_heartbeat()

        # Check if dummy object was saved to DB
        host = models.Host.objects.get(id=2)
        self.assertEqual(host.hostname, 'host1')

        # Check if suite keyval  was saved to DB
        suite_keyval = models.JobKeyval.objects.filter(job_id=0)[0]
        self.assertEqual(suite_keyval.key, 'test_key')

        sut.do_heartbeat()

        # Ensure it wasn't overwritten
        host = models.Host.objects.get(id=2)
        self.assertEqual(host.hostname, 'host1')

        job = models.Job.objects.all()[0]
        job.shard = None
        job.save()
        hqe = job.hostqueueentry_set.all()[0]
        hqe.status = 'Completed'
        hqe.save()

        sut.do_heartbeat()


        self.mox.VerifyAll()


    def testFailAndRedownloadJobs(self):
        self.setup_mocks()
        self.setup_global_config()

        job1_serialized = self._get_sample_serialized_job()
        job2_serialized = self._get_sample_serialized_job()
        job2_serialized['id'] = 2
        job2_serialized['hostqueueentry_set'][0]['id'] = 2

        self.expect_heartbeat(return_jobs=[job1_serialized])
        self.expect_heartbeat(return_jobs=[job1_serialized, job2_serialized])
        self.expect_heartbeat(known_job_ids=[job1_serialized['id'],
                                             job2_serialized['id']])
        self.expect_heartbeat(known_job_ids=[job2_serialized['id']])

        self.mox.ReplayAll()
        sut = shard_client.get_shard_client()

        original_process_heartbeat_response = sut.process_heartbeat_response
        def failing_process_heartbeat_response(*args, **kwargs):
            raise RuntimeError

        sut.process_heartbeat_response = failing_process_heartbeat_response
        self.assertRaises(RuntimeError, sut.do_heartbeat)

        sut.process_heartbeat_response = original_process_heartbeat_response
        sut.do_heartbeat()
        sut.do_heartbeat()

        job2 = models.Job.objects.get(pk=job1_serialized['id'])
        job2.hostqueueentry_set.all().update(complete=True)

        sut.do_heartbeat()

        self.mox.VerifyAll()


    def testFailAndRedownloadHosts(self):
        self.setup_mocks()
        self.setup_global_config()

        host1_serialized = self._get_sample_serialized_host()
        host2_serialized = self._get_sample_serialized_host()
        host2_serialized['id'] = 3
        host2_serialized['hostname'] = 'host2'

        self.expect_heartbeat(return_hosts=[host1_serialized])
        self.expect_heartbeat(return_hosts=[host1_serialized, host2_serialized])
        self.expect_heartbeat(known_host_ids=[host1_serialized['id'],
                                              host2_serialized['id']],
                              known_host_statuses=[host1_serialized['status'],
                                                   host2_serialized['status']])

        self.mox.ReplayAll()
        sut = shard_client.get_shard_client()

        original_process_heartbeat_response = sut.process_heartbeat_response
        def failing_process_heartbeat_response(*args, **kwargs):
            raise RuntimeError

        sut.process_heartbeat_response = failing_process_heartbeat_response
        self.assertRaises(RuntimeError, sut.do_heartbeat)

        self.assertEqual(models.Host.objects.count(), 0)

        sut.process_heartbeat_response = original_process_heartbeat_response
        sut.do_heartbeat()
        sut.do_heartbeat()

        self.mox.VerifyAll()


    def testHeartbeatNoShardMode(self):
        """Ensure an exception is thrown when run on a non-shard machine."""
        self.mox.ReplayAll()

        self.assertRaises(error.HeartbeatOnlyAllowedInShardModeException,
                          shard_client.get_shard_client)

        self.mox.VerifyAll()


    def testLoop(self):
        """Test looping over heartbeats and aborting that loop works."""
        self.setup_mocks()
        self.setup_global_config()

        global_config.global_config.override_config_value(
                'SHARD', 'heartbeat_pause_sec', '0.01')

        self.expect_heartbeat()

        sut = None

        def shutdown_sut(*args, **kwargs):
            sut.shutdown()

        self.expect_heartbeat(side_effect=shutdown_sut)

        self.mox.ReplayAll()
        sut = shard_client.get_shard_client()
        sut.loop()

        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()

#pylint: disable-msg=C0111
#!/usr/bin/python

import datetime
import common

from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models, rpc_interface, frontend_test_utils
from autotest_lib.frontend.afe import model_logic, model_attributes
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.client.common_lib.test_utils import unittest
from autotest_lib.server import frontend
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers

CLIENT = control_data.CONTROL_TYPE_NAMES.CLIENT
SERVER = control_data.CONTROL_TYPE_NAMES.SERVER

_hqe_status = models.HostQueueEntry.Status


class RpcInterfaceTest(unittest.TestCase,
                       frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()
        self.god = mock.mock_god()


    def tearDown(self):
        self.god.unstub_all()
        self._frontend_common_teardown()
        global_config.global_config.reset_config_values()


    def test_validation(self):
        # non-number for a numeric field
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.add_atomic_group, name='foo',
                          max_number_of_machines='bar')
        # omit a required field
        self.assertRaises(model_logic.ValidationError, rpc_interface.add_label,
                          name=None)
        # violate uniqueness constraint
        self.assertRaises(model_logic.ValidationError, rpc_interface.add_host,
                          hostname='host1')


    def test_multiple_platforms(self):
        platform2 = models.Label.objects.create(name='platform2', platform=True)
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface. label_add_hosts, id='platform2',
                          hosts=['host1', 'host2'])
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.host_add_labels,
                          id='host1', labels=['platform2'])
        # make sure the platform didn't get added
        platforms = rpc_interface.get_labels(
            host__hostname__in=['host1', 'host2'], platform=True)
        self.assertEquals(len(platforms), 1)
        self.assertEquals(platforms[0]['name'], 'myplatform')


    def _check_hostnames(self, hosts, expected_hostnames):
        self.assertEquals(set(host['hostname'] for host in hosts),
                          set(expected_hostnames))


    def test_get_hosts(self):
        hosts = rpc_interface.get_hosts()
        self._check_hostnames(hosts, [host.hostname for host in self.hosts])

        hosts = rpc_interface.get_hosts(hostname='host1')
        self._check_hostnames(hosts, ['host1'])
        host = hosts[0]
        self.assertEquals(sorted(host['labels']), ['label1', 'myplatform'])
        self.assertEquals(host['platform'], 'myplatform')
        self.assertEquals(host['atomic_group'], None)
        self.assertEquals(host['acls'], ['my_acl'])
        self.assertEquals(host['attributes'], {})


    def test_get_hosts_multiple_labels(self):
        hosts = rpc_interface.get_hosts(
                multiple_labels=['myplatform', 'label1'])
        self._check_hostnames(hosts, ['host1'])


    def test_get_hosts_exclude_only_if_needed(self):
        self.hosts[0].labels.add(self.label3)

        hosts = rpc_interface.get_hosts(hostname__in=['host1', 'host2'],
                                        exclude_only_if_needed_labels=True)
        self._check_hostnames(hosts, ['host2'])


    def test_get_hosts_exclude_atomic_group_hosts(self):
        hosts = rpc_interface.get_hosts(
                exclude_atomic_group_hosts=True,
                hostname__in=['host4', 'host5', 'host6'])
        self._check_hostnames(hosts, ['host4'])


    def test_get_hosts_exclude_both(self):
        self.hosts[0].labels.add(self.label3)

        hosts = rpc_interface.get_hosts(
                hostname__in=['host1', 'host2', 'host5'],
                exclude_only_if_needed_labels=True,
                exclude_atomic_group_hosts=True)
        self._check_hostnames(hosts, ['host2'])


    def test_job_keyvals(self):
        keyval_dict = {'mykey': 'myvalue'}
        job_id = rpc_interface.create_job(name='test', priority='Medium',
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'],
                                          keyvals=keyval_dict)
        jobs = rpc_interface.get_jobs(id=job_id)
        self.assertEquals(len(jobs), 1)
        self.assertEquals(jobs[0]['keyvals'], keyval_dict)


    def test_test_retry(self):
        job_id = rpc_interface.create_job(name='flake', priority='Medium',
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'],
                                          test_retry=10)
        jobs = rpc_interface.get_jobs(id=job_id)
        self.assertEquals(len(jobs), 1)
        self.assertEquals(jobs[0]['test_retry'], 10)


    def test_get_jobs_summary(self):
        job = self._create_job(hosts=xrange(1, 4))
        entries = list(job.hostqueueentry_set.all())
        entries[1].status = _hqe_status.FAILED
        entries[1].save()
        entries[2].status = _hqe_status.FAILED
        entries[2].aborted = True
        entries[2].save()

        # Mock up tko_rpc_interface.get_status_counts.
        self.god.stub_function_to_return(rpc_interface.tko_rpc_interface,
                                         'get_status_counts',
                                         None)

        job_summaries = rpc_interface.get_jobs_summary(id=job.id)
        self.assertEquals(len(job_summaries), 1)
        summary = job_summaries[0]
        self.assertEquals(summary['status_counts'], {'Queued': 1,
                                                     'Failed': 2})


    def _check_job_ids(self, actual_job_dicts, expected_jobs):
        self.assertEquals(
                set(job_dict['id'] for job_dict in actual_job_dicts),
                set(job.id for job in expected_jobs))


    def test_get_jobs_status_filters(self):
        HqeStatus = models.HostQueueEntry.Status
        def create_two_host_job():
            return self._create_job(hosts=[1, 2])
        def set_hqe_statuses(job, first_status, second_status):
            entries = job.hostqueueentry_set.all()
            entries[0].update_object(status=first_status)
            entries[1].update_object(status=second_status)

        queued = create_two_host_job()

        queued_and_running = create_two_host_job()
        set_hqe_statuses(queued_and_running, HqeStatus.QUEUED,
                           HqeStatus.RUNNING)

        running_and_complete = create_two_host_job()
        set_hqe_statuses(running_and_complete, HqeStatus.RUNNING,
                           HqeStatus.COMPLETED)

        complete = create_two_host_job()
        set_hqe_statuses(complete, HqeStatus.COMPLETED, HqeStatus.COMPLETED)

        started_but_inactive = create_two_host_job()
        set_hqe_statuses(started_but_inactive, HqeStatus.QUEUED,
                           HqeStatus.COMPLETED)

        parsing = create_two_host_job()
        set_hqe_statuses(parsing, HqeStatus.PARSING, HqeStatus.PARSING)

        self._check_job_ids(rpc_interface.get_jobs(not_yet_run=True), [queued])
        self._check_job_ids(rpc_interface.get_jobs(running=True),
                      [queued_and_running, running_and_complete,
                       started_but_inactive, parsing])
        self._check_job_ids(rpc_interface.get_jobs(finished=True), [complete])


    def test_get_jobs_type_filters(self):
        self.assertRaises(AssertionError, rpc_interface.get_jobs,
                          suite=True, sub=True)
        self.assertRaises(AssertionError, rpc_interface.get_jobs,
                          suite=True, standalone=True)
        self.assertRaises(AssertionError, rpc_interface.get_jobs,
                          standalone=True, sub=True)

        parent_job = self._create_job(hosts=[1])
        child_jobs = self._create_job(hosts=[1, 2],
                                      parent_job_id=parent_job.id)
        standalone_job = self._create_job(hosts=[1])

        self._check_job_ids(rpc_interface.get_jobs(suite=True), [parent_job])
        self._check_job_ids(rpc_interface.get_jobs(sub=True), [child_jobs])
        self._check_job_ids(rpc_interface.get_jobs(standalone=True),
                            [standalone_job])


    def _create_job_helper(self, **kwargs):
        return rpc_interface.create_job(name='test', priority='Medium',
                                        control_file='control file',
                                        control_type=SERVER, **kwargs)


    def test_one_time_hosts(self):
        job = self._create_job_helper(one_time_hosts=['testhost'])
        host = models.Host.objects.get(hostname='testhost')
        self.assertEquals(host.invalid, True)
        self.assertEquals(host.labels.count(), 0)
        self.assertEquals(host.aclgroup_set.count(), 0)


    def test_create_job_duplicate_hosts(self):
        self.assertRaises(model_logic.ValidationError, self._create_job_helper,
                          hosts=[1, 1])


    def test_create_unrunnable_metahost_job(self):
        self.assertRaises(error.NoEligibleHostException,
                          self._create_job_helper, meta_hosts=['unused'])


    def test_create_hostless_job(self):
        job_id = self._create_job_helper(hostless=True)
        job = models.Job.objects.get(pk=job_id)
        queue_entries = job.hostqueueentry_set.all()
        self.assertEquals(len(queue_entries), 1)
        self.assertEquals(queue_entries[0].host, None)
        self.assertEquals(queue_entries[0].meta_host, None)
        self.assertEquals(queue_entries[0].atomic_group, None)


    def _setup_special_tasks(self):
        host = self.hosts[0]

        job1 = self._create_job(hosts=[1])
        job2 = self._create_job(hosts=[1])

        entry1 = job1.hostqueueentry_set.all()[0]
        entry1.update_object(started_on=datetime.datetime(2009, 1, 2),
                             execution_subdir='host1')
        entry2 = job2.hostqueueentry_set.all()[0]
        entry2.update_object(started_on=datetime.datetime(2009, 1, 3),
                             execution_subdir='host1')

        self.task1 = models.SpecialTask.objects.create(
                host=host, task=models.SpecialTask.Task.VERIFY,
                time_started=datetime.datetime(2009, 1, 1), # ran before job 1
                is_complete=True, requested_by=models.User.current_user())
        self.task2 = models.SpecialTask.objects.create(
                host=host, task=models.SpecialTask.Task.VERIFY,
                queue_entry=entry2, # ran with job 2
                is_active=True, requested_by=models.User.current_user())
        self.task3 = models.SpecialTask.objects.create(
                host=host, task=models.SpecialTask.Task.VERIFY,
                requested_by=models.User.current_user()) # not yet run


    def test_get_special_tasks(self):
        self._setup_special_tasks()
        tasks = rpc_interface.get_special_tasks(host__hostname='host1',
                                                queue_entry__isnull=True)
        self.assertEquals(len(tasks), 2)
        self.assertEquals(tasks[0]['task'], models.SpecialTask.Task.VERIFY)
        self.assertEquals(tasks[0]['is_active'], False)
        self.assertEquals(tasks[0]['is_complete'], True)


    def test_get_latest_special_task(self):
        # a particular usage of get_special_tasks()
        self._setup_special_tasks()
        self.task2.time_started = datetime.datetime(2009, 1, 2)
        self.task2.save()

        tasks = rpc_interface.get_special_tasks(
                host__hostname='host1', task=models.SpecialTask.Task.VERIFY,
                time_started__isnull=False, sort_by=['-time_started'],
                query_limit=1)
        self.assertEquals(len(tasks), 1)
        self.assertEquals(tasks[0]['id'], 2)


    def _common_entry_check(self, entry_dict):
        self.assertEquals(entry_dict['host']['hostname'], 'host1')
        self.assertEquals(entry_dict['job']['id'], 2)


    def test_get_host_queue_entries_and_special_tasks(self):
        self._setup_special_tasks()

        host = self.hosts[0].id
        entries_and_tasks = (
                rpc_interface.get_host_queue_entries_and_special_tasks(host))

        paths = [entry['execution_path'] for entry in entries_and_tasks]
        self.assertEquals(paths, ['hosts/host1/3-verify',
                                  '2-autotest_system/host1',
                                  'hosts/host1/2-verify',
                                  '1-autotest_system/host1',
                                  'hosts/host1/1-verify'])

        verify2 = entries_and_tasks[2]
        self._common_entry_check(verify2)
        self.assertEquals(verify2['type'], 'Verify')
        self.assertEquals(verify2['status'], 'Running')
        self.assertEquals(verify2['execution_path'], 'hosts/host1/2-verify')

        entry2 = entries_and_tasks[1]
        self._common_entry_check(entry2)
        self.assertEquals(entry2['type'], 'Job')
        self.assertEquals(entry2['status'], 'Queued')
        self.assertEquals(entry2['started_on'], '2009-01-03 00:00:00')


    def test_view_invalid_host(self):
        # RPCs used by View Host page should work for invalid hosts
        self._create_job_helper(hosts=[1])
        host = self.hosts[0]
        host.delete()

        self.assertEquals(1, rpc_interface.get_num_hosts(hostname='host1',
                                                         valid_only=False))
        data = rpc_interface.get_hosts(hostname='host1', valid_only=False)
        self.assertEquals(1, len(data))

        self.assertEquals(1, rpc_interface.get_num_host_queue_entries(
                host__hostname='host1'))
        data = rpc_interface.get_host_queue_entries(host__hostname='host1')
        self.assertEquals(1, len(data))

        count = rpc_interface.get_num_host_queue_entries_and_special_tasks(
                host=host.id)
        self.assertEquals(1, count)
        data = rpc_interface.get_host_queue_entries_and_special_tasks(
                host=host.id)
        self.assertEquals(1, len(data))


    def test_reverify_hosts(self):
        hostname_list = rpc_interface.reverify_hosts(id__in=[1, 2])
        self.assertEquals(hostname_list, ['host1', 'host2'])
        tasks = rpc_interface.get_special_tasks()
        self.assertEquals(len(tasks), 2)
        self.assertEquals(set(task['host']['id'] for task in tasks),
                          set([1, 2]))

        task = tasks[0]
        self.assertEquals(task['task'], models.SpecialTask.Task.VERIFY)
        self.assertEquals(task['requested_by'], 'autotest_system')


    def test_repair_hosts(self):
        hostname_list = rpc_interface.repair_hosts(id__in=[1, 2])
        self.assertEquals(hostname_list, ['host1', 'host2'])
        tasks = rpc_interface.get_special_tasks()
        self.assertEquals(len(tasks), 2)
        self.assertEquals(set(task['host']['id'] for task in tasks),
                          set([1, 2]))

        task = tasks[0]
        self.assertEquals(task['task'], models.SpecialTask.Task.REPAIR)
        self.assertEquals(task['requested_by'], 'autotest_system')


    def test_parameterized_job(self):
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'parameterized_jobs', 'True')

        string_type = model_attributes.ParameterTypes.STRING

        test = models.Test.objects.create(
                name='test', test_type=control_data.CONTROL_TYPE.SERVER)
        test_parameter = test.testparameter_set.create(name='key')
        profiler = models.Profiler.objects.create(name='profiler')

        kernels = ({'version': 'version', 'cmdline': 'cmdline'},)
        profilers = ('profiler',)
        profiler_parameters = {'profiler': {'key': ('value', string_type)}}
        job_parameters = {'key': ('value', string_type)}

        job_id = rpc_interface.create_parameterized_job(
                name='job', priority=priorities.Priority.DEFAULT, test='test',
                parameters=job_parameters, kernel=kernels, label='label1',
                profilers=profilers, profiler_parameters=profiler_parameters,
                profile_only=False, hosts=('host1',))
        parameterized_job = models.Job.smart_get(job_id).parameterized_job

        self.assertEqual(parameterized_job.test, test)
        self.assertEqual(parameterized_job.label, self.labels[0])
        self.assertEqual(parameterized_job.kernels.count(), 1)
        self.assertEqual(parameterized_job.profilers.count(), 1)

        kernel = models.Kernel.objects.get(**kernels[0])
        self.assertEqual(parameterized_job.kernels.all()[0], kernel)
        self.assertEqual(parameterized_job.profilers.all()[0], profiler)

        parameterized_profiler = models.ParameterizedJobProfiler.objects.get(
                parameterized_job=parameterized_job, profiler=profiler)
        profiler_parameters_obj = (
                models.ParameterizedJobProfilerParameter.objects.get(
                parameterized_job_profiler=parameterized_profiler))
        self.assertEqual(profiler_parameters_obj.parameter_name, 'key')
        self.assertEqual(profiler_parameters_obj.parameter_value, 'value')
        self.assertEqual(profiler_parameters_obj.parameter_type, string_type)

        self.assertEqual(
                parameterized_job.parameterizedjobparameter_set.count(), 1)
        parameters_obj = (
                parameterized_job.parameterizedjobparameter_set.all()[0])
        self.assertEqual(parameters_obj.test_parameter, test_parameter)
        self.assertEqual(parameters_obj.parameter_value, 'value')
        self.assertEqual(parameters_obj.parameter_type, string_type)


    def _modify_host_helper(self, on_shard=False, host_on_shard=False):
        shard_hostname = 'shard1'
        if on_shard:
            global_config.global_config.override_config_value(
                'SHARD', 'shard_hostname', shard_hostname)

        host = models.Host.objects.all()[0]
        if host_on_shard:
            shard = models.Shard.objects.create(hostname=shard_hostname)
            host.shard = shard
            host.save()

        self.assertFalse(host.locked)

        self.god.stub_class_method(frontend.AFE, 'run')

        if host_on_shard and not on_shard:
            mock_afe = self.god.create_mock_class_obj(
                    frontend_wrappers.RetryingAFE, 'MockAFE')
            self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

            mock_afe2 = frontend_wrappers.RetryingAFE.expect_new(
                    server=shard_hostname, user=None)
            mock_afe2.run.expect_call('modify_host_local', id=host.id,
                    locked=True, lock_reason='_modify_host_helper lock',
                    lock_time=datetime.datetime(2015, 12, 15))
        elif on_shard:
            mock_afe = self.god.create_mock_class_obj(
                    frontend_wrappers.RetryingAFE, 'MockAFE')
            self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

            mock_afe2 = frontend_wrappers.RetryingAFE.expect_new(
                    server=server_utils.get_global_afe_hostname(), user=None)
            mock_afe2.run.expect_call('modify_host', id=host.id,
                    locked=True, lock_reason='_modify_host_helper lock',
                    lock_time=datetime.datetime(2015, 12, 15))

        rpc_interface.modify_host(id=host.id, locked=True,
                                  lock_reason='_modify_host_helper lock',
                                  lock_time=datetime.datetime(2015, 12, 15))

        host = models.Host.objects.get(pk=host.id)
        if on_shard:
            # modify_host on shard does nothing but routing the RPC to master.
            self.assertFalse(host.locked)
        else:
            self.assertTrue(host.locked)
        self.god.check_playback()


    def test_modify_host_on_master_host_on_master(self):
        """Call modify_host to master for host in master."""
        self._modify_host_helper()


    def test_modify_host_on_master_host_on_shard(self):
        """Call modify_host to master for host in shard."""
        self._modify_host_helper(host_on_shard=True)


    def test_modify_host_on_shard(self):
        """Call modify_host to shard for host in shard."""
        self._modify_host_helper(on_shard=True, host_on_shard=True)


    def test_modify_hosts_on_master_host_on_shard(self):
        """Ensure calls to modify_hosts are correctly forwarded to shards."""
        host1 = models.Host.objects.all()[0]
        host2 = models.Host.objects.all()[1]

        shard1 = models.Shard.objects.create(hostname='shard1')
        host1.shard = shard1
        host1.save()

        shard2 = models.Shard.objects.create(hostname='shard2')
        host2.shard = shard2
        host2.save()

        self.assertFalse(host1.locked)
        self.assertFalse(host2.locked)

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        # The statuses of one host might differ on master and shard.
        # Filters are always applied on the master. So the host on the shard
        # will be affected no matter what his status is.
        filters_to_use = {'status': 'Ready'}

        mock_afe2 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard2', user=None)
        mock_afe2.run.expect_call(
            'modify_hosts_local',
            host_filter_data={'id__in': [shard1.id, shard2.id]},
            update_data={'locked': True,
                         'lock_reason': 'Testing forward to shard',
                         'lock_time' : datetime.datetime(2015, 12, 15) })

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call(
            'modify_hosts_local',
            host_filter_data={'id__in': [shard1.id, shard2.id]},
            update_data={'locked': True,
                         'lock_reason': 'Testing forward to shard',
                         'lock_time' : datetime.datetime(2015, 12, 15)})

        rpc_interface.modify_hosts(
                host_filter_data={'status': 'Ready'},
                update_data={'locked': True,
                             'lock_reason': 'Testing forward to shard',
                             'lock_time' : datetime.datetime(2015, 12, 15) })

        host1 = models.Host.objects.get(pk=host1.id)
        self.assertTrue(host1.locked)
        host2 = models.Host.objects.get(pk=host2.id)
        self.assertTrue(host2.locked)
        self.god.check_playback()


    def test_delete_host(self):
        """Ensure an RPC is made on delete a host, if it is on a shard."""
        host1 = models.Host.objects.all()[0]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host1.shard = shard1
        host1.save()
        host1_id = host1.id

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                 'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call('delete_host', id=host1.id)

        rpc_interface.delete_host(id=host1.id)

        self.assertRaises(models.Host.DoesNotExist,
                          models.Host.smart_get, host1_id)

        self.god.check_playback()


    def test_modify_label(self):
        label1 = models.Label.objects.all()[0]
        self.assertEqual(label1.invalid, 0)

        host2 = models.Host.objects.all()[1]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host2.shard = shard1
        host2.labels.add(label1)
        host2.save()

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call('modify_label', id=label1.id, invalid=1)

        rpc_interface.modify_label(label1.id, invalid=1)

        self.assertEqual(models.Label.objects.all()[0].invalid, 1)
        self.god.check_playback()


    def test_delete_label(self):
        label1 = models.Label.objects.all()[0]

        host2 = models.Host.objects.all()[1]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host2.shard = shard1
        host2.labels.add(label1)
        host2.save()

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call('delete_label', id=label1.id)

        rpc_interface.delete_label(id=label1.id)

        self.assertRaises(models.Label.DoesNotExist,
                          models.Label.smart_get, label1.id)
        self.god.check_playback()


if __name__ == '__main__':
    unittest.main()

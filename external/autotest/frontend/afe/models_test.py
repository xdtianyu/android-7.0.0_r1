#!/usr/bin/python

import datetime
import unittest
import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import models, model_attributes, model_logic
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import control_data


class AclGroupTest(unittest.TestCase,
                   frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def _check_acls(self, host, acl_name_list):
        actual_acl_names = [acl_group.name for acl_group
                            in host.aclgroup_set.all()]
        self.assertEquals(set(actual_acl_names), set(acl_name_list))


    def test_on_host_membership_change(self):
        host1, host2 = self.hosts[1:3]
        everyone_acl = models.AclGroup.objects.get(name='Everyone')

        host1.aclgroup_set.clear()
        self._check_acls(host1, [])
        host2.aclgroup_set.add(everyone_acl)
        self._check_acls(host2, ['Everyone', 'my_acl'])

        models.AclGroup.on_host_membership_change()

        self._check_acls(host1, ['Everyone'])
        self._check_acls(host2, ['my_acl'])


class HostTest(unittest.TestCase,
               frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_add_host_previous_one_time_host(self):
        # ensure that when adding a host which was previously used as a one-time
        # host, the status isn't reset, since this can interfere with the
        # scheduler.
        host = models.Host.create_one_time_host('othost')
        self.assertEquals(host.invalid, True)
        self.assertEquals(host.status, models.Host.Status.READY)

        host.status = models.Host.Status.RUNNING
        host.save()

        host2 = models.Host.add_object(hostname='othost')
        self.assertEquals(host2.id, host.id)
        self.assertEquals(host2.status, models.Host.Status.RUNNING)


class SpecialTaskUnittest(unittest.TestCase,
                          frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def _create_task(self):
        return models.SpecialTask.objects.create(
                host=self.hosts[0], task=models.SpecialTask.Task.VERIFY,
                requested_by=models.User.current_user())


    def test_execution_path(self):
        task = self._create_task()
        self.assertEquals(task.execution_path(), 'hosts/host1/1-verify')


    def test_status(self):
        task = self._create_task()
        self.assertEquals(task.status, 'Queued')

        task.update_object(is_active=True)
        self.assertEquals(task.status, 'Running')

        task.update_object(is_active=False, is_complete=True, success=True)
        self.assertEquals(task.status, 'Completed')

        task.update_object(success=False)
        self.assertEquals(task.status, 'Failed')


    def test_activate(self):
        task = self._create_task()
        task.activate()
        self.assertTrue(task.is_active)
        self.assertFalse(task.is_complete)


    def test_finish(self):
        task = self._create_task()
        task.activate()
        task.finish(True)
        self.assertFalse(task.is_active)
        self.assertTrue(task.is_complete)
        self.assertTrue(task.success)


    def test_requested_by_from_queue_entry(self):
        job = self._create_job(hosts=[0])
        task = models.SpecialTask.objects.create(
                host=self.hosts[0], task=models.SpecialTask.Task.VERIFY,
                queue_entry=job.hostqueueentry_set.all()[0])
        self.assertEquals(task.requested_by.login, 'autotest_system')


class HostQueueEntryUnittest(unittest.TestCase,
                             frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_execution_path(self):
        entry = self._create_job(hosts=[1]).hostqueueentry_set.all()[0]
        entry.execution_subdir = 'subdir'
        entry.save()

        self.assertEquals(entry.execution_path(), '1-autotest_system/subdir')


class ModelWithInvalidTest(unittest.TestCase,
                           frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_model_with_invalid_delete(self):
        self.assertFalse(self.hosts[0].invalid)
        self.hosts[0].delete()
        self.assertTrue(self.hosts[0].invalid)
        self.assertTrue(models.Host.objects.get(id=self.hosts[0].id))


    def test_model_with_invalid_delete_queryset(self):
        for host in self.hosts:
            self.assertFalse(host.invalid)

        hosts = models.Host.objects.all()
        hosts.delete()
        self.assertEqual(hosts.count(), len(self.hosts))

        for host in hosts:
            self.assertTrue(host.invalid)


    def test_cloned_queryset_delete(self):
        """
        Make sure that a cloned queryset maintains the custom delete()
        """
        to_delete = ('host1', 'host2')

        for host in self.hosts:
            self.assertFalse(host.invalid)

        hosts = models.Host.objects.all().filter(hostname__in=to_delete)
        hosts.delete()
        all_hosts = models.Host.objects.all()
        self.assertEqual(all_hosts.count(), len(self.hosts))

        for host in all_hosts:
            if host.hostname in to_delete:
                self.assertTrue(
                        host.invalid,
                        '%s.invalid expected to be True' % host.hostname)
            else:
                self.assertFalse(
                        host.invalid,
                        '%s.invalid expected to be False' % host.hostname)


    def test_normal_delete(self):
        job = self._create_job(hosts=[1])
        self.assertEqual(1, models.Job.objects.all().count())

        job.delete()
        self.assertEqual(0, models.Job.objects.all().count())


    def test_normal_delete_queryset(self):
        self._create_job(hosts=[1])
        self._create_job(hosts=[2])

        self.assertEqual(2, models.Job.objects.all().count())

        models.Job.objects.all().delete()
        self.assertEqual(0, models.Job.objects.all().count())


class KernelTest(unittest.TestCase, frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_create_kernels_none(self):
        self.assertEqual(None, models.Kernel.create_kernels(None))


    def test_create_kernels(self):
        self.god.stub_function(models.Kernel, '_create')

        num_kernels = 3
        kernel_list = [object() for _ in range(num_kernels)]
        result = [object() for _ in range(num_kernels)]

        for kernel, response in zip(kernel_list, result):
            models.Kernel._create.expect_call(kernel).and_return(response)
        self.assertEqual(result, models.Kernel.create_kernels(kernel_list))
        self.god.check_playback()


    def test_create(self):
        kernel = models.Kernel._create({'version': 'version'})
        self.assertEqual(kernel.version, 'version')
        self.assertEqual(kernel.cmdline, '')
        self.assertEqual(kernel, models.Kernel._create({'version': 'version'}))


class ParameterizedJobTest(unittest.TestCase,
                           frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_job(self):
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'parameterized_jobs', 'True')

        test = models.Test.objects.create(
                name='name', author='author', test_class='class',
                test_category='category',
                test_type=control_data.CONTROL_TYPE.SERVER, path='path')
        parameterized_job = models.ParameterizedJob.objects.create(test=test)
        job = self._create_job(hosts=[1], control_file=None,
                               parameterized_job=parameterized_job)

        self.assertEqual(job, parameterized_job.job())


class JobTest(unittest.TestCase, frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def test_check_parameterized_jobs_no_args(self):
        self.assertRaises(Exception, models.Job.check_parameterized_job,
                          control_file=None, parameterized_job=None)


    def test_check_parameterized_jobs_both_args(self):
        self.assertRaises(Exception, models.Job.check_parameterized_job,
                          control_file=object(), parameterized_job=object())


    def test_check_parameterized_jobs_disabled(self):
        self.assertRaises(Exception, models.Job.check_parameterized_job,
                          control_file=None, parameterized_job=object())


    def test_check_parameterized_jobs_enabled(self):
        global_config.global_config.override_config_value(
                'AUTOTEST_WEB', 'parameterized_jobs', 'True')
        self.assertRaises(Exception, models.Job.check_parameterized_job,
                          control_file=object(), parameterized_job=None)


class SerializationTest(unittest.TestCase,
                        frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup(fill_data=False)


    def tearDown(self):
        self._frontend_common_teardown()


    def _get_example_response(self):
        return {'hosts': [{'aclgroup_set': [{'description': '',
                                             'id': 1,
                                             'name': 'Everyone',
                                             'users': [{
                                                 'access_level': 100,
                                                 'id': 1,
                                                 'login': 'autotest_system',
                                                 'reboot_after': 0,
                                                 'reboot_before': 1,
                                                 'show_experimental': False}]}],
                           'dirty': True,
                           'hostattribute_set': [],
                           'hostname': '100.107.2.163',
                           'id': 2,
                           'invalid': False,
                           'labels': [{'id': 7,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'power:battery',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 9,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'hw_video_acc_h264',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 10,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'hw_video_acc_enc_h264',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 11,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'webcam',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 12,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'touchpad',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 13,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'spring',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 14,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'board:daisy',
                                       'only_if_needed': False,
                                       'platform': True},
                                      {'id': 15,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'board_freq_mem:daisy_1.7GHz',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 16,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'bluetooth',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 17,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'gpu_family:mali',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 19,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'ec:cros',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 20,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'storage:mmc',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 21,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'hw_video_acc_vp8',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 22,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'video_glitch_detection',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 23,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'pool:suites',
                                       'only_if_needed': False,
                                       'platform': False},
                                      {'id': 25,
                                       'invalid': False,
                                       'kernel_config': '',
                                       'name': 'daisy-board-name',
                                       'only_if_needed': False,
                                       'platform': False}],
                           'leased': False,
                           'lock_reason': '',
                           'lock_time': None,
                           'locked': False,
                           'protection': 0,
                           'shard': {'hostname': '1', 'id': 1},
                           'status': 'Ready',
                           'synch_id': None}],
                'jobs': [{'control_file': 'some control file\n\n\n',
                          'control_type': 2,
                          'created_on': '2014-09-04T13:09:35',
                          'dependency_labels': [{'id': 14,
                                                 'invalid': False,
                                                 'kernel_config': '',
                                                 'name': 'board:daisy',
                                                 'only_if_needed': False,
                                                 'platform': True},
                                                {'id': 23,
                                                 'invalid': False,
                                                 'kernel_config': '',
                                                 'name': 'pool:suites',
                                                 'only_if_needed': False,
                                                 'platform': False},
                                                {'id': 25,
                                                 'invalid': False,
                                                 'kernel_config': '',
                                                 'name': 'daisy-board-name',
                                                 'only_if_needed': False,
                                                 'platform': False}],
                          'email_list': '',
                          'hostqueueentry_set': [{'aborted': False,
                                                  'active': False,
                                                  'complete': False,
                                                  'deleted': False,
                                                  'execution_subdir': '',
                                                  'finished_on': None,
                                                  'id': 5,
                                                  'meta_host': {
                                                      'id': 14,
                                                      'invalid': False,
                                                      'kernel_config': '',
                                                      'name': 'board:daisy',
                                                      'only_if_needed': False,
                                                      'platform': True},
                                                  'host_id': None,
                                                  'started_on': None,
                                                  'status': 'Queued'}],
                          'id': 5,
                          'jobkeyval_set': [{'id': 10,
                                             'job_id': 5,
                                             'key': 'suite',
                                             'value': 'dummy'},
                                            {'id': 11,
                                             'job_id': 5,
                                             'key': 'build',
                                             'value': 'daisy-release'},
                                            {'id': 12,
                                             'job_id': 5,
                                             'key': 'experimental',
                                             'value': 'False'}],
                          'max_runtime_hrs': 72,
                          'max_runtime_mins': 1440,
                          'name': 'daisy-experimental',
                          'owner': 'autotest',
                          'parse_failed_repair': True,
                          'priority': 40,
                          'reboot_after': 0,
                          'reboot_before': 1,
                          'run_reset': True,
                          'run_verify': False,
                          'shard': {'hostname': '1', 'id': 1},
                          'synch_count': 1,
                          'test_retry': 0,
                          'timeout': 24,
                          'timeout_mins': 1440,
                          'require_ssp': None},
                         {'control_file': 'some control file\n\n\n',
                          'control_type': 2,
                          'created_on': '2014-09-04T13:09:35',
                          'dependency_labels': [{'id': 14,
                                                 'invalid': False,
                                                 'kernel_config': '',
                                                 'name': 'board:daisy',
                                                 'only_if_needed': False,
                                                 'platform': True},
                                                {'id': 23,
                                                 'invalid': False,
                                                 'kernel_config': '',
                                                 'name': 'pool:suites',
                                                 'only_if_needed': False,
                                                 'platform': False},
                                                {'id': 25,
                                                 'invalid': False,
                                                 'kernel_config': '',
                                                 'name': 'daisy-board-name',
                                                 'only_if_needed': False,
                                                 'platform': False}],
                          'email_list': '',
                          'hostqueueentry_set': [{'aborted': False,
                                                  'active': False,
                                                  'complete': False,
                                                  'deleted': False,
                                                  'execution_subdir': '',
                                                  'finished_on': None,
                                                  'id': 7,
                                                  'meta_host': {
                                                      'id': 14,
                                                      'invalid': False,
                                                      'kernel_config': '',
                                                      'name': 'board:daisy',
                                                      'only_if_needed': False,
                                                      'platform': True},
                                                  'host_id': None,
                                                  'started_on': None,
                                                  'status': 'Queued'}],
                          'id': 7,
                          'jobkeyval_set': [{'id': 16,
                                             'job_id': 7,
                                             'key': 'suite',
                                             'value': 'dummy'},
                                            {'id': 17,
                                             'job_id': 7,
                                             'key': 'build',
                                             'value': 'daisy-release'},
                                            {'id': 18,
                                             'job_id': 7,
                                             'key': 'experimental',
                                             'value': 'False'}],
                          'max_runtime_hrs': 72,
                          'max_runtime_mins': 1440,
                          'name': 'daisy-experimental',
                          'owner': 'autotest',
                          'parse_failed_repair': True,
                          'priority': 40,
                          'reboot_after': 0,
                          'reboot_before': 1,
                          'run_reset': True,
                          'run_verify': False,
                          'shard': {'hostname': '1', 'id': 1},
                          'synch_count': 1,
                          'test_retry': 0,
                          'timeout': 24,
                          'timeout_mins': 1440,
                          'require_ssp': None}]}


    def test_response(self):
        heartbeat_response = self._get_example_response()
        hosts_serialized = heartbeat_response['hosts']
        jobs_serialized = heartbeat_response['jobs']

        # Persisting is automatically done inside deserialize
        hosts = [models.Host.deserialize(host) for host in hosts_serialized]
        jobs = [models.Job.deserialize(job) for job in jobs_serialized]

        generated_heartbeat_response = {
            'hosts': [host.serialize() for host in hosts],
            'jobs': [job.serialize() for job in jobs]
        }
        example_response = self._get_example_response()
        # For attribute-like objects, we don't care about its id.
        for r in [generated_heartbeat_response, example_response]:
            for job in r['jobs']:
                for keyval in job['jobkeyval_set']:
                    keyval.pop('id')
            for host in r['hosts']:
                for attribute in host['hostattribute_set']:
                    keyval.pop('id')
        self.assertEqual(generated_heartbeat_response, example_response)


    def test_update(self):
        job = self._create_job(hosts=[1])
        serialized = job.serialize(include_dependencies=False)
        serialized['owner'] = 'some_other_owner'

        job.update_from_serialized(serialized)
        self.assertEqual(job.owner, 'some_other_owner')

        serialized = job.serialize()
        self.assertRaises(
            ValueError,
            job.update_from_serialized, serialized)


    def test_sync_aborted(self):
        job = self._create_job(hosts=[1])
        serialized = job.serialize()

        serialized['hostqueueentry_set'][0]['aborted'] = True
        serialized['hostqueueentry_set'][0]['status'] = 'Running'

        models.Job.deserialize(serialized)

        job = models.Job.objects.get(pk=job.id)
        self.assertTrue(job.hostqueueentry_set.all()[0].aborted)
        self.assertEqual(job.hostqueueentry_set.all()[0].status, 'Queued')


if __name__ == '__main__':
    unittest.main()

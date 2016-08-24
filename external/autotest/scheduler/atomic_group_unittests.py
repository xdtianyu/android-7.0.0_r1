#!/usr/bin/python
#pylint: disable-msg=C0111

import gc, time
import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.client.common_lib.test_utils import unittest
from autotest_lib.database import database_connection
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import agent_task
from autotest_lib.scheduler import monitor_db, drone_manager, email_manager
from autotest_lib.scheduler import pidfile_monitor
from autotest_lib.scheduler import scheduler_config, gc_stats, host_scheduler
from autotest_lib.scheduler import monitor_db_functional_test
from autotest_lib.scheduler import monitor_db_unittest
from autotest_lib.scheduler import scheduler_models

_DEBUG = False


class AtomicGroupTest(monitor_db_unittest.DispatcherSchedulingTest):

    def test_atomic_group_hosts_blocked_from_non_atomic_jobs(self):
        # Create a job scheduled to run on label6.
        self._create_job(metahosts=[self.label6.id])
        self._run_scheduler()
        # label6 only has hosts that are in atomic groups associated with it,
        # there should be no scheduling.
        self._check_for_extra_schedulings()


    def test_atomic_group_hosts_blocked_from_non_atomic_jobs_explicit(self):
        # Create a job scheduled to run on label5.  This is an atomic group
        # label but this job does not request atomic group scheduling.
        self._create_job(metahosts=[self.label5.id])
        self._run_scheduler()
        # label6 only has hosts that are in atomic groups associated with it,
        # there should be no scheduling.
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_basics(self):
        # Create jobs scheduled to run on an atomic group.
        job_a = self._create_job(synchronous=True, metahosts=[self.label4.id],
                         atomic_group=1)
        job_b = self._create_job(synchronous=True, metahosts=[self.label5.id],
                         atomic_group=1)
        self._run_scheduler()
        # atomic_group.max_number_of_machines was 2 so we should run on 2.
        self._assert_job_scheduled_on_number_of(job_a.id, (5, 6, 7), 2)
        self._assert_job_scheduled_on(job_b.id, 8)  # label5
        self._assert_job_scheduled_on(job_b.id, 9)  # label5
        self._check_for_extra_schedulings()

        # The three host label4 atomic group still has one host available.
        # That means a job with a synch_count of 1 asking to be scheduled on
        # the atomic group can still use the final machine.
        #
        # This may seem like a somewhat odd use case.  It allows the use of an
        # atomic group as a set of machines to run smaller jobs within (a set
        # of hosts configured for use in network tests with eachother perhaps?)
        onehost_job = self._create_job(atomic_group=1)
        self._run_scheduler()
        self._assert_job_scheduled_on_number_of(onehost_job.id, (5, 6, 7), 1)
        self._check_for_extra_schedulings()

        # No more atomic groups have hosts available, no more jobs should
        # be scheduled.
        self._create_job(atomic_group=1)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_obeys_acls(self):
        # Request scheduling on a specific atomic label but be denied by ACLs.
        self._do_query('DELETE FROM afe_acl_groups_hosts '
                       'WHERE host_id in (8,9)')
        job = self._create_job(metahosts=[self.label5.id], atomic_group=1)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_dependency_label_exclude(self):
        # A dependency label that matches no hosts in the atomic group.
        job_a = self._create_job(atomic_group=1)
        job_a.dependency_labels.add(self.label3)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_metahost_dependency_label_exclude(self):
        # A metahost and dependency label that excludes too many hosts.
        job_b = self._create_job(synchronous=True, metahosts=[self.label4.id],
                                 atomic_group=1)
        job_b.dependency_labels.add(self.label7)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_dependency_label_match(self):
        # A dependency label that exists on enough atomic group hosts in only
        # one of the two atomic group labels.
        job_c = self._create_job(synchronous=True, atomic_group=1)
        job_c.dependency_labels.add(self.label7)
        self._run_scheduler()
        self._assert_job_scheduled_on_number_of(job_c.id, (8, 9), 2)
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_no_metahost(self):
        # Force it to schedule on the other group for a reliable test.
        self._do_query('UPDATE afe_hosts SET invalid=1 WHERE id=9')
        # An atomic job without a metahost.
        job = self._create_job(synchronous=True, atomic_group=1)
        self._run_scheduler()
        self._assert_job_scheduled_on_number_of(job.id, (5, 6, 7), 2)
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_partial_group(self):
        # Make one host in labels[3] unavailable so that there are only two
        # hosts left in the group.
        self._do_query('UPDATE afe_hosts SET status="Repair Failed" WHERE id=5')
        job = self._create_job(synchronous=True, metahosts=[self.label4.id],
                         atomic_group=1)
        self._run_scheduler()
        # Verify that it was scheduled on the 2 ready hosts in that group.
        self._assert_job_scheduled_on(job.id, 6)
        self._assert_job_scheduled_on(job.id, 7)
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_not_enough_available(self):
        # Mark some hosts in each atomic group label as not usable.
        # One host running, another invalid in the first group label.
        self._do_query('UPDATE afe_hosts SET status="Running" WHERE id=5')
        self._do_query('UPDATE afe_hosts SET invalid=1 WHERE id=6')
        # One host invalid in the second group label.
        self._do_query('UPDATE afe_hosts SET invalid=1 WHERE id=9')
        # Nothing to schedule when no group label has enough (2) good hosts..
        self._create_job(atomic_group=1, synchronous=True)
        self._run_scheduler()
        # There are not enough hosts in either atomic group,
        # No more scheduling should occur.
        self._check_for_extra_schedulings()

        # Now create an atomic job that has a synch count of 1.  It should
        # schedule on exactly one of the hosts.
        onehost_job = self._create_job(atomic_group=1)
        self._run_scheduler()
        self._assert_job_scheduled_on_number_of(onehost_job.id, (7, 8), 1)


    def test_atomic_group_scheduling_no_valid_hosts(self):
        self._do_query('UPDATE afe_hosts SET invalid=1 WHERE id in (8,9)')
        self._create_job(synchronous=True, metahosts=[self.label5.id],
                         atomic_group=1)
        self._run_scheduler()
        # no hosts in the selected group and label are valid.  no schedulings.
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_metahost_works(self):
        # Test that atomic group scheduling also obeys metahosts.
        self._create_job(metahosts=[0], atomic_group=1)
        self._run_scheduler()
        # There are no atomic group hosts that also have that metahost.
        self._check_for_extra_schedulings()

        job_b = self._create_job(metahosts=[self.label5.id], atomic_group=1)
        self._run_scheduler()
        self._assert_job_scheduled_on(job_b.id, 8)
        self._assert_job_scheduled_on(job_b.id, 9)
        self._check_for_extra_schedulings()


    def test_atomic_group_skips_ineligible_hosts(self):
        # Test hosts marked ineligible for this job are not eligible.
        # How would this ever happen anyways?
        job = self._create_job(metahosts=[self.label4.id], atomic_group=1)
        models.IneligibleHostQueue.objects.create(job=job, host_id=5)
        models.IneligibleHostQueue.objects.create(job=job, host_id=6)
        models.IneligibleHostQueue.objects.create(job=job, host_id=7)
        self._run_scheduler()
        # No scheduling should occur as all desired hosts were ineligible.
        self._check_for_extra_schedulings()


    def test_atomic_group_scheduling_fail(self):
        # If synch_count is > the atomic group number of machines, the job
        # should be aborted immediately.
        model_job = self._create_job(synchronous=True, atomic_group=1)
        model_job.synch_count = 4
        model_job.save()
        job = scheduler_models.Job(id=model_job.id)
        self._run_scheduler()
        self._check_for_extra_schedulings()
        queue_entries = job.get_host_queue_entries()
        self.assertEqual(1, len(queue_entries))
        self.assertEqual(queue_entries[0].status,
                         models.HostQueueEntry.Status.ABORTED)


    def test_atomic_group_no_labels_no_scheduling(self):
        # Never schedule on atomic groups marked invalid.
        job = self._create_job(metahosts=[self.label5.id], synchronous=True,
                               atomic_group=1)
        # Deleting an atomic group via the frontend marks it invalid and
        # removes all label references to the group.  The job now references
        # an invalid atomic group with no labels associated with it.
        self.label5.atomic_group.invalid = True
        self.label5.atomic_group.save()
        self.label5.atomic_group = None
        self.label5.save()

        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_schedule_directly_on_atomic_group_host_fail(self):
        # Scheduling a job directly on hosts in an atomic group must
        # fail to avoid users inadvertently holding up the use of an
        # entire atomic group by using the machines individually.
        job = self._create_job(hosts=[5])
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_schedule_directly_on_atomic_group_host(self):
        # Scheduling a job directly on one host in an atomic group will
        # work when the atomic group is listed on the HQE in addition
        # to the host (assuming the sync count is 1).
        job = self._create_job(hosts=[5], atomic_group=1)
        self._run_scheduler()
        self._assert_job_scheduled_on(job.id, 5)
        self._check_for_extra_schedulings()


    def test_schedule_directly_on_atomic_group_hosts_sync2(self):
        job = self._create_job(hosts=[5,8], atomic_group=1, synchronous=True)
        self._run_scheduler()
        self._assert_job_scheduled_on(job.id, 5)
        self._assert_job_scheduled_on(job.id, 8)
        self._check_for_extra_schedulings()


    def test_schedule_directly_on_atomic_group_hosts_wrong_group(self):
        job = self._create_job(hosts=[5,8], atomic_group=2, synchronous=True)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    # TODO(gps): These should probably live in their own TestCase class
    # specific to testing HostScheduler methods directly.  It was convenient
    # to put it here for now to share existing test environment setup code.
    def test_HostScheduler_check_atomic_group_labels(self):
        normal_job = self._create_job(metahosts=[0])
        atomic_job = self._create_job(atomic_group=1)
        # Indirectly initialize the internal state of the host scheduler.
        self._dispatcher._refresh_pending_queue_entries()

        atomic_hqe = scheduler_models.HostQueueEntry.fetch(where='job_id=%d' %
                                                     atomic_job.id)[0]
        normal_hqe = scheduler_models.HostQueueEntry.fetch(where='job_id=%d' %
                                                     normal_job.id)[0]

        host_scheduler = self._dispatcher._host_scheduler
        self.assertTrue(host_scheduler._check_atomic_group_labels(
                [self.label4.id], atomic_hqe))
        self.assertFalse(host_scheduler._check_atomic_group_labels(
                [self.label4.id], normal_hqe))
        self.assertFalse(host_scheduler._check_atomic_group_labels(
                [self.label5.id, self.label6.id, self.label7.id], normal_hqe))
        self.assertTrue(host_scheduler._check_atomic_group_labels(
                [self.label4.id, self.label6.id], atomic_hqe))
        self.assertTrue(host_scheduler._check_atomic_group_labels(
                        [self.label4.id, self.label5.id],
                        atomic_hqe))


class OnlyIfNeededTest(monitor_db_unittest.DispatcherSchedulingTest):

    def _setup_test_only_if_needed_labels(self):
        # apply only_if_needed label3 to host1
        models.Host.smart_get('host1').labels.add(self.label3)
        return self._create_job_simple([1], use_metahost=True)


    def test_only_if_needed_labels_avoids_host(self):
        job = self._setup_test_only_if_needed_labels()
        # if the job doesn't depend on label3, there should be no scheduling
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_only_if_needed_labels_schedules(self):
        job = self._setup_test_only_if_needed_labels()
        job.dependency_labels.add(self.label3)
        self._run_scheduler()
        self._assert_job_scheduled_on(1, 1)
        self._check_for_extra_schedulings()


    def test_only_if_needed_labels_via_metahost(self):
        job = self._setup_test_only_if_needed_labels()
        job.dependency_labels.add(self.label3)
        # should also work if the metahost is the only_if_needed label
        self._do_query('DELETE FROM afe_jobs_dependency_labels')
        self._create_job(metahosts=[3])
        self._run_scheduler()
        self._assert_job_scheduled_on(2, 1)
        self._check_for_extra_schedulings()


    def test_metahosts_obey_blocks(self):
        """
        Metahosts can't get scheduled on hosts already scheduled for
        that job.
        """
        self._create_job(metahosts=[1], hosts=[1])
        # make the nonmetahost entry complete, so the metahost can try
        # to get scheduled
        self._update_hqe(set='complete = 1', where='host_id=1')
        self._run_scheduler()
        self._check_for_extra_schedulings()



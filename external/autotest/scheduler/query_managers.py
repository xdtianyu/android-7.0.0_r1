#pylint: disable-msg=C0111

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Scheduler library classes.
"""

import collections
import logging

import common

from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.scheduler import scheduler_models
from autotest_lib.scheduler import scheduler_lib


_job_timer = autotest_stats.Timer('scheduler.job_query_manager')
class AFEJobQueryManager(object):
    """Query manager for AFE Jobs."""

    # A subquery to only get inactive hostless jobs.
    hostless_query = 'host_id IS NULL AND meta_host IS NULL'


    @_job_timer.decorate
    def get_pending_queue_entries(self, only_hostless=False):
        """
        Fetch a list of new host queue entries.

        The ordering of this list is important, as every new agent
        we schedule can potentially contribute to the process count
        on the drone, which has a static limit. The sort order
        prioritizes jobs as follows:
        1. High priority jobs: Based on the afe_job's priority
        2. With hosts and metahosts: This will only happen if we don't
            activate the hqe after assigning a host to it in
            schedule_new_jobs.
        3. With hosts but without metahosts: When tests are scheduled
            through the frontend the owner of the job would have chosen
            a host for it.
        4. Without hosts but with metahosts: This is the common case of
            a new test that needs a DUT. We assign a host and set it to
            active so it shouldn't show up in case 2 on the next tick.
        5. Without hosts and without metahosts: Hostless suite jobs, that
            will result in new jobs that fall under category 4.

        A note about the ordering of cases 3 and 4:
        Prioritizing one case above the other leads to earlier acquisition
        of the following resources: 1. process slots on the drone 2. machines.
        - When a user schedules a job through the afe they choose a specific
          host for it. Jobs with metahost can utilize any host that satisfies
          the metahost criterion. This means that if we had scheduled 4 before
          3 there is a good chance that a job which could've used another host,
          will now use the host assigned to a metahost-less job. Given the
          availability of machines in pool:suites, this almost guarantees
          starvation for jobs scheduled through the frontend.
        - Scheduling 4 before 3 also has its pros however, since a suite
          has the concept of a time out, whereas users can wait. If we hit the
          process count on the drone a suite can timeout waiting on the test,
          but a user job generally has a much longer timeout, and relatively
          harmless consequences.
        The current ordering was chosed because it is more likely that we will
        run out of machines in pool:suites than processes on the drone.

        @returns A list of HQEs ordered according to sort_order.
        """
        sort_order = ('afe_jobs.priority DESC, '
                      'ISNULL(host_id), '
                      'ISNULL(meta_host), '
                      'parent_job_id, '
                      'job_id')
        # Don't execute jobs that should be executed by a shard in the global
        # scheduler.
        # This won't prevent the shard scheduler to run this, as the shard db
        # doesn't have an an entry in afe_shards_labels.
        query=('NOT complete AND NOT active AND status="Queued"'
               'AND NOT aborted AND afe_shards_labels.id IS NULL')

        # TODO(jakobjuelich, beeps): Optimize this query. Details:
        # Compressed output of EXPLAIN <query>:
        # +------------------------+--------+-------------------------+-------+
        # | table                  | type   | key                     | rows  |
        # +------------------------+--------+-------------------------+-------+
        # | afe_host_queue_entries | ref    | host_queue_entry_status | 30536 |
        # | afe_shards_labels      | ref    | shard_label_id_fk       |     1 |
        # | afe_jobs               | eq_ref | PRIMARY                 |     1 |
        # +------------------------+--------+-------------------------+-------+
        # This shows the first part of the query fetches a lot of objects, that
        # are then filtered. The joins are comparably fast: There's usually just
        # one or none shard mapping that can be answered fully using an index
        # (shard_label_id_fk), similar thing applies to the job.
        #
        # This works for now, but once O(#Jobs in shard) << O(#Jobs in Queued),
        # it might be more efficient to filter on the meta_host first, instead
        # of the status.
        if only_hostless:
            query = '%s AND (%s)' % (query, self.hostless_query)
        return list(scheduler_models.HostQueueEntry.fetch(
            joins=('INNER JOIN afe_jobs ON (job_id=afe_jobs.id) '
                   'LEFT JOIN afe_shards_labels ON ('
                   'meta_host=afe_shards_labels.label_id)'),
            where=query, order_by=sort_order))


    @_job_timer.decorate
    def get_prioritized_special_tasks(self, only_tasks_with_leased_hosts=False):
        """
        Returns all queued SpecialTasks prioritized for repair first, then
        cleanup, then verify.

        @param only_tasks_with_leased_hosts: If true, this method only returns
            tasks with leased hosts.

        @return: list of afe.models.SpecialTasks sorted according to priority.
        """
        queued_tasks = models.SpecialTask.objects.filter(is_active=False,
                                                         is_complete=False,
                                                         host__locked=False)
        # exclude hosts with active queue entries unless the SpecialTask is for
        # that queue entry
        queued_tasks = models.SpecialTask.objects.add_join(
                queued_tasks, 'afe_host_queue_entries', 'host_id',
                join_condition='afe_host_queue_entries.active',
                join_from_key='host_id', force_left_join=True)
        queued_tasks = queued_tasks.extra(
                where=['(afe_host_queue_entries.id IS NULL OR '
                       'afe_host_queue_entries.id = '
                               'afe_special_tasks.queue_entry_id)'])
        if only_tasks_with_leased_hosts:
            queued_tasks = queued_tasks.filter(host__leased=True)

        # reorder tasks by priority
        task_priority_order = [models.SpecialTask.Task.REPAIR,
                               models.SpecialTask.Task.CLEANUP,
                               models.SpecialTask.Task.VERIFY,
                               models.SpecialTask.Task.RESET,
                               models.SpecialTask.Task.PROVISION]
        def task_priority_key(task):
            return task_priority_order.index(task.task)
        return sorted(queued_tasks, key=task_priority_key)


    @classmethod
    def get_overlapping_jobs(cls):
        """A helper method to get all active jobs using the same host.

        @return: A list of dictionaries with the hqe id, job_id and host_id
            of the currently overlapping jobs.
        """
        # Filter all active hqes and stand alone special tasks to make sure
        # a host isn't being used by two jobs at the same time. An incomplete
        # stand alone special task can share a host with an active hqe, an
        # example of this is the cleanup scheduled in gathering.
        hqe_hosts = list(models.HostQueueEntry.objects.filter(
                active=1, complete=0, host_id__isnull=False).values_list(
                'host_id', flat=True))
        special_task_hosts = list(models.SpecialTask.objects.filter(
                is_active=1, is_complete=0, host_id__isnull=False,
                queue_entry_id__isnull=True).values_list('host_id', flat=True))
        host_counts = collections.Counter(
                hqe_hosts + special_task_hosts).most_common()
        multiple_hosts = [count[0] for count in host_counts if count[1] > 1]
        return list(models.HostQueueEntry.objects.filter(
                host_id__in=multiple_hosts, active=True).values(
                        'id', 'job_id', 'host_id'))


    @_job_timer.decorate
    def get_suite_host_assignment(self):
        """A helper method to get how many hosts each suite is holding.

        @return: Two dictionaries (suite_host_num, hosts_to_suites)
                 suite_host_num maps suite job id to number of hosts
                 holding by its child jobs.
                 hosts_to_suites contains current hosts held by
                 any suites, and maps the host id to its parent_job_id.
        """
        query = models.HostQueueEntry.objects.filter(
                host_id__isnull=False, complete=0, active=1,
                job__parent_job_id__isnull=False)
        suite_host_num = {}
        hosts_to_suites = {}
        for hqe in query:
            host_id = hqe.host_id
            parent_job_id = hqe.job.parent_job_id
            count = suite_host_num.get(parent_job_id, 0)
            suite_host_num[parent_job_id] = count + 1
            hosts_to_suites[host_id] = parent_job_id
        return suite_host_num, hosts_to_suites


    @_job_timer.decorate
    def get_min_duts_of_suites(self, suite_job_ids):
        """Load suite_min_duts job keyval for a set of suites.

        @param suite_job_ids: A set of suite job ids.

        @return: A dictionary where the key is a suite job id,
                 the value is the value of 'suite_min_duts'.
        """
        query = models.JobKeyval.objects.filter(
                job_id__in=suite_job_ids,
                key=constants.SUITE_MIN_DUTS_KEY, value__isnull=False)
        return dict((keyval.job_id, int(keyval.value)) for keyval in query)


_host_timer = autotest_stats.Timer('scheduler.host_query_manager')
class AFEHostQueryManager(object):
    """Query manager for AFE Hosts."""

    def __init__(self):
        """Create an AFEHostQueryManager.

        @param db: A connection to the database with the afe_hosts table.
        """
        self._db = scheduler_lib.ConnectionManager().get_connection()


    def _process_many2many_dict(self, rows, flip=False):
        result = {}
        for row in rows:
            left_id, right_id = int(row[0]), int(row[1])
            if flip:
                left_id, right_id = right_id, left_id
            result.setdefault(left_id, set()).add(right_id)
        return result


    def _get_sql_id_list(self, id_list):
        return ','.join(str(item_id) for item_id in id_list)


    def _get_many2many_dict(self, query, id_list, flip=False):
        if not id_list:
            return {}
        query %= self._get_sql_id_list(id_list)
        rows = self._db.execute(query)
        return self._process_many2many_dict(rows, flip)


    @_host_timer.decorate
    def _get_ready_hosts(self):
        # We don't lose anything by re-doing these checks
        # even though we release hosts on the same conditions.
        # In the future we might have multiple clients that
        # release_hosts and/or lock them independent of the
        # scheduler tick.
        hosts = scheduler_models.Host.fetch(
            where="NOT afe_hosts.leased "
                  "AND NOT afe_hosts.locked "
                  "AND (afe_hosts.status IS NULL "
                      "OR afe_hosts.status = 'Ready')")
        return dict((host.id, host) for host in hosts)


    @_host_timer.decorate
    def _get_job_acl_groups(self, job_ids):
        query = """
        SELECT afe_jobs.id, afe_acl_groups_users.aclgroup_id
        FROM afe_jobs
        INNER JOIN afe_users ON afe_users.login = afe_jobs.owner
        INNER JOIN afe_acl_groups_users ON
                afe_acl_groups_users.user_id = afe_users.id
        WHERE afe_jobs.id IN (%s)
        """
        return self._get_many2many_dict(query, job_ids)


    @_host_timer.decorate
    def _get_job_ineligible_hosts(self, job_ids):
        query = """
        SELECT job_id, host_id
        FROM afe_ineligible_host_queues
        WHERE job_id IN (%s)
        """
        return self._get_many2many_dict(query, job_ids)


    @_host_timer.decorate
    def _get_job_dependencies(self, job_ids):
        query = """
        SELECT job_id, label_id
        FROM afe_jobs_dependency_labels
        WHERE job_id IN (%s)
        """
        return self._get_many2many_dict(query, job_ids)

    @_host_timer.decorate
    def _get_host_acls(self, host_ids):
        query = """
        SELECT host_id, aclgroup_id
        FROM afe_acl_groups_hosts
        WHERE host_id IN (%s)
        """
        return self._get_many2many_dict(query, host_ids)


    @_host_timer.decorate
    def _get_label_hosts(self, host_ids):
        if not host_ids:
            return {}, {}
        query = """
        SELECT label_id, host_id
        FROM afe_hosts_labels
        WHERE host_id IN (%s)
        """ % self._get_sql_id_list(host_ids)
        rows = self._db.execute(query)
        labels_to_hosts = self._process_many2many_dict(rows)
        hosts_to_labels = self._process_many2many_dict(rows, flip=True)
        return labels_to_hosts, hosts_to_labels


    @classmethod
    def find_unused_healty_hosts(cls):
        """Get hosts that are currently unused and in the READY state.

        @return: A list of host objects, one for each unused healthy host.
        """
        # Avoid any host with a currently active queue entry against it.
        hqe_join = ('LEFT JOIN afe_host_queue_entries AS active_hqe '
                    'ON (afe_hosts.id = active_hqe.host_id AND '
                    'active_hqe.active)')

        # Avoid any host with a new special task against it. There are 2 cases
        # when an inactive but incomplete special task will not use the host
        # this tick: 1. When the host is locked 2. When an active hqe already
        # has special tasks for the same host. In both these cases this host
        # will not be in the ready hosts list anyway. In all other cases,
        # an incomplete special task will grab the host before a new job does
        # by assigning an agent to it.
        special_task_join = ('LEFT JOIN afe_special_tasks as new_tasks '
                             'ON (afe_hosts.id = new_tasks.host_id AND '
                             'new_tasks.is_complete=0)')

        return scheduler_models.Host.fetch(
            joins='%s %s' % (hqe_join, special_task_join),
            where="active_hqe.host_id IS NULL AND new_tasks.host_id IS NULL "
                  "AND afe_hosts.leased "
                  "AND NOT afe_hosts.locked "
                  "AND (afe_hosts.status IS NULL "
                          "OR afe_hosts.status = 'Ready')")


    @_host_timer.decorate
    def set_leased(self, leased_value, **kwargs):
        """Modify the leased bit on the hosts with ids in host_ids.

        @param leased_value: The True/False value of the leased column for
            the hosts with ids in host_ids.
        @param kwargs: The args to use in finding matching hosts.
        """
        logging.info('Setting leased = %s for the hosts that match %s',
                     leased_value, kwargs)
        models.Host.objects.filter(**kwargs).update(leased=leased_value)


    @_host_timer.decorate
    def _get_labels(self, job_dependencies):
        """
        Calculate a dict mapping label id to label object so that we don't
        frequently round trip to the database every time we need a label.

        @param job_dependencies: A dict mapping an integer job id to a list of
            integer label id's.  ie. {job_id: [label_id]}
        @return: A dict mapping an integer label id to a scheduler model label
            object.  ie. {label_id: label_object}

        """
        id_to_label = dict()
        # Pull all the labels on hosts we might look at
        host_labels = scheduler_models.Label.fetch(
                where="id IN (SELECT label_id FROM afe_hosts_labels)")
        id_to_label.update([(label.id, label) for label in host_labels])
        # and pull all the labels on jobs we might look at.
        job_label_set = set()
        for job_deps in job_dependencies.values():
            job_label_set.update(job_deps)
        # On the rare/impossible chance that no jobs have any labels, we
        # can skip this.
        if job_label_set:
            job_string_label_list = ','.join([str(x) for x in job_label_set])
            job_labels = scheduler_models.Label.fetch(
                    where="id IN (%s)" % job_string_label_list)
            id_to_label.update([(label.id, label) for label in job_labels])
        return id_to_label


    @_host_timer.decorate
    def refresh(self, pending_queue_entries):
        """Update the query manager.

        Cache information about a list of queue entries and eligible hosts
        from the database so clients can avoid expensive round trips during
        host acquisition.

        @param pending_queue_entries: A list of queue entries about which we
            need information.
        """
        self._hosts_available = self._get_ready_hosts()
        relevant_jobs = [queue_entry.job_id
                         for queue_entry in pending_queue_entries]
        self._job_acls = self._get_job_acl_groups(relevant_jobs)
        self._ineligible_hosts = (self._get_job_ineligible_hosts(relevant_jobs))
        self._job_dependencies = (self._get_job_dependencies(relevant_jobs))
        host_ids = self._hosts_available.keys()
        self._host_acls = self._get_host_acls(host_ids)
        self._label_hosts, self._host_labels = (
                self._get_label_hosts(host_ids))
        self._labels = self._get_labels(self._job_dependencies)

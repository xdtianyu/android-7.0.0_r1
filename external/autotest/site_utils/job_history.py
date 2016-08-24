#! /usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module provides functions for caller to retrieve a job's history,
# including special tasks executed before and after the job, and each steps
# start/end time.

import argparse
import datetime as datetime_base

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.tko import models as tko_models

CONFIG = global_config.global_config
AUTOTEST_SERVER = CONFIG.get_config_value('SERVER', 'hostname', type=str)

LOG_BASE_URL = 'http://%s/tko/retrieve_logs.cgi?job=/results/' % AUTOTEST_SERVER
JOB_URL = LOG_BASE_URL + '%(job_id)s-%(owner)s/%(hostname)s'
LOG_PATH_FMT = 'hosts/%(hostname)s/%(task_id)d-%(task_name)s'
TASK_URL = LOG_BASE_URL + LOG_PATH_FMT
AUTOSERV_DEBUG_LOG = 'debug/autoserv.DEBUG'

# Add some buffer before and after job start/end time when searching for special
# tasks. This is to guarantee to include reset before the job starts and repair
# and cleanup after the job finishes.
TIME_BUFFER = datetime_base.timedelta(hours=2)


class JobHistoryObject(object):
    """A common interface to call get_history to return a dictionary of the
    object's history record, e.g., start/end time.
    """

    def build_history_entry(self):
        """Build a history entry.

        This function expect the object has required attributes. Any missing
        attributes will lead to failure.

        @return: A dictionary as the history entry of given job/task.
        """
        return  {'id': self.id,
                 'name': self.name,
                 'hostname': self.hostname,
                 'status': self.status,
                 'log_url': self.log_url,
                 'autoserv_log_url': self.autoserv_log_url,
                 'start_time': self.start_time,
                 'end_time': self.end_time,
                 'time_used': self.time_used,
                 }


    def get_history(self):
        """Return a list of dictionaries of select job/task's history.
        """
        raise NotImplementedError('You must override this method in child '
                                  'class.')


class SpecialTaskInfo(JobHistoryObject):
    """Information of a special task.

    Its properties include:
        id: Special task ID.
        task: An AFE models.SpecialTask object.
        hostname: hostname of the DUT that runs the special task.
        log_url: Url to debug log.
        autoserv_log_url: Url to the autoserv log.
    """

    def __init__(self, task):
        """Constructor

        @param task: An AFE models.SpecialTask object, which has the information
                     of the special task from database.
        """
        # Special task ID
        self.id = task.id
        # AFE special_task model
        self.task = task
        self.name = task.task
        self.hostname = task.host.hostname
        self.status = task.status

        # Link to log
        task_info = {'task_id': task.id, 'task_name': task.task.lower(),
                     'hostname': self.hostname}
        self.log_url = TASK_URL % task_info
        self.autoserv_log_url = '%s/%s' % (self.log_url, AUTOSERV_DEBUG_LOG)

        self.start_time = self.task.time_started
        self.end_time = self.task.time_finished
        if self.start_time and self.end_time:
            self.time_used = (self.end_time - self.start_time).total_seconds()
        else:
            self.time_used = None


    def __str__(self):
        """Get a formatted string of the details of the task info.
        """
        return ('Task %d: %s from %s to %s, for %s seconds.\n' %
                (self.id, self.task.task, self.start_time, self.end_time,
                 self.time_used))


    def get_history(self):
        """Return a dictionary of selected object properties.
        """
        return [self.build_history_entry()]


class TaskCacheCollection(dict):
    """A cache to hold tasks for multiple hosts.

    It's a dictionary of host_id: TaskCache.
    """

    def try_get(self, host_id, job_id, start_time, end_time):
        """Try to get tasks from cache.

        @param host_id: ID of the host.
        @param job_id: ID of the test job that's related to the special task.
        @param start_time: Start time to search for special task.
        @param end_time: End time to search for special task.
        @return: The list of special tasks that are related to given host and
                 Job id. Note that, None means the cache is not available.
                 However, [] means no special tasks found in cache.
        """
        if not host_id in self:
            return None
        return self[host_id].try_get(job_id, start_time, end_time)


    def update(self, host_id, start_time, end_time):
        """Update the cache of the given host by searching database.

        @param host_id: ID of the host.
        @param start_time: Start time to search for special task.
        @param end_time: End time to search for special task.
        """
        search_start_time = start_time - TIME_BUFFER
        search_end_time = end_time + TIME_BUFFER
        tasks = models.SpecialTask.objects.filter(
                host_id=host_id,
                time_started__gte=search_start_time,
                time_started__lte=search_end_time)
        self[host_id] = TaskCache(tasks, search_start_time, search_end_time)


class TaskCache(object):
    """A cache that hold tasks for a host.
    """

    def __init__(self, tasks=[], start_time=None, end_time=None):
        """Constructor
        """
        self.tasks = tasks
        self.start_time = start_time
        self.end_time = end_time

    def try_get(self, job_id, start_time, end_time):
        """Try to get tasks from cache.

        @param job_id: ID of the test job that's related to the special task.
        @param start_time: Start time to search for special task.
        @param end_time: End time to search for special task.
        @return: The list of special tasks that are related to the job id.
                 Note that, None means the cache is not available.
                 However, [] means no special tasks found in cache.
        """
        if start_time < self.start_time or end_time > self.end_time:
            return None
        return [task for task in self.tasks if task.queue_entry and
                task.queue_entry.job.id == job_id]


class TestJobInfo(JobHistoryObject):
    """Information of a test job
    """

    def __init__(self, hqe, task_caches=None, suite_start_time=None,
                 suite_end_time=None):
        """Constructor

        @param hqe: HostQueueEntry of the job.
        @param task_caches: Special tasks that's from a previous query.
        @param suite_start_time: Start time of the suite job, default is
                None. Used to build special task search cache.
        @param suite_end_time: End time of the suite job, default is
                None. Used to build special task search cache.
        """
        # AFE job ID
        self.id = hqe.job.id
        # AFE job model
        self.job = hqe.job
        # Name of the job, strip all build and suite info.
        self.name = hqe.job.name.split('/')[-1]
        self.status = hqe.status if hqe else None

        try:
            self.tko_job = tko_models.Job.objects.filter(afe_job_id=self.id)[0]
            self.host = models.Host.objects.filter(
                    hostname=self.tko_job.machine.hostname)[0]
            self.hostname = self.tko_job.machine.hostname
            self.start_time = self.tko_job.started_time
            self.end_time = self.tko_job.finished_time
        except IndexError:
            # The test job was never started.
            self.tko_job = None
            self.host = None
            self.hostname = None
            self.start_time = None
            self.end_time = None

        if self.end_time and self.start_time:
            self.time_used = (self.end_time - self.start_time).total_seconds()
        else:
            self.time_used = None

        # Link to log
        self.log_url = JOB_URL % {'job_id': hqe.job.id, 'owner': hqe.job.owner,
                                  'hostname': self.hostname}
        self.autoserv_log_url = '%s/%s' % (self.log_url, AUTOSERV_DEBUG_LOG)

        self._get_special_tasks(hqe, task_caches, suite_start_time,
                                suite_end_time)


    def _get_special_tasks(self, hqe, task_caches=None, suite_start_time=None,
                           suite_end_time=None):
        """Get special tasks ran before and after the test job.

        @param hqe: HostQueueEntry of the job.
        @param task_caches: Special tasks that's from a previous query.
        @param suite_start_time: Start time of the suite job, default is
                None. Used to build special task search cache.
        @param suite_end_time: End time of the suite job, default is
                None. Used to build special task search cache.
        """
        # Special tasks run before job starts.
        self.tasks_before = []
        # Special tasks run after job finished.
        self.tasks_after = []

        # Skip locating special tasks if hqe is None, or not started yet, as
        # that indicates the test job might not be started.
        if not hqe or not hqe.started_on:
            return

        # Assume special tasks for the test job all start within 2 hours
        # before the test job starts or 2 hours after the test finishes. In most
        # cases, special task won't take longer than 2 hours to start before
        # test job starts and after test job finishes.
        search_start_time = hqe.started_on - TIME_BUFFER
        search_end_time = (hqe.finished_on + TIME_BUFFER if hqe.finished_on else
                           hqe.started_on + TIME_BUFFER)

        if task_caches is not None and suite_start_time and suite_end_time:
            tasks = task_caches.try_get(self.host.id, self.id,
                                        suite_start_time, suite_end_time)
            if tasks is None:
                task_caches.update(self.host.id, search_start_time,
                                   search_end_time)
                tasks = task_caches.try_get(self.host.id, self.id,
                                            suite_start_time, suite_end_time)
        else:
            tasks = models.SpecialTask.objects.filter(
                        host_id=self.host.id,
                        time_started__gte=search_start_time,
                        time_started__lte=search_end_time)
            tasks = [task for task in tasks if task.queue_entry and
                     task.queue_entry.job.id == self.id]

        for task in tasks:
            task_info = SpecialTaskInfo(task)
            if task.time_started < self.start_time:
                self.tasks_before.append(task_info)
            else:
                self.tasks_after.append(task_info)


    def get_history(self):
        """Get the history of a test job.

        @return: A list of special tasks and test job information.
        """
        history = []
        history.extend([task.build_history_entry() for task in
                        self.tasks_before])
        history.append(self.build_history_entry())
        history.extend([task.build_history_entry() for task in
                        self.tasks_after])
        return history


    def __str__(self):
        """Get a formatted string of the details of the job info.
        """
        result = '%d: %s\n' % (self.id, self.name)
        for task in self.tasks_before:
            result += str(task)

        result += ('Test from %s to %s, for %s seconds.\n' %
                   (self.start_time, self.end_time, self.time_used))

        for task in self.tasks_after:
            result += str(task)

        return result


class SuiteJobInfo(JobHistoryObject):
    """Information of a suite job
    """

    def __init__(self, hqe):
        """Constructor

        @param hqe: HostQueueEntry of the job.
        """
        # AFE job ID
        self.id = hqe.job.id
        # AFE job model
        self.job = hqe.job
        # Name of the job, strip all build and suite info.
        self.name = hqe.job.name.split('/')[-1]
        self.status = hqe.status if hqe else None

        self.log_url = JOB_URL % {'job_id': hqe.job.id, 'owner': hqe.job.owner,
                                  'hostname': 'hostless'}

        hqe = models.HostQueueEntry.objects.filter(job_id=hqe.job.id)[0]
        self.start_time = hqe.started_on
        self.end_time = hqe.finished_on
        if self.start_time and self.end_time:
            self.time_used = (self.end_time - self.start_time).total_seconds()
        else:
            self.time_used = None

        # Cache of special tasks, hostname: ((start_time, end_time), [tasks])
        task_caches = TaskCacheCollection()
        self.test_jobs = []
        for job in models.Job.objects.filter(parent_job_id=self.id):
            try:
                job_hqe = models.HostQueueEntry.objects.filter(job_id=job.id)[0]
            except IndexError:
                continue
            self.test_jobs.append(TestJobInfo(job_hqe, task_caches,
                                                self.start_time, self.end_time))


    def get_history(self):
        """Get the history of a suite job.

        @return: A list of special tasks and test job information that has
                 suite job as the parent job.
        """
        history = []
        for job in sorted(self.test_jobs,
                          key=lambda j: (j.hostname, j.start_time)):
            history.extend(job.get_history())
        return history


    def __str__(self):
        """Get a formatted string of the details of the job info.
        """
        result = '%d: %s\n' % (self.id, self.name)
        for job in self.test_jobs:
            result += str(job)
            result += '-' * 80 + '\n'
        return result


def get_job_info(job_id):
    """Get the history of a job.

    @param job_id: ID of the job.
    @return: A TestJobInfo object that contains the test job and its special
             tasks' start/end time, if the job is a test job. Otherwise, return
             a SuiteJobInfo object if the job is a suite job.
    @raise Exception: if the test job can't be found in database.
    """
    try:
        hqe = models.HostQueueEntry.objects.filter(job_id=job_id)[0]
    except IndexError:
        raise Exception('No HQE found for job ID %d' % job_id)

    if hqe and hqe.execution_subdir != 'hostless':
        return TestJobInfo(hqe)
    else:
        return SuiteJobInfo(hqe)


def main():
    """Main script.

    The script accepts a job ID and print out the test job and its special
    tasks' start/end time.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--job_id', type=int, dest='job_id', required=True)
    options = parser.parse_args()

    job_info = get_job_info(options.job_id)

    print job_info


if __name__ == '__main__':
    main()

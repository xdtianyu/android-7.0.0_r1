#! /usr/bin/python

# The tool gathers the time used by special tasks/test jobs to a tab-separated
# output.
# Import the output to google spreadsheet can generate a sheet like this:
# https://docs.google.com/a/google.com/spreadsheets/d/
# 1iLPSRAgSVz9rGVusTb2yaHJ88iv0QY3hZI_ZI-RdatI/edit#gid=51630294


import os
import argparse
from datetime import datetime
import re
import subprocess
import sys
import urllib2

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.server.cros.dynamic_suite import job_status


CONFIG = global_config.global_config
AUTOTEST_SERVER = CONFIG.get_config_value('SERVER', 'hostname', type=str)

LOG_BASE_URL = 'http://%s/tko/retrieve_logs.cgi?job=/results/' % AUTOTEST_SERVER
JOB_URL = LOG_BASE_URL + '%(job_id)s-%(owner)s/%(hostname)s'
LOG_PATH_FMT = 'hosts/%(hostname)s/%(task_id)d-%(taskname)s'
TASK_URL = LOG_BASE_URL + LOG_PATH_FMT
AUTOSERV_DEBUG_LOG = 'debug/autoserv.DEBUG'
HYPERLINK = '=HYPERLINK("%s","%0.1f")'

GS_URI =  'gs://chromeos-autotest-results'

# A cache of special tasks, hostname:[task]
tasks_cache = {}

def get_debug_log(autoserv_log_url, autoserv_log_path):
    """Get a list of strings or a stream for autoserv.DEBUG log file.

    @param autoserv_log_url: Url to the repair job's autoserv.DEBUG log.
    @param autoserv_log_path: Path to autoserv.DEBUG log, e.g.,
                        hosts/hostname/job_id-repair/debug/autoserv.DEBUG.
    @return: A list of string if debug log was moved to GS already, or a
             stream of the autoserv.DEBUG file.
    """
    url = urllib2.urlopen(autoserv_log_url).geturl()
    if not 'accounts.google.com' in url:
        return urllib2.urlopen(url)
    else:
        # The file was moved to Google storage already, read the file from GS.
        debug_log_link = '%s/%s' % (GS_URI, autoserv_log_path)
        cmd = 'gsutil cat %s' % debug_log_link
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        if proc.returncode == 0:
            return stdout.split(os.linesep)
        else:
            print 'Failed to read %s: %s' % (debug_log_link, stderr)


class task_runtime(object):
    """Details about the task run time.
    """
    def __init__(self, task):
        """Init task_runtime
        """
        # Special task ID
        self.id = task.id
        # AFE special_task model
        self.task = task
        self.hostname = task.host.hostname

        # Link to log
        task_info = {'task_id': task.id, 'taskname': task.task.lower(),
                     'hostname': self.hostname}
        self.log = TASK_URL % task_info

        autoserv_log_url = '%s/%s' % (self.log, AUTOSERV_DEBUG_LOG)
        log_path = LOG_PATH_FMT % task_info
        autoserv_log_path = '%s/%s' % (log_path, AUTOSERV_DEBUG_LOG)
        debug_log = get_debug_log(autoserv_log_url, autoserv_log_path)
        lines = [line for line in debug_log if line]
        # Task start time
        self.start_time = _get_timestamp(lines[0])
        # Task end time
        self.end_time = _get_timestamp(lines[-1])
        # Run time across end of a year.
        if self.end_time < self.start_time:
            self.end_time = self.end_time.replace(year=self.end_time.year+1)
        self.time_used = self.end_time - self.start_time

        # Start/end time from afe_special_tasks table.
        # To support old special tasks, the value falls back to the ones from
        # debug log.
        self.start_time_db = (self.task.time_started if self.task.time_started
                              else self.start_time)
        self.end_time_db = (self.task.time_finished if self.task.time_finished
                            else self.end_time)
        self.time_used_db = self.end_time_db - self.start_time_db


class job_runtime(object):
    """Details about the job run time, including time spent on special tasks.
    """
    def __init__(self, job, suite_start_time=None, suite_end_time=None):
        """Init job_run_time
        """
        # AFE job ID
        self.id = job.id
        # AFE job model
        self.job = job
        # Name of the job, strip all build and suite info.
        self.name = job.name.split('/')[-1]

        try:
            self.tko_job = tko_models.Job.objects.filter(afe_job_id=self.id)[0]
            self.host_id = self.tko_job.machine_id
            self.hostname = self.tko_job.machine.hostname
            # Job start time
            self.start_time = self.tko_job.started_time
            # Job end time
            self.end_time = self.tko_job.finished_time
            self.time_used = self.end_time - self.start_time
        except IndexError:
            self.tko_job = None
            self.host_id = 0
            self.time_used = 0

        # Link to log
        self.log = JOB_URL % {'job_id': job.id, 'owner': job.owner,
                              'hostname': self.hostname}

        # Special tasks run before job starts.
        self.tasks_before = []
        # Special tasks run after job finished.
        self.tasks_after = []

        # If job time used is not calculated, skip locating special tasks.
        if not self.time_used:
            return

        # suite job has no special tasks
        if not self.hostname:
            return

        tasks = tasks_cache.get(self.hostname, None)
        if not tasks:
            tasks = models.SpecialTask.objects.filter(
                    host__hostname=self.hostname,
                    time_started__gte=suite_start_time,
                    time_started__lte=suite_end_time)
            tasks_cache[self.hostname] = tasks
        for task in tasks:
            if not task.queue_entry or task.queue_entry.job_id != self.id:
                continue
            t_runtime = task_runtime(task)
            if task.time_started < self.start_time:
                self.tasks_before.append(t_runtime)
            else:
                self.tasks_after.append(t_runtime)


    def get_all_tasks(self):
        return self.tasks_before + self.tasks_after


    def get_first_task_start_time(self):
        """Get the start time of first task, return test job start time if
        there is no tasks_before
        """
        start = self.start_time
        for task in self.tasks_before:
            if task.start_time_db < start:
                start = task.start_time_db
        return start


    def get_last_task_end_time(self):
        """Get the end time of last task, return test job end time if there is
        no tasks_after.
        """
        end = self.end_time
        for task in self.tasks_after:
            if end < task.end_time_db:
                end = task.end_time_db
        return end


    def get_total_time(self):
        return (self.get_last_task_end_time() -
                self.get_first_task_start_time()).total_seconds()


    def get_time_on_tasks(self):
        time = 0
        for task in self.tasks_before + self.tasks_after:
            time += task.time_used.total_seconds()
        return time


    def get_time_on_tasks_from_db(self):
        time = 0
        for task in self.tasks_before + self.tasks_after:
            time += task.time_used_db.total_seconds()
        return time


    def get_time_on_wait(self):
        return (self.get_total_time() -
                self.get_time_on_tasks() -
                self.time_used.total_seconds())


    def get_time_on_wait_from_db(self):
        return (self.get_total_time() -
                self.get_time_on_tasks_from_db() -
                self.time_used.total_seconds())


    def get_time_per_task_type(self, task_type):
        """only used for suite job
        """
        tasks = []
        for job in self.test_jobs:
            tasks += [task for task in job.get_all_tasks()
                          if task.task.task == task_type]
        if not tasks:
            return None

        task_min = tasks[0]
        task_max = tasks[0]
        total = 0
        for task in tasks:
            if task.time_used < task_min.time_used:
                task_min = task
            if task.time_used > task_max.time_used:
                task_max = task
            total += task.time_used.total_seconds()
        return (task_min, task_max, total/len(tasks), len(tasks))


    def get_time_per_task_type_from_db(self, task_type):
        """only used for suite job
        """
        tasks = []
        for job in self.test_jobs:
            tasks += [task for task in job.get_all_tasks()
                          if task.task.task == task_type]
        if not tasks:
            return None

        task_min = tasks[0]
        task_max = tasks[0]
        total = 0
        for task in tasks:
            if task.time_used_db < task_min.time_used_db:
                task_min = task
            if task.time_used_db > task_max.time_used_db:
                task_max = task
            total += task.time_used_db.total_seconds()
        return (task_min, task_max, total/len(tasks), len(tasks))


def _get_timestamp(line):
    """Get the time from the beginning of a log entry.

    @param line: A line of log entry, e.g., "06/19 19:56:53.602 INFO |"
    @return: A datetime value of the log entry.
    """
    try:
        time = re.match( '^(\d\d\/\d\d \d\d:\d\d:\d\d.\d+).*', line).group(1)
        time = '%d/%s' % (datetime.now().year, time)
        return datetime.strptime(time, '%Y/%m/%d %H:%M:%S.%f')
    except:
        return None


def _parse_args(args):
    if not args:
        print 'Try ./contrib/compare_suite.py --jobs 51,52,53'
        sys.exit(0)
    parser = argparse.ArgumentParser(
            description=('A script to compare the performance of multiple suite'
                         ' runs.'))
    parser.add_argument('--jobs',
                        help='A list of job IDs.')
    return parser.parse_args(args)


def merge_time_dict(time_dict_list):
    merged = {}
    for time_dict in time_dict_list:
        for key,val in time_dict.items():
            time_used = merged.get(key, 0)
            merged[key] = time_used + val
    return merged


def print_task_data(all_jobs, time_data):
    percent_string = delimiter.join(
            [str(100.0*data[2]*data[3]/suite_job.total_time)
             if data else '_' for (data, suite_job) in
             zip(time_data, all_jobs.keys())])
    print '%% on %s %s%s' % (task_type, delimiter, percent_string)
    min_string = delimiter.join(
            [(HYPERLINK % (data[0].log, data[0].time_used.total_seconds()))
             if data else '_' for data in time_data])
    print '  %s min (seconds)%s%s' % (task_type, delimiter, min_string)
    max_string = delimiter.join(
            [HYPERLINK % (data[1].log, data[1].time_used.total_seconds())
             if data else '_' for data in time_data])
    print '  %s max (seconds)%s%s' % (task_type, delimiter, max_string)
    ave_string = delimiter.join(
            [str(data[2]) if data else '_' for data in time_data])
    print '  %s average (seconds)%s%s' % (task_type, delimiter, ave_string)


if __name__ == '__main__':
    args = _parse_args(sys.argv[1:])
    print 'Comparing jobs: %s' % args.jobs
    job_ids = [int(id) for id in args.jobs.split(',')]

    # Make sure all jobs are suite jobs
    parent_jobs = [job.parent_job_id for job in
                   models.Job.objects.filter(id__in=job_ids)]
    if any(parent_jobs):
        print ('Some jobs are not suite job. Please provide a list of suite job'
               ' IDs.')
        sys.exit(1)

    # A dictionary of suite_job_runtime:{test_job_name:test_job_runtime}
    all_jobs = {}
    for job_id in job_ids:
        suite_job = models.Job.objects.filter(id=job_id)[0]
        suite_job_runtime = job_runtime(suite_job)
        test_jobs = models.Job.objects.filter(parent_job_id=job_id)
        if len(test_jobs) == 0:
            print 'No child job found for suite job: %d' % job_id
            sys.exit(1)
        test_job_runtimes = [job_runtime(job, suite_job_runtime.start_time,
                                         suite_job_runtime.end_time)
                             for job in test_jobs]
        suite_job_runtime.test_jobs = test_job_runtimes
        suite_job_runtime.hosts = set([job.host_id
                                       for job in test_job_runtimes
                                       if job.host_id != 0])
        suite_job_runtime.total_time = sum(
                [job.get_total_time() for job in test_job_runtimes])
        suite_job_runtime.total_time_on_tasks = sum(
                [job.get_time_on_tasks() for job in test_job_runtimes])
        suite_job_runtime.total_time_on_tasks_from_db = sum(
                [job.get_time_on_tasks_from_db() for job in test_job_runtimes])
        suite_job_runtime.total_time_on_wait = sum(
                [job.get_time_on_wait() for job in test_job_runtimes])
        suite_job_runtime.total_time_on_wait_from_db = sum(
                [job.get_time_on_wait_from_db() for job in test_job_runtimes])
        suite_job_runtime.percent_on_tasks = 100*(
                suite_job_runtime.total_time_on_tasks /
                suite_job_runtime.total_time)
        suite_job_runtime.percent_on_wait = 100*(
                suite_job_runtime.total_time_on_wait /
                suite_job_runtime.total_time)
        suite_job_runtime.percent_on_tasks_from_db = 100*(
                suite_job_runtime.total_time_on_tasks_from_db /
                suite_job_runtime.total_time)
        suite_job_runtime.percent_on_wait_from_db = 100*(
                suite_job_runtime.total_time_on_wait_from_db /
                suite_job_runtime.total_time)
        all_jobs[suite_job_runtime] = {r.name:r for r in test_job_runtimes}

    delimiter = '\t'
    # Print a row of suite job IDs.
    print ('job ID%s' % delimiter +
           delimiter.join([str(suite_job.id)
                           for suite_job in all_jobs.keys()]))

    # Print a row of platforms, e.g., lumpy-release.
    print ('platform%s' % delimiter +
           delimiter.join([suite_job.job.name.split('/')[0]
                           for suite_job in all_jobs.keys()]))

    # Print time to run suite
    print ('time(mins)%s' % delimiter +
           delimiter.join([str(suite_job.time_used.total_seconds()/60)
                           for suite_job in all_jobs.keys()]))

    # Print percent of time on tasks
    print ('%% on special tasks%s' % delimiter +
           delimiter.join([str(suite_job.percent_on_tasks)
                           for suite_job in all_jobs.keys()]))

    # Print percent of time on tasks based on time data from afe_special_tasks
    print ('%% on special tasks with data from DB%s' % delimiter +
           delimiter.join([str(suite_job.percent_on_tasks_from_db)
                           for suite_job in all_jobs.keys()]))

    # Print percent of time on tasks per task type
    all_task_types = set()
    for suite_job in all_jobs.keys():
        for job in suite_job.test_jobs:
            all_task_types.update(
                    [task.task.task for task in job.get_all_tasks()])
    for task_type in all_task_types:
        print 'Time data retrieved from debug log.'
        time_data = [suite_job.get_time_per_task_type(task_type)
                     for suite_job in all_jobs.keys()]
        print_task_data(all_jobs, time_data)

        print 'Time data retrieved from database'
        time_data = [suite_job.get_time_per_task_type_from_db(task_type)
                     for suite_job in all_jobs.keys()]
        print_task_data(all_jobs, time_data)

        count_string = delimiter.join(
                [str(data[3]) if data else '_' for data in time_data])
        print '  %s count%s%s' % (task_type, delimiter, count_string)

    # Print percent of time on wait
    print ('%% on wait%s' % delimiter +
           delimiter.join([str(suite_job.percent_on_wait)
                           for suite_job in all_jobs.keys()]))

    # Print percent of time on wait based on special task time data from
    # database
    print ('%% on wait based on time data from db%s' % delimiter +
           delimiter.join([str(suite_job.percent_on_wait_from_db)
                           for suite_job in all_jobs.keys()]))

    # Print the number of duts used for suite.
    print ('dut #%s' % delimiter +
           delimiter.join([str(len(suite_job.hosts))
                           for suite_job in all_jobs.keys()]))

    for test_name in [job for job in all_jobs.values()[0].keys()]:
        line = '%s%s' % (test_name, delimiter)
        for suite_job in all_jobs.keys():
            test_job = all_jobs[suite_job].get(test_name)
            if test_job:
                line += (HYPERLINK %
                         (test_job.log, test_job.time_used.total_seconds())
                         + delimiter)
            else:
                line += '_%s' % delimiter
        print line

#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Script to calculate timing stats for suites.

This script measures nine stats for a suite run.
1. Net suite runtime.
2. Suite scheduling overhead.
3. Average scheduling overhead.
4. Average Queuing time.
5. Average Resetting time.
6. Average provisioning time.
7. Average Running time.
8. Average Parsing time.
9. Average Gathering time.

When the cron_mode is enabled, this script throws all stats but the first one
(Net suite runtime) to Graphite because the first one is already
being sent to Graphite by Autotest online.

Net suite runtime is end-to-end time for a suite from the beginning
to the end.
It is stored in a field, "duration", of a type, "suite_runtime" in
elasticsearch (ES).

Suite scheduling overhead is defined by the average of DUT overheads.
Suite is composed of one or more jobs, and those jobs are run on
one or more DUTs that are available.
A DUT overhead is defined by:
    DUT_i overhead = sum(net time for job_k - runtime for job_k
                         - runtime for special tasks of job_k)
    Job_k are the jobs run on DUT_i.

Net time for a job is the time from job_queued_time to hqe_finished_time.
job_queued_time is stored in the "queued_time" column of "tko_jobs" table.
hqe_finished_time is stored in the "finished_on" of "afe_host_queue_entries"
table.
We do not use "job_finished_time" of "tko_jobs" as job_finished_time is
recorded before gathering/parsing/archiving.
We do not use hqe started time ("started_on" of "afe_host_queue_entries"),
as it does not account for the lag from a host is assigned to the job till
the scheduler sees the assignment.

Runtime for job_k is the sum of durations for the records of
"job_time_breakdown" type in ES that have "Queued" or "Running" status.
It is possible that a job has multiple "Queued" records when the job's test
failed and tried again.
We take into account only the last "Queued" record.

Runtime for special tasks of job_k is the sum of durations for the records
of "job_time_breakdown" type in ES that have "Resetting", "Provisioning",
"Gathering", or "Parsing" status.
We take into account only the records whose timestamp is larger than
the timestamp of the last "Queued" record.
"""

import argparse
from datetime import datetime
from datetime import timedelta

import common
from autotest_lib.client.common_lib import host_queue_entry_states
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.server import utils
from autotest_lib.site_utils import job_overhead


_options = None

_hqes = host_queue_entry_states.Status
_states = [
        _hqes.QUEUED, _hqes.RESETTING, _hqes.PROVISIONING,
        _hqes.RUNNING, _hqes.GATHERING, _hqes.PARSING]


def mean(l):
    """
    Calculates an Arithmetic Mean for the numbers in a list.

    @param l: A list of numbers.
    @return: Arithmetic mean if the list is not empty.
             Otherwise, returns zero.
    """
    return float(sum(l)) / len(l) if l else 0


def print_verbose(string, *args):
    if _options.verbose:
        print(string % args)


def get_nontask_runtime(job_id, dut, job_info_dict):
    """
    Get sum of durations for "Queued", "Running", "Parsing", and "Gathering"
    status records.
    job_info_dict will be modified in this function to store the duration
    for each status.

    @param job_id: The job id of interest.
    @param dut: Hostname of a DUT that the job ran on.
    @param job_info_dict: Dictionary that has information for jobs.
    @return: Tuple of sum of durations and the timestamp for the last
             Queued record.
    """
    results = autotest_es.query(
            fields_returned=['status', 'duration', 'time_recorded'],
            equality_constraints=[('_type', 'job_time_breakdown'),
                                  ('job_id', job_id),
                                  ('hostname', dut)],
            sort_specs=[{'time_recorded': 'desc'}])

    sum = 0
    last_queued_timestamp = 0
    # There could be multiple "Queued" records.
    # Get sum of durations for the records after the last "Queued" records
    # (including the last "Queued" record).
    # Exploits the fact that "results" are ordered in the descending order
    # of time_recorded.
    for hit in results.hits:
        job_info_dict[job_id][hit['status']] = float(hit['duration'])
        if hit['status'] == 'Queued':
            # The first Queued record is the last one because of the descending
            # order of "results".
            last_queued_timestamp = float(hit['time_recorded'])
            sum += float(hit['duration'])
            break
        else:
            sum += float(hit['duration'])
    return (sum, last_queued_timestamp)


def get_tasks_runtime(task_list, dut, t_start, job_id, job_info_dict):
    """
    Get sum of durations for special tasks.
    job_info_dict will be modified in this function to store the duration
    for each special task.

    @param task_list: List of task id.
    @param dut: Hostname of a DUT that the tasks ran on.
    @param t_start: Beginning timestamp.
    @param job_id: The job id that is related to the tasks.
                   This is used only for debugging purpose.
    @param job_info_dict: Dictionary that has information for jobs.
    @return: Sum of durations of the tasks.
    """
    t_start_epoch = time_utils.to_epoch_time(t_start)
    results = autotest_es.query(
            fields_returned=['status', 'task_id', 'duration'],
            equality_constraints=[('_type', 'job_time_breakdown'),
                                  ('hostname', dut)],
            range_constraints=[('time_recorded', t_start_epoch, None)],
            batch_constraints=[('task_id', task_list)])
    sum = 0
    for hit in results.hits:
        sum += float(hit['duration'])
        job_info_dict[job_id][hit['status']] = float(hit['duration'])
        print_verbose('Task %s for Job %s took %s',
                      hit['task_id'], job_id, hit['duration'])
    return sum


def get_job_runtime(job_id, dut, job_info_dict):
    """
    Get sum of durations for the entries that are related to a job.
    job_info_dict will be modified in this function.

    @param job_id: The job id of interest.
    @param dut: Hostname of a DUT that the job ran on.
    @param job_info_dict: Dictionary that has information for jobs.
    @return: Total duration taken by a job.
    """
    sum, t_last_queued = get_nontask_runtime(job_id, dut, job_info_dict)
    print_verbose('Job %s took %f, last Queued: %s',
                  job_id, sum, t_last_queued)
    sum += get_tasks_runtime(
            list(job_info_dict[job_id]['tasks']), dut, t_last_queued,
            job_id, job_info_dict)
    return sum


def get_dut_overhead(dut, jobs, job_info_dict):
    """
    Calculates the scheduling overhead of a DUT.

    The scheduling overhead of a DUT is defined by the sum of scheduling
    overheads for the jobs that ran on the DUT.
    The scheduling overhead for a job is defined by the difference
    of net job runtime and real job runtime.
    job_info_dict will be modified in this function.

    @param dut: Hostname of a DUT.
    @param jobs: The list of jobs that ran on the DUT.
    @param job_info_dict: Dictionary that has information for jobs.
    @return: Scheduling overhead of a DUT in a floating point value.
             The unit is a second.
    """
    overheads = []
    for job_id in jobs:
        (t_start, t_end) = job_info_dict[job_id]['timestamps']
        runtime = get_job_runtime(job_id, dut, job_info_dict)
        overheads.append(t_end - t_start - runtime)
        print_verbose('Job: %s, Net runtime: %f, Real runtime: %f, '
                      'Overhead: %f', job_id, t_end - t_start, runtime,
                      t_end - t_start - runtime)
    return sum(overheads)


def get_child_jobs_info(suite_job_id, num_child_jobs, sanity_check):
    """
    Gets information about child jobs of a suite.

    @param suite_job_id: Job id of a suite.
    @param num_child_jobs: Number of child jobs of the suite.
    @param sanity_check: Do sanity check if True.
    @return: A tuple of (dictionary, list). For dictionary, the key is
             a DUT's hostname and the value is a list of jobs that ran on
             the DUT. List is the list of all jobs of the suite.
    """
    results = autotest_es.query(
            fields_returned=['job_id', 'hostname'],
            equality_constraints=[('_type', 'host_history'),
                                  ('parent_job_id', suite_job_id),
                                  ('status', 'Running'),])

    dut_jobs_dict = {}
    job_filter = set()
    for hit in results.hits:
        job_id = hit['job_id']
        dut = hit['hostname']
        if job_id in job_filter:
            continue
        job_list = dut_jobs_dict.setdefault(dut, [])
        job_list.append(job_id)
        job_filter.add(job_id)

    if sanity_check and len(job_filter) != num_child_jobs:
        print('WARNING: Mismatch number of child jobs of a suite (%d): '
              '%d != %d' % (suite_job_id, len(job_filter), num_child_jobs))
    return dut_jobs_dict, list(job_filter)


def get_job_timestamps(job_list, job_info_dict):
    """
    Get beginning time and ending time for each job.

    The beginning time of a job is "queued_time" of "tko_jobs" table.
    The ending time of a job is "finished_on" of "afe_host_queue_entries" table.
    job_info_dict will be modified in this function to store the timestamps.

    @param job_list: List of job ids
    @param job_info_dict: Dictionary that timestamps for each job will be stored
    """
    tko = tko_models.Job.objects.filter(afe_job_id__in=job_list)
    hqe = models.HostQueueEntry.objects.filter(job_id__in=job_list)
    job_start = {}
    for t in tko:
        job_start[t.afe_job_id] = time_utils.to_epoch_time(t.queued_time)
    job_end = {}
    for h in hqe:
        job_end[h.job_id] = time_utils.to_epoch_time(h.finished_on)

    for job_id in job_list:
        info_dict = job_info_dict.setdefault(job_id, {})
        info_dict.setdefault('timestamps', (job_start[job_id], job_end[job_id]))


def get_job_tasks(job_list, job_info_dict):
    """
    Get task ids for each job.
    job_info_dict will be modified in this function to store the task ids.

    @param job_list: List of job ids
    @param job_info_dict: Dictionary that task ids for each job will be stored.
    """
    results = autotest_es.query(
            fields_returned=['job_id', 'task_id'],
            equality_constraints=[('_type', 'host_history')],
            batch_constraints=[('job_id', job_list)])
    for hit in results.hits:
        if 'task_id' in hit:
            info_dict = job_info_dict.setdefault(hit['job_id'], {})
            task_set = info_dict.setdefault('tasks', set())
            task_set.add(hit['task_id'])


def get_scheduling_overhead(suite_job_id, num_child_jobs, sanity_check=True):
    """
    Calculates a scheduling overhead.

    A scheduling overhead is defined by the average of DUT overheads
    for the DUTs that the child jobs of a suite ran on.

    @param suite_job_id: Job id of a suite.
    @param num_child_jobs: Number of child jobs of the suite.
    @param sanity_check: Do sanity check if True.
    @return: Dictionary storing stats.
    """
    dut_jobs_dict, job_list = get_child_jobs_info(
            suite_job_id, num_child_jobs, sanity_check)
    job_info_dict = {}
    get_job_timestamps(job_list, job_info_dict)
    get_job_tasks(job_list, job_info_dict)

    dut_overheads = []
    avg_overhead = 0
    for dut, jobs in dut_jobs_dict.iteritems():
        print_verbose('Dut: %s, Jobs: %s', dut, jobs)
        overhead = get_dut_overhead(dut, jobs, job_info_dict)
        avg_overhead += overhead
        print_verbose('Dut overhead: %f', overhead)
        dut_overheads.append(overhead)

    if job_list:
        avg_overhead = avg_overhead / len(job_list)

    state_samples_dict = {}
    for info in job_info_dict.itervalues():
        for state in _states:
            if state in info:
                samples = state_samples_dict.setdefault(state, [])
                samples.append(info[state])

    if state_samples_dict:
        result = {state: mean(state_samples_dict[state])
                  if state in state_samples_dict else 0
                  for state in _states}
    result['suite_overhead'] = mean(dut_overheads)
    result['overhead'] = avg_overhead
    result['num_duts'] = len(dut_jobs_dict)
    return result


def print_suite_stats(suite_stats):
    """Prints out statistics for a suite to standard output."""
    print('suite_overhead: %(suite_overhead)f, overhead: %(overhead)f,' %
          suite_stats),
    for state in _states:
        if state in suite_stats:
            print('%s: %f,' % (state, suite_stats[state])),
    print('num_duts: %(num_duts)d' % suite_stats)


def analyze_suites(start_time, end_time):
    """
    Calculates timing stats (i.e., suite runtime, scheduling overhead)
    for the suites that finished within the timestamps given by parameters.

    @param start_time: Beginning timestamp.
    @param end_time: Ending timestamp.
    """
    print('Analyzing suites from %s to %s...' % (
          time_utils.epoch_time_to_date_string(start_time),
          time_utils.epoch_time_to_date_string(end_time)))

    if _options.bvtonly:
        batch_constraints = [
                ('suite_name', ['bvt-inline', 'bvt-cq', 'bvt-perbuild'])]
    else:
        batch_constraints = []

    start_time_epoch = time_utils.to_epoch_time(start_time)
    end_time_epoch = time_utils.to_epoch_time(end_time)
    results = autotest_es.query(
            fields_returned=['suite_name', 'suite_job_id', 'board', 'build',
                             'num_child_jobs', 'duration'],
            equality_constraints=[('_type', job_overhead.SUITE_RUNTIME_KEY),],
            range_constraints=[('time_recorded', start_time_epoch,
                                end_time_epoch)],
            sort_specs=[{'time_recorded': 'asc'}],
            batch_constraints=batch_constraints)
    print('Found %d suites' % (results.total))

    for hit in results.hits:
        suite_job_id = hit['suite_job_id']

        try:
            suite_name = hit['suite_name']
            num_child_jobs = int(hit['num_child_jobs'])
            suite_runtime = float(hit['duration'])

            print('Suite: %s (%s), Board: %s, Build: %s, Num child jobs: %d' % (
                    suite_name, suite_job_id, hit['board'], hit['build'],
                    num_child_jobs))

            suite_stats = get_scheduling_overhead(suite_job_id, num_child_jobs)
            print('Suite: %s (%s) runtime: %f,' % (
                    suite_name, suite_job_id, suite_runtime)),
            print_suite_stats(suite_stats)

            if _options.cron_mode:
                key = utils.get_data_key(
                        'suite_time_stats', suite_name, hit['build'],
                        hit['board'])
                autotest_stats.Timer(key).send('suite_runtime', suite_runtime)
                for stat, val in suite_stats.iteritems():
                    autotest_stats.Timer(key).send(stat, val)
        except Exception as e:
            print('ERROR: Exception is raised while processing suite %s' % (
                    suite_job_id))
            print e


def analyze_suite(suite_job_id):
    suite_stats = get_scheduling_overhead(suite_job_id, 0, False)
    print('Suite (%s)' % suite_job_id),
    print_suite_stats(suite_stats)


def main():
    """main script."""
    parser = argparse.ArgumentParser(
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-c', dest='cron_mode', action='store_true',
                        help=('Run in a cron mode. Cron mode '
                              'sends calculated stat data to Graphite.'),
                        default=False)
    parser.add_argument('-s', type=int, dest='span',
                        help=('Number of hours that stats should be '
                              'collected.'),
                        default=1)
    parser.add_argument('--bvtonly', dest='bvtonly', action='store_true',
                        help=('Gets bvt suites only (i.e., bvt-inline,'
                              'bvt-cq, bvt-perbuild).'),
                        default=False)
    parser.add_argument('--suite', type=int, dest='suite_job_id',
                        help=('Job id of a suite.'))
    parser.add_argument('--verbose', dest='verbose', action='store_true',
                        help=('Prints out more info if True.'),
                        default=False)
    global _options
    _options = parser.parse_args()

    if _options.suite_job_id:
        analyze_suite(_options.suite_job_id)
    else:
        end_time = time_utils.to_epoch_time(datetime.now())
        start_time = end_time - timedelta(hours=_options.span).total_seconds()
        analyze_suites(start_time, end_time)


if __name__ == '__main__':
    main()

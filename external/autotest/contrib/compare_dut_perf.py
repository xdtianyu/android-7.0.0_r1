#!/usr/bin/env python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to compare the performance of duts when running the same
# test/special task. For example:
#
# python compare_dut_perf.py -l 240 --board stumpy
#
# compares the test runtime of all stumpy for the last 10 days. Sample output:
# ==============================================================================
# Test hardware_MemoryTotalSize
# ==============================================================================
# chromeos2-row2-rack8-host8  : min= 479, max= 479, mean= 479, med= 479, cnt= 1
# chromeos2-row2-rack8-host12 : min= 440, max= 440, mean= 440, med= 440, cnt= 1
# chromeos2-row2-rack8-host11 : min= 504, max= 504, mean= 504, med= 504, cnt= 1
#
# At the end of each row, it also lists the last 5 jobs running in the dut.


import argparse
import datetime
import multiprocessing.pool
import pprint
import time
from itertools import groupby

import common
import numpy
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.afe import rpc_utils
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


def get_matched_duts(hostnames=None, board=None, pool=None, other_labels=None):
    """Get duts with matching board and pool labels from given autotest instance

    @param hostnames: A list of hostnames.
    @param board: board of DUT, set to None if board doesn't need to match.
                  Default is None.
    @param pool: pool of DUT, set to None if pool doesn't need to match. Default
                 is None.
    @param other_labels: Other labels to filter duts.
    @return: A list of duts that match the specified board and pool.
    """
    if hostnames:
        hosts = models.Host.objects.filter(hostname__in=hostnames)
    else:
        multiple_labels = ()
        if pool:
            multiple_labels += ('pool:%s' % pool,)
        if board:
            multiple_labels += ('board:%s' % board,)
        if other_labels:
            for label in other_labels:
                multiple_labels += (label,)
        hosts = rpc_utils.get_host_query(multiple_labels,
                                         exclude_only_if_needed_labels=False,
                                         exclude_atomic_group_hosts=False,
                                         valid_only=True, filter_data={})
    return [host_obj.get_object_dict() for host_obj in hosts]


def get_job_runtime(input):
    """Get all test jobs and special tasks' runtime for a given host during
    a give time period.

    @param input: input arguments, including:
                  start_time: Start time of the search interval.
                  end_time: End time of the search interval.
                  host_id: id of the dut.
                  hostname: Name of the dut.
    @return: A list of records, e.g.,
                     [{'job_name':'dummy_Pass', 'time_used': 3, 'id': 12313,
                       'hostname': '1.2.3.4'},
                      {'task_name':'Cleanup', 'time_used': 30, 'id': 5687,
                       'hostname': '1.2.3.4'}]
    """
    start_time = input['start_time']
    end_time = input['end_time']
    host_id = input['host_id']
    hostname = input['hostname']
    records = []
    special_tasks = models.SpecialTask.objects.filter(
            host_id=host_id,
            time_started__gte=start_time,
            time_started__lte=end_time,
            time_started__isnull=False,
            time_finished__isnull=False).values('task', 'id', 'time_started',
                                                'time_finished')
    for task in special_tasks:
        time_used = task['time_finished'] - task['time_started']
        records.append({'name': task['task'],
                        'id': task['id'],
                        'time_used': time_used.total_seconds(),
                        'hostname': hostname})
    hqes = models.HostQueueEntry.objects.filter(
            host_id=host_id,
            started_on__gte=start_time,
            started_on__lte=end_time,
            started_on__isnull=False,
            finished_on__isnull=False)
    for hqe in hqes:
        time_used = (hqe.finished_on - hqe.started_on).total_seconds()
        records.append({'name': hqe.job.name.split('/')[-1],
                        'id': hqe.job.id,
                        'time_used': time_used,
                        'hostname': hostname})
    return records

def get_job_stats(jobs):
    """Get the stats of a list of jobs.

    @param jobs: A list of jobs.
    @return: Stats of the jobs' runtime, including:
             t_min: minimum runtime.
             t_max: maximum runtime.
             t_average: average runtime.
             t_median: median runtime.
    """
    runtimes = [job['time_used'] for job in jobs]
    t_min = min(runtimes)
    t_max = max(runtimes)
    t_mean = numpy.mean(runtimes)
    t_median = numpy.median(runtimes)
    return t_min, t_max, t_mean, t_median, len(runtimes)


def process_results(results):
    """Compare the results.

    @param results: A list of a list of job/task information.
    """
    # Merge list of all results.
    all_results = []
    for result in results:
        all_results.extend(result)
    all_results = sorted(all_results, key=lambda r: r['name'])
    for name,jobs_for_test in groupby(all_results, lambda r: r['name']):
        print '='*80
        print 'Test %s' % name
        print '='*80
        for hostname,jobs_for_dut in groupby(jobs_for_test,
                                             lambda j: j['hostname']):
            jobs = list(jobs_for_dut)
            t_min, t_max, t_mean, t_median, count = get_job_stats(jobs)
            ids = [str(job['id']) for job in jobs]
            print ('%-28s: min= %-3.0f max= %-3.0f mean= %-3.0f med= %-3.0f '
                   'cnt= %-3s IDs: %s' %
                   (hostname, t_min, t_max, t_mean, t_median, count,
                    ','.join(sorted(ids)[-5:])))


def main():
    """main script. """
    t_now = time.time()
    t_now_minus_one_day = t_now - 3600 * 24
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', type=float, dest='last',
                        help='last hours to search results across',
                        default=24)
    parser.add_argument('--board', type=str, dest='board',
                        help='restrict query by board',
                        default=None)
    parser.add_argument('--pool', type=str, dest='pool',
                        help='restrict query by pool',
                        default=None)
    parser.add_argument('--hosts', nargs='+', dest='hosts',
                        help='Enter space deliminated hostnames',
                        default=[])
    parser.add_argument('--start', type=str, dest='start',
                        help=('Enter start time as: yyyy-mm-dd hh-mm-ss,'
                              'defualts to 24h ago.'))
    parser.add_argument('--end', type=str, dest='end',
                        help=('Enter end time in as: yyyy-mm-dd hh-mm-ss,'
                              'defualts to current time.'))
    options = parser.parse_args()

    if not options.start or not options.end:
        end_time = datetime.datetime.now()
        start_time = end_time - datetime.timedelta(seconds=3600 * options.last)
    else:
        start_time = time_utils.time_string_to_datetime(options.start)
        end_time = time_utils.time_string_to_datetime(options.end)

    hosts = get_matched_duts(hostnames=options.hosts, board=options.board,
                             pool=options.pool)
    if not hosts:
        raise Exception('No host found to search for history.')
    print 'Found %d duts.' % len(hosts)
    print 'Start time: %s' % start_time
    print 'End time:   %s' % end_time
    args = []
    for host in hosts:
        args.append({'start_time': start_time,
                     'end_time': end_time,
                     'host_id': host['id'],
                     'hostname': host['hostname']})
    get_job_runtime(args[0])
    # Parallizing this process.
    pool = multiprocessing.pool.ThreadPool()
    results = pool.imap_unordered(get_job_runtime, args)
    process_results(results)


if __name__ == '__main__':
    main()

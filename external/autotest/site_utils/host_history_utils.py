# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file contains utility functions for host_history.

import collections
import copy
import multiprocessing.pool
from itertools import groupby

import common
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.site_utils import host_label_utils
from autotest_lib.site_utils import job_history


_HOST_HISTORY_TYPE = 'host_history'
_LOCK_HISTORY_TYPE = 'lock_history'

# The maximum number of days that the script will lookup for history.
_MAX_DAYS_FOR_HISTORY = 90

class NoHostFoundException(Exception):
    """Exception raised when no host is found to search for history.
    """


def get_matched_hosts(board, pool):
    """Get duts with matching board and pool labels from metaDB.

    @param board: board of DUT, set to None if board doesn't need to match.
    @param pool: pool of DUT, set to None if pool doesn't need to match.
    @return: A list of duts that match the specified board and pool.
    """
    labels = []
    if pool:
        labels.append('pool:%s' % pool)
    if board:
        labels.append('board:%s' % board)
    host_labels = host_label_utils.get_host_labels(labels=labels)
    return host_labels.keys()


def prepopulate_dict(keys, value, extras=None):
    """Creates a dictionary with val=value for each key.

    @param keys: list of keys
    @param value: the value of each entry in the dict.
    @param extras: list of additional keys
    @returns: dictionary
    """
    result = collections.OrderedDict()
    extra_keys = tuple(extras if extras else [])
    for key in keys + extra_keys:
        result[key] = value
    return result


def lock_history_to_intervals(initial_lock_val, t_start, t_end, lock_history):
    """Converts lock history into a list of intervals of locked times.

    @param initial_lock_val: Initial value of the lock (False or True)
    @param t_start: beginning of the time period we are interested in.
    @param t_end: end of the time period we are interested in.
    @param lock_history: Result of querying es for locks (dict)
           This dictionary should contain keys 'locked' and 'time_recorded'
    @returns: Returns a list of tuples where the elements of each tuples
           represent beginning and end of intervals of locked, respectively.
    """
    locked_intervals = []
    t_prev = t_start
    state_prev = initial_lock_val
    for entry in lock_history.hits:
        t_curr = entry['time_recorded']

        #If it is locked, then we put into locked_intervals
        if state_prev:
            locked_intervals.append((t_prev, t_curr))

        # update vars
        t_prev = t_curr
        state_prev = entry['locked']
    if state_prev:
        locked_intervals.append((t_prev, t_end))
    return locked_intervals


def find_most_recent_entry_before(t, type_str, hostname, fields):
    """Returns the fields of the most recent entry before t.

    @param t: time we are interested in.
    @param type_str: _type in esdb, such as 'host_history' (string)
    @param hostname: hostname of DUT (string)
    @param fields: list of fields we are interested in
    @returns: time, field_value of the latest entry.
    """
    # History older than 90 days are ignored. This helps the ES query faster.
    t_epoch = time_utils.to_epoch_time(t)
    result = autotest_es.query(
            fields_returned=fields,
            equality_constraints=[('_type', type_str),
                                  ('hostname', hostname)],
            range_constraints=[('time_recorded',
                               t_epoch-3600*24*_MAX_DAYS_FOR_HISTORY, t_epoch)],
            size=1,
            sort_specs=[{'time_recorded': 'desc'}])
    if result.total > 0:
        return result.hits[0]
    return {}


def get_host_history_intervals(input):
    """Gets stats for a host.

    This method uses intervals found in metaDB to build a full history of the
    host. The intervals argument contains a list of metadata from querying ES
    for records between t_start and t_end. To get the status from t_start to
    the first record logged in ES, we need to look back to the last record
    logged in ES before t_start.

    @param input: A dictionary of input args, which including following args:
            t_start: beginning of time period we are interested in.
            t_end: end of time period we are interested in.
            hostname: hostname for the host we are interested in (string)
            intervals: intervals from ES query.
    @returns: dictionary, num_entries_found
        dictionary of status: time spent in that status
        num_entries_found: number of host history entries
                           found in [t_start, t_end]

    """
    t_start = input['t_start']
    t_end = input['t_end']
    hostname = input['hostname']
    intervals = input['intervals']
    lock_history_recent = find_most_recent_entry_before(
            t=t_start, type_str=_LOCK_HISTORY_TYPE, hostname=hostname,
            fields=['time_recorded', 'locked'])
    # I use [0] and [None] because lock_history_recent's type is list.
    t_lock = lock_history_recent.get('time_recorded', None)
    t_lock_val = lock_history_recent.get('locked', None)
    t_metadata = find_most_recent_entry_before(
            t=t_start, type_str=_HOST_HISTORY_TYPE, hostname=hostname,
            fields=None)
    t_host = t_metadata.pop('time_recorded', None)
    t_host_stat = t_metadata.pop('status', None)
    status_first = t_host_stat if t_host else 'Ready'
    t = min([t for t in [t_lock, t_host, t_start] if t])

    t_epoch = time_utils.to_epoch_time(t)
    t_end_epoch = time_utils.to_epoch_time(t_end)
    lock_history_entries = autotest_es.query(
            fields_returned=['locked', 'time_recorded'],
            equality_constraints=[('_type', _LOCK_HISTORY_TYPE),
                                  ('hostname', hostname)],
            range_constraints=[('time_recorded', t_epoch, t_end_epoch)],
            sort_specs=[{'time_recorded': 'asc'}])

    # Validate lock history. If an unlock event failed to be recorded in metadb,
    # lock history will show the dut being locked while host still has status
    # changed over the time. This check tries to remove the lock event in lock
    # history if:
    # 1. There is only one entry in lock_history_entries (it's a good enough
    #    assumption to avoid the code being over complicated.
    # 2. The host status has changes after the lock history starts as locked.
    if (len(lock_history_entries.hits) == 1 and t_lock_val and
        len(intervals) >1):
        locked_intervals = None
        print ('Lock history of dut %s is ignored, the dut may have missing '
               'data in lock history in metadb. Try to lock and unlock the dut '
               'in AFE will force the lock history to be updated in metadb.'
               % hostname)
    else:
        locked_intervals = lock_history_to_intervals(t_lock_val, t, t_end,
                                                     lock_history_entries)
    num_entries_found = len(intervals)
    t_prev = t_start
    status_prev = status_first
    metadata_prev = t_metadata
    intervals_of_statuses = collections.OrderedDict()

    for entry in intervals:
        metadata = entry.copy()
        t_curr = metadata.pop('time_recorded')
        status_curr = metadata.pop('status')
        intervals_of_statuses.update(calculate_status_times(
                t_prev, t_curr, status_prev, metadata_prev, locked_intervals))
        # Update vars
        t_prev = t_curr
        status_prev = status_curr
        metadata_prev = metadata

    # Do final as well.
    intervals_of_statuses.update(calculate_status_times(
            t_prev, t_end, status_prev, metadata_prev, locked_intervals))
    return hostname, intervals_of_statuses, num_entries_found


def calculate_total_times(intervals_of_statuses):
    """Calculates total times in each status.

    @param intervals_of_statuses: ordereddict where key=(ti, tf) and val=status
    @returns: dictionary where key=status value=time spent in that status
    """
    total_times = prepopulate_dict(models.Host.Status.names, 0.0,
                                   extras=['Locked'])
    for key, status_info in intervals_of_statuses.iteritems():
        ti, tf = key
        total_times[status_info['status']] += tf - ti
    return total_times


def aggregate_hosts(intervals_of_statuses_list):
    """Aggregates history of multiple hosts

    @param intervals_of_statuses_list: A list of dictionaries where keys
        are tuple (ti, tf), and value is the status along with other metadata.
    @returns: A dictionary where keys are strings, e.g. 'status' and
              value is total time spent in that status among all hosts.
    """
    stats_all = prepopulate_dict(models.Host.Status.names, 0.0,
                                 extras=['Locked'])
    num_hosts = len(intervals_of_statuses_list)
    for intervals_of_statuses in intervals_of_statuses_list:
        total_times = calculate_total_times(intervals_of_statuses)
        for status, delta in total_times.iteritems():
            stats_all[status] += delta
    return stats_all, num_hosts


def get_stats_string_aggregate(labels, t_start, t_end, aggregated_stats,
                               num_hosts):
    """Returns string reporting overall host history for a group of hosts.

    @param labels: A list of labels useful for describing the group
                   of hosts these overall stats represent.
    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param aggregated_stats: A dictionary where keys are string, e.g. 'status'
        value is total time spent in that status among all hosts.
    @returns: string representing the aggregate stats report.
    """
    result = 'Overall stats for hosts: %s \n' % (', '.join(labels))
    result += ' %s - %s \n' % (time_utils.epoch_time_to_date_string(t_start),
                               time_utils.epoch_time_to_date_string(t_end))
    result += ' Number of total hosts: %s \n' % (num_hosts)
    # This is multiplied by time_spent to get percentage_spent
    multiplication_factor = 100.0 / ((t_end - t_start) * num_hosts)
    for status, time_spent in aggregated_stats.iteritems():
        # Normalize by the total time we are interested in among ALL hosts.
        spaces = ' ' * (15 - len(status))
        percent_spent = multiplication_factor * time_spent
        result += '    %s: %s %.2f %%\n' % (status, spaces, percent_spent)
    result += '- -- --- ---- ----- ---- --- -- -\n'
    return result


def get_overall_report(label, t_start, t_end, intervals_of_statuses_list):
    """Returns string reporting overall host history for a group of hosts.

    @param label: A string that can be useful for showing what type group
        of hosts these overall stats represent.
    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param intervals_of_statuses_list: A list of dictionaries where keys
        are tuple (ti, tf), and value is the status along with other metadata,
        e.g., task_id, task_name, job_id etc.
    """
    stats_all, num_hosts = aggregate_hosts(
            intervals_of_statuses_list)
    return get_stats_string_aggregate(
            label, t_start, t_end, stats_all, num_hosts)


def get_intervals_for_host(t_start, t_end, hostname):
    """Gets intervals for the given.

    Query metaDB to return all intervals between given start and end time.
    Note that intervals found in metaDB may miss the history from t_start to
    the first interval found.

    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param hosts: A list of hostnames to look for history.
    @param board: Name of the board to look for history. Default is None.
    @param pool: Name of the pool to look for history. Default is None.
    @returns: A dictionary of hostname: intervals.
    """
    t_start_epoch = time_utils.to_epoch_time(t_start)
    t_end_epoch = time_utils.to_epoch_time(t_end)
    host_history_entries = autotest_es.query(
                fields_returned=None,
                equality_constraints=[('_type', _HOST_HISTORY_TYPE),
                                      ('hostname', hostname)],
                range_constraints=[('time_recorded', t_start_epoch,
                                    t_end_epoch)],
                sort_specs=[{'time_recorded': 'asc'}])
    return host_history_entries.hits


def get_intervals_for_hosts(t_start, t_end, hosts=None, board=None, pool=None):
    """Gets intervals for given hosts or board/pool.

    Query metaDB to return all intervals between given start and end time.
    If a list of hosts is provided, the board and pool constraints are ignored.
    If hosts is set to None, and board or pool is set, this method will attempt
    to search host history with labels for all hosts, to help the search perform
    faster.
    If hosts, board and pool are all set to None, return intervals for all
    hosts.
    Note that intervals found in metaDB may miss the history from t_start to
    the first interval found.

    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param hosts: A list of hostnames to look for history.
    @param board: Name of the board to look for history. Default is None.
    @param pool: Name of the pool to look for history. Default is None.
    @returns: A dictionary of hostname: intervals.
    """
    hosts_intervals = {}
    if hosts:
        for host in hosts:
            hosts_intervals[host] = get_intervals_for_host(t_start, t_end, host)
    else:
        hosts = get_matched_hosts(board, pool)
        if not hosts:
            raise NoHostFoundException('No host is found for board:%s, pool:%s.'
                                       % (board, pool))
        equality_constraints=[('_type', _HOST_HISTORY_TYPE),]
        if board:
            equality_constraints.append(('labels', 'board:'+board))
        if pool:
            equality_constraints.append(('labels', 'pool:'+pool))
        t_start_epoch = time_utils.to_epoch_time(t_start)
        t_end_epoch = time_utils.to_epoch_time(t_end)
        results =  autotest_es.query(
                equality_constraints=equality_constraints,
                range_constraints=[('time_recorded', t_start_epoch,
                                    t_end_epoch)],
                sort_specs=[{'hostname': 'asc'}])
        results_group_by_host = {}
        for hostname,intervals_for_host in groupby(results.hits,
                                                   lambda h: h['hostname']):
            results_group_by_host[hostname] = intervals_for_host
        for host in hosts:
            intervals = results_group_by_host.get(host, None)
            # In case the host's board or pool label was modified after
            # the last status change event was reported, we need to run a
            # separate query to get its history. That way the host's
            # history won't be shown as blank.
            if not intervals:
                intervals = get_intervals_for_host(t_start, t_end, host)
            hosts_intervals[host] = intervals
    return hosts_intervals


def get_report(t_start, t_end, hosts=None, board=None, pool=None,
                print_each_interval=False):
    """Gets history for given hosts or board/pool

    If a list of hosts is provided, the board and pool constraints are ignored.

    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param hosts: A list of hostnames to look for history.
    @param board: Name of the board to look for history. Default is None.
    @param pool: Name of the pool to look for history. Default is None.
    @param print_each_interval: True display all intervals, default is False.
    @returns: stats report for this particular host. The report is a list of
              tuples (stat_string, intervals, hostname), intervals is a sorted
              dictionary.
    """
    if hosts:
        board=None
        pool=None

    hosts_intervals = get_intervals_for_hosts(t_start, t_end, hosts, board,
                                              pool)
    history = {}
    pool = multiprocessing.pool.ThreadPool(processes=16)
    args = []
    for hostname,intervals in hosts_intervals.items():
        args.append({'t_start': t_start,
                     't_end': t_end,
                     'hostname': hostname,
                     'intervals': intervals})
    results = pool.imap_unordered(get_host_history_intervals, args)
    for hostname, intervals, count in results:
        history[hostname] = (intervals, count)
    report = []
    for hostname,intervals in history.items():
        total_times = calculate_total_times(intervals[0])
        stats = get_stats_string(
                t_start, t_end, total_times, intervals[0], hostname,
                intervals[1], print_each_interval)
        report.append((stats, intervals[0], hostname))
    return report


def get_report_for_host(t_start, t_end, hostname, print_each_interval):
    """Gets stats report for a host

    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param hostname: hostname for the host we are interested in (string)
    @param print_each_interval: True or False, whether we want to
                                display all intervals
    @returns: stats report for this particular host (string)
    """
    # Search for status change intervals during given time range.
    intervals = get_intervals_for_host(t_start, t_end, hostname)
    num_entries_found = len(intervals)
    # Update the status change intervals with status before the first entry and
    # host's lock history.
    _, intervals_of_statuses = get_host_history_intervals(
            {'t_start': t_start,
             't_end': t_end,
             'hostname': hostname,
             'intervals': intervals})
    total_times = calculate_total_times(intervals_of_statuses)
    return (get_stats_string(
                    t_start, t_end, total_times, intervals_of_statuses,
                    hostname, num_entries_found, print_each_interval),
                    intervals_of_statuses)


def get_stats_string(t_start, t_end, total_times, intervals_of_statuses,
                     hostname, num_entries_found, print_each_interval):
    """Returns string reporting host_history for this host.
    @param t_start: beginning of time period we are interested in.
    @param t_end: end of time period we are interested in.
    @param total_times: dictionary where key=status,
                        value=(time spent in that status)
    @param intervals_of_statuses: dictionary where keys is tuple (ti, tf),
              and value is the status along with other metadata.
    @param hostname: hostname for the host we are interested in (string)
    @param num_entries_found: Number of entries found for the host in es
    @param print_each_interval: boolean, whether to print each interval
    """
    delta = t_end - t_start
    result = 'usage stats for host: %s \n' % (hostname)
    result += ' %s - %s \n' % (time_utils.epoch_time_to_date_string(t_start),
                               time_utils.epoch_time_to_date_string(t_end))
    result += ' Num entries found in this interval: %s\n' % (num_entries_found)
    for status, value in total_times.iteritems():
        spaces = (15 - len(status)) * ' '
        result += '    %s: %s %.2f %%\n' % (status, spaces, 100*value/delta)
    result += '- -- --- ---- ----- ---- --- -- -\n'
    if print_each_interval:
        for interval, status_info in intervals_of_statuses.iteritems():
            t0, t1 = interval
            t0_string = time_utils.epoch_time_to_date_string(t0)
            t1_string = time_utils.epoch_time_to_date_string(t1)
            status = status_info['status']
            delta = int(t1-t0)
            id_info = status_info['metadata'].get(
                    'task_id', status_info['metadata'].get('job_id', ''))
            result += ('    %s  :  %s %-15s %-10s %ss\n' %
                       (t0_string, t1_string, status, id_info, delta))
    return result


def calculate_status_times(t_start, t_end, int_status, metadata,
                           locked_intervals):
    """Returns a list of intervals along w/ statuses associated with them.

    If the dut is in status Ready, i.e., int_status==Ready, the lock history
    should be applied so that the time period when dut is locked is considered
    as not available. Any other status is considered that dut is doing something
    and being used. `Repair Failed` and Repairing are not checked with lock
    status, since these two statuses indicate the dut is not available any way.

    @param t_start: start time
    @param t_end: end time
    @param int_status: status of [t_start, t_end] if not locked
    @param metadata: metadata of the status change, e.g., task_id, task_name.
    @param locked_intervals: list of tuples denoting intervals of locked states
    @returns: dictionary where key = (t_interval_start, t_interval_end),
                               val = (status, metadata)
              t_interval_start: beginning of interval for that status
              t_interval_end: end of the interval for that status
              status: string such as 'Repair Failed', 'Locked', etc.
              metadata: A dictionary of metadata, e.g.,
                              {'task_id':123, 'task_name':'Reset'}
    """
    statuses = collections.OrderedDict()

    prev_interval_end = t_start

    # TODO: Put allow more information here in info/locked status
    status_info = {'status': int_status,
                   'metadata': metadata}
    locked_info = {'status': 'Locked',
                   'metadata': {}}
    if not locked_intervals:
        statuses[(t_start, t_end)] = status_info
        return statuses
    for lock_start, lock_end in locked_intervals:
        if prev_interval_end >= t_end:
            break
        if lock_start > t_end:
            # optimization to break early
            # case 0
            # Timeline of status change: t_start t_end
            # Timeline of lock action:                   lock_start lock_end
            break
        elif lock_end < prev_interval_end:
            # case 1
            #                      prev_interval_end    t_end
            # lock_start lock_end
            continue
        elif lock_end <= t_end and lock_start >= prev_interval_end:
            # case 2
            # prev_interval_end                       t_end
            #                    lock_start lock_end
            # Lock happened in the middle, while the host stays in the same
            # status, consider the lock has no effect on host history.
            statuses[(prev_interval_end, lock_end)] = status_info
            prev_interval_end = lock_end
        elif lock_end > prev_interval_end and lock_start < prev_interval_end:
            # case 3
            #             prev_interval_end          t_end
            # lock_start                    lock_end        (or lock_end)
            # If the host status changed in the middle of being locked, consider
            # the new status change as part of the host history.
            statuses[(prev_interval_end, min(lock_end, t_end))] = locked_info
            prev_interval_end = lock_end
        elif lock_start < t_end and lock_end > t_end:
            # case 4
            # prev_interval_end             t_end
            #                    lock_start        lock_end
            # If the lock happens in the middle of host status change, consider
            # the lock has no effect on the host history for that status.
            statuses[(prev_interval_end, t_end)] = status_info
            statuses[(lock_start, t_end)] = locked_info
            prev_interval_end = t_end
        # Otherwise we are in the case where lock_end < t_start OR
        # lock_start > t_end, which means the lock doesn't apply.
    if t_end > prev_interval_end:
        # This is to avoid logging the same time
        statuses[(prev_interval_end, t_end)] = status_info
    return statuses


def get_log_url(hostname, metadata):
    """Compile a url to job's debug log from debug string.

    @param hostname: Hostname of the dut.
    @param metadata: A dictionary of other metadata, e.g.,
                                     {'task_id':123, 'task_name':'Reset'}
    @return: Url of the debug log for special task or job url for test job.
    """
    log_url = None
    if 'task_id' in metadata and 'task_name' in metadata:
        log_url = job_history.TASK_URL % {'hostname': hostname,
                                          'task_id': metadata['task_id'],
                                          'task_name': metadata['task_name']}
    elif 'job_id' in metadata and 'owner' in metadata:
        log_url = job_history.JOB_URL % {'hostname': hostname,
                                         'job_id': metadata['job_id'],
                                         'owner': metadata['owner']}

    return log_url


def build_history(hostname, status_intervals):
    """Get host history information from given state intervals.

    @param hostname: Hostname of the dut.
    @param status_intervals: A ordered dictionary with
                    key as (t_start, t_end) and value as (status, metadata)
                    status = status of the host. e.g. 'Repair Failed'
                    t_start is the beginning of the interval where the DUT's has
                            that status
                    t_end is the end of the interval where the DUT has that
                            status
                    metadata: A dictionary of other metadata, e.g.,
                                        {'task_id':123, 'task_name':'Reset'}
    @return: A list of host history, e.g.,
             [{'status': 'Resetting'
               'start_time': '2014-08-07 10:02:16',
               'end_time': '2014-08-07 10:03:16',
               'log_url': 'http://autotest/reset-546546/debug',
               'task_id': 546546},
              {'status': 'Running'
               'start_time': '2014-08-07 10:03:18',
               'end_time': '2014-08-07 10:13:00',
               'log_url': 'http://autotest/afe/#tab_id=view_job&object_id=1683',
               'job_id': 1683}
             ]
    """
    history = []
    for time_interval, status_info in status_intervals.items():
        start_time = time_utils.epoch_time_to_date_string(time_interval[0])
        end_time = time_utils.epoch_time_to_date_string(time_interval[1])
        interval = {'status': status_info['status'],
                    'start_time': start_time,
                    'end_time': end_time}
        interval['log_url'] = get_log_url(hostname, status_info['metadata'])
        interval.update(status_info['metadata'])
        history.append(interval)
    return history


def get_status_intervals(history_details):
    """Get a list of status interval from history details.

    This is a reverse method of above build_history. Caller gets the history
    details from RPC get_host_history, and use this method to get the list of
    status interval, which can be used to calculate stats from
    host_history_utils.aggregate_hosts.

    @param history_details: A dictionary of host history for each host, e.g.,
            {'172.22.33.51': [{'status': 'Resetting'
                               'start_time': '2014-08-07 10:02:16',
                               'end_time': '2014-08-07 10:03:16',
                               'log_url': 'http://autotest/reset-546546/debug',
                               'task_id': 546546},]
            }
    @return: A list of dictionaries where keys are tuple (start_time, end_time),
             and value is a dictionary containing at least key 'status'.
    """
    status_intervals = []
    for host,history in history_details.iteritems():
        intervals = collections.OrderedDict()
        for interval in history:
            start_time = time_utils.to_epoch_time(interval['start_time'])
            end_time = time_utils.to_epoch_time(interval['end_time'])
            metadata = copy.deepcopy(interval)
            metadata['hostname'] = host
            intervals[(start_time, end_time)] = {'status': interval['status'],
                                                 'metadata': metadata}
        status_intervals.append(intervals)
    return status_intervals


def get_machine_utilization_rate(stats):
    """Get machine utilization rate from given stats.

    @param stats: A dictionary with a status as key and value is the total
                  number of seconds spent on the status.
    @return: The percentage of time when dut is running test jobs.
    """
    not_utilized_status = ['Repairing', 'Repair Failed', 'Ready', 'Verifying']
    excluded_status = ['Locked']
    total_time = 0
    total_time_not_utilized = 0.0
    for status, interval in stats.iteritems():
        if status in excluded_status:
            continue
        total_time += interval
        if status in not_utilized_status:
            total_time_not_utilized += interval
    if total_time == 0:
        # All duts are locked, assume MUR is 0%
        return 0
    else:
        return 1 - total_time_not_utilized/total_time


def get_machine_availability_rate(stats):
    """Get machine availability rate from given stats.

    @param stats: A dictionary with a status as key and value is the total
                  number of seconds spent on the status.
    @return: The percentage of time when dut is available to run jobs.
    """
    not_available_status = ['Repairing', 'Repair Failed', 'Verifying']
    excluded_status = ['Locked']
    total_time = 0
    total_time_not_available = 0.0
    for status, interval in stats.iteritems():
        if status in excluded_status:
            continue
        total_time += interval
        if status in not_available_status:
            total_time_not_available += interval
    if total_time == 0:
        # All duts are locked, assume MAR is 0%
        return 0
    else:
        return 1 - total_time_not_available/total_time

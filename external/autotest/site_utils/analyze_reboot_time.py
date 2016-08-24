#!/usr/bin/env python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file defines script for getting entries from ES concerning reboot time.

"""
Example usage:
    python analyze_reboot_time.py -l 12 --server cautotest --board daisy_spring

Usage: analyze_reboot_time.py [-h] [-l LAST] --server AUTOTEST_SERVER
                              [--board BOARD] [--pool POOL] [--start START]
                              [--end END] [--gte GTE] [--lte LTE] [-n SIZE]
                              [--hosts HOSTS [HOSTS ...]]

optional arguments:
  -h, --help            show this help message and exit
  -l LAST               last hours to search results across
  --server AUTOTEST_SERVER
                        Enter Autotest instance name, e.g. "cautotest".
  --board BOARD         restrict query by board, not implemented yet
  --pool POOL           restrict query by pool, not implemented yet
  --start START         Enter start time as: yyyy-mm-dd hh-mm-ss,defualts to
                        24h ago.
  --end END             Enter end time as: yyyy-mm-dd hh-mm-ss,defualts to
                        current time.
  --gte GTE             Enter lower bound on reboot time for entries to
                        return.
  --lte LTE             Enter upper bound on reboot time for entries to
                        return.
  -n SIZE               Maximum number of entries to return.
  --hosts HOSTS [HOSTS ...]
                        Enter space deliminated hostnames
"""

import argparse
import time

import common
import host_history
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_es


def get_entries(time_start, time_end, gte, lte, size, index, hostname):
    """Gets all entries from es db with the given constraints.

    @param time_start: Earliest time entry was recorded
    @param time_end: Latest time entry was recorded
    @param gte: Lowest reboot_time to return
    @param lte: Highest reboot_time to return
    @param size: Max number of entries to return
    @param index: es db index to get entries for, i.e. 'cautotest'
    @param hostname: string representing hostname to query for
    @returns: Entries from esdb.
    """
    time_start_epoch = time_utils.to_epoch_time(time_start)
    time_end_epoch = time_utils.to_epoch_time(time_end)
    gte_epoch = time_utils.to_epoch_time(gte)
    lte_epoch = time_utils.to_epoch_time(lte)
    return autotest_es.query(
        index=index,
        fields_returned=['hostname', 'time_recorded', 'value'],
        equality_constraints=[('_type', 'reboot_total'),
                              ('hostname', hostname)],
        range_constraints=[('time_recorded', time_start_epoch, time_end_epoch),
                           ('value', gte_epoch, lte_epoch)],
        size=size,
        sort_specs=[{'hostname': 'asc'}, {'value': 'desc'}])
    return results


def get_results_string(hostname, time_start, time_end, results):
    """Prints entries from esdb in a readable fashion.

    @param hostname: Hostname of DUT we are printing result for.
    @param time_start: Earliest time entry was recorded
    @param time_end: Latest time entry was recorded
    @param gte: Lowest reboot_time to return
    @param lte: Highest reboot_time to return
    @param size: Max number of entries to return
    @returns: String reporting reboot times for this host.
    """
    return_string = ' Host: %s \n   Number of entries: %s \n' % (
            hostname, results.total)
    return_string += ' %s - %s \n' % (
            time_utils.epoch_time_to_date_string(time_start),
            time_utils.epoch_time_to_date_string(time_end))
    if results.total <= 0:
        return return_string
    for result in results.hits:
        time_recorded = result['time_recorded'][0]
        time_string = time_utils.epoch_time_to_date_string(
                time_recorded)
        reboot_total = result['value'][0]
        spaces = (15 - len(str(time_string))) * ' '
        return_string += '    %s  Reboot_time:  %.3fs\n' % (
                time_string, reboot_total)
    return return_string


if __name__ == '__main__':
    """main script"""
    t_now = time.time()
    t_now_minus_one_day = t_now - 3600 * 24
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', type=float, dest='last',
                        help='last hours to search results across',
                        default=24)
    parser.add_argument('--server', type=str, dest='autotest_server',
                        required=True,
                        help='Enter Autotest instance name, e.g. "cautotest".')
    parser.add_argument('--board', type=str, dest='board',
                        help='restrict query by board, not implemented yet',
                        default=None)
    parser.add_argument('--pool', type=str, dest='pool',
                        help='restrict query by pool, not implemented yet',
                        default=None)
    parser.add_argument('--start', type=str, dest='start',
                        help=('Enter start time as: yyyy-mm-dd hh-mm-ss,'
                              'defualts to 24h ago.'),
                        default=t_now_minus_one_day)
    parser.add_argument('--end', type=str, dest='end',
                        help=('Enter end time as: yyyy-mm-dd hh-mm-ss,'
                              'defualts to current time.'),
                        default=t_now)
    parser.add_argument('--gte', type=float, dest='gte',
                        help=('Enter lower bound on reboot time '
                              'for entries to return.'),
                        default=0)
    parser.add_argument('--lte', type=float, dest='lte',
                        help=('Enter upper bound on reboot time '
                              'for entries to return.'),
                        default=None)
    parser.add_argument('-n', type=int, dest='size',
                        help='Maximum number of entries to return.',
                        default=10000)
    parser.add_argument('--hosts', nargs='+', dest='hosts',
                        help='Enter space deliminated hostnames',
                        default=[])
    options = parser.parse_args()

    if options.last:
        t_start = t_now - 3600 * options.last
        t_end = t_now
    else:
        t_start = time_utils.to_epoch_time(options.start)
        t_end = time_utils.to_epoch_time(options.end)
    if options.hosts:
        hosts = options.hosts
    else:
        hosts = host_history.get_matched_hosts(options.autotest_server,
                                               options.board, options.pool)

    for hostname in hosts:
        results = get_entries(
                t_start, t_end, options.gte, options.lte, options.size,
                options.autotest_server, hostname)
        print get_results_string(hostname, t_start, t_end, results)

#!/usr/bin/env python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is to be run daily to report machine utilization stats across
each board and pool.
"""


import argparse
from datetime import date
from datetime import datetime
from datetime import timedelta

import common
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.site_utils import gmail_lib
from autotest_lib.site_utils import host_history
from autotest_lib.site_utils import host_history_utils
from autotest_lib.site_utils import host_label_utils


def report_stats(board, pool, start_time, end_time, span):
    """Report machine stats for given board, pool and time period.

    @param board: Name of board.
    @param pool: Name of pool.
    @param start_time: start time to collect stats.
    @param end_time: end time to collect stats.
    @param span: Number of hours that the stats should be collected for.
    @return: Error message collected when calculating the stats.
    """
    print '================ %-12s %-12s ================' % (board, pool)
    try:
        history = host_history.get_history_details(start_time=start_time,
                                                   end_time=end_time,
                                                   board=board,
                                                   pool=pool)
    except host_history_utils.NoHostFoundException as e:
        print 'No history found. Error:\n%s' % e
        history = None
        mur = -1
        mar = -1
        mir = -1

    if history:
        status_intervals = host_history_utils.get_status_intervals(history)
        stats_all, num_hosts = host_history_utils.aggregate_hosts(
                status_intervals)
        total = 0
        total_time = span*3600*num_hosts
        for status, interval in stats_all.iteritems():
            total += interval
        if abs(total - total_time) > 10:
            error = ('Status intervals do not add up. No stats will be '
                     'collected for board: %s, pool: %s, diff: %s' %
                     (board, pool, total - total_time))
            hosts = []
            for history_for_host in status_intervals:
                total = 0
                for interval in history_for_host.keys():
                    total += interval[1] - interval[0]
                if total > span*3600:
                    hosts.append(history_for_host.values()[0]['metadata']['hostname'])
            error += ' hosts: %s' % ','.join(hosts)
            print error
            return error

        mur = host_history_utils.get_machine_utilization_rate(stats_all)
        mar = host_history_utils.get_machine_availability_rate(stats_all)
        mir = mar - mur

        for status, interval in stats_all.iteritems():
            print '%-18s %-16s %-10.2f%%' % (status, interval,
                                             100*interval/total_time)
        print 'Machine utilization rate  = %-4.2f%%' % (100*mur)
        print 'Machine availability rate = %-4.2f%%' % (100*mar)

    autotest_stats.Gauge('machine_utilization_rate').send('%s_hours.%s.%s' %
                                                          (span, board, pool),
                                                          mur)
    autotest_stats.Gauge('machine_availability_rate').send('%s_hours.%s.%s' %
                                                           (span, board, pool),
                                                           mar)
    autotest_stats.Gauge('machine_idle_rate').send('%s_hours.%s.%s' %
                                                   (span, board, pool), mir)


def main():
    """main script. """
    parser = argparse.ArgumentParser()
    parser.add_argument('--span', type=int, dest='span', default=1,
                        help=('Number of hours that stats should be collected. '
                              'If it is set to 24, the end time of stats being '
                              'collected will set to the mid of the night. '
                              'Default is set to 1 hour.'))
    parser.add_argument('-e', '--email', dest='email', default=None,
                        help='Email any errors to the given email address.')
    options = parser.parse_args()

    boards = host_label_utils.get_all_boards()
    pools = ['bvt', 'suites', 'cq']

    if options.span == 24:
        today = datetime.combine(date.today(), datetime.min.time())
        end_time = time_utils.to_epoch_time(today)
    else:
        now = datetime.now()
        end_time = datetime(year=now.year, month=now.month, day=now.day,
                            hour=now.hour)
        end_time = time_utils.to_epoch_time(end_time)

    start_time = end_time - timedelta(hours=options.span).total_seconds()
    print ('Collecting host stats from %s to %s...' %
           (time_utils.epoch_time_to_date_string(start_time),
            time_utils.epoch_time_to_date_string(end_time)))

    errors = []
    if not boards:
        errors.append('Error! No board found in metadb.')
    for board in boards:
        for pool in pools:
            error = report_stats(board, pool, start_time, end_time,
                                 options.span)
            if error:
                errors.append(error)
    if options.email and errors:
        gmail_lib.send_email(options.email,
                             'Error occured when collecting host stats.',
                             '\n'.join(errors))


if __name__ == '__main__':
    main()

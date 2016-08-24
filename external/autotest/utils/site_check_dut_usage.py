#!/usr/bin/python

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Tool to check DUT usage by querying the Autotest DB.

Sample usage:

utils/site_check_dut_usage.py 11/1/2011 11/5/2011 netbook_LABEL
"""

import datetime
import optparse
import sys

import common
from autotest_lib.database import database_connection

_DATE_FORMAT = '%m/%d/%Y'


class CheckDutUsageRunner(object):
    """Checks DUT usage for given label or hostname during a time period."""

    def __init__(self, start_time, end_time, label, hostname, list_hostnames):
        """
        Instantiates a CheckDUTUsageRunner.

        @start_time: start date of time period we are interested in.
        @end_time: end date of time period we are interested in.  Note the
            time period is (start_date, end_date].
        @label: If not None, the platform label of the hostnames we are
            interested in.
        @hostname: If not None, the hostname we are intersted in.
        @list_hostnames: If set, print out the list of hostnames found that ran
            jobs during the given time period.
        """
        self._start_time = start_time
        self._end_time = end_time
        self._list_hostnames = list_hostnames
        self._label = label
        self._hostname = hostname
        self._database_connection = None


    def find_all_durations(self):
        """
        Returns all list of tuples containing durations.

        A duration is a 4-tuple containing |queued_time|, |started_time|,
        |finished_time|, |hostname|.
        """
        query = ('select queued_time, started_time, finished_time, '
                 '  hostname '
                 'from tko_jobs left join tko_machines on '
                 '  tko_jobs.machine_idx=tko_machines.machine_idx '
                 'where tko_jobs.started_time>=DATE(%s) and '
                 '  tko_jobs.finished_time<DATE(%s)')
        if self._label:
            query += ' and tko_machines.machine_group=%s'
            filter_value = self._label
        else:
            query += ' and tko_machines.hostname=%s'
            filter_value = self._hostname

        results = self._database_connection.execute(
                query, [self._start_time, self._end_time, filter_value])
        return results


    @staticmethod
    def _total_seconds(time_delta):
        """
        Returns a float that has the total seconds in a datetime.timedelta.
        """
        return float(time_delta.days * 86400 + time_delta.seconds)


    def calculate_usage(self, durations):
        """
        Calculates and prints out usage information given list of durations.
        """
        total_run_time = datetime.timedelta()
        total_queued_time = datetime.timedelta()
        machines = set()
        for q_time, s_time, f_time, machine in durations:
            total_run_time += f_time - s_time
            total_queued_time += s_time - q_time
            machines.add(machine)

        num_machines = len(machines)
        avg_run_time = total_run_time / num_machines
        avg_job_run_time = self._total_seconds(total_run_time) / len(durations)
        avg_job_queued_time = (self._total_seconds(total_queued_time) /
                               len(durations))
        duration = self._end_time - self._start_time
        usage = self._total_seconds(avg_run_time) / self._total_seconds(
                duration)

        # Print the list of hostnames if the user requested.
        if self._list_hostnames:
            print '=================================================='
            print 'Machines with label:'
            for machine in machines:
                print machine
            print '=================================================='

        # Print the usage summary.
        print '=================================================='
        print 'Total running time', total_run_time
        print 'Total queued time', total_queued_time
        print 'Total number of machines', num_machines
        print 'Average time spent running tests per machine ', avg_run_time
        print 'Average Job Time ', datetime.timedelta(seconds=int(
                avg_job_run_time))
        print 'Average Time Job Queued ', datetime.timedelta(seconds=int(
                avg_job_queued_time))
        print 'Total duration ', duration
        print 'Usage ', usage
        print '=================================================='


    def run(self):
        """Connects to SQL DB and calculates DUT usage given args."""
        # Force the database connection to use the read the readonly options.
        database_connection._GLOBAL_CONFIG_NAMES.update(
                {'username': 'readonly_user',
                 'password': 'readonly_password',
                })
        self._database_connection = database_connection.DatabaseConnection(
                global_config_section='AUTOTEST_WEB')
        self._database_connection.connect()

        durations = self.find_all_durations()
        if not durations:
            print 'Query returned no results.'
        else:
            self.calculate_usage(durations)

        self._database_connection.disconnect()


def parse_args(options, args, parser):
    """Returns a tuple containing start time, end time, and label, hostname."""
    label, hostname = None, None

    if len(args) != 4:
        parser.error('Should have exactly 3 arguments.')

    if options.hostname:
        hostname = args[-1]
    else:
        label = args[-1]

    start_time, end_time = args[1:3]
    return (datetime.datetime.strptime(start_time, _DATE_FORMAT).date(),
            datetime.datetime.strptime(end_time, _DATE_FORMAT).date(),
            label, hostname)


def main(argv):
    """Main method.  Parses options and runs main program."""
    usage = ('usage: %prog [options] start_date end_date platform_Label|'
             'hostname')
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('--hostname', action='store_true', default=False,
                      help='If set, interpret argument as hostname.')
    parser.add_option('--list', action='store_true', default=False,
                      help='If set, print out list of hostnames with '
                      'the given label that ran jobs during this time.')
    options, args = parser.parse_args(argv)

    start_time, end_time, label, hostname = parse_args(options, args, parser)
    runner = CheckDutUsageRunner(start_time, end_time, label, hostname,
                                 options.list)
    runner.run()


if __name__ == '__main__':
    main(sys.argv)

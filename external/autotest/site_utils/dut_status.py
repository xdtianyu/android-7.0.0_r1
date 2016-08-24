#!/usr/bin/env python
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Report whether DUTs are working or broken.

usage: dut_status [ <options> ] [hostname ...]

Reports on the history and status of selected DUT hosts, to
determine whether they're "working" or "broken".  For purposes of
the script, "broken" means "the DUT requires manual intervention
before it can be used for further testing", and "working" means "not
broken".  The status determination is based on the history of
completed jobs for the DUT in a given time interval; still-running
jobs are not considered.

Time Interval Selection
~~~~~~~~~~~~~~~~~~~~~~~
A DUT's reported status is based on the DUT's job history in a time
interval determined by command line options.  The interval is
specified with up to two of three options:
  --until/-u DATE/TIME - Specifies an end time for the search
      range.  (default: now)
  --since/-s DATE/TIME - Specifies a start time for the search
      range. (no default)
  --duration/-d HOURS - Specifies the length of the search interval
      in hours. (default: 24 hours)

Any two time options completely specify the time interval.  If only
one option is provided, these defaults are used:
  --until - Use the given end time with the default duration.
  --since - Use the given start time with the default end time.
  --duration - Use the given duration with the default end time.

If no time options are given, use the default end time and duration.

DATE/TIME values are of the form '2014-11-06 17:21:34'.

DUT Selection
~~~~~~~~~~~~~
By default, information is reported for DUTs named as command-line
arguments.  Options are also available for selecting groups of
hosts:
  --board/-b BOARD - Only include hosts with the given board.
  --pool/-p POOL - Only include hosts in the given pool.

The selected hosts may also be filtered based on status:
  -w/--working - Only include hosts in a working state.
  -n/--broken - Only include hosts in a non-working state.  Hosts
      with no job history are considered non-working.

Output Formats
~~~~~~~~~~~~~~
There are four available output formats:
  * A simple list of host names.
  * A status summary showing one line per host.
  * A detailed job history for all selected DUTs, sorted by
    time of execution.
  * A job history for all selected DUTs showing only the history
    surrounding the DUT's last change from working to broken,
    or vice versa.

The default format depends on whether hosts are filtered by
status:
  * With the --working or --broken options, the list of host names
    is the default format.
  * Without those options, the default format is the one-line status
    summary.

These options override the default formats:
  -o/--oneline - Use the one-line summary with the --working or
      --broken options.
  -f/--full_history - Print detailed per-host job history.
  -g/--diagnosis - Print the job history surrounding a status
      change.

Examples
~~~~~~~~
    $ dut_status chromeos2-row4-rack2-host12
    hostname                     S   last checked         URL
    chromeos2-row4-rack2-host12  NO  2014-11-06 15:25:29  http://...

'NO' means the DUT is broken.  That diagnosis is based on a job that
failed:  'last checked' is the time of the failed job, and the URL
points to the job's logs.

    $ dut_status.py -u '2014-11-06 15:30:00' -d 1 -f chromeos2-row4-rack2-host12
    chromeos2-row4-rack2-host12
        2014-11-06 15:25:29  NO http://...
        2014-11-06 14:44:07  -- http://...
        2014-11-06 14:42:56  OK http://...

The times are the start times of the jobs; the URL points to the
job's logs.  The status indicates the working or broken status after
the job:
  'NO' Indicates that the DUT was believed broken after the job.
  'OK' Indicates that the DUT was believed working after the job.
  '--' Indicates that the job probably didn't change the DUT's
       status.
Typically, logs of the actual failure will be found at the last job
to report 'OK', or the first job to report '--'.

"""

import argparse
import sys
import time

import common
from autotest_lib.client.common_lib import time_utils
from autotest_lib.server import frontend
from autotest_lib.site_utils import status_history


# The fully qualified name makes for lines that are too long, so
# shorten it locally.
HostJobHistory = status_history.HostJobHistory

# _DIAGNOSIS_IDS -
#     Dictionary to map the known diagnosis codes to string values.

_DIAGNOSIS_IDS = {
    status_history.UNUSED: '??',
    status_history.UNKNOWN: '--',
    status_history.WORKING: 'OK',
    status_history.BROKEN: 'NO'
}


# Default time interval for the --duration option when a value isn't
# specified on the command line.
_DEFAULT_DURATION = 24


def _include_status(status, arguments):
    """Determine whether the given status should be filtered.

    Checks the given `status` against the command line options in
    `arguments`.  Return whether a host with that status should be
    printed based on the options.

    @param status Status of a host to be printed or skipped.
    @param arguments Parsed arguments object as returned by
                     ArgumentParser.parse_args().

    @return Returns `True` if the command-line options call for
            printing hosts with the status, or `False` otherwise.

    """
    if status == status_history.WORKING:
        return arguments.working
    else:
        return arguments.broken


def _print_host_summaries(history_list, arguments):
    """Print one-line summaries of host history.

    This function handles the output format of the --oneline option.

    @param history_list A list of HostHistory objects to be printed.
    @param arguments    Parsed arguments object as returned by
                        ArgumentParser.parse_args().

    """
    fmt = '%-30s %-2s  %-19s  %s'
    print fmt % ('hostname', 'S', 'last checked', 'URL')
    for history in history_list:
        status, event = history.last_diagnosis()
        if not _include_status(status, arguments):
            continue
        datestr = '---'
        url = '---'
        if event is not None:
            datestr = time_utils.epoch_time_to_date_string(
                    event.start_time)
            url = event.job_url

        print fmt % (history.hostname,
                     _DIAGNOSIS_IDS[status],
                     datestr,
                     url)


def _print_event_summary(event):
    """Print a one-line summary of a job or special task."""
    start_time = time_utils.epoch_time_to_date_string(
            event.start_time)
    print '    %s  %s %s' % (
            start_time,
            _DIAGNOSIS_IDS[event.diagnosis],
            event.job_url)


def _print_hosts(history_list, arguments):
    """Print hosts, optionally with a job history.

    This function handles both the default format for --working
    and --broken options, as well as the output for the
    --full_history and --diagnosis options.  The `arguments`
    parameter determines the format to use.

    @param history_list A list of HostHistory objects to be printed.
    @param arguments    Parsed arguments object as returned by
                        ArgumentParser.parse_args().

    """
    for history in history_list:
        status, _ = history.last_diagnosis()
        if not _include_status(status, arguments):
            continue
        print history.hostname
        if arguments.full_history:
            for event in history:
                _print_event_summary(event)
        elif arguments.diagnosis:
            for event in history.diagnosis_interval():
                _print_event_summary(event)


def _validate_time_range(arguments):
    """Validate the time range requested on the command line.

    Enforces the rules for the --until, --since, and --duration
    options are followed, and calculates defaults:
      * It isn't allowed to supply all three options.
      * If only two options are supplied, they completely determine
        the time interval.
      * If only one option is supplied, or no options, then apply
        specified defaults to the arguments object.

    @param arguments Parsed arguments object as returned by
                     ArgumentParser.parse_args().

    """
    if (arguments.duration is not None and
            arguments.since is not None and arguments.until is not None):
        print >>sys.stderr, ('FATAL: Can specify at most two of '
                             '--since, --until, and --duration')
        sys.exit(1)
    if (arguments.until is None and (arguments.since is None or
                                     arguments.duration is None)):
        arguments.until = int(time.time())
    if arguments.since is None:
        if arguments.duration is None:
            arguments.duration = _DEFAULT_DURATION
        arguments.since = (arguments.until -
                           arguments.duration * 60 * 60)
    elif arguments.until is None:
        arguments.until = (arguments.since +
                           arguments.duration * 60 * 60)


def _get_host_histories(afe, arguments):
    """Return HostJobHistory objects for the requested hosts.

    Checks that individual hosts specified on the command line are
    valid.  Invalid hosts generate a warning message, and are
    omitted from futher processing.

    The return value is a list of HostJobHistory objects for the
    valid requested hostnames, using the time range supplied on the
    command line.

    @param afe       Autotest frontend
    @param arguments Parsed arguments object as returned by
                     ArgumentParser.parse_args().
    @return List of HostJobHistory objects for the hosts requested
            on the command line.

    """
    histories = []
    saw_error = False
    for hostname in arguments.hostnames:
        try:
            h = HostJobHistory.get_host_history(
                    afe, hostname, arguments.since, arguments.until)
            histories.append(h)
        except:
            print >>sys.stderr, ('WARNING: Ignoring unknown host %s' %
                                  hostname)
            saw_error = True
    if saw_error:
        # Create separation from the output that follows
        print >>sys.stderr
    return histories


def _validate_host_list(afe, arguments):
    """Validate the user-specified list of hosts.

    Hosts may be specified implicitly with --board or --pool, or
    explictly as command line arguments.  This enforces these
    rules:
      * If --board or --pool, or both are specified, individual
        hosts may not be specified.
      * However specified, there must be at least one host.

    The return value is a list of HostJobHistory objects for the
    requested hosts, using the time range supplied on the command
    line.

    @param afe       Autotest frontend
    @param arguments Parsed arguments object as returned by
                     ArgumentParser.parse_args().
    @return List of HostJobHistory objects for the hosts requested
            on the command line.

    """
    if arguments.board or arguments.pool:
        if arguments.hostnames:
            print >>sys.stderr, ('FATAL: Hostname arguments provided '
                                 'with --board or --pool')
            sys.exit(1)
        histories = HostJobHistory.get_multiple_histories(
                afe, arguments.since, arguments.until,
                board=arguments.board, pool=arguments.pool)
    else:
        histories = _get_host_histories(afe, arguments)
    if not histories:
        print >>sys.stderr, 'FATAL: no valid hosts found'
        sys.exit(1)
    return histories


def _validate_format_options(arguments):
    """Check the options for what output format to use.

    Enforce these rules:
      * If neither --broken nor --working was used, then --oneline
        becomes the selected format.
      * If neither --broken nor --working was used, included both
        working and broken DUTs.

    @param arguments Parsed arguments object as returned by
                     ArgumentParser.parse_args().

    """
    if (not arguments.oneline and not arguments.diagnosis and
            not arguments.full_history):
        arguments.oneline = (not arguments.working and
                             not arguments.broken)
    if not arguments.working and not arguments.broken:
        arguments.working = True
        arguments.broken = True


def _validate_command(afe, arguments):
    """Check that the command's arguments are valid.

    This performs command line checking to enforce command line
    rules that ArgumentParser can't handle.  Additionally, this
    handles calculation of default arguments/options when a simple
    constant default won't do.

    Areas checked:
      * Check that a valid time range was provided, supplying
        defaults as necessary.
      * Identify invalid host names.

    @param afe       Autotest frontend
    @param arguments Parsed arguments object as returned by
                     ArgumentParser.parse_args().
    @return List of HostJobHistory objects for the hosts requested
            on the command line.

    """
    _validate_time_range(arguments)
    _validate_format_options(arguments)
    return _validate_host_list(afe, arguments)


def _parse_command(argv):
    """Parse the command line arguments.

    Create an argument parser for this command's syntax, parse the
    command line, and return the result of the ArgumentParser
    parse_args() method.

    @param argv Standard command line argument vector; argv[0] is
                assumed to be the command name.
    @return Result returned by ArgumentParser.parse_args().

    """
    parser = argparse.ArgumentParser(
            prog=argv[0],
            description='Report DUT status and execution history',
            epilog='You can specify one or two of --since, --until, '
                   'and --duration, but not all three.\n'
                   'The date/time format is "YYYY-MM-DD HH:MM:SS".')
    parser.add_argument('-s', '--since', type=status_history.parse_time,
                        metavar='DATE/TIME',
                        help='starting time for history display')
    parser.add_argument('-u', '--until', type=status_history.parse_time,
                        metavar='DATE/TIME',
                        help='ending time for history display'
                             ' (default: now)')
    parser.add_argument('-d', '--duration', type=int,
                        metavar='HOURS',
                        help='number of hours of history to display'
                             ' (default: %d)' % _DEFAULT_DURATION)

    format_group = parser.add_mutually_exclusive_group()
    format_group.add_argument('-f', '--full_history', action='store_true',
                              help='Display host history from most '
                                   'to least recent for each DUT')
    format_group.add_argument('-g', '--diagnosis', action='store_true',
                              help='Display host history for the '
                                   'most recent DUT status change')
    format_group.add_argument('-o', '--oneline', action='store_true',
                              help='Display host status summary')

    parser.add_argument('-w', '--working', action='store_true',
                        help='List working devices by name only')
    parser.add_argument('-n', '--broken', action='store_true',
                        help='List non-working devices by name only')

    parser.add_argument('-b', '--board',
                        help='Display history for all DUTs '
                             'of the given board')
    parser.add_argument('-p', '--pool',
                        help='Display history for all DUTs '
                             'in the given pool')
    parser.add_argument('hostnames',
                        nargs='*',
                        help='host names of DUTs to report on')
    parser.add_argument('--web',
                        help='Master autotest frontend hostname. If no value '
                             'is given, the one in global config will be used.',
                        default=None)
    arguments = parser.parse_args(argv[1:])
    return arguments


def main(argv):
    """Standard main() for command line processing.

    @param argv Command line arguments (normally sys.argv).

    """
    arguments = _parse_command(argv)
    afe = frontend.AFE(server=arguments.web)
    history_list = _validate_command(afe, arguments)
    if arguments.oneline:
        _print_host_summaries(history_list, arguments)
    else:
        _print_hosts(history_list, arguments)


if __name__ == '__main__':
    main(sys.argv)

#!/usr/bin/env python

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to check the history of stage calls made to devserver.
# Following are some sample use cases:
#
# 1. Find all stage request for autotest and image nyan_big-release/R38-6055.0.0
#    in the last 10 days across all devservers.
# ./devserver_history.py --image_filters nyan_big 38 6055.0.0 -l 240 \
#                        --artifact_filters autotest -v
# output:
# ==============================================================================
# 170.21.64.22
# ==============================================================================
# Number of calls:         1
# Number of unique images: 1
# 2014-08-23 12:45:00: nyan_big-release/R38-6055.0.0    autotest
# ==============================================================================
# 170.21.64.23
# ==============================================================================
# Number of calls:         2
# Number of unique images: 1
# 2014-08-23 12:45:00: nyan_big-release/R38-6055.0.0    autotest, test_suites
# 2014-08-23 12:55:00: nyan_big-release/R38-6055.0.0    autotest, test_suites
#
# 2. Find all duplicated stage request for the last 10 days.
# ./devserver_history.py -d -l 240
# output:
# Detecting artifacts staged in multiple devservers.
# ==============================================================================
# nyan_big-release/R38-6055.0.0
# ==============================================================================
# 170.21.64.22: 23  requests 2014-09-04 22:44:28 -- 2014-09-05 00:03:23
# 170.21.64.23: 6   requests 2014-09-04 22:48:58 -- 2014-09-04 22:49:42
#
# Count of images with duplicated stages on each devserver:
# 170.21.64.22   : 22
# 170.21.64.23   : 11


import argparse
import datetime
import logging
import operator
import re
import time
from itertools import groupby

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros.graphite import autotest_es


class devserver_call(object):
    """A container to store the information of devserver stage call.
    """

    def __init__(self, hit):
        """Retrieve information from a ES query hit.
        """
        self.devserver = hit['devserver']
        self.subname = hit['subname']
        self.artifacts = hit['artifacts'].split(' ')
        self.image = hit['image']
        self.value = hit['value']
        self.time_recorded = time_utils.epoch_time_to_date_string(
                hit['time_recorded'])


    def __str__(self):
        pairs = ['%-20s: %s' % (attr, getattr(self, attr)) for attr in dir(self)
                  if not attr.startswith('__') and
                  not callable(getattr(self, attr))]
        return '\n'.join(pairs)


def get_calls(time_start, time_end, artifact_filters=None,
              regex_constraints=None, devserver=None, size=1e7):
    """Gets all devserver calls from es db with the given constraints.

    @param time_start: Earliest time entry was recorded.
    @param time_end: Latest time entry was recorded.
    @param artifact_filters: A list of names to match artifacts.
    @param regex_constraints: A list of regex constraints for ES query.
    @param devserver: name of devserver to query for. If it's set to None,
                      return calls for all devservers. Default is set to None.
    @param size: Max number of entries to return, default to 1 million.

    @returns: Entries from esdb.
    """
    eqs = [('_type', 'devserver')]
    if devserver:
        eqs.append(('devserver', devserver))
    if artifact_filters:
        for artifact in artifact_filters:
            eqs.append(('artifacts', artifact))
    time_start_epoch = time_utils.to_epoch_time(time_start)
    time_end_epoch = time_utils.to_epoch_time(time_end)
    results = autotest_es.query(
            fields_returned=None,
            equality_constraints=eqs,
            range_constraints=[('time_recorded', time_start_epoch,
                                time_end_epoch)],
            size=size,
            sort_specs=[{'time_recorded': 'desc'}],
            regex_constraints=regex_constraints)
    devserver_calls = []
    for hit in results.hits:
        devserver_calls.append(devserver_call(hit))
    logging.info('Found %d calls.', len(devserver_calls))
    return devserver_calls


def print_call_details(calls, verbose):
    """Print details of each call to devserver to stage artifacts.

    @param calls: A list of devserver stage requests.
    @param verbose: Set to True to print out all devserver calls.
    """
    calls = sorted(calls, key=lambda c: c.devserver)
    for devserver,calls_for_devserver in groupby(calls, lambda c: c.devserver):
        calls_for_devserver = list(calls_for_devserver)
        print '='*80
        print devserver
        print '='*80
        print 'Number of calls:         %d' % len(calls_for_devserver)
        print ('Number of unique images: %d' %
               len(set([call.image for call in calls_for_devserver])))
        if verbose:
            for call in sorted(calls_for_devserver,
                               key=lambda c: c.time_recorded):
                print ('%s %s    %s' % (call.time_recorded, call.image,
                                         ', '.join(call.artifacts)))


def detect_duplicated_stage(calls):
    """Detect any artifact for same build was staged in multiple devservers.

    @param calls: A list of devserver stage requests.
    """
    print '\nDetecting artifacts staged in multiple devservers.'
    calls = sorted(calls, key=lambda c: c.image)
    # Count how many times a devserver staged duplicated artifacts. A number
    # significantly larger then others can indicate that the devserver failed
    # check_health too often and needs to be removed from production.
    duplicated_stage_count = {}
    for image,calls_for_image in groupby(calls, lambda c: c.image):
        calls_for_image = list(calls_for_image)
        devservers = set([call.devserver for call in calls_for_image])
        if len(devservers) > 1:
            print '='*80
            print image
            print '='*80
            calls_for_image = sorted(calls_for_image, key=lambda c: c.devserver)
            for devserver,calls_for_devserver in groupby(calls_for_image,
                                                         lambda c: c.devserver):
                timestamps = [c.time_recorded for c in calls_for_devserver]
                print ('%s: %-3d requests %s -- %s' %
                       (devserver, len(timestamps), min(timestamps),
                        max(timestamps)))
                duplicated_stage_count[devserver] = (
                        duplicated_stage_count.get(devserver, 0) + 1)
    print '\nCount of images with duplicated stages on each devserver:'
    counts = sorted(duplicated_stage_count.iteritems(),
                    key=operator.itemgetter(1), reverse=True)
    for k,v in counts:
        print '%-15s: %d' % (k, v)


def main():
    """main script. """
    t_now = time.time()
    t_now_minus_one_day = t_now - 3600 * 24
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', type=float, dest='last',
                        help='last hours to search results across',
                        default=None)
    parser.add_argument('--start', type=str, dest='start',
                        help=('Enter start time as: yyyy-mm-dd hh-mm-ss,'
                              'defualts to 24h ago. This option is ignored when'
                              ' -l is used.'),
                        default=time_utils.epoch_time_to_date_string(
                                t_now_minus_one_day))
    parser.add_argument('--end', type=str, dest='end',
                        help=('Enter end time in as: yyyy-mm-dd hh-mm-ss,'
                              'defualts to current time. This option is ignored'
                              ' when -l is used.'),
                        default=time_utils.epoch_time_to_date_string(t_now))
    parser.add_argument('--devservers', nargs='+', dest='devservers',
                         help=('Enter space deliminated devservers. Default are'
                               ' all devservers specified in global config.'),
                         default=[])
    parser.add_argument('--artifact_filters', nargs='+',
                        dest='artifact_filters',
                        help=('Enter space deliminated filters on artifact '
                              'name. For example "autotest test_suites". The '
                              'filter does not support regex.'),
                        default=[])
    parser.add_argument('--image_filters', nargs='+', dest='image_filters',
                         help=('Enter space deliminated filters on image name. '
                               'For example "nyan 38 6566", search will use '
                               'regex to match each filter. Do not use filters '
                               'with mixed letter and number, e.g., R38.'),
                         default=[])
    parser.add_argument('-d', '--detect_duplicated_stage', action='store_true',
                        dest='detect_duplicated_stage',
                        help=('Set to True to detect if an artifacts for a same'
                              ' build was staged in multiple devservers. '
                              'Default is True.'),
                        default=False)
    parser.add_argument('-v', action='store_true', dest='verbose',
                        default=False,
                        help='-v to print out ALL entries.')
    options = parser.parse_args()
    if options.verbose:
        logging.getLogger().setLevel(logging.INFO)

    if options.last:
        end_time = datetime.datetime.now()
        start_time = end_time - datetime.timedelta(seconds=3600 * options.last)
    else:
        start_time = datetime.datetime.strptime(options.start,
                                                time_utils.TIME_FMT)
        end_time = datetime.datetime.strptime(options.end, time_utils.TIME_FMT)
    logging.info('Searching devserver calls from %s to %s', start_time,
                 end_time)

    devservers = options.devservers
    if not devservers:
        devserver_urls = global_config.global_config.get_config_value(
                'CROS', 'dev_server', type=list, default=[])
        devservers = []
        for url in devserver_urls:
            match = re.match('http://([^:]*):*\d*', url)
            devservers.append(match.groups(0)[0] if match else url)
    logging.info('Found devservers: %s', devservers)

    regex_constraints = []
    for filter in options.image_filters:
        regex_constraints.append(('image', '.*%s.*' % filter))
    calls = []
    for devserver in devservers:
        calls.extend(get_calls(start_time, end_time, options.artifact_filters,
                               regex_constraints, devserver=devserver))

    print_call_details(calls, options.verbose)

    if options.detect_duplicated_stage:
        detect_duplicated_stage(calls)


if __name__ == '__main__':
    main()

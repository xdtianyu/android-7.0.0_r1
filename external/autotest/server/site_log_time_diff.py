#!/usr/bin/python

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Return the time difference between two logfile entries
"""

import logging
import optparse
import os
import re
import sys
import time

logger = logging.getLogger('log_time_diff')
handler = logging.StreamHandler(file('/dev/stderr', 'w'))
formatter = logging.Formatter('\tlog_time_diff: %(levelname)s\t%(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)


class StampParser(object):
    saved_msgs = '/var/tmp/messages.autotest_start'
    def __init__(self, from_str, to_str, start = None):
        self.from_re = re.compile(from_str)
        self.to_re = re.compile(to_str)
        self.start_line = None
        self.end_line = None
        if start:
            self.start = self.syslog_to_float(start)
        else:
            if os.path.exists(self.saved_msgs):
                for line in file(self.saved_msgs):
                    pass
                self.start = self.syslog_to_float(line.split(' ')[0])

    def parse_file(self, filename):
        for line in file(filename):
            if self.from_re.search(line):
                self.end_line = None
                self.start_line = line
            if self.to_re.search(line):
                self.end_line = line

    def syslog_to_float(self, syslog_time):
        # Lines end up like 2011-05-13T07:38:05.238129-07:00 ...
        date, sep, fraction = syslog_time.partition('.')
        int_time = time.mktime(time.strptime(date, '%Y-%m-%dT%H:%M:%S'))
        return float('%d.%s' % (int_time, re.split('[+-]', fraction)[0]))

    def results(self):
        if not self.start_line or not self.end_line:
            logger.error('Could not find strings in file')
            return '-'

        logger.debug('Start line: %s', self.start_line)
        logger.debug('End line: %s', self.end_line)

        syslog_from = self.start_line.split(' ')[0]
        syslog_from_time = self.syslog_to_float(syslog_from)
        if self.start and syslog_from_time < self.start:
            logger.error('Search string only appears before start time!')
            return '-'

        from_match = re.search('kernel:\s*\[\s*([0-9.]*)', self.start_line)
        to_match = re.search('kernel:\s*\[\s*([0-9.]*)', self.end_line)
        if from_match and to_match:
            # Lines end up like <syslog time> host kernel: [1307112.080338] ...
            logger.info('Using kernel timestamp %s %s' %
                         (from_match.group(1), to_match.group(1)))
            from_time = float(from_match.group(1))
            to_time = float(to_match.group(1))
        else:
            syslog_to = self.end_line.split(' ')[0]
            logger.info('Using syslog timestamp %s %s' %
                         (syslog_from, syslog_to))
            from_time = syslog_from_time
            to_time = self.syslog_to_float(syslog_to)
        return (to_time - from_time)


def main(argv):
    parser = optparse.OptionParser('Usage: %prog [options...]')
    parser.add_option('--from', dest='from_str',
                      help='First regexp to search for')
    parser.add_option('--to', dest='to_str',
                      help='Second regexp to search for')
    parser.add_option('--file', dest='file', default='/var/log/messages',
                      help='File to search for regexps in')
    parser.add_option('--no-rotate', dest='no_rotate', action='store_true',
                      help='Do not search in file.1 for the same expression')
    parser.add_option('--start', dest='start',
                      help='Do not accept events that start before this time')
    parser.add_option('--debug', dest='debug', action='store_true',
                      help='Show extra verbose messages')
    (options, args) = parser.parse_args(argv[1:])

    if not options.from_str or not options.to_str:
        parser.error('Required arguments: --from=<from_re> --to=<to_re>')


    if options.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    parser = StampParser(options.from_str, options.to_str, options.start)

    # If file rotation is enabled, try to parse previous file
    if not options.no_rotate:
        rotate_file = '%s.1' % options.file
        if os.path.exists(rotate_file):
            parser.parse_file(rotate_file)

    parser.parse_file(options.file)
    print parser.results()

if __name__ == '__main__':
    main(sys.argv)

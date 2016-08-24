#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import time

class ChaosLogAnalyzer(object):
    """ Class to analyze the debug logs from a chaos test . """

    MESSAGE_LOG_ATTEMPT_START_RE = "Connection attempt %d"
    NET_LOG_ATTEMPT_START_RE = "%s.*PushProfileInternal finished"
    NET_LOG_ATTEMPT_END_RE = ".*PopProfileInternal finished"
    LOG_TIMESTAMP_DATE_RE = "[0-9]{4}-[0-9]{2}-[0-9]{2}"
    LOG_TIMESTAMP_TIME_RE = "[0-9]{2}:[0-9]{2}:[0-9]{2}"
    LOG_TIMESTAMP_TIMESTAMP_RE = (
            LOG_TIMESTAMP_DATE_RE + "T" + LOG_TIMESTAMP_TIME_RE)
    LOG_TIMESTAMP_FORMAT = "%Y-%m-%dT%H:%M:%S"
    LOG_ERROR_RE = ".*ERROR:.*"

    def __init__(self, message_log, net_log, logger):
        self._net_log = net_log
        self._message_log = message_log
        self._log = logger

    def _find_line_in_log(self, search_pattern, log_file):
        search_regex = re.compile(search_pattern)
        log_file.seek(0)
        for line in log_file:
            if search_regex.search(line):
                return line

    def _find_line_in_message_log(self, search_pattern):
        return self._find_line_in_log(search_pattern, self._message_log)

    def _find_line_in_net_log(self, search_pattern):
        return self._find_line_in_log(search_pattern, self._net_log)

    def _extract_timestamp_from_line(self, line):
        timestamp_re = re.compile(self.LOG_TIMESTAMP_TIMESTAMP_RE)
        timestamp_string = timestamp_re.search(line).group(0)
        timestamp = time.strptime(timestamp_string, self.LOG_TIMESTAMP_FORMAT)
        return timestamp

    def _extract_attempt_timestamp(self, attempt_num):
        search_pattern = self.MESSAGE_LOG_ATTEMPT_START_RE % (int(attempt_num))
        line = self._find_line_in_message_log(search_pattern)
        return self._extract_timestamp_from_line(line)

    def _extract_log_lines(self, log_file, start_pattern=None, end_pattern=None,
                           stop_pattern=None, match_pattern=None):
        start_re = None
        stop_re = None
        end_re = None
        match_re = None
        start_copy = False
        lines = ""
        if start_pattern:
            start_re = re.compile(start_pattern)
        else:
            # If there is no specific start pattern, start copying from
            # begining of the file.
            start_copy = True
        if stop_pattern:
           stop_re = re.compile(stop_pattern)
        if end_pattern:
            end_re = re.compile(end_pattern)
        if match_pattern:
            match_re = re.compile(match_pattern)
        log_file.seek(0)
        for line in log_file:
            if ((start_copy == False) and (start_re and start_re.search(line))):
                start_copy = True
            if ((start_copy == True) and (stop_re and stop_re.search(line))):
                break
            if ((start_copy == True) and
                ((not match_re) or (match_re and match_re.search(line)))):
                    lines += line
            if ((start_copy == True) and (end_re and end_re.search(line))):
                break
        return lines

    def _extract_message_log_lines(self, attempt_num):
        self._log.log_start_section("Extracted Messages Log")
        start = self.MESSAGE_LOG_ATTEMPT_START_RE % (int(attempt_num))
        stop = self.MESSAGE_LOG_ATTEMPT_START_RE % (int(attempt_num) + 1)
        lines = self._extract_log_lines(
                self._message_log, start_pattern=start, stop_pattern=stop)
        self._log.log_to_output_file(lines)

    def _extract_net_log_lines(self, timestamp):
        self._log.log_start_section("Extracted Net Log")
        start = self.NET_LOG_ATTEMPT_START_RE % \
                (time.strftime(self.LOG_TIMESTAMP_FORMAT, timestamp))
        end = self.NET_LOG_ATTEMPT_END_RE
        lines = self._extract_log_lines(
                self._net_log, start_pattern=start, end_pattern=end)
        # Let's go back 1 sec and search again
        if lines == "":
            timestamp_secs = time.mktime(timestamp)
            new_timestamp = time.localtime(timestamp_secs - 1)
            start = self.NET_LOG_ATTEMPT_START_RE % \
                    (time.strftime(self.LOG_TIMESTAMP_FORMAT, new_timestamp))
            lines = self._extract_log_lines(
                    self._net_log, start_pattern=start, end_pattern=end)
        self._log.log_to_output_file(lines)

    def analyze(self, attempt_num):
        """
        Extracts the snippet of logs for given attempt from the Chaos log file.

        @param attempt_num: Attempt number for which the logs are to be
                            extracted.

        """
        timestamp = self._extract_attempt_timestamp(attempt_num)
        print "Attempt started at: " + time.asctime(timestamp)
        self._extract_message_log_lines(attempt_num)
        self._extract_net_log_lines(timestamp)

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import os
import re
import StringIO

IW_REMOTE_EVENT_LOG_FILE_NAME = 'iw_event.log'

LogEntry = collections.namedtuple('LogEntry', ['timestamp',
                                               'interface',
                                               'message'])

class IwEventLogger(object):
    """Context enclosing the use of iw event logger."""
    def __init__(self, host, command_iw, local_file):
        self._host = host
        self._command_iw = command_iw
        self._iw_event_log_path = os.path.join(self._host.get_tmp_dir(),
                                               IW_REMOTE_EVENT_LOG_FILE_NAME)
        self._local_file = local_file
        self._pid = None
        self._start_time = 0


    def __enter__(self):
        return self


    def __exit__(self, exception, value, traceback):
        self.stop()


    def _check_message_for_disconnect(self, message):
        """Check log message for disconnect event.

        This function checks log messages for signs the connection was
        terminated.

        @param: message String message to check for disconnect event.

        @returns True if the log message is a disconnect event, false otherwise.

        """
        return (message.startswith('disconnected') or
            message.startswith('Deauthenticated') or
            message == 'Previous authentication no longer valid')


    @property
    def local_file(self):
        """@return string local host path for log file."""
        return self._local_file


    def start(self):
        """Start event logger.

        This function will start iw event process in remote host, and redirect
        output to a temporary file in remote host.

        """
        command = 'nohup %s event -t </dev/null >%s 2>&1 & echo $!' % (
                self._command_iw, self._iw_event_log_path)
        command += ';date +%s'
        out_lines = self._host.run(command).stdout.splitlines()
        self._pid = int(out_lines[0])
        self._start_time = float(out_lines[1])


    def stop(self):
        """Stop event logger.

        This function will kill iw event process, and copy the log file from
        remote to local.

        """
        if self._pid is None:
            return
        # Kill iw event process
        self._host.run('kill %d' % self._pid, ignore_status=True)
        self._pid = None
        # Copy iw event log file from remote host
        self._host.get_file(self._iw_event_log_path, self._local_file)
        logging.info('iw event log saved to %s', self._local_file)


    def get_log_entries(self):
        """Parse local log file and yield LogEntry named tuples.

        This function will parse the iw event log and return individual
        LogEntry tuples for each parsed line.
        Here are example of lines to be parsed:
            1393961008.058711: wlan0 (phy #0): scan started
            1393961019.758599: wlan0 (phy #0): connected to 04:f0:21:03:7d:bd

        @yields LogEntry tuples for each log entry.

        """
        iw_log = self._host.run('cat %s' % self._iw_event_log_path).stdout
        iw_log_file = StringIO.StringIO(iw_log)
        for line in iw_log_file.readlines():
            parse_line = re.match('\s*(\d+).(\d+): (\w.*): (\w.*)', line)
            if parse_line:
                time_integer = parse_line.group(1)
                time_decimal = parse_line.group(2)
                timestamp = float('%s.%s' % (time_integer, time_decimal))
                yield LogEntry(timestamp=timestamp,
                               interface=parse_line.group(3),
                               message=parse_line.group(4))


    def get_reassociation_time(self):
        """Return reassociation time.

        This function will search the iw event log to determine the time it
        takes from start of reassociation request to being connected. Start of
        reassociation request could be either an attempt to scan or to
        disconnect. Assume the one that appeared in the log first is the start
        of the reassociation request.

        @returns float number of seconds it take from start of reassociation
                request to being connected. Return None if unable to determine
                the time based on the log.

        """
        start_time = None
        end_time = None
        # Figure out the time when reassociation process started and the time
        # when client is connected.
        for entry in self.get_log_entries():
            if (entry.message.startswith('scan started') and
                    start_time is None):
                start_time = entry.timestamp
            # Newer wpa_supplicant would attempt to disconnect then reconnect
            # without scanning. So if no scan event is detected before the
            # disconnect attempt, we'll assume the disconnect attempt is the
            # beginning of the reassociate attempt.
            if (self._check_message_for_disconnect(entry.message) and
                    start_time is None):
                start_time = entry.timestamp
            if entry.message.startswith('connected'):
                if start_time is None:
                    return None
                end_time = entry.timestamp
                break;
        else:
            return None
        return end_time - start_time


    def get_disconnect_count(self):
        """Return number of times the system disconnected during the log.

        This function will search the iw event log to determine how many
        times the "disconnect" and "Deauthenticated" messages appear.

        @returns int number of times the system disconnected in the logs.

        """
        count = 0
        for entry in self.get_log_entries():
            if self._check_message_for_disconnect(entry.message):
                count += 1
        return count


    def get_time_to_disconnected(self):
        """Return disconnect time.

        This function will search the iw event log to determine the number of
        seconds between the time iw event logger is started to the time the
        first "disconnected" or "Deauthenticated" event is received.

        @return float number of seconds between the time iw event logger is
                started to the time "disconnected" or "Deauthenticated" event
                is received. Return None if no "disconnected" or
                "Deauthenticated" event is detected in the iw event log.
        """
        for entry in self.get_log_entries():
          if self._check_message_for_disconnect(entry.message):
                return entry.timestamp - self._start_time
        return None

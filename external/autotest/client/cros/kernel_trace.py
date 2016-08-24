# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, utils

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

class KernelTrace(object):
    """Allows access and control to Kernel tracing facilities.

    Example code snippet:
        trace = KernelTrace(events=['mali_dvfs:mali_dvfs_set_clock'])
        results = trace.read(regexp=r'frequency=(\d+)')

    Public methods:
        on          : Enables tracing
        off         : Disables tracing
        is_tracing  : Returns Boolean of tracing status.
        event_on    : Turns event on.  Returns boolean of success
        event_off   : Turns event off.  Returns boolean of success
        flush       : Flushes trace buffer
        read        : Reads trace buffer returns list of
                      - tuples if regexp provided
                      - else matching string
        uptime_secs : Returns float of current uptime.

    Private functions:
        _onoff       : Disable/enable tracing
        _onoff_event : Disable/enable events

    Private attributes:
        _buffer      : list to hold parsed results from trace buffer
        _buffer_ptr  : integer pointing to last byte read

    TODO(tbroch):  List of potential enhancements
       - currently only supports trace events.  Add other tracers.
    """
    _TRACE_ROOT = '/sys/kernel/debug/tracing'
    _TRACE_EN_PATH = os.path.join(_TRACE_ROOT, 'tracing_enabled')

    def __init__(self, flush=True, events=None, on=True):
        """Constructor for KernelTrace class"""
        self._buffer = []
        self._buffer_ptr = 0
        self._events = []
        self._on = on

        if flush:
            self.flush()
        for event in events:
            if self.event_on(event):
                self._events.append(event)
        if on:
            self.on()


    def __del__(self, flush=True, events=None, on=True):
        """Deconstructor for KernelTrace class"""
        for event in self._events:
            self.event_off(event)
        if self._on:
            self.off()


    def _onoff(self, val):
        """Enable/Disable tracing.

        Arguments:
            val: integer, 1 for on, 0 for off

        Raises:
            error.TestFail: If unable to enable/disable tracing
              boolean of tracing on/off status
        """
        utils.write_one_line(self._TRACE_EN_PATH, val)
        fname = os.path.join(self._TRACE_ROOT, 'tracing_on')
        result = int(utils.read_one_line(fname).strip())
        if not result == val:
            raise error.TestFail("Unable to %sable tracing" %
                                 'en' if val == 1 else 'dis')


    def on(self):
        """Enable tracing."""
        return self._onoff(1)


    def off(self):
        """Disable tracing."""
        self._onoff(0)


    def is_tracing(self):
        """Is tracing on?

        Returns:
            True if tracing enabled and at least one event is enabled.
        """
        fname = os.path.join(self._TRACE_ROOT, 'tracing_on')
        result = int(utils.read_one_line(fname).strip())
        if result == 1 and len(self._events) > 0:
            return True
        return False


    def _event_onoff(self, event, val):
        """Enable/Disable tracing event.

        TODO(tbroch) Consider allowing wild card enabling of trace events via
            /sys/kernel/debug/tracing/set_event although it makes filling buffer
            really easy

        Arguments:
            event: list of events.
                   See kernel(Documentation/trace/events.txt) for formatting.
            val: integer, 1 for on, 0 for off

         Returns:
            True if success, false otherwise
        """
        logging.debug("event_onoff: event:%s val:%d", event, val)
        event_path = event.replace(':', '/')
        fname = os.path.join(self._TRACE_ROOT, 'events', event_path, 'enable')

        if not os.path.exists(fname):
            logging.warning("Unable to locate tracing event %s", fname)
            return False
        utils.write_one_line(fname, val)

        fname = os.path.join(self._TRACE_ROOT, "set_event")
        found = False
        with open(fname) as fd:
            for ln in fd.readlines():
                logging.debug("set_event ln:%s", ln)
                if re.findall(event, ln):
                    found = True
                    break

        if val == 1 and not found:
            logging.warning("Event %s not enabled", event)
            return False

        if val == 0 and found:
            logging.warning("Event %s not disabled", event)
            return False

        return True


    def event_on(self, event):
        return self._event_onoff(event, 1)


    def event_off(self, event):
        return self._event_onoff(event, 0)


    def flush(self):
        """Flush trace buffer.

        Raises:
            error.TestFail: If unable to flush
        """
        self.off()
        fname = os.path.join(self._TRACE_ROOT, 'free_buffer')
        utils.write_one_line(fname, 1)
        self._buffer_ptr = 0

        fname = os.path.join(self._TRACE_ROOT, 'buffer_size_kb')
        result = utils.read_one_line(fname).strip()
        if result is '0':
            return True
        return False


    def read(self, regexp=None):
        fname = os.path.join(self._TRACE_ROOT, 'trace')
        fd = open(fname)
        fd.seek(self._buffer_ptr)
        for ln in fd.readlines():
            if regexp is None:
                self._buffer.append(ln)
                continue
            results = re.findall(regexp, ln)
            if results:
                logging.debug(ln)
                self._buffer.append(results[0])
        self._buffer_ptr = fd.tell()
        fd.close()
        return self._buffer


    @staticmethod
    def uptime_secs():
        results = utils.read_one_line("/proc/uptime")
        return float(results.split()[0])

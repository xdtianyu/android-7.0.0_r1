# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import numpy
import os
import re
import threading
import time

TRACING_DIR = '/sys/kernel/debug/tracing'
BUFFER_SIZE_RUNNING = '5000'
BUFFER_SIZE_IDLE = '1408'

def tracing_write(file_name, msg):
    """Helper method to write a file in kernel debugfs.

    @param file_name: The file to write in debugfs.
    @param msg: The content to write.
    """
    with open(os.path.join(TRACING_DIR, file_name), 'w') as f:
        f.write(msg)
        f.flush()


def enable_tracing(events_list=None):
    """Enable kernel tracing.

    @param events_list: The list of events to monitor.  Defaults to None to
        monitor all events.
    """
    tracing_write('trace_clock', 'global')
    tracing_write('buffer_size_kb', BUFFER_SIZE_RUNNING)
    if events_list:
        tracing_write('set_event', '\n'.join(events_list))
    tracing_write('tracing_on', '1')


def disable_tracing():
    """Disable kernel tracing."""
    tracing_write('tracing_on', '0')
    tracing_write('set_event', '')
    tracing_write('trace', '0')
    tracing_write('buffer_size_kb', BUFFER_SIZE_IDLE)


def get_trace_log():
    """Get kernel tracing log."""
    with open(os.path.join(TRACING_DIR, 'trace'), 'r') as f:
        return f.read()


class Sampler(object):
    """Base sampler class."""

    def __init__(self, period, duration, events=None):
        self.period = period
        self.duration = duration
        self.events = events or []
        self.sampler_callback = None
        self.stop_sampling = threading.Event()
        self.sampling_thread = None

    @property
    def stopped(self):
        """Check if sampler is stopped."""
        return self.stop_sampling.is_set()

    def _do_sampling(self):
        """Main sampling loop."""
        while True:
            next_sampling_time = time.time() + self.period
            try:
                enable_tracing(events_list=self.events)
                if self.stop_sampling.wait(self.duration):
                    return
                self.parse_ftrace(get_trace_log())
                self.sampler_callback(self)
            finally:
                disable_tracing()
                self.reset()
            if self.stop_sampling.wait(next_sampling_time - time.time()):
                return

    def start_sampling_thread(self):
        """Start a thread to sample events."""
        if not self.sampler_callback:
            raise RuntimeError('Sampler callback function is not set')
        self.stop_sampling.clear()
        self.sampling_thread = threading.Thread(target=self._do_sampling)
        self.sampling_thread.daemon = True
        self.sampling_thread.start()

    def stop_sampling_thread(self):
        """Stop sampling thread."""
        self.stop_sampling.set()
        self.reset()

    def parse_ftrace(self, data):
        """Parse ftrace data.

        @param data: The ftrace data to parse.
        """
        raise NotImplementedError

    def reset(self):
        """Reset the sampler."""
        raise NotImplementedError


class ExynosSampler(Sampler):
    """Sampler for Exynos platform."""
    def __init__(self, *args, **kwargs):
        kwargs['events'] = ['exynos_page_flip_state']
        super(ExynosSampler, self).__init__(*args, **kwargs)
        self.frame_buffers = {}

    def reset(self):
        self.frame_buffers.clear()

    def parse_ftrace(self, data):
        """Read and parse the ftrace file"""
        # Parse using RE and named group match (?P<xxx>yyy) for clarity.
        # Format:
        # TASK-PID CPU#  |||| TIMESTAMP  FUNCTION
        #    X-007 [001] .... 001.000001: (a line-wrap in Python here ...)
        #          exynos_page_flip_state: pipe=0, fb=25, state=wait_kds
        re_pattern = (
            '\s*(?P<task>.+)-' + # task
            '(?P<pid>\d+)\s+' + # pid
            '\[(?P<cpu>\d+)\]\s+' + # cpu#
            '(?P<inhp>....)\s+' + # inhp: irqs-off, need-resched,
                                  #       hardirq/softirq, preempt-depth
            '(?P<timestamp>\d+\.\d+):\s+' + # timestamp
            '(?P<head>exynos_page_flip_state):\s+' + # head
            '(?P<detail>.*)') # detail: 'pipe=0, fb=25, state=wait_kds'

        for line in data.split('\n'):
            m = re.match(re_pattern, line)
            if m is None: # not a valid trace line (e.g. comment)
                continue
            m_dict = m.groupdict() # named group of RE match (m_dict['task']...)
            pipe, fb, state = self.exynos_parser(m_dict['detail'])
            timestamp = float(m_dict['timestamp'])
            self.end_last_fb_state(pipe, fb, timestamp)
            self.start_new_fb_state(pipe, fb, state, timestamp)

        self.calc_stat()

    def exynos_parser(self, detail):
        """Parse exynos event's detail.

        @param detail: a string like 'pipe=0, fb=25, state=wait_kds'
        @return: tuple of (pipe, fb, state), pipe and fb in int,
                 state in string
        """
        re_pattern = (
            'pipe=(?P<pipe>\d+), ' + # pipe
            'fb=(?P<fb>\d+), ' + # fb
            'state=(?P<state>.*)') # state
        if re.match(re_pattern, detail) is None:
            logging.debug('==fail==' + re_pattern + ', ' + detail)
        m_dict = re.match(re_pattern, detail).groupdict()
        return int(m_dict['pipe']), int(m_dict['fb']), m_dict['state']

    def end_last_fb_state(self, pipe, fb, end_time):
        """End the currently opened state of the specified frame buffer

        @param pipe: the pipe id
        @param fb: the frame buffer id
        @param end_time: timestamp when the state ends
        """
        self.get_frame_buffer(pipe, fb).end_state(end_time)

    def start_new_fb_state(self, pipe, fb, state, start_time):
        """Start the specified state on the specified frame buffer

        @param pipe: the pipe id
        @param fb: the frame buffer id
        @param state: which state to start
        @param start_time: timestamp when the state starts
        """
        self.get_frame_buffer(pipe, fb).start_state(state, start_time)

    def calc_stat(self):
        """Calculate the statistics of state duration of all frame buffers"""
        for fb in self.frame_buffers.values():
            fb.calc_state_avg_stdev()

    def frame_buffer_unique_hash(self, pipe, fb):
        """A hash function that returns the unique identifier of a frame buffer.
           The hash is a string that is unique in the sense of pipe and fb.

        @param pipe: the pipe id
        @param fb: the frame buffer id
        @return: a unique hash string, like "pipe:0,fb:25"
        """
        return "pipe:%d,fb:%d" % (pipe, fb)

    def get_frame_buffer(self, pipe, fb):
        """Return the frame buffer with specified pipe and fb.
           If the frame buffer does not exist, create one and return it.

        @param pipe: the pipe id
        @param fb: the frame buffer id
        @return: the frame buffer specified by pipe and fb
        """
        key = self.frame_buffer_unique_hash(pipe, fb)
        if key not in self.frame_buffers:
            self.frame_buffers[key] = FrameBuffer(pipe, fb)
        return self.frame_buffers[key]


class FrameBuffer():
    """Represents a frame buffer, which holds all its states"""
    def __init__(self, pipe, fb):
        """Initialize the frame buffer.

        @param pipe: the pipe id of the frame buffer
        @param fb: the fb id of the frame buffer
        """
        self.pipe = pipe
        self.fb = fb
        self.states = {}
        self.active_state = None # currently active state (to be ended later)

    def start_state(self, state_name, start_time):
        """Start the specified state

        @param state_name: name of the state to be started
        @param start_time: timestamp when the state starts
        """
        if state_name not in self.states:
            self.states[state_name] = State(state_name)
        self.states[state_name].start(start_time)
        self.active_state = state_name

    def end_state(self, end_time):
        """End the specified state, in which the duration will be stored

        @param end_time: timestamp when the state ends
        """
        if self.active_state is not None:
            self.states[self.active_state].end(end_time)
            self.active_state = None

    def calc_state_avg_stdev(self):
        """Call all states to compute its own average and standard deviation"""
        logging.debug("====pipe:%d, fb:%d====", self.pipe, self.fb)
        for s in self.states.values():
            s.calc_avg_stdev()


class State():
    """Hold the data of a specific state (e.g. wait_kds, wait_apply, ...)"""
    def __init__(self, state_name):
        """Initialize data

        @param state_name: name of this state
        """
        self.state_name = state_name
        self.last_start_time = None
        self.duration_data = []
        self.avg = None
        self.stdev = None

    def start(self, start_time):
        """Mark this state as started by saving the start time

        @param start_time: timestamp when the state starts
        """
        self.last_start_time = start_time

    def end(self, end_time):
        """Save the state's duration and mark this state as ended

        @param end_time: timestamp when the state ends
        """
        if self.last_start_time is not None:
            self.duration_data.append(end_time - self.last_start_time)
            self.last_start_time = None

    def calc_avg_stdev(self):
        """Calculate the average and standard deviation of all duration data"""
        self.avg = numpy.mean(self.duration_data)
        self.stdev = numpy.std(self.duration_data)

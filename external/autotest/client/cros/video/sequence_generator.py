# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import math
import random

from autotest_lib.client.cros.video import method_logger


@method_logger.log
def generate_interval_sequence(start, stop, interval_in_s):
    """
    Generates a list of timestamps sequence given the configuration.

    If specified upper limit would coincide as the last value it will be
    included in the list.

    We use this method when we are capturing golden images for a particular
    video. We would typically specify an interval of a second and then store
    obtained those golden images in a server.

    @param start: timedelta, start time.
    @param stop: timedelta, stop_time.
    @param interval_in_s: Time gap between two successive timestamps.

    @returns a list of timedelta values specifying points in time to take a
    screenshot.

    """

    start_total_ms = int(start.total_seconds() * 1000)
    stop_total_ms = int(stop.total_seconds() * 1000)
    interval_total_ms = interval_in_s * 1000
    duration_total_ms = stop_total_ms - start_total_ms

    if interval_total_ms > duration_total_ms:
        raise ValueError('Interval too large. Duration = %ms, interval = %ms',
                         duration_total_ms, interval_total_ms)

    # xrange is exclusive of upper limit, add 1 second so that we include
    # the stop time
    return [datetime.timedelta(milliseconds=x) for x in
            xrange(start_total_ms,
                   stop_total_ms + 1,
                   interval_total_ms)]


@method_logger.log
def generate_random_sequence(start, stop, samples_per_min):
    """
    Generates a list of random values per given configuration.

    @param start: timedelta, start time.
    @param stop: timedelta, stop time.
    @param samples_per_min: int, number of values to get per minute.

    This method exists because we need to capture images at random time
    instances. We need to do that so as to maximize the chance of catching a
    glitch that we wouldn't otherwise catch if we captured in a wrong interval.

    @returns a list of random values between start and stop as per
    'samples_per_min' configuration.

    """
    start_total_s = int(start.total_seconds())

    stop_total_s = int(stop.total_seconds())

    duration = stop_total_s - start_total_s

    # Round up the total number of samples. e.g: 8.5 becomes 9

    total_num_samples = int(
        math.ceil((samples_per_min * duration) / 60.0))

    # xrange is exclusive of upper limit, add 1 second so that we include
    # the stop time
    randomized_time_seq = random.sample(xrange(start_total_s, stop_total_s + 1),
                                        total_num_samples)

    return [datetime.timedelta(seconds=t) for t in randomized_time_seq]

# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import hashlib
import logging
import os
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros import chrome_binary_test

from contextlib import closing
from math import ceil, floor

KEY_DELIVERY_TIME = 'delivery_time'
KEY_DELIVERY_TIME_FIRST = 'delivery_time.first'
KEY_DELIVERY_TIME_75 = 'delivery_time.percentile_0.75'
KEY_DELIVERY_TIME_50 = 'delivery_time.percentile_0.50'
KEY_DELIVERY_TIME_25 = 'delivery_time.percentile_0.25'
KEY_FRAME_DROP_RATE = 'frame_drop_rate'
KEY_CPU_KERNEL_USAGE = 'cpu_usage.kernel'
KEY_CPU_USER_USAGE = 'cpu_usage.user'
KEY_DECODE_TIME_50 = 'decode_time.percentile_0.50'

DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com'
                 '/chromiumos-test-assets-public/')
BINARY = 'video_decode_accelerator_unittest'
OUTPUT_LOG = 'test_output.log'
TIME_LOG = 'time.log'

TIME_BINARY = '/usr/local/bin/time'
MICROSECONDS_PER_SECOND = 1000000

RENDERING_WARM_UP_ITERS = 30

# These strings should match chromium/src/tools/perf/unit-info.json.
UNIT_MILLISECOND = 'milliseconds'
UNIT_MICROSECOND = 'us'
UNIT_PERCENT = 'percent'

# The format used for 'time': <real time>, <kernel time>, <user time>
TIME_OUTPUT_FORMAT = '%e %S %U'

RE_FRAME_DELIVERY_TIME = re.compile('frame \d+: (\d+) us')
RE_DECODE_TIME_MEDIAN = re.compile('Decode time median: (\d+)')


def _percentile(values, k):
    assert k >= 0 and k <= 1
    i = k * (len(values) - 1)
    c, f = int(ceil(i)), int(floor(i))

    if c == f: return values[c]
    return (i - f) * values[c] + (c - i) * values[f]


def _remove_if_exists(filepath):
    try:
        os.remove(filepath)
    except OSError, e:
        if e.errno != errno.ENOENT: # no such file
            raise


class video_VDAPerf(chrome_binary_test.ChromeBinaryTest):
    """
    This test monitors several performance metrics reported by Chrome test
    binary, video_decode_accelerator_unittest.
    """

    version = 1


    def _logperf(self, name, key, value, units, higher_is_better=False):
        description = '%s.%s' % (name, key)
        self.output_perf_value(
                description=description, value=value, units=units,
                higher_is_better=higher_is_better)


    def _analyze_frame_delivery_times(self, name, frame_delivery_times):
        """
        Analyzes the frame delivery times and output the statistics to the
        Chrome Performance dashboard.

        @param name: The name of the test video.
        @param frame_delivery_times: The delivery time of each frame in the
                test video.
        """
        # Log the delivery time of the first frame.
        self._logperf(name, KEY_DELIVERY_TIME_FIRST, frame_delivery_times[0],
                      UNIT_MICROSECOND)

        # Log all the delivery times, the Chrome performance dashboard will do
        # the statistics.
        self._logperf(name, KEY_DELIVERY_TIME, frame_delivery_times,
                      UNIT_MICROSECOND)

        # Log the 25%, 50%, and 75% percentile of the frame delivery times.
        t = sorted(frame_delivery_times)
        self._logperf(name, KEY_DELIVERY_TIME_75, _percentile(t, 0.75),
                      UNIT_MICROSECOND)
        self._logperf(name, KEY_DELIVERY_TIME_50, _percentile(t, 0.50),
                      UNIT_MICROSECOND)
        self._logperf(name, KEY_DELIVERY_TIME_25, _percentile(t, 0.25),
                      UNIT_MICROSECOND)


    def _analyze_frame_drop_rate(
            self, name, frame_delivery_times, rendering_fps):
        frame_duration = MICROSECONDS_PER_SECOND / rendering_fps

        render_time = frame_duration;
        delivery_time = 0;
        drop_count = 0

        # Ignore the delivery time of the first frame since we delay the
        # rendering until we get the first frame.
        #
        # Note that we keep accumulating delivery times and don't use deltas
        # between current and previous delivery time. If the decoder cannot
        # catch up after falling behind, it will keep dropping frames.
        for t in frame_delivery_times[1:]:
            render_time += frame_duration
            delivery_time += t
            if delivery_time > render_time:
                drop_count += 1

        n = len(frame_delivery_times)

        # Since we won't drop the first frame, don't add it to the number of
        # frames.
        drop_rate = float(drop_count) / (n - 1) if n > 1 else 0
        self._logperf(name, KEY_FRAME_DROP_RATE, drop_rate, UNIT_PERCENT)

        # The performance keys would be used as names of python variables when
        # evaluating the test constraints. So we cannot use '.' as we did in
        # _logperf.
        self._perf_keyvals['%s_%s' % (name, KEY_FRAME_DROP_RATE)] = drop_rate


    def _analyze_cpu_usage(self, name, time_log_file):
        with open(time_log_file) as f:
            content = f.read()
        r, s, u = (float(x) for x in content.split())

        self._logperf(name, KEY_CPU_USER_USAGE, u / r, UNIT_PERCENT)
        self._logperf(name, KEY_CPU_KERNEL_USAGE, s / r, UNIT_PERCENT)


    def _load_frame_delivery_times(self, test_log_file):
        """
        Gets the frame delivery times from the |test_log_file|.

        The |test_log_file| could contain frame delivery times for more than
        one decoder. However, we use only one in this test.

        The expected content in the |test_log_file|:

        The first line is the frame number of the first decoder. For exmplae:
          frame count: 250
        It is followed by the delivery time of each frame. For example:
          frame 0000: 16123 us
          frame 0001: 16305 us
          :

        Then it is the frame number of the second decoder followed by the
        delivery times, and so on so forth.

        @param test_log_file: The test log file where we load the frame
                delivery times from.
        @returns a list of integers which are the delivery times of all frames
                (in microsecond).
        """
        result = []
        with open(test_log_file, 'r') as f:
            while True:
                line = f.readline()
                if not line: break
                _, count = line.split(':')
                times = []
                for i in xrange(int(count)):
                    line = f.readline()
                    m = RE_FRAME_DELIVERY_TIME.match(line)
                    assert m, 'invalid format: %s' % line
                    times.append(int(m.group(1)))
                result.append(times)
        if len(result) != 1:
            raise error.TestError('Frame delivery times load failed.')
        return result[0]


    def _get_test_case_name(self, path):
        """Gets the test_case_name from the video's path.

        For example: for the path
            "/crowd/crowd1080-1edaaca36b67e549c51e5fea4ed545c3.vp8"
        We will derive the test case's name as "crowd1080_vp8".
        """
        s = path.split('/')[-1] # get the filename
        return '%s_%s' % (s[:s.rfind('-')], s[s.rfind('.') + 1:])


    def _download_video(self, download_path, local_file):
        url = '%s%s' % (DOWNLOAD_BASE, download_path)
        logging.info('download "%s" to "%s"', url, local_file)

        file_utils.download_file(url, local_file)

        with open(local_file, 'r') as r:
            md5sum = hashlib.md5(r.read()).hexdigest()
            if md5sum not in download_path:
                raise error.TestError('unmatched md5 sum: %s' % md5sum)


    def _results_file(self, test_name, type_name, filename):
        return os.path.join(self.resultsdir,
            '%s_%s_%s' % (test_name, type_name, filename))


    def _append_freon_switch_if_needed(self, cmd_line):
        if utils.is_freon():
            return cmd_line + ' --ozone-platform=gbm'
        else:
            return cmd_line


    def _run_test_case(self, name, test_video_data, frame_num, rendering_fps):

        # Get frame delivery time, decode as fast as possible.
        test_log_file = self._results_file(name, 'no_rendering', OUTPUT_LOG)
        cmd_line = self._append_freon_switch_if_needed(
            '--test_video_data="%s" ' % test_video_data +
            '--gtest_filter=DecodeVariations/*/0 ' +
            '--disable_rendering ' +
            '--output_log="%s"' % test_log_file)

        self.run_chrome_test_binary(BINARY, cmd_line)

        frame_delivery_times = self._load_frame_delivery_times(test_log_file)
        if len(frame_delivery_times) != frame_num:
            raise error.TestError(
                "frames number mismatch - expected: %d, got: %d" %
                (frame_num, len(frame_delivery_times)));
        self._analyze_frame_delivery_times(name, frame_delivery_times)

        # Get frame drop rate & CPU usage, decode at the specified fps
        test_log_file = self._results_file(name, 'with_rendering', OUTPUT_LOG)
        time_log_file = self._results_file(name, 'with_rendering', TIME_LOG)
        cmd_line = self._append_freon_switch_if_needed(
            '--test_video_data="%s" ' % test_video_data +
            '--gtest_filter=DecodeVariations/*/0 ' +
            '--rendering_warm_up=%d ' % RENDERING_WARM_UP_ITERS +
            '--rendering_fps=%s ' % rendering_fps +
            '--output_log="%s"' % test_log_file)
        time_cmd = ('%s -f "%s" -o "%s" ' %
                    (TIME_BINARY, TIME_OUTPUT_FORMAT, time_log_file))
        self.run_chrome_test_binary(BINARY, cmd_line, prefix=time_cmd)

        frame_delivery_times = self._load_frame_delivery_times(test_log_file)
        self._analyze_frame_drop_rate(name, frame_delivery_times, rendering_fps)
        self._analyze_cpu_usage(name, time_log_file)

        # Get decode time median.
        test_log_file = self._results_file(name, 'decode_time', OUTPUT_LOG)
        cmd_line = self._append_freon_switch_if_needed(
            '--test_video_data="%s" ' % test_video_data +
            '--gtest_filter=*TestDecodeTimeMedian ' +
            '--output_log="%s"' % test_log_file)
        self.run_chrome_test_binary(BINARY, cmd_line)
        line = open(test_log_file, 'r').read()
        m = RE_DECODE_TIME_MEDIAN.match(line)
        assert m, 'invalid format: %s' % line
        decode_time = int(m.group(1))
        self._logperf(name, KEY_DECODE_TIME_50, decode_time, UNIT_MICROSECOND)


    @chrome_binary_test.nuke_chrome
    def run_once(self, test_cases):
        self._perf_keyvals = {}
        last_error = None
        for (path, width, height, frame_num, frag_num, profile,
             fps)  in test_cases:
            name = self._get_test_case_name(path)
            video_path = os.path.join(self.bindir, '%s.download' % name)
            test_video_data = '%s:%s:%s:%s:%s:%s:%s:%s' % (
                video_path, width, height, frame_num, frag_num, 0, 0, profile)
            try:
                self._download_video(path, video_path)
                self._run_test_case(name, test_video_data, frame_num, fps)
            except Exception as last_error:
                # log the error and continue to the next test case.
                logging.exception(last_error)
            finally:
                _remove_if_exists(video_path)

        if last_error:
            raise # the last error

        self.write_perf_keyval(self._perf_keyvals)

# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib, hashlib, logging, os, pipes, re, sys, time, tempfile

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros import power_status, power_utils
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.audio import cmd_utils

# The download base for test assets.
DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com'
                 '/chromiumos-test-assets-public/')

# The executable name of the vda unittest
VDA_BINARY = 'video_decode_accelerator_unittest'

# The executable name of the vea unittest
VEA_BINARY = 'video_encode_accelerator_unittest'

# The input frame rate for the vea_unittest.
INPUT_FPS = 30

# The rendering fps in the vda_unittest.
RENDERING_FPS = 30

# The unit(s) should match chromium/src/tools/perf/unit-info.json.
UNIT_PERCENT = '%'
UNIT_WATT = 'W'

# The regex of the versioning file.
# e.g., crowd720-3cfe7b096f765742b4aa79e55fe7c994.yuv
RE_VERSIONING_FILE = re.compile(r'(.+)-([0-9a-fA-F]{32})(\..+)?')

# Time in seconds to wait for cpu idle until giveup.
WAIT_FOR_IDLE_CPU_TIMEOUT = 60

# Maximum percent of cpu usage considered as idle.
CPU_IDLE_USAGE = 0.1

# List of thermal throttling services that should be disabled.
# - temp_metrics for link.
# - thermal for daisy, snow, pit etc.
THERMAL_SERVICES = ['temp_metrics', 'thermal']

# Measurement duration in seconds.
MEASUREMENT_DURATION = 30

# Time to exclude from calculation after playing a video [seconds].
STABILIZATION_DURATION = 10

# The number of frames used to warm up the rendering.
RENDERING_WARM_UP = 15

# A big number, used to keep the [vda|vea]_unittest running during the
# measurement.
MAX_INT = 2 ** 31 - 1

# Minimum battery charge percentage to run the test
BATTERY_INITIAL_CHARGED_MIN = 10


class CpuUsageMeasurer(object):
    """ Class used to measure the CPU usage."""

    def __init__(self):
        self._service_stopper = None
        self._original_governors = None

    def __enter__(self):
        # Stop the thermal service that may change the cpu frequency.
        self._service_stopper = service_stopper.ServiceStopper(THERMAL_SERVICES)
        self._service_stopper.stop_services()

        if not utils.wait_for_idle_cpu(
                WAIT_FOR_IDLE_CPU_TIMEOUT, CPU_IDLE_USAGE):
            raise error.TestError('Could not get idle CPU.')
        if not utils.wait_for_cool_machine():
            raise error.TestError('Could not get cold machine.')

        # Set the scaling governor to performance mode to set the cpu to the
        # highest frequency available.
        self._original_governors = utils.set_high_performance_mode()
        return self

    def start(self):
        self.start_cpu_usage_ = utils.get_cpu_usage()

    def stop(self):
        return utils.compute_active_cpu_time(
                self.start_cpu_usage_, utils.get_cpu_usage())

    def __exit__(self, type, value, tb):
        if self._service_stopper:
            self._service_stopper.restore_services()
            self._service_stopper = None
        if self._original_governors:
            utils.restore_scaling_governor_states(self._original_governors)
            self._original_governors = None


class PowerMeasurer(object):
    """ Class used to measure the power consumption."""

    def __init__(self):
        self._backlight = None
        self._service_stopper = None

    def __enter__(self):
        self._backlight = power_utils.Backlight()
        self._backlight.set_default()

        self._service_stopper = service_stopper.ServiceStopper(
                service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._service_stopper.stop_services()

        status = power_status.get_status()

        # Verify that we are running on battery and the battery is sufficiently
        # charged.
        status.assert_battery_state(BATTERY_INITIAL_CHARGED_MIN)
        self._system_power = power_status.SystemPower(status.battery_path)
        self._power_logger = power_status.PowerLogger([self._system_power])
        return self

    def start(self):
        self._power_logger.start()

    def stop(self):
        self._power_logger.checkpoint('result')
        keyval = self._power_logger.calc()
        logging.info(keyval)
        return keyval['result_' + self._system_power.domain + '_pwr']

    def __exit__(self, type, value, tb):
        if self._backlight:
            self._backlight.restore()
        if self._service_stopper:
            self._service_stopper.restore_services()


class DownloadManager(object):
    """Use this class to download and manage the resources for testing."""

    def __init__(self, tmpdir=None):
        self._download_map = {}
        self._tmpdir = tmpdir

    def get_path(self, name):
        return self._download_map[name]

    def clear(self):
        map(os.unlink, self._download_map.values())
        self._download_map.clear()

    def _download_single_file(self, remote_path):
        url = DOWNLOAD_BASE + remote_path
        tmp = tempfile.NamedTemporaryFile(delete=False, dir=self._tmpdir)
        logging.info('download "%s" to "%s"', url, tmp.name)

        file_utils.download_file(url, tmp.name)
        md5 = hashlib.md5()
        with open(tmp.name, 'r') as r:
            md5.update(r.read())

        filename = os.path.basename(remote_path)
        m = RE_VERSIONING_FILE.match(filename)
        if m:
            prefix, md5sum, suffix = m.groups()
            if md5.hexdigest() != md5sum:
                raise error.TestError(
                        'unmatched md5 sum: %s' % md5.hexdigest())
            filename = prefix + (suffix or '')
        self._download_map[filename] = tmp.name

    def download_all(self, resources):
        for r in resources:
            self._download_single_file(r)


class video_HangoutHardwarePerf(chrome_binary_test.ChromeBinaryTest):
    """
    The test outputs the cpu usage when doing video encoding and video
    decoding concurrently.
    """

    version = 1

    def get_vda_unittest_cmd_line(self, decode_videos):
        test_video_data = []
        for v in decode_videos:
            assert len(v) == 6
            # Convert to strings, also make a copy of the list.
            v = map(str, v)
            v[0] = self._downloads.get_path(v[0])
            v[-1:-1] = ['0', '0'] # no fps requirements
            test_video_data.append(':'.join(v))
        cmd_line = [
            self.get_chrome_binary_path(VDA_BINARY),
            '--gtest_filter=DecodeVariations/*/0',
            '--test_video_data=%s' % ';'.join(test_video_data),
            '--rendering_warm_up=%d' % RENDERING_WARM_UP,
            '--rendering_fps=%f' % RENDERING_FPS,
            '--num_play_throughs=%d' % MAX_INT]
        if utils.is_freon():
            cmd_line.append('--ozone-platform=gbm')
        return cmd_line


    def get_vea_unittest_cmd_line(self, encode_videos):
        test_stream_data = []
        for v in encode_videos:
            assert len(v) == 5
            # Convert to strings, also make a copy of the list.
            v = map(str, v)
            v[0] = self._downloads.get_path(v[0])
            # The output destination, ignore the output.
            v.insert(4, '/dev/null')
            # Insert the FPS requirement
            v.append(str(INPUT_FPS))
            test_stream_data.append(':'.join(v))
        cmd_line = [
            self.get_chrome_binary_path(VEA_BINARY),
            '--gtest_filter=SimpleEncode/*/0',
            '--test_stream_data=%s' % ';'.join(test_stream_data),
            '--run_at_fps',
            '--num_frames_to_encode=%d' % MAX_INT]
        if utils.is_freon():
            cmd_line.append('--ozone-platform=gbm')
        return cmd_line

    def run_in_parallel(self, *commands):
        env = os.environ.copy()

        # To clear the temparory files created by vea_unittest.
        env['TMPDIR'] = self.tmpdir
        if not utils.is_freon():
            env['DISPLAY'] = ':0'
            env['XAUTHORITY'] = '/home/chronos/.Xauthority'
        return map(lambda c: cmd_utils.popen(c, env=env), commands)

    def simulate_hangout(self, decode_videos, encode_videos, measurer):
        popens = self.run_in_parallel(
            self.get_vda_unittest_cmd_line(decode_videos),
            self.get_vea_unittest_cmd_line(encode_videos))
        try:
            time.sleep(STABILIZATION_DURATION)
            measurer.start()
            time.sleep(MEASUREMENT_DURATION)
            measurement = measurer.stop()

            # Ensure both encoding and decoding are still alive
            if any(p.poll() is not None for p in popens):
                raise error.TestError('vea/vda_unittest failed')

            return measurement
        finally:
            cmd_utils.kill_or_log_returncode(*popens)

    @chrome_binary_test.nuke_chrome
    def run_once(self, resources, decode_videos, encode_videos, measurement):
        self._downloads = DownloadManager(tmpdir = self.tmpdir)
        try:
            self._downloads.download_all(resources)
            if measurement == 'cpu':
                with CpuUsageMeasurer() as measurer:
                    value = self.simulate_hangout(
                            decode_videos, encode_videos, measurer)
                    self.output_perf_value(
                            description='cpu_usage', value=value * 100,
                            units=UNIT_PERCENT, higher_is_better=False)
            elif measurement == 'power':
                with PowerMeasurer() as measurer:
                    value = self.simulate_hangout(
                            decode_videos, encode_videos, measurer)
                    self.output_perf_value(
                            description='power_usage', value=value,
                            units=UNIT_WATT, higher_is_better=False)
            else:
                raise error.TestError('Unknown measurement: ' + measurement)
        finally:
            self._downloads.clear()

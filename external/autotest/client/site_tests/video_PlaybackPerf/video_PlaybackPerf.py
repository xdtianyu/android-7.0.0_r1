# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import site_utils, test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import power_status, power_utils
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import constants


DISABLE_ACCELERATED_VIDEO_DECODE_BROWSER_ARGS = [
        '--disable-accelerated-video-decode']
DOWNLOAD_BASE = 'http://commondatastorage.googleapis.com/chromiumos-test-assets-public/traffic/'

PLAYBACK_WITH_HW_ACCELERATION = 'playback_with_hw_acceleration'
PLAYBACK_WITHOUT_HW_ACCELERATION = 'playback_without_hw_acceleration'

# Measurement duration in seconds.
MEASUREMENT_DURATION = 30
# Time to exclude from calculation after playing a video [seconds].
STABILIZATION_DURATION = 10

# List of thermal throttling services that should be disabled.
# - temp_metrics for link.
# - thermal for daisy, snow, pit etc.
THERMAL_SERVICES = ['temp_metrics', 'thermal']

# Time in seconds to wait for cpu idle until giveup.
WAIT_FOR_IDLE_CPU_TIMEOUT = 60.0
# Maximum percent of cpu usage considered as idle.
CPU_IDLE_USAGE = 0.1

CPU_USAGE_DESCRIPTION = 'video_cpu_usage_'
DROPPED_FRAMES_DESCRIPTION = 'video_dropped_frames_'
DROPPED_FRAMES_PERCENT_DESCRIPTION = 'video_dropped_frames_percent_'
POWER_DESCRIPTION = 'video_mean_energy_rate_'

# Minimum battery charge percentage to run the test
BATTERY_INITIAL_CHARGED_MIN = 10


class video_PlaybackPerf(test.test):
    """
    The test outputs the cpu usage, the dropped frame count and the power
    consumption for video playback to performance dashboard.
    """
    version = 1


    def initialize(self):
        self._service_stopper = None
        self._original_governors = None
        self._backlight = None


    def start_playback(self, cr, local_path):
        """
        Opens the video and plays it.

        @param cr: Autotest Chrome instance.
        @param local_path: path to the local video file to play.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        tab = cr.browser.tabs[0]
        tab.Navigate(cr.browser.platform.http_server.UrlOf(local_path))
        tab.WaitForDocumentReadyStateToBeComplete()
        tab.EvaluateJavaScript("document.getElementsByTagName('video')[0]."
                               "loop=true")


    def run_once(self, video_name, video_description, power_test=False):
        """
        Runs the video_PlaybackPerf test.

        @param video_name: the name of video to play in the DOWNLOAD_BASE
        @param video_description: a string describes the video to play which
                will be part of entry name in dashboard.
        @param power_test: True if this is a power test and it would only run
                the power test. If False, it would run the cpu usage test and
                the dropped frame count test.
        """
        # Download test video.
        url = DOWNLOAD_BASE + video_name
        local_path = os.path.join(self.bindir, video_name)
        file_utils.download_file(url, local_path)

        if not power_test:
            # Run the video playback dropped frame tests.
            keyvals = self.test_dropped_frames(local_path)

            # Every dictionary value is a tuple. The first element of the tuple
            # is dropped frames. The second is dropped frames percent.
            keyvals_dropped_frames = {k: v[0] for k, v in keyvals.iteritems()}
            keyvals_dropped_frames_percent = {
                    k: v[1] for k, v in keyvals.iteritems()}

            self.log_result(keyvals_dropped_frames, DROPPED_FRAMES_DESCRIPTION +
                                video_description, 'frames')
            self.log_result(keyvals_dropped_frames_percent,
                            DROPPED_FRAMES_PERCENT_DESCRIPTION +
                                video_description, 'percent')

            # Run the video playback cpu usage tests.
            keyvals = self.test_cpu_usage(local_path)
            self.log_result(keyvals, CPU_USAGE_DESCRIPTION + video_description,
                            'percent')
        else:
            keyvals = self.test_power(local_path)
            self.log_result(keyvals, POWER_DESCRIPTION + video_description, 'W')


    def test_dropped_frames(self, local_path):
        """
        Runs the video dropped frame test.

        @param local_path: the path to the video file.

        @return a dictionary that contains the test result.
        """
        def get_dropped_frames(cr):
            time.sleep(MEASUREMENT_DURATION)
            tab = cr.browser.tabs[0]
            decoded_frame_count = tab.EvaluateJavaScript(
                    "document.getElementsByTagName"
                    "('video')[0].webkitDecodedFrameCount")
            dropped_frame_count = tab.EvaluateJavaScript(
                    "document.getElementsByTagName"
                    "('video')[0].webkitDroppedFrameCount")
            if decoded_frame_count != 0:
                dropped_frame_percent = \
                        100.0 * dropped_frame_count / decoded_frame_count
            else:
                logging.error("No frame is decoded. Set drop percent to 100.")
                dropped_frame_percent = 100.0
            logging.info("Decoded frames=%d, dropped frames=%d, percent=%f",
                              decoded_frame_count,
                              dropped_frame_count,
                              dropped_frame_percent)
            return (dropped_frame_count, dropped_frame_percent)
        return self.test_playback(local_path, get_dropped_frames)


    def test_cpu_usage(self, local_path):
        """
        Runs the video cpu usage test.

        @param local_path: the path to the video file.

        @return a dictionary that contains the test result.
        """
        def get_cpu_usage(cr):
            time.sleep(STABILIZATION_DURATION)
            cpu_usage_start = site_utils.get_cpu_usage()
            time.sleep(MEASUREMENT_DURATION)
            cpu_usage_end = site_utils.get_cpu_usage()
            return site_utils.compute_active_cpu_time(cpu_usage_start,
                                                      cpu_usage_end) * 100
        if not utils.wait_for_idle_cpu(WAIT_FOR_IDLE_CPU_TIMEOUT,
                                       CPU_IDLE_USAGE):
            raise error.TestError('Could not get idle CPU.')
        if not utils.wait_for_cool_machine():
            raise error.TestError('Could not get cold machine.')
        # Stop the thermal service that may change the cpu frequency.
        self._service_stopper = service_stopper.ServiceStopper(THERMAL_SERVICES)
        self._service_stopper.stop_services()
        # Set the scaling governor to performance mode to set the cpu to the
        # highest frequency available.
        self._original_governors = utils.set_high_performance_mode()
        return self.test_playback(local_path, get_cpu_usage)


    def test_power(self, local_path):
        """
        Runs the video power consumption test.

        @param local_path: the path to the video file.

        @return a dictionary that contains the test result.
        """

        self._backlight = power_utils.Backlight()
        self._backlight.set_default()

        self._service_stopper = service_stopper.ServiceStopper(
                service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._service_stopper.stop_services()

        self._power_status = power_status.get_status()
        # Verify that we are running on battery and the battery is sufficiently
        # charged.
        self._power_status.assert_battery_state(BATTERY_INITIAL_CHARGED_MIN)

        measurements = [power_status.SystemPower(
                self._power_status.battery_path)]

        def get_power(cr):
            power_logger = power_status.PowerLogger(measurements)
            power_logger.start()
            time.sleep(STABILIZATION_DURATION)
            start_time = time.time()
            time.sleep(MEASUREMENT_DURATION)
            power_logger.checkpoint('result', start_time)
            keyval = power_logger.calc()
            return keyval['result_' + measurements[0].domain + '_pwr']

        return self.test_playback(local_path, get_power)


    def test_playback(self, local_path, gather_result):
        """
        Runs the video playback test with and without hardware acceleration.

        @param local_path: the path to the video file.
        @param gather_result: a function to run and return the test result
                after chrome opens. The input parameter of the funciton is
                Autotest chrome instance.

        @return a dictionary that contains test the result.
        """
        keyvals = {}

        with chrome.Chrome() as cr:
            # Open the video playback page and start playing.
            self.start_playback(cr, local_path)
            result = gather_result(cr)

            # Check if decode is hardware accelerated.
            if histogram_verifier.is_bucket_present(
                    cr,
                    constants.MEDIA_GVD_INIT_STATUS,
                    constants.MEDIA_GVD_BUCKET):
                keyvals[PLAYBACK_WITH_HW_ACCELERATION] = result
            else:
                logging.info("Can not use hardware decoding.")
                keyvals[PLAYBACK_WITHOUT_HW_ACCELERATION] = result
                return keyvals

        # Start chrome with disabled video hardware decode flag.
        with chrome.Chrome(extra_browser_args=
                DISABLE_ACCELERATED_VIDEO_DECODE_BROWSER_ARGS) as cr:
            # Open the video playback page and start playing.
            self.start_playback(cr, local_path)
            result = gather_result(cr)

            # Make sure decode is not hardware accelerated.
            if histogram_verifier.is_bucket_present(
                    cr,
                    constants.MEDIA_GVD_INIT_STATUS,
                    constants.MEDIA_GVD_BUCKET):
                raise error.TestError(
                        'Video decode acceleration should not be working.')
            keyvals[PLAYBACK_WITHOUT_HW_ACCELERATION] = result

        return keyvals


    def log_result(self, keyvals, description, units):
        """
        Logs the test result output to the performance dashboard.

        @param keyvals: a dictionary that contains results returned by
                test_playback.
        @param description: a string that describes the video and test result
                and it will be part of the entry name in the dashboard.
        @param units: the units of test result.
        """
        result_with_hw = keyvals.get(PLAYBACK_WITH_HW_ACCELERATION)
        if result_with_hw is not None:
            self.output_perf_value(
                    description= 'hw_' + description, value=result_with_hw,
                    units=units, higher_is_better=False)

        result_without_hw = keyvals[PLAYBACK_WITHOUT_HW_ACCELERATION]
        self.output_perf_value(
                description= 'sw_' + description, value=result_without_hw,
                units=units, higher_is_better=False)


    def cleanup(self):
        # cleanup() is run by common_lib/test.py.
        if self._backlight:
            self._backlight.restore()
        if self._service_stopper:
            self._service_stopper.restore_services()
        if self._original_governors:
            utils.restore_scaling_governor_states(self._original_governors)

        super(video_PlaybackPerf, self).cleanup()

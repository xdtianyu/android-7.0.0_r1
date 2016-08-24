# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This is a client side WebGL many planets deep test."""

import numpy
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.graphics import graphics_utils


class graphics_WebGLManyPlanetsDeep(test.test):
    """WebGL many planets deep graphics test."""
    version = 1
    GSC = None
    frame_data = {}
    perf_keyval = {}
    test_duration_secs = 30

    def setup(self):
        self.job.setup_dep(['webgl_mpd'])
        self.job.setup_dep(['graphics'])

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()

    def cleanup(self):
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            for key, val in keyvals.iteritems():
                self.output_perf_value(description=key,
                                       value=val,
                                       units='bytes',
                                       higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)

    def run_many_planets_deep_test(self, browser, test_url):
        """Runs the many planets deep test from the given url.

        @param browser: The Browser object to run the test with.
        @param test_url: The URL to the many planets deep test site.
        """
        if not utils.wait_for_idle_cpu(60.0, 0.1):
            if not utils.wait_for_idle_cpu(20.0, 0.2):
                raise error.TestFail('Could not get idle CPU.')

        tab = browser.tabs.New()
        tab.Navigate(test_url)
        tab.Activate()
        tab.WaitForDocumentReadyStateToBeComplete()

        # Wait 3 seconds for the page to stabilize.
        time.sleep(3)

        # Reset our own FPS counter and start recording FPS and rendering time.
        end_time = time.time() + self.test_duration_secs
        tab.ExecuteJavaScript('g_crosFpsCounter.reset();')
        while time.time() < end_time:
            frame_data = tab.EvaluateJavaScript(
                'g_crosFpsCounter.getFrameData();')
            for datum in frame_data:
                if not datum or datum['seq'] in self.frame_data:
                    continue
                self.frame_data[datum['seq']] = {
                    'start_time': datum['startTime'],
                    'frame_elapsed_time': datum['frameElapsedTime'],
                    'js_elapsed_time': datum['jsElapsedTime']
                }
            time.sleep(1)
        tab.Close()

    def calculate_perf_values(self):
        """Calculates all the perf values from the collected data."""
        arr = numpy.array([[v['frame_elapsed_time'], v['js_elapsed_time']]
                           for v in self.frame_data.itervalues()])
        std = arr.std(axis=0)
        mean = arr.mean(axis=0)
        avg_fps = 1000.0 / mean[0]
        self.perf_keyval.update({
            'average_fps': avg_fps,
            'per_frame_dt_ms_std': std[0],
            'per_frame_dt_ms_mean': mean[0],
            'js_render_time_ms_std': std[1],
            'js_render_time_ms_mean': mean[1]
        })
        self.output_perf_value(description='average_fps',
                               value=avg_fps,
                               units='fps',
                               higher_is_better=True)

        with open('frame_data', 'w') as f:
            line_format = '%10s %20s %20s %20s\n'
            f.write(line_format % ('seq', 'start_time', 'frame_render_time_ms',
                                   'js_render_time_ms'))
            for k in sorted(self.frame_data.keys()):
                d = self.frame_data[k]
                f.write(line_format %
                        (k, d['start_time'], d['frame_elapsed_time'],
                         d['js_elapsed_time']))

    def run_once(self, test_duration_secs=30, fullscreen=True):
        """Finds a brower with telemetry, and run the test.

        @param test_duration_secs: The test duration in seconds to run the test
                for.
        @param fullscreen: Whether to run the test in fullscreen.
        """
        self.test_duration_secs = test_duration_secs

        ext_paths = []
        if fullscreen:
            ext_paths.append(os.path.join(self.autodir, 'deps', 'graphics',
                                          'graphics_test_extension'))

        with chrome.Chrome(logged_in=False, extension_paths=ext_paths) as cr:
            websrc_dir = os.path.join(self.autodir, 'deps', 'webgl_mpd', 'src')
            if not cr.browser.platform.SetHTTPServerDirectories(websrc_dir):
                raise error.TestError('Unable to start HTTP server')
            test_url = cr.browser.platform.http_server.UrlOf(os.path.join(
                websrc_dir, 'ManyPlanetsDeep.html'))
            self.run_many_planets_deep_test(cr.browser, test_url)

        self.calculate_perf_values()
        self.write_perf_keyval(self.perf_keyval)

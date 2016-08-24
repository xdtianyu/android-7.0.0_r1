# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This is a client side WebGL aquarium test."""

import logging
import math
import os
import sampler
import threading
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros import power_status, power_utils
from autotest_lib.client.cros import service_stopper

# Minimum battery charge percentage to run the test
BATTERY_INITIAL_CHARGED_MIN = 10

# Measurement duration in seconds.
MEASUREMENT_DURATION = 30

POWER_DESCRIPTION = 'avg_energy_rate_1000_fishes'

# Time to exclude from calculation after playing a webgl demo [seconds].
STABILIZATION_DURATION = 10


class graphics_WebGLAquarium(test.test):
    """WebGL aquarium graphics test."""
    version = 1

    _backlight = None
    _power_status = None
    _service_stopper = None
    _test_power = False
    active_tab = None
    flip_stats = {}
    GSC = None
    kernel_sampler = None
    perf_keyval = {}
    sampler_lock = None
    test_duration_secs = 30
    test_setting_num_fishes = 50
    test_settings = {50: ('setSetting2', 2), 1000: ('setSetting6', 6),}

    def setup(self):
        tarball_path = os.path.join(self.bindir,
                                    'webgl_aquarium_static.tar.bz2')
        utils.extract_tarball_to_dir(tarball_path, self.srcdir)

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()
        self.sampler_lock = threading.Lock()
        # TODO: Create samplers for other platforms (e.g. x86).
        if utils.get_board().lower() in ['daisy', 'daisy_spring']:
            # Enable ExynosSampler on Exynos platforms.  The sampler looks for
            # exynos-drm page flip states: 'wait_kds', 'rendered', 'prepared',
            # and 'flipped' in kernel debugfs.

            # Sample 3-second durtaion for every 5 seconds.
            self.kernel_sampler = sampler.ExynosSampler(period=5, duration=3)
            self.kernel_sampler.sampler_callback = self.exynos_sampler_callback
            self.kernel_sampler.output_flip_stats = (
                self.exynos_output_flip_stats)

    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        if self._service_stopper:
            self._service_stopper.restore_services()
        if self.GSC:
            keyvals = self.GSC.get_memory_keyvals()
            if not self._test_power:
                for key, val in keyvals.iteritems():
                    self.output_perf_value(description=key,
                                           value=val,
                                           units='bytes',
                                           higher_is_better=False)
            self.GSC.finalize()
            self.write_perf_keyval(keyvals)

    def run_fish_test(self, browser, test_url, num_fishes, perf_log=True):
        """Run the test with the given number of fishes.

        @param browser: The Browser object to run the test with.
        @param test_url: The URL to the aquarium test site.
        @param num_fishes: The number of fishes to run the test with.
        @param perf_log: Report perf data only if it's set to True.
        """
        # Create tab and load page. Set the number of fishes when page is fully
        # loaded.
        tab = browser.tabs.New()
        tab.Navigate(test_url)
        tab.Activate()
        self.active_tab = tab
        tab.WaitForDocumentReadyStateToBeComplete()

        # Set the number of fishes when document finishes loading.  Also reset
        # our own FPS counter and start recording FPS and rendering time.
        utils.wait_for_value(
            lambda: tab.EvaluateJavaScript(
                'if (document.readyState === "complete") {'
                '  setSetting(document.getElementById("%s"), %d);'
                '  g_crosFpsCounter.reset();'
                '  true;'
                '} else {'
                '  false;'
                '}' % self.test_settings[num_fishes]),
            expected_value=True,
            timeout_sec=30)

        if self.kernel_sampler:
            self.kernel_sampler.start_sampling_thread()
        time.sleep(self.test_duration_secs)
        if self.kernel_sampler:
            self.kernel_sampler.stop_sampling_thread()
            self.kernel_sampler.output_flip_stats('flip_stats_%d' % num_fishes)
            self.flip_stats = {}

        if perf_log:
            # Get average FPS and rendering time, then close the tab.
            avg_fps = tab.EvaluateJavaScript('g_crosFpsCounter.getAvgFps();')
            if math.isnan(float(avg_fps)):
                raise error.TestFail('Could not get FPS count.')
            avg_render_time = tab.EvaluateJavaScript(
                'g_crosFpsCounter.getAvgRenderTime();')
            self.perf_keyval['avg_fps_%04d_fishes' % num_fishes] = avg_fps
            self.perf_keyval['avg_render_time_%04d_fishes' % num_fishes] = (
                avg_render_time)
            self.output_perf_value(
                description='avg_fps_%04d_fishes' % num_fishes,
                value=avg_fps,
                units='fps',
                higher_is_better=True)
            logging.info('%d fish(es): Average FPS = %f, '
                         'average render time = %f', num_fishes, avg_fps,
                         avg_render_time)

    def run_power_test(self, browser, test_url):
        """Runs the webgl power consumption test and reports the perf results.

        @param browser: The Browser object to run the test with.
        @param test_url: The URL to the aquarium test site.
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

        measurements = [
            power_status.SystemPower(self._power_status.battery_path)
        ]

        def get_power():
            power_logger = power_status.PowerLogger(measurements)
            power_logger.start()
            time.sleep(STABILIZATION_DURATION)
            start_time = time.time()
            time.sleep(MEASUREMENT_DURATION)
            power_logger.checkpoint('result', start_time)
            keyval = power_logger.calc()
            logging.info('Power output %s', keyval)
            return keyval['result_' + measurements[0].domain + '_pwr']

        self.run_fish_test(browser, test_url, 1000, perf_log=False)
        energy_rate = get_power()
        # This is a power specific test so we are not capturing
        # avg_fps and avg_render_time in this test.
        self.perf_keyval[POWER_DESCRIPTION] = energy_rate
        self.output_perf_value(description=POWER_DESCRIPTION,
                               value=energy_rate,
                               units='W',
                               higher_is_better=False)

    def exynos_sampler_callback(self, sampler_obj):
        """Sampler callback function for ExynosSampler.

        @param sampler_obj: The ExynosSampler object that invokes this callback
                function.
        """
        if sampler_obj.stopped:
            return

        with self.sampler_lock:
            now = time.time()
            results = {}
            info_str = ['\nfb_id wait_kds flipped']
            for value in sampler_obj.frame_buffers.itervalues():
                results[value.fb] = {}
                for state, stats in value.states.iteritems():
                    results[value.fb][state] = (stats.avg, stats.stdev)
                info_str.append('%s: %s %s' %
                                (value.fb, results[value.fb]['wait_kds'][0],
                                 results[value.fb]['flipped'][0]))
            results['avg_fps'] = self.active_tab.EvaluateJavaScript(
                'g_crosFpsCounter.getAvgFps();')
            results['avg_render_time'] = self.active_tab.EvaluateJavaScript(
                'g_crosFpsCounter.getAvgRenderTime();')
            self.active_tab.ExecuteJavaScript('g_crosFpsCounter.reset();')
            info_str.append('avg_fps: %s, avg_render_time: %s' %
                            (results['avg_fps'], results['avg_render_time']))
            self.flip_stats[now] = results
            logging.info('\n'.join(info_str))

    def exynos_output_flip_stats(self, file_name):
        """Pageflip statistics output function for ExynosSampler.

        @param file_name: The output file name.
        """
        # output format:
        # time fb_id avg_rendered avg_prepared avg_wait_kds avg_flipped
        # std_rendered std_prepared std_wait_kds std_flipped
        with open(file_name, 'w') as f:
            for t in sorted(self.flip_stats.keys()):
                if ('avg_fps' in self.flip_stats[t] and
                        'avg_render_time' in self.flip_stats[t]):
                    f.write('%s %s %s\n' %
                            (t, self.flip_stats[t]['avg_fps'],
                             self.flip_stats[t]['avg_render_time']))
                for fb, stats in self.flip_stats[t].iteritems():
                    if not isinstance(fb, int):
                        continue
                    f.write('%s %s ' % (t, fb))
                    f.write('%s %s %s %s ' % (stats['rendered'][0],
                                              stats['prepared'][0],
                                              stats['wait_kds'][0],
                                              stats['flipped'][0]))
                    f.write('%s %s %s %s\n' % (stats['rendered'][1],
                                               stats['prepared'][1],
                                               stats['wait_kds'][1],
                                               stats['flipped'][1]))
    def run_once(self,
                 test_duration_secs=30,
                 test_setting_num_fishes=(50, 1000),
                 power_test=False):
        """Find a brower with telemetry, and run the test.

        @param test_duration_secs: The duration in seconds to run each scenario
                for.
        @param test_setting_num_fishes: A list of the numbers of fishes to
                enable in the test.
        """
        self.test_duration_secs = test_duration_secs
        self.test_setting_num_fishes = test_setting_num_fishes

        with chrome.Chrome(logged_in=False) as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.srcdir)
            test_url = cr.browser.platform.http_server.UrlOf(os.path.join(
                self.srcdir, 'aquarium.html'))

            if not utils.wait_for_idle_cpu(60.0, 0.1):
                if not utils.wait_for_idle_cpu(20.0, 0.2):
                    raise error.TestFail('Could not get idle CPU.')
            if not utils.wait_for_cool_machine():
                raise error.TestFail('Could not get cold machine.')
            if power_test:
                self._test_power = True
                self.run_power_test(cr.browser, test_url)
                with self.sampler_lock:
                    self.active_tab.Close()
                    self.active_tab = None
            else:
                for n in self.test_setting_num_fishes:
                    self.run_fish_test(cr.browser, test_url, n)
                    # Do not close the tab when the sampler_callback is doing
                    # his work.
                    with self.sampler_lock:
                        self.active_tab.Close()
                        self.active_tab = None
        self.write_perf_keyval(self.perf_keyval)

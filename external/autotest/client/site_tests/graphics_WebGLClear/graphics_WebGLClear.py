# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This is a client side WebGL clear test that measures how many frames-per-second
(FPS) can be achieved (possibly limited by vsync) with glClear only.  It can be
useful to gauge that most WebGL applications won't likely achieve anything
higher than the FPS output of this test, assuming all else equal.  It may also
be useful to various types of graphics developers as a sanity check.  However,
your mileage may vary and we make no claims with respect to any particular
platform or application.

This test is expected to run 30 seconds by default.  Any run that reports a FPS
value and finishes is a PASS.  An acceptable FPS value is subjective and
platform, driver and/or project dependent... interpretation of the FPS results
are left up to the interested parties.
"""

import logging
import os
import time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.graphics import graphics_utils


class graphics_WebGLClear(test.test):
    """WebGL clear graphics test."""
    version = 1
    GSC = None
    perf_keyval = {}
    test_duration_secs = 30

    def setup(self):
        self.job.setup_dep(['webgl_clear'])

    def initialize(self):
        self.GSC = graphics_utils.GraphicsStateChecker()
        self.perf_keyval = {}

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

    def run_clear_test(self, browser, test_url):
        """Runs the clear test from the given url.

        @param browser: The Browser object to run the test with.
        @param test_url: The URL to the clear test suite.
        """
        tab = browser.tabs.New()
        tab.Navigate(test_url)
        tab.Activate()
        tab.WaitForDocumentReadyStateToBeComplete()
        time.sleep(self.test_duration_secs)
        avg_fps = tab.EvaluateJavaScript('g_fpsTimer.averageFPS;')
        self.perf_keyval['avg_fps'] = avg_fps
        self.output_perf_value(description='avg_fps',
                               value=avg_fps,
                               units='fps',
                               higher_is_better=True)
        logging.info('Average FPS = %f', avg_fps)

        tab.Close()

    def run_once(self, test_duration_secs=30):
        """Finds a brower with telemetry, and run the test.

        @param test_duration_secs: The test duration in seconds to run the test
                for.
        """
        self.test_duration_secs = test_duration_secs

        # For this to have any noticable effect, SwapbuffersWait needs to be false
        # in xorg.conf.
        browser_args = '--disable-gpu-vsync'

        with chrome.Chrome(logged_in=False,
                           extra_browser_args=browser_args) as cr:
            clearsrc = os.path.join(self.autodir, 'deps', 'webgl_clear', 'src')
            if not cr.browser.platform.SetHTTPServerDirectories(clearsrc):
                raise error.TestError('Unable to start HTTP server')
            test_url = cr.browser.platform.http_server.UrlOf(os.path.join(
                clearsrc, 'WebGLClear.html'))
            self.run_clear_test(cr.browser, test_url)

        self.write_perf_keyval(self.perf_keyval)

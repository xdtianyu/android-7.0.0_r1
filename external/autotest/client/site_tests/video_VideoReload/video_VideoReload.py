# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

WAIT_TIMEOUT_S = 30

class video_VideoReload(test.test):
    """This test verifies reloading video works in Chrome."""
    version = 1

    def run_once(self, html):
        """Tests whether Chrome reloads video after reloading the tab.

        @param html: Sample html file to be loaded and reloaded in Chrome.
        """
        with chrome.Chrome() as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab = cr.browser.tabs[0]
            tab.Navigate(cr.browser.platform.http_server.UrlOf(
                    os.path.join(self.bindir, html)))

            def is_video_at_start():
                """Checks if video is at the start position."""
                return tab.EvaluateJavaScript(
                        '(typeof videoAtStart != "undefined") && videoAtStart')

            # Expect video being loaded and started for the first time.
            utils.poll_for_condition(
                    is_video_at_start,
                    exception=error.TestError('Video is not started'),
                    timeout=WAIT_TIMEOUT_S,
                    sleep_interval=1)

            # Reload the tab after playing video for a while.
            tab.EvaluateJavaScript('playAndReload()')

            # Expect video being restarted after reloading the tab.
            utils.poll_for_condition(
                    is_video_at_start,
                    exception=error.TestError('Video is not restarted'),
                    timeout=WAIT_TIMEOUT_S,
                    sleep_interval=1)

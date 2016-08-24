# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import sys_power

class power_VideoSuspend(test.test):
    """Suspend the system with a video playing."""
    version = 1

    def run_once(self, video_urls=None):
        if video_urls is None:
            raise error.TestError('no videos to play')

        with chrome.Chrome() as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab = cr.browser.tabs[0]
            tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'play.html')))
            tab.WaitForDocumentReadyStateToBeComplete()

            for url in video_urls:
                self.suspend_with_video(cr.browser, tab, url)


    def check_video_is_playing(self, tab):
        def get_current_time():
            return tab.EvaluateJavaScript('player.currentTime')

        old_time = get_current_time()
        utils.poll_for_condition(
            condition=lambda: get_current_time() > old_time,
            exception=error.TestError('Player stuck until timeout.'))


    def suspend_with_video(self, browser, tab, video_url):
        logging.info('testing %s', video_url)
        tab.EvaluateJavaScript('play("%s")' % video_url)

        self.check_video_is_playing(tab)

        time.sleep(2)
        sys_power.kernel_suspend(10)
        time.sleep(2)

        self.check_video_is_playing(tab)

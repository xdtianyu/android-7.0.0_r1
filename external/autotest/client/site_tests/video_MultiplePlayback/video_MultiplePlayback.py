# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, time


from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import youtube_helper


FLASH_PROCESS_NAME = 'chrome/chrome --type=ppapi'
PLAYER_PLAYING_STATE = 'Playing'
PLAYBACK_TEST_TIME_S = 10


class video_MultiplePlayback(test.test):
    """This test verify simultaneous video playback.
    We are testing using Youtube html5, flash and a local video.

    """
    version = 1


    def verify_localvideo_playback(self, tab1):
        """To verify local video playback

        @param tab1: browser tab.
        """

        playback = 0 # seconds
        prev_playback = 0
        while (int(tab1.EvaluateJavaScript('testvideo.currentTime'))
               < int(tab1.EvaluateJavaScript('testvideo.duration'))
               and playback < PLAYBACK_TEST_TIME_S):
            if (int(tab1.EvaluateJavaScript('testvideo.currentTime'))
                    <= prev_playback):
                raise error.TestError('Video is not playing.')
            prev_playback = int(tab1.EvaluateJavaScript(
                    'testvideo.currentTime'))
            time.sleep(1)
            playback = playback + 1


    def run_video_tests(self, browser):
        """Play youtube html5, flash and a loca video, and verify the playback.

        @param browser: The Browser object to run the test with.

        """
        browser.platform.SetHTTPServerDirectories(self.bindir)
        tab1 = browser.tabs.New()
        # Verifying <video> support.
        tab1.Navigate(browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'video.html')))

        # Waiting for test video to load.
        tab1.WaitForJavaScriptExpression('testvideo.currentTime < 1.0', 5)

        tab2 = browser.tabs.New()
        tab2.Navigate(browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'youtube5.html')))
        yh = youtube_helper.YouTubeHelper(tab2)
        # Waiting for test video to load.
        yh.wait_for_player_state(PLAYER_PLAYING_STATE)
        yh.set_video_duration()
        # Verify that YouTube is running in html5 mode.
        prc = utils.get_process_list('chrome', '--type=ppapi')
        if prc:
            raise error.TestFail('Tab2: Running YouTube in Flash mode.')

        tab3 = browser.tabs.New()
        tab3.Navigate(browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'youtube.html')))
        yh1 = youtube_helper.YouTubeHelper(tab3)
        # Waiting for test video to load.
        yh1.wait_for_player_state(PLAYER_PLAYING_STATE)
        yh1.set_video_duration()
        # Verify that YouTube is running in html5 mode.
        prc1 = utils.get_process_list('chrome', '--type=ppapi')
        if not prc1:
            raise error.TestFail('Tab3: No flash process is Running .')

        # Verifying video playback.
        self.verify_localvideo_playback(tab1)
        yh.verify_video_playback()
        yh1.verify_video_playback()


    def run_once(self):
        # TODO(scottz): Remove this when crbug.com/220147 is fixed.
        dut_board = utils.get_current_board()
        if dut_board == 'x86-mario':
           raise error.TestNAError('This test is not available on %s' %
                                    dut_board)
        with chrome.Chrome() as cr:
            self.run_video_tests(cr.browser)

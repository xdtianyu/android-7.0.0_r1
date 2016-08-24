# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import httpd
from autotest_lib.client.cros.video import youtube_helper


FLASH_PROCESS_NAME = 'chrome/chrome --type=ppapi'
PLAYER_PLAYING_STATE = 'Playing'


class video_YouTubeHTML5(test.test):
    """This test verify the YouTube HTML5 video.

    - verify the video playback.
    - verify the available video resolutions.
    - verify the player functionalities.

    """
    version = 2


    def initialize(self):
        self._testServer = httpd.HTTPListener(8000, docroot=self.bindir)
        self._testServer.run()


    def cleanup(self):
        if self._testServer:
            self._testServer.stop()


    def run_youtube_tests(self, browser):
        """Run YouTube HTML5 sanity tests.

        @param browser: The Browser object to run the test with.

        """
        tab = browser.tabs[0]
        tab.Navigate('http://localhost:8000/youtube5.html')
        yh = youtube_helper.YouTubeHelper(tab)
        # Waiting for test video to load.
        yh.wait_for_player_state(PLAYER_PLAYING_STATE)
        yh.set_video_duration()

        # Verify that YouTube is running in html5 mode.
        prc = utils.get_process_list('chrome', '--type=ppapi( |$)')
        if prc:
            raise error.TestFail('Running YouTube in Flash mode. '
                                 'Process list: %s.' % prc)

        tab.ExecuteJavaScript('player.mute()')
        yh.verify_video_playback()
        yh.verify_video_resolutions()
        yh.verify_player_states()


    def run_once(self):
        with chrome.Chrome() as cr:
            self.run_youtube_tests(cr.browser)

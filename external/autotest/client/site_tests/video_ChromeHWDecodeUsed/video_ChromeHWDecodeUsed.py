# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import time
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import native_html5_player


class video_ChromeHWDecodeUsed(test.test):
    """This test verifies VDA works in Chrome."""
    version = 1


    def run_once(self, is_mse, video_file):
        """
        Tests whether VDA works by verifying histogram for the loaded video.

        @param is_mse: bool, True if the content uses MSE, False otherwise.
        @param video_file: Sample video file to be loaded in Chrome.

        """
        with chrome.Chrome() as cr:
            # This will execute for MSE video by accesing shaka player
            if is_mse:
                 tab1 = cr.browser.tabs[0]
                 tab1.Navigate(video_file)
                 tab1.WaitForDocumentReadyStateToBeComplete()
                 # Running the test longer to check errors and longer playback
                 # for MSE videos.
                 time.sleep(30)
            else:
            #This execute for normal video for downloading html file
                 shutil.copy2(constants.VIDEO_HTML_FILEPATH, self.bindir)
                 cr.browser.platform.SetHTTPServerDirectories(self.bindir)
                 tab = cr.browser.tabs[0]
                 html_fullpath = os.path.join(self.bindir, 'video.html')
                 url = cr.browser.platform.http_server.UrlOf(html_fullpath)
                 player = native_html5_player.NativeHtml5Player(
                         tab,
                         full_url = url,
                         video_id = 'video',
                         video_src_path = video_file,
                         event_timeout = 120)
                 player.load_video()
                 player.play()

            # Waits for histogram updated for the test video.
            histogram_verifier.verify(
                    cr,
                    constants.MEDIA_GVD_INIT_STATUS,
                    constants.MEDIA_GVD_BUCKET)

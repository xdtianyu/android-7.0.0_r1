# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import time, logging, shutil

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import native_html5_player


class video_ChromeVidResChangeHWDecode(test.test):
    """Verify that VDA works in Chrome for video with resolution changes."""
    version = 1


    def run_once(self, video_file, video_len):
        """Verify VDA and playback for the video_file.

        @param video_file: test video file.
        @param video_len : test video file length.
        """

        with chrome.Chrome() as cr:
            shutil.copy2(constants.VIDEO_HTML_FILEPATH, self.bindir)
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab1 = cr.browser.tabs[0]
            html_fullpath = os.path.join(self.bindir, 'video.html')
            url = cr.browser.platform.http_server.UrlOf(html_fullpath)
            logging.info("full url is %s", url)
            player = native_html5_player.NativeHtml5Player(tab1,
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

            # Verify the video playback.
            for i in range(1, video_len/2):
                if (player.paused() or player.ended()):
                    raise error.TestError('Video either stopped or ended.')
                time.sleep(1)

            # Verify that video ends successfully.
            utils.poll_for_condition(
                    lambda: player.ended(),
                    timeout=video_len,
                    exception=error.TestError('Video did not end successfully'),
                    sleep_interval=1)

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from contextlib import closing
import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import histogram_verifier


# Chrome flags to use fake camera and skip camera permission.
EXTRA_BROWSER_ARGS = ['--use-fake-device-for-media-stream',
                      '--use-fake-ui-for-media-stream']
FAKE_FILE_ARG = '--use-file-for-fake-video-capture="%s"'
DOWNLOAD_BASE = 'http://commondatastorage.googleapis.com/chromiumos-test-assets-public/crowd/'

HISTOGRAMS_URL = 'chrome://histograms/'


class video_ChromeRTCHWDecodeUsed(test.test):
    """The test verifies HW Encoding for WebRTC video."""
    version = 1


    def start_loopback(self, cr):
        """
        Opens WebRTC loopback page.

        @param cr: Autotest Chrome instance.
        """
        tab = cr.browser.tabs[0]
        tab.Navigate(cr.browser.platform.http_server.UrlOf(
            os.path.join(self.bindir, 'loopback.html')))
        tab.WaitForDocumentReadyStateToBeComplete()


    def run_once(self, video_name, histogram_name, histogram_bucket_val):
        # Download test video.
        url = DOWNLOAD_BASE + video_name
        local_path = os.path.join(self.bindir, video_name)
        file_utils.download_file(url, local_path)

        # Start chrome with test flags.
        EXTRA_BROWSER_ARGS.append(FAKE_FILE_ARG % local_path)
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS) as cr:
            # Open WebRTC loopback page.
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            self.start_loopback(cr)

            # Make sure decode is hardware accelerated.
            histogram_verifier.verify(cr, histogram_name, histogram_bucket_val)

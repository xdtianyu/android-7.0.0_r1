# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream',
                      '--use-fake-device-for-media-stream',
                      '--enable-experimental-web-platform-features']

# Statistics from the loopback.html page.
TEST_PROGRESS = 'testProgress'

# Polling timeout
TIMEOUT = 90


class video_WebRtcMediaRecorder(test.test):
    """WebRTC Media Recorder test."""
    version = 1


    def launch_recorder_test(self, test_name):
        """Launch a recorder test.

        @param test_name: Name of test to run.
        """
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS) as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            self.tab = cr.browser.tabs[0]
            self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                    os.path.join(self.bindir, 'loopback_mediarecorder.html')))
            self.tab.WaitForDocumentReadyStateToBeComplete()
            self.tab.EvaluateJavaScript(test_name + "();")
            if not self.is_test_completed():
                logging.error('%s did not complete', test_name)
                raise error.TestFail('Failed %s' %(test_name))
            try:
                result = self.tab.EvaluateJavaScript('result;')
            except:
                logging.error('Cannot retrieve results from javascript')
                raise error.TestFail('Failed %s' %(test_name))
            if result != 'PASS':
                raise error.TestFail('Failed %s, got %s' %(test_name,
                        result))


    def is_test_completed(self):
        """Checks if WebRTC MediaRecorder test is done.

        @returns True if test complete, False otherwise.

        """
        def test_done():
            """Check the testProgress variable in HTML page."""

            # Wait for test completion on web page.
            test_progress = self.tab.EvaluateJavaScript(TEST_PROGRESS)
            return test_progress == 1

        try:
            utils.poll_for_condition(
                    test_done, timeout=TIMEOUT,
                    exception=error.TestError('Cannot find testProgress.'),
                    sleep_interval=1)
        except error.TestError:
            return False
        else:
            return True


    def run_once(self):
        """Runs the video_WebRtcMediaRecorder test."""
        self.launch_recorder_test('testStartAndRecorderState')
        self.launch_recorder_test('testStartStopAndRecorderState')
        self.launch_recorder_test('testStartAndDataAvailable')
        self.launch_recorder_test('testStartWithTimeSlice')
        self.launch_recorder_test('testResumeAndRecorderState')
        self.launch_recorder_test('testIllegalResumeThrowsDOMError')
        self.launch_recorder_test('testResumeAndDataAvailable')


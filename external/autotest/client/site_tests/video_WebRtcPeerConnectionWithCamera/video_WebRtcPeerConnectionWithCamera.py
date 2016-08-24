# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream']

# Statistics from the loopback.html page.
TEST_PROGRESS = 'testProgress'

# Polling timeout
TIMEOUT = 90

RES_720P = [1280, 720]
RES_VGA = [640, 480]

# max number of allowed blackframes or frozen frames
BLACK_FRAMES_THRESHOLD = 10
FROZEN_FRAMES_THRESHOLD = 10

class video_WebRtcPeerConnectionWithCamera(test.test):
    """Local Peer connection test with webcam at 720p."""
    version = 1

    def start_loopback(self, cr):
        """Opens WebRTC loopback page.

        @param cr: Autotest Chrome instance.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        self.tab = cr.browser.tabs[0]
        self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'loopback.html')))
        self.tab.WaitForDocumentReadyStateToBeComplete()
        self.tab.EvaluateJavaScript("testCamera(%s)" %
                                    self.chosen_resolution)


    def webcam_supports_720p(self):
        """Checks if 720p capture supported.

        @returns: True if 720p supported, false if VGA is supported.
        @raises: TestError if neither 720p nor VGA are supported.
        """
        cmd = 'lsusb -v'
        # Get usb devices and make output a string with no newline marker.
        usb_devices = utils.system_output(cmd, ignore_status=True).splitlines()
        usb_devices = ''.join(usb_devices)

        # Check if 720p resolution supported.
        if re.search(r'\s+wWidth\s+1280\s+wHeight\s+720', usb_devices):
            return True
        # The device should support at least VGA.
        # Otherwise the cam must be broken.
        if re.search(r'\s+wWidth\s+640\s+wHeight\s+480', usb_devices):
            return False
        # This should not happen.
        raise error.TestFail(
                'Could not find any cameras reporting '
                'either VGA or 720p in lsusb output: %s' % usb_devices)


    def is_test_completed(self):
        """Checks if WebRTC peerconnection test is done.

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
                    exception=error.TestError('Cannot find testProgress value.'),
                    sleep_interval=1)
        except error.TestError:
            partial_results = self.tab.EvaluateJavaScript('getResults()')
            logging.info('Here are the partial results so far: %s',
                         partial_results)
            return False
        else:
            return True


    def run_once(self):
        """Runs the video_WebRtcPeerConnectionWithCamera test."""
        # Check webcamera resolution capabilities.
        # Some laptops have low resolution capture.
        if self.webcam_supports_720p():
          self.chosen_resolution = RES_720P
        else:
          self.chosen_resolution =  RES_VGA
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS) as cr:
            # Open WebRTC loopback page and start the loopback.
            self.start_loopback(cr)
            if not self.check_loopback_result():
                raise error.TestFail('Failed %s local peer connection test' %
                                     self.chosen_resolution)


    def check_loopback_result(self):
        """Get the WebRTC Peerconnection loopback results."""
        if not self.is_test_completed():
            logging.error('loopback.html did not complete')
            return False
        try:
            results = self.tab.EvaluateJavaScript('getResults()')
        except:
            logging.error('Cannot retrieve results from loopback.html page')
            return False
        logging.info('Camera Type: %s', results['cameraType'])
        logging.info('Camera Errors: %s', results['cameraErrors'])
        logging.info('PeerConnectionstats: %s',
                     results['peerConnectionStats'])
        logging.info('FrameStats: %s', results['frameStats'])
        if results['cameraErrors']:
            logging.error('Camera error: %s', results['cameraErrors'])
            return False
        if not results['peerConnectionStats']:
            logging.info('Peer Connection Stats is empty')
            return False
        if results['peerConnectionStats'][1] == 0:
            logging.error('Max Input FPS is zero')
            return False
        if results['peerConnectionStats'][4] == 0:
            logging.error('Max Sent FPS is zero')
            return False
        if not results['frameStats']:
            logging.error('Frame Stats is empty')
            return False
        if results['frameStats']['numBlackFrames'] > BLACK_FRAMES_THRESHOLD:
            logging.error('BlackFrames threshold overreach: '
                          'got %s > %s allowed',
                          results['frameStats']['numBlackFrames'],
                          BLACK_FRAMES_THRESHOLD)
            return False
        if results['frameStats']['numFrozenFrames'] > FROZEN_FRAMES_THRESHOLD:
            logging.error('Frozen Frames threshold overreach: '
                          'got %s > %s allowed',
                          results['frameStats']['numFrozenFrames'],
                          FROZEN_FRAMES_THRESHOLD)
            return False
        if results['frameStats']['numFrames'] == 0:
            logging.error('%s Frames were found',
                          results['frameStats']['numFrames'])
            return False
        return True

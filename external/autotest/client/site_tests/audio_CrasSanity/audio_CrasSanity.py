# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import cras_utils

_CRASH_PATH = '/var/spool/crash'
_DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com/'
                  'chromiumos-test-assets-public/')


class audio_CrasSanity(test.test):
    """Verifies cras sanity using its status, active streams and crashes"""

    version = 1

    _audio = ('audio.mp3', 'audio.wav', 'audio.m4a')
    _video = ('traffic-1920x1080-8005020218f6b86bfa978e550d04956e.mp4',
              'traffic-1920x1080-ad53f821ff3cf8ffa7e991c9d2e0b854.vp8.webm',
              'traffic-1920x1080-83a1e5f8b7944577425f039034e64c76.vp9.webm')

    _check = {'crashes_on_boot': False,
              'stream_activation': False,
              'cras_status': False,
              'crashes_at_end': False
             }

    def run_once(self):
        boards_to_skip = ['x86-mario']
        dut_board = utils.get_current_board()
        if dut_board in boards_to_skip:
            logging.info("Skipping test run on this board.")
            return
        # Check for existing cras crashes which might occur during UI bring up.
        # TODO: (rohitbm) check if we need to reboot the DUT before the test
        #       start to verify cras crashes during boot.
        existing_crash_reports = self.collect_cras_crash()
        if len(existing_crash_reports) == 0:
            self._check['crashes_on_boot'] = True

        # Capturing cras pid before startig the test.
        cras_pid_1 = utils.get_oldest_pid_by_name('/usr/bin/cras')

        with chrome.Chrome() as self._cr:
            try:
                # This will be used on Chrome PFQ since it's using a more recent
                # version of Chrome. crbug.com/537655.
                self._cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            except:
                # This will be used on ChromeOS CQ since Chrome hasn't uprev'ed
                # yet. crbug.com/538140.
                self._cr.browser.SetHTTPServerDirectories(self.bindir)
            for test_file in self._audio:
                url = _DOWNLOAD_BASE + 'audio_test/' + test_file
                self.push_new_stream(self._cr.browser.tabs.New(), url)

            # Capturing cras pid before opening a new set of audio streams.
            cras_pid_2 = utils.get_oldest_pid_by_name('/usr/bin/cras')
            for test_file in self._video:
                url = _DOWNLOAD_BASE + 'traffic/' + test_file
                self.push_new_stream(self._cr.browser.tabs.New(), url)

            # Let's play audio for sometime to ensure that
            # long playback is good.
            time.sleep(10)

            total_tests = len(self._audio) + len(self._video)
            active_streams = cras_utils.get_active_stream_count()
            logging.debug('NUmber of active streams after opening all tabs: %d.'
                          % active_streams)
            if active_streams >= total_tests:
                self._check['stream_activation'] = True

            # Capturing cras pid after opening all audio/video streams.
            cras_pid_3 = utils.get_oldest_pid_by_name('/usr/bin/cras')

            # Close all open audio streams.
            while total_tests > 0:
                self._cr.browser.tabs[total_tests].Close()
                total_tests -= 1
                time.sleep(1)
            active_streams = cras_utils.get_active_stream_count()
            logging.debug('Number of active streams after closing all tabs: %d.'
                          % active_streams)

            # Capturing cras pid after closing all audio/stream streams.
            cras_pid_4 = utils.get_oldest_pid_by_name('/usr/bin/cras')

            if cras_pid_1 == cras_pid_2 == cras_pid_3 == cras_pid_4:
                self._check['cras_status'] = True

        new_crash_reports = self.collect_cras_crash()
        new_reports = list(set(new_crash_reports) -
                           set(existing_crash_reports))
        if len(new_reports) == 0:
            self._check['crashes_at_end'] = True

        err_msg = ''
        if self._check.values().count(False) > 0:
            if not self._check['crashes_on_boot']:
                err_msg = ('1. Found cras crashes on boot: %s.\n'
                           % existing_crash_reports)
            if not self._check['stream_activation']:
                err_msg += ('2. CRAS stream count is not matching with '
                            'number of streams.\n')
            if not self._check['cras_status']:
                err_msg += ('CRAS PID changed during the test. CRAS might be '
                            'crashing while adding/removing streams.\n')
            if not self._check['crashes_at_end']:
                err_msg += ('Found cras crashes at the end of the test : %s.' %
                            new_reports)
            raise error.TestError(err_msg)


    def push_new_stream(self, tab, url):
        """Starts next audio stream from self._streams list.

        @param tab: tab to open an audio stream.
        @url: audio/video test url.
        """
        tab.Activate()
        tab.Navigate(url)
        tab.ExecuteJavaScript(
                "document.getElementsByTagName('video')[0].loop=true")
        tab.ExecuteJavaScript(
                "document.getElementsByTagName('video')[0].volume=0.1")
        time.sleep(1) # Adding a delay so cras can update the active count.


    def collect_cras_crash(self):
        """Check for cras crashes.

        @return a list of cras crash reports found.
        """

        crash_reports = []
        if not os.path.isdir(_CRASH_PATH):
            logging.debug('No cras crash detected!')
        else:
            cras_reports = os.listdir(_CRASH_PATH)
            crash_reports = [report for report in cras_reports
                             if report.startswith('cras')]
        return crash_reports

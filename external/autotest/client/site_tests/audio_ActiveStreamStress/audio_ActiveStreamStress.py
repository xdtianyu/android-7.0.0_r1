# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import random
import shutil
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import cras_utils

_STRESS_ITERATIONS = 100 # Total number of iterations.
_MAX_OPENED_TAB = 20     # Max number of tabs open and playing audio.
_RETAIN_TAB = 5          # In case we hit the _MAX_OPENED_TAB limit,
                         # close all except last 5 tabs.
_MAX_TABS_TO_OPEN = 10   # Max number of tabs can be opened in one iteration.
_CRASH_PATH = '/var/spool/crash'


class audio_ActiveStreamStress(test.test):
    """Verifies the active audio streams."""

    version = 1

    _active_stream_count = 0
    _existing_cras_reports = []
    _cr = None
    # TODO(rohitbm): add more(including video) file types and download them from gs://.
    _streams = ('audio.mp3', 'audio.wav', 'audio.m4a')
    _stream_index = 0
    _tab_count = 0

    def run_once(self):

        # Collect existing cras crash reports.
        self._existing_cras_reports = self.collect_cras_crash()

        with chrome.Chrome() as self._cr:
            self._cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            self.push_new_stream(self._cr.browser.tabs[0])
            # TODO(rohitbm): decide whether to perform verification on each
            # open/close or at end of the iteration.
            self.verify_active_streams()
            push_count = 0
            pop_count = 0

            # Stress test logic:
            # Test runs for n number of iterations. For one iteration,
            # a = random(10) tabs(streams) are created and
            # b = random(a) tabs are closed. If the next iteration finds that,
            # total number of opened tabs are more than _MAX_OPENED_TAB,
            # test will close (total opened tabs - 5) tabs.
            # This will balance number of opened tabs and will allow to close
            # tabs in a control manner.

            for count in xrange(1, _STRESS_ITERATIONS):
                if self._tab_count > _MAX_OPENED_TAB:
                     for i in xrange(1, (self._tab_count - _RETAIN_TAB)):
                         pop_count += 1
                         self.pop_stream()
                         logging.info('Total streams closed: %d', pop_count)
                random_tab = random.randint(1, 10)
                for i in xrange(1, random_tab):
                    push_count += 1
                    self.push_new_stream(self._cr.browser.tabs.New())
                    logging.info('Total new streams created: %d', push_count)
                time.sleep(5) # Delay for active streams to play.
                for i in xrange(1, random.randint(1, random_tab)):
                    pop_count += 1
                    self.pop_stream()
                    logging.info('Total streams closed: %d', pop_count)


    def get_stream_index(self):
        if self._stream_index == len(self._streams):
            # Reset the stream index if the index reached to the end.
            self._stream_index = 0
        return self._stream_index


    def push_new_stream(self, tab):
        """Starts next audio stream from self._streams list.

        @param tab: tab to open an audio stream.
        """
        self._tab_count += 1
        tab.Navigate(self._cr.browser.platform.http_server.UrlOf(
                    os.path.join(self.bindir,
                                 self._streams[self.get_stream_index()])))
        tab.ExecuteJavaScript(
                "document.getElementsByTagName('video')[0].loop=true")
        # TODO(rohitbm): add playback verification.
        self._stream_index += 1
        self._active_stream_count += 1
        time.sleep(1) # Adding a delay so cras can update the active count.
        self.verify_active_streams()


    def pop_stream(self):
        """Turns off the first available stream by closing the first tab."""
        if len(self._cr.browser.tabs) > 0:
            self._cr.browser.tabs[0].Close()
            self._tab_count -= 1
            self._active_stream_count -= 1
        time.sleep(1) # Adding delay so cras can update the active count.
        self.verify_active_streams()


    def verify_active_streams(self):
        """Verifies test active audio streams with cras active streams."""
        cras_stream_count = cras_utils.get_active_stream_count()
        if self._active_stream_count != cras_stream_count:
            cras_crash_reports = self.collect_cras_crash()
            new_reports = list(set(cras_crash_reports) -
                               set(self._existing_cras_reports))
            error_msg = ('Active stream count: %d is not matching with '
                         'cras active stream count: %d. '
                         'Number of cras crashes %d : %s' %
                         (self._active_stream_count, cras_stream_count,
                         len(new_reports), new_reports))
            raise error.TestError(error_msg)


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

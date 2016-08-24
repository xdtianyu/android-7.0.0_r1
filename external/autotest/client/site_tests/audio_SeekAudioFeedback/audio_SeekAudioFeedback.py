# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio import sox_utils


NUMBER_OF_SEEKING = 6
SEEKING_TIME = 50

class audio_SeekAudioFeedback(audio_helper.chrome_rms_test):
    """Verifies audio output on seeking forward/back."""

    version = 1

    def run_once(self, test_file, test_duration):
        self._rms_values = {}
        noise_file = os.path.join(self.resultsdir, 'noise.wav')
        noiseprof_file = tempfile.NamedTemporaryFile()

        # Record a sample of "silence" to use as a noise profile.
        cras_utils.capture(noise_file, duration=2)
        sox_utils.noise_profile(noise_file, noiseprof_file.name)

        # Open the test page
        self.chrome.browser.platform.SetHTTPServerDirectories(self.bindir)
        tab = self.chrome.browser.tabs[0]
        tab.Navigate(self.chrome.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'play.html')))
        tab.WaitForDocumentReadyStateToBeComplete()

        # Test audio file.
        self.rms_test(tab, test_file, noiseprof_file.name, test_duration)
        self.write_perf_keyval(self._rms_values)


    def rms_test(self, tab, test_file, noiseprof_file, test_duration):
        logging.info('rms test on media file %s.', test_file)
        recorded_file = os.path.join(self.resultsdir, 'recorded.wav')
        loopback_file = os.path.join(self.resultsdir, 'loopback.wav')


        # Plays the test_file in the browser and seek 6 times.
        for x in range(0,NUMBER_OF_SEEKING):
            self.play_media(tab, test_file, x)
            # Record the audio output and also the CRAS loopback output.
            p1 = cmd_utils.popen(cras_utils.capture_cmd(
                    recorded_file, duration=test_duration ))
            p2 = cmd_utils.popen(cras_utils.loopback_cmd(
                    loopback_file, duration=test_duration ))
            cmd_utils.wait_and_check_returncode(p1, p2)

            # See if we recorded something.

            # We captured two channels of audio in the CRAS loopback.
            # The RMS values are for debugging only.
            loopback_stats = [audio_helper.get_channel_sox_stat(
                    loopback_file, i) for i in (1, 2)]
            logging.info('loopback stat: %s', [str(s) for s in loopback_stats])

            reduced_file = tempfile.NamedTemporaryFile()
            sox_utils.noise_reduce(
                    recorded_file, reduced_file.name, noiseprof_file)
            rms = audio_helper.get_rms(reduced_file.name)[0]

            self._rms_values['%s_rms_value' % test_file.replace('.', '_')]=rms


    def wait_player_end(self, tab):
        """Wait for player ends playing."""
        utils.poll_for_condition(
            condition=lambda: tab.EvaluateJavaScript('player.ended'),
            exception=error.TestError('Player never end until timeout.'))


    def play_media(self, tab, test_file, value):
        """Plays a media file in Chromium.

        @param test_file: Media file to test.
        @param vlaue: Index of the loop
        """
        tab.EvaluateJavaScript('play("%s")' % test_file)
        def get_current_time():
            return tab.EvaluateJavaScript('player.currentTime')

        if value <=(NUMBER_OF_SEEKING/2):
            new_seek = (value * SEEKING_TIME)
        else:
            new_seek = (((NUMBER_OF_SEEKING - value) * SEEKING_TIME))
        # Make sure the audio is being played
        old_time = get_current_time()
        utils.poll_for_condition(
            condition=lambda: get_current_time() > old_time,
            exception=error.TestError('Player never start until timeout.'))
        tab.EvaluateJavaScript('player.currentTime = %d' % new_seek)

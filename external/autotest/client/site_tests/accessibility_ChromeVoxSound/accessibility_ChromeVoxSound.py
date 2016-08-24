# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import logging

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio import sox_utils


class accessibility_ChromeVoxSound(test.test):
    """Check whether ChromeVox makes noise on real hardware."""
    version = 1

    _audio_chunk_size = 1 # Length of chunk size in seconds.
    _detect_time = 20 # Max length of time to spend detecting audio in seconds.


    def _enable_ChromeVox(self):
        """Enable ChromeVox using a11y API call."""
        cmd = '''
            window.__result = false;
            chrome.accessibilityFeatures.spokenFeedback.set({value: true});
            chrome.accessibilityFeatures.spokenFeedback.get({},
                function(d) {window.__result = d[\'value\'];}
            );
        '''
        self._extension.ExecuteJavaScript(cmd)
        utils.poll_for_condition(
                lambda: self._extension.EvaluateJavaScript('window.__result'),
                exception = error.TestError(
                        'Timeout waiting for ChromeVox to be enabled.'))


    def _detect_audio(self):
        """Detects whether audio was heard and returns the approximate time.

        Runs for at most self._detect_time, checking each chunk for sound.
        After first detecting a chunk that has audio, counts the subsequent
        chunks that also do.

        @return: Approximate length of time in seconds there was audio.

        """
        count = 0
        counting = False
        for i in xrange(self._detect_time / self._audio_chunk_size):
            rms = self._rms_of_next_audio_chunk()
            if rms > 0:
                logging.info('Found passing chunk: %d.', i)
                count += 1
                counting = True
            elif counting:
                return count * self._audio_chunk_size

        logging.warning('Timeout before end of audio!')
        return count * self._audio_chunk_size


    def _rms_of_next_audio_chunk(self):
        """Finds the sox_stats values of the next chunk of audio."""
        cras_utils.loopback(self._loopback_file, channels=1,
                            duration=self._audio_chunk_size)
        stat_output = sox_utils.get_stat(self._loopback_file)
        logging.info(stat_output)
        return vars(stat_output)['rms']


    def warmup(self):
        self._loopback_file = os.path.join(self.bindir, 'cras_loopback.wav')


    def run_once(self):
        """Entry point of this test."""
        extension_path = os.path.join(os.path.dirname(__file__), 'a11y_ext')

        with chrome.Chrome(extension_paths=[extension_path],
                           is_component=False) as cr:
            # Setup ChromeVox extension
            self._extension = cr.get_extension(extension_path)

            # Begin actual test
            logging.info('Detecting initial ChromeVox welcome sound.')
            self._enable_ChromeVox()
            audio_length = self._detect_audio()
            if audio_length < 1:
                raise error.TestError('No sound after enabling Chromevox!')

            logging.info('Detecting initial ChromeVox welcome speech.')
            audio_length = self._detect_audio()
            if audio_length < 2:
                raise error.TestError('Speech after enabling ChromeVox was <= '
                                      '%f seconds long!' % audio_length)

            logging.info('Detecting page navigation sound.')
            cr.browser.tabs[0].Navigate('chrome://version')
            audio_length = self._detect_audio()
            if audio_length < 2:
                raise error.TestError('Speech after loading a page was <= '
                                      '%f seconds long!' % audio_length)

            logging.info('Detecting new tab sound.')
            tab = cr.browser.tabs.New()
            audio_length = self._detect_audio()
            if audio_length < 1:
                raise error.TestError('No sound after opening new tab!')


    def cleanup(self):
        try:
            os.remove(self._loopback_file)
        except OSError:
            pass



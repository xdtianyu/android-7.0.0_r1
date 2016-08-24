# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils


TEST_DURATION = 1

class audio_CrasLoopback(audio_helper.cras_rms_test):
    """Verifies audio playback and capture function."""
    version = 1

    @staticmethod
    def wait_for_active_stream_count(expected_count):
        utils.poll_for_condition(
            lambda: cras_utils.get_active_stream_count() == expected_count,
            exception=error.TestError(
                'Timeout waiting active stream count to become %d' %
                 expected_count))


    def run_once(self):
        """Entry point of this test."""

        # Multitone wav file lasts 10 seconds
        wav_path = os.path.join(self.bindir, '10SEC.wav')

        noise_file = os.path.join(self.resultsdir, 'cras_noise.wav')
        recorded_file = os.path.join(self.resultsdir, 'cras_recorded.wav')

        # Record a sample of "silence" to use as a noise profile.
        cras_utils.capture(noise_file, duration=1)

        self.wait_for_active_stream_count(0)
        p = cmd_utils.popen(cras_utils.playback_cmd(wav_path))
        try:
            self.wait_for_active_stream_count(1)
            cras_utils.capture(recorded_file, duration=TEST_DURATION)

            # Make sure the audio is still playing.
            if p.poll() != None:
                raise error.TestError('playback stopped')
        finally:
            cmd_utils.kill_or_log_returncode(p)

        rms_value = audio_helper.reduce_noise_and_get_rms(
                recorded_file, noise_file)[0]
        self.write_perf_keyval({'rms_value': rms_value})


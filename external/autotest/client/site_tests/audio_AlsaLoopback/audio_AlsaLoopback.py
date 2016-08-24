# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import alsa_utils
from autotest_lib.client.cros.audio import cmd_utils


TEST_DURATION = 1

class audio_AlsaLoopback(audio_helper.alsa_rms_test):
    """Verifies audio playback and capture function."""
    version = 1

    def run_once(self):
        """Entry point of this test."""

        # Multitone wav file lasts 10 seconds
        wav_path = os.path.join(self.bindir, '10SEC.wav')

        noise_file = os.path.join(self.resultsdir, 'hw_noise.wav')
        recorded_file = os.path.join(self.resultsdir, 'hw_recorded.wav')

        # Record a sample of "silence" to use as a noise profile.
        alsa_utils.record(noise_file, duration=1)

        p = cmd_utils.popen(alsa_utils.playback_cmd(wav_path))
        try:
            # Wait one second to make sure the playback has been started.
            time.sleep(1)
            alsa_utils.record(recorded_file, duration=TEST_DURATION)

            # Make sure the audio is still playing.
            if p.poll() != None:
                raise error.TestError('playback stopped')
        finally:
            cmd_utils.kill_or_log_returncode(p)

        rms_value = audio_helper.reduce_noise_and_get_rms(
            recorded_file, noise_file)[0]

        self.write_perf_keyval({'rms_value': rms_value})


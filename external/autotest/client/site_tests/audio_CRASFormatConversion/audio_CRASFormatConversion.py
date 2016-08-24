# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio import sox_utils


_TEST_TONE_ONE = 440
_TEST_TONE_TWO = 523

class audio_CRASFormatConversion(audio_helper.cras_rms_test):
    version = 1


    def play_sine_tone(self, frequence, rate):
        """Plays a sine tone by cras and returns the processes.
        Args:
            frequence: the frequence of the sine wave.
            rate: the sampling rate.
        """
        p1 = cmd_utils.popen(
            sox_utils.generate_sine_tone_cmd(
                    filename='-', rate=rate, frequence=frequence, gain=-6),
            stdout=cmd_utils.PIPE)
        p2 = cmd_utils.popen(
            cras_utils.playback_cmd(playback_file='-', rate=rate),
            stdin=p1.stdout)
        return [p1, p2]


    def wait_for_active_stream_count(self, expected_count):
        utils.poll_for_condition(
                lambda: cras_utils.get_active_stream_count() == expected_count,
                exception=error.TestError(
                        'Timeout waiting active stream count to become %d' %
                        expected_count),
                timeout=1, sleep_interval=0.05)

    def loopback(self, noise_profile, primary, secondary):
        """Gets the rms value of the recorded audio of playing two different
        tones (the 440 and 523 Hz sine wave) at the specified sampling rate.

        @param noise_profile: The noise profile which is used to reduce the
                              noise of the recored audio.
        @param primary: The first sample rate, HW will be set to this.
        @param secondary: The second sample rate, will be SRC'd to the first.
        """
        popens = []

        record_file = os.path.join(self.resultsdir,
                'record-%s-%s.wav' % (primary, secondary))

        # There should be no other active streams.
        self.wait_for_active_stream_count(0)

        # Start with the primary sample rate, then add the secondary.  This
        # causes the secondary to be SRC'd to the primary rate.
        try:
            # Play the first audio stream and make sure it has been played
            popens += self.play_sine_tone(_TEST_TONE_ONE, primary)
            self.wait_for_active_stream_count(1)

            # Play the second audio stream and make sure it has been played
            popens += self.play_sine_tone(_TEST_TONE_TWO, secondary)
            self.wait_for_active_stream_count(2)

            cras_utils.capture(record_file, duration=1, rate=44100)

            # Make sure the playback is still in good shape
            if any(p.poll() is not None for p in popens):
                # We will log more details later in finally.
                raise error.TestFail('process unexpectly stopped')

            reduced_file = tempfile.NamedTemporaryFile()
            sox_utils.noise_reduce(
                    record_file, reduced_file.name, noise_profile, rate=44100)

            sox_stat = sox_utils.get_stat(reduced_file.name, rate=44100)

            logging.info('The sox stat of (%d, %d) is %s',
                         primary, secondary, str(sox_stat))

            return sox_stat.rms

        finally:
            cmd_utils.kill_or_log_returncode(*popens)

    def run_once(self, test_sample_rates):
        """Runs the format conversion test.
        """

        rms_values = {}

        # Record silence to use as the noise profile.
        noise_file = os.path.join(self.resultsdir, "noise.wav")
        noise_profile = tempfile.NamedTemporaryFile()
        cras_utils.capture(noise_file, duration=1)
        sox_utils.noise_profile(noise_file, noise_profile.name)

        # Try all sample rate pairs.
        for primary in test_sample_rates:
            for secondary in test_sample_rates:
                key = 'rms_value_%d_%d' % (primary, secondary)
                rms_values[key] = self.loopback(
                        noise_profile.name, primary, secondary)

        # Record at all sample rates
        record_file = tempfile.NamedTemporaryFile()
        for rate in test_sample_rates:
            cras_utils.capture(record_file.name, duration=1, rate=rate)

        # Add min_rms_value to the result
        rms_values['min_rms_value'] = min(rms_values.values())

        self.write_perf_keyval(rms_values)

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess
import tempfile
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.feedback import client
from autotest_lib.server import test


_BITS_PER_BYTE = 8
# The amount of time to wait when producing silence (i.e. no playback).
_SILENCE_DURATION_SECS = 5

# Number of channels to generate.
_DEFAULT_NUM_CHANNELS = 1
# Sine wave sample rate (48kHz).
_DEFAULT_SAMPLE_RATE = 48000
# Sine wave default sample format is signed 16-bit PCM (two bytes).
_DEFAULT_SAMPLE_WIDTH = 2
# Default sine wave frequency.
_DEFAULT_SINE_FREQUENCY = 440
# Default duration of the sine wave in seconds.
_DEFAULT_DURATION_SECS = 10

class brillo_PlaybackAudioTest(test.test):
    """Verify that basic audio playback works."""
    version = 1

    def __init__(self, *args, **kwargs):
        super(brillo_PlaybackAudioTest, self).__init__(*args, **kwargs)
        self.host = None


    def _get_playback_cmd(self, method, dut_play_file):
        """Get the playback command to execute based on the playback method.

        @param method: A string specifiying which method to use.
        @param dut_play_file: A string containing the path to the file to play
                              on the DUT.
        @return: A string containing the command to play audio using the
                 specified method.

        @raises TestError: Invalid playback method.
        """
        if dut_play_file:
            return 'su root slesTest_playFdPath %s 0' % dut_play_file
        if method == 'libmedia':
            return 'brillo_audio_test --playback --libmedia --sine'
        elif method == 'stagefright':
            return 'brillo_audio_test --playback --stagefright --sine'
        elif method == 'opensles':
            return 'slesTest_sawtoothBufferQueue'
        else:
            raise error.TestError('Test called with invalid playback method.')


    def test_playback(self, fb_query, playback_cmd, sample_width, sample_rate,
                      duration_secs, num_channels, play_file_path=None):
        """Performs a playback test.

        @param fb_query: A feedback query.
        @param playback_cmd: The playback generating command, or None for no-op.
        @param play_file_path: A string of the path to the file being played.
        @param sample_width: Sample width to test playback at.
        @param sample_rate: Sample rate to test playback at.
        @param num_channels: Number of channels to test playback with.
        """
        fb_query.prepare(sample_width=sample_width,
                         sample_rate=sample_rate,
                         duration_secs=duration_secs,
                         num_channels=num_channels)
        if playback_cmd:
            self.host.run(playback_cmd)
        else:
            time.sleep(_SILENCE_DURATION_SECS)
        if play_file_path:
            fb_query.validate(audio_file=play_file_path)
        else:
            fb_query.validate()


    def run_once(self, host, fb_client, playback_method, use_file=False,
                 sample_width=_DEFAULT_SAMPLE_WIDTH,
                 sample_rate=_DEFAULT_SAMPLE_RATE,
                 num_channels=_DEFAULT_NUM_CHANNELS,
                 duration_secs=_DEFAULT_DURATION_SECS):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param fb_client: A feedback client implementation.
        @param playback_method: A string representing a playback method to use.
                                Either 'opensles', 'libmedia', or 'stagefright'.
        @param use_file: Use a file to test audio. Must be used with
                         playback_method 'opensles'.
        @param sample_width: Sample width to test playback at.
        @param sample_rate: Sample rate to test playback at.
        @param num_channels: Number of channels to test playback with.
        @param duration_secs: Duration to play file for.
        """
        self.host = host
        with fb_client.initialize(self, host):
            logging.info('Testing silent playback')
            fb_query = fb_client.new_query(client.QUERY_AUDIO_PLAYBACK_SILENT)
            self.test_playback(fb_query=fb_query,
                               playback_cmd=None,
                               sample_rate=sample_rate,
                               sample_width=sample_width,
                               num_channels=num_channels,
                               duration_secs=duration_secs)

            dut_play_file = None
            host_filename = None
            if use_file:
                _, host_filename = tempfile.mkstemp(
                        prefix='sine-', suffix='.wav',
                        dir=tempfile.mkdtemp(dir=fb_client.tmp_dir))
                if sample_width == 1:
                    sine_format = '-e unsigned'
                else:
                    sine_format = '-e signed'
                gen_file_cmd = ('sox -n -t wav -c %d %s -b %d -r %d %s synth %d '
                       'sine %d vol 0.9' % (num_channels, sine_format,
                                            sample_width * _BITS_PER_BYTE,
                                            sample_rate, host_filename,
                                            duration_secs,
                                            _DEFAULT_SINE_FREQUENCY))
                logging.info('Command to generate sine wave: %s', gen_file_cmd)
                subprocess.call(gen_file_cmd, shell=True)
                logging.info('Send file to DUT.')
                dut_tmp_dir = '/data'
                dut_play_file = os.path.join(dut_tmp_dir, 'sine.wav')
                logging.info('dut_play_file %s', dut_play_file)
                host.send_file(host_filename, dut_play_file)

            logging.info('Testing audible playback')
            fb_query = fb_client.new_query(client.QUERY_AUDIO_PLAYBACK_AUDIBLE)
            playback_cmd = self._get_playback_cmd(playback_method, dut_play_file)

            self.test_playback(fb_query=fb_query,
                               playback_cmd=playback_cmd,
                               sample_rate=sample_rate,
                               sample_width=sample_width,
                               num_channels=num_channels,
                               duration_secs=duration_secs,
                               play_file_path=host_filename)

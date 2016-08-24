# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import alsa_utils


APLAY_FILE = '/dev/zero' # raw data

# Expected results of 'aplay -v' commands.
APLAY_EXPECTED = set([
      ('stream', 'PLAYBACK')])


def _play_audio(duration=1):
    """Play a tone and try to ensure it played properly.

    Sample output from aplay -v:

    Playing raw data '/dev/zero' : Signed 16 bit Little Endian, Rate 44100 Hz,
    Stereo
    Hardware PCM card 0 'HDA Intel PCH' device 0 subdevice 0
    Its setup is:
      stream       : PLAYBACK
      access       : RW_INTERLEAVED  format       : S16_LE
      subformat    : STD
      channels     : 2
      rate         : 44100
      exact rate   : 44100 (44100/1)
      msbits       : 16
      buffer_size  : 16384
      period_size  : 4096
      period_time  : 92879
      tstamp_mode  : NONE
      period_step  : 1
      avail_min    : 4096
      period_event : 0
      start_threshold  : 16384
      stop_threshold   : 16384
      silence_threshold: 0
      silence_size : 0
      boundary     : 4611686018427387904
      appl_ptr     : 0
      hw_ptr       : 0

    @param duration: Duration supplied to aplay.
    @return String output from the command (may be empty).
    @raises CmdError when cmd returns <> 0.
    """
    device = alsa_utils.get_sysdefault_playback_device()
    cmd = ['aplay',
           '-v', # show verbose details
           '-D %s' % device,  # select default device
           '-d %d' % duration,
           '-f cd', # format
           APLAY_FILE,
           '2>&1'] # verbose details
    return utils.system_output(' '.join(cmd)).strip()


def _check_play(duration, expected):
    """Runs aplay command and checks the output against an expected result.

    The expected results are compared as sets of tuples.

    @param duration: Duration supplied to aplay.
    @param expected: The set of expected tuples.
    @raises error.TestError for invalid output or invalidly matching expected.
    """
    error_msg = 'invalid response from aplay'
    results = _play_audio(duration)
    if not results.startswith("Playing raw data '%s' :" % APLAY_FILE):
        raise error.TestError('%s: %s' % (error_msg, results))
    result_set = utils.set_from_keyval_output(results, '[\s]*:[\s]*')
    if set(expected) <= result_set:
        return
    raise error.TestError('%s: expected=%s.' %
                          (error_msg, sorted(set(expected) - result_set)))


class audio_Aplay(test.test):
    """Checks that simple aplay functions correctly."""
    version = 1


    def run_once(self, duration=1):
        """Run aplay and verify its output is as expected.

        @param duration: the duration to run aplay in seconds.
        """
        _check_play(duration, APLAY_EXPECTED)

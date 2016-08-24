# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper


_DEFAULT_VOLUME_LEVEL = 100
_DEFAULT_CAPTURE_GAIN = 2500

_LATENCY_DIFF_LIMIT_US = 3000
_NOISE_THRESHOLD = 1600

class audio_LoopbackLatency(test.test):
    """Verifies if the measured latency is as accurate as reported"""
    version = 1

    def initialize(self,
                   default_volume_level=_DEFAULT_VOLUME_LEVEL,
                   default_capture_gain=_DEFAULT_CAPTURE_GAIN):
        """Setup the deps for the test.

        Args:
            default_volume_level: The default volume level.
            defalut_capture_gain: The default capture gain.

        Raises: error.TestError if the deps can't be run
        """
        self._volume_level = default_volume_level
        self._capture_gain = default_capture_gain

        super(audio_LoopbackLatency, self).initialize()

    def run_once(self):
        """Entry point of this test"""
        audio_helper.set_volume_levels(self._volume_level, self._capture_gain)
        success = False

        # Run loopback latency check once, which takes at most 1 sec to
        # complete and parse the latency values measured in loopback path
        # and reported by system.  Assert the difference is within
        # acceptable range.
        result = audio_helper.loopback_latency_check(n=_NOISE_THRESHOLD)
        if result:
            diff = abs(result[0] - result[1])
            logging.info('Tested latency with threshold %d.\nMeasured %d,'
                         'reported %d uS, diff %d us\n', _NOISE_THRESHOLD,
                         result[0], result[1], diff)

            # Difference between measured and reported latency should
            # within _LATENCY_DIFF_LIMIT_US.
            if diff < _LATENCY_DIFF_LIMIT_US:
                success = True
        else:
            # Raise error if audio is not detected at all in the loopback path.
            raise error.TestError('Audio not detected at threshold %d' %
                                  _NOISE_THRESHOLD)

        if not success:
            # Test fails when latency difference is greater then the limit.
            raise error.TestFail('Latency difference too much, diff limit '
                                 '%d us, measured %d us, reported %d us' %
                                 (_LATENCY_DIFF_LIMIT_US, result[0], result[1]))

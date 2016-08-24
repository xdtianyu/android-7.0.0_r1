# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import tempfile

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import alsa_utils, cras_utils

DURATION = 3
TOLERANT_RATIO = 0.1

class audio_Microphone(test.test):
    version = 1


    def check_recorded_filesize(
            self, filesize, duration, channels, rate, bits=16):
        expected = duration * channels * (bits / 8) * rate
        if abs(float(filesize) / expected - 1) > TOLERANT_RATIO:
            raise error.TestFail('File size not correct: %d' % filesize)


    def verify_alsa_capture(self, channels, rate, bits=16):
        recorded_file = tempfile.NamedTemporaryFile()
        alsa_utils.record(
                recorded_file.name, duration=DURATION, channels=channels,
                bits=bits, rate=rate)
        self.check_recorded_filesize(
                os.path.getsize(recorded_file.name),
                DURATION, channels, rate, bits)


    def verify_cras_capture(self, channels, rate):
        recorded_file = tempfile.NamedTemporaryFile()
        cras_utils.capture(
                recorded_file.name, duration=DURATION, channels=channels,
                rate=rate)
        self.check_recorded_filesize(
                os.path.getsize(recorded_file.name),
                DURATION, channels, rate)


    def run_once(self):
        # Mono and stereo capturing should work fine @ 44.1KHz and 48KHz.
        # Verify recording using ALSA utils.
        self.verify_alsa_capture(1, 44100)
        self.verify_alsa_capture(1, 48000)
        self.verify_alsa_capture(2, 48000)
        self.verify_alsa_capture(2, 44100)
        # Verify recording of CRAS.
        self.verify_cras_capture(1, 44100)
        self.verify_cras_capture(1, 48000)
        self.verify_cras_capture(2, 48000)
        self.verify_cras_capture(2, 44100)

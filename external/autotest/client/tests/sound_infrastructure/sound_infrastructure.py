# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re
import stat
import subprocess

from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import test

_SND_DEV_DIR = '/dev/snd/'

class sound_infrastructure(test.test):
    """
    Tests that the expected sound infrastructure is present.

    Check that at least one playback and capture device exists and that their
    permissions are configured properly.

    """
    version = 2

    def check_snd_dev_perms(self, filename):
        desired_mode = (stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP |
                        stat.S_IWGRP | stat.S_IFCHR)
        st = os.stat(filename)
        if (st.st_mode != desired_mode):
            raise error.TestError("Incorrect permissions for %s" % filename)

    def check_sound_files(self):
        patterns = {'^controlC(\d+)': False,
                    '^pcmC(\d+)D(\d+)p$': False,
                    '^pcmC(\d+)D(\d+)c$': False}

        filenames = os.listdir(_SND_DEV_DIR)

        for filename in filenames:
            for pattern in patterns:
                if re.match(pattern, filename):
                    patterns[pattern] = True
                    self.check_snd_dev_perms(_SND_DEV_DIR + filename)

        for pattern in patterns:
            if not patterns[pattern]:
                raise error.TestError("Missing device %s" % pattern)

    def check_aplay_list(self):
        no_cards_pattern = '.*no soundcards found.*'

        aplay = subprocess.Popen(["aplay", "-l"], stderr=subprocess.PIPE)
        aplay_list = aplay.communicate()[1]
        if aplay.returncode or re.match(no_cards_pattern, aplay_list):
            raise error.TestError("No playback devices found by aplay")

        arecord = subprocess.Popen(["arecord", "-l"], stderr=subprocess.PIPE)
        arecord_list = arecord.communicate()[1]
        if arecord.returncode or re.match(no_cards_pattern, arecord_list):
            raise error.TestError("No playback devices found by arecord")

    def run_once(self):
        self.check_sound_files()
        self.check_aplay_list()

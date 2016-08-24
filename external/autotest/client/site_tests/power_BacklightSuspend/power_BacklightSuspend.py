# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import power_utils, sys_power


class power_BacklightSuspend(test.test):
    version = 1

    def run_once(self, resume_percent=70):
        results = {}
        backlight = power_utils.Backlight()

        results['initial_brightness'] = backlight.get_level()
        max_level = backlight.get_max_level()
        resume_level = int(round(max_level * resume_percent / 100))

        # If the current brightness is the same as the requested brightness,
        # request 100 - |resume_brightness| instead.
        if resume_level == results['initial_brightness']:
            resume_level = max_level - resume_level
        backlight.set_resume_level(resume_level)

        sys_power.kernel_suspend(seconds=5)

        final_level = backlight.get_level()
        if final_level != resume_level:
            raise error.TestFail(
                ('Brightness level after resume did not match requested ' + \
                 'brightness: %d vs %d') % (final_level, resume_level))

        results['resume_brightness'] = resume_level
        self.write_perf_keyval(results)

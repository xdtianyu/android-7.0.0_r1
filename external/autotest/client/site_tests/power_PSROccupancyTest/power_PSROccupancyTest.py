# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import os.path
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros.graphics import graphics_utils


class power_PSROccupancyTest(test.test):
    """
    Tests that PSR is entered on a static content.

    The purpose of this test is to verify that display enters the PSR state when
    the content being displayed is static. It also verifies that the display
    stays in PSR state for as long as the display is static. It first enables
    PSR if not enabled and makes the displayed content static. It then waits for
    some time, after which it checks whether the display was in PSR state for
    close to (<wait time> - <vblankoffdelay>).
    """
    version = 1


    def _is_psr_enabled(self):
        enable_psr_file_path = '/sys/module/i915/parameters/enable_psr'
        if not os.path.exists(enable_psr_file_path):
            raise error.TestFail('sysfs entry for "enable_psr" is missing.')
        return int(utils.read_file(enable_psr_file_path)) == 1


    def _get_perf_count(self):
        debugfs_file_path = '/sys/kernel/debug/dri/0/i915_edp_psr_status'
        if not os.path.exists(debugfs_file_path):
            raise error.TestFail('debugfs entry for PSR status is missing.')
        psr_status = utils.read_file(debugfs_file_path).splitlines()
        if len(psr_status) != 4:
            raise error.TestFail(
                    'Incorrect number of lines in %s.' % debugfs_file_path)
        perf_count_chunks = psr_status[3].split()
        if len(perf_count_chunks) != 2:
            raise error.TestFail('Unknown format in %s.' % debugfs_file_path)
        return int(perf_count_chunks[1])


    def _get_vblank_timeout(self):
        return int(utils.read_file('/sys/module/drm/parameters/vblankoffdelay'))


    def run_once(self):
        if utils.get_board() not in ['samus', 'gandof']:
            raise error.TestNAError(
                    'Trying to run PSR tests on unsupported board.')
        psr_enabled = self._is_psr_enabled()
        if (not psr_enabled and
            graphics_utils.call_xrandr('--output eDP1 --set psr on')):
            error.TestFail('Unable to enable PSR via xrandr.')
        # Start chrome in full screen mode so that there is no blinking cursor
        # or ticking clock on the screen.
        with chrome.Chrome(logged_in=False, extra_browser_args=['--kiosk']):
            # Sample the PSR performance count from debugfs and wait for 20s.
            # At the end of 20s, re-sample the PSR performance count. The time
            # spent in PSR should be close to (20s - <vblankoffdelay>).
            sleep_time_milliseconds = 20 * 1000
            min_occupancy = 0.9 * (sleep_time_milliseconds -
                                   self._get_vblank_timeout())
            perf_count_old = self._get_perf_count()
            time.sleep(sleep_time_milliseconds / 1000)
            perf_count_new = self._get_perf_count()
            occupancy_time = perf_count_new - perf_count_old
            if occupancy_time < min_occupancy:
                raise error.TestFail(
                        'PSR occupancy time %dms less than expected.' %
                        occupancy_time)
            # Disable PSR if it was not enabled to begin with.
            if (not psr_enabled and
                graphics_utils.call_xrandr('--output eDP1 --set psr off')):
                raise error.TestWarn('Unable to disable PSR via xrandr.')

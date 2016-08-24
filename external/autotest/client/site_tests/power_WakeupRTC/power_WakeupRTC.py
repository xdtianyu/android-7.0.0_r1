# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno, logging, os, shutil, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros import rtc, sys_power
from autotest_lib.client.common_lib import error


def read_rtc_wakeup(rtc_device):
    """
    Read the wakeup setting for for the RTC device.
    """
    sysfs_path = '/sys/class/rtc/%s/device/power/wakeup' % rtc_device
    return file(sysfs_path).read().strip()


def read_rtc_wakeup_count(rtc_device):
    """
    Read the current wakeup count for the RTC device.
    """
    sysfs_path = '/sys/class/rtc/%s/device/power/wakeup_count' % rtc_device
    return int(file(sysfs_path).read())


def fire_wakealarm(rtc_device):
    """
    Schedule a wakealarm and wait for it to fire.
    """
    rtc.set_wake_alarm('+1', rtc_device)
    time.sleep(2)


class power_WakeupRTC(test.test):
    """Test RTC wake events."""

    version = 1

    def run_once(self):
        for rtc_device in rtc.get_rtc_devices():
            self.run_once_rtc(rtc_device)

    def run_once_rtc(self, rtc_device):
        logging.info('testing rtc device %s', rtc_device)

        # Test that RTC wakeup is enabled
        rtc_wakeup = read_rtc_wakeup(rtc_device)
        if rtc_wakeup != 'enabled':
            raise error.TestError('RTC wakeup is not enabled: %s' % rtc_device)

        # Test that RTC can generate wake events
        old_sys_wakeup_count = sys_power.read_wakeup_count()
        old_rtc_wakeup_count = read_rtc_wakeup_count(rtc_device)
        fire_wakealarm(rtc_device)
        new_sys_wakeup_count = sys_power.read_wakeup_count()
        new_rtc_wakeup_count = read_rtc_wakeup_count(rtc_device)
        if new_rtc_wakeup_count == old_rtc_wakeup_count:
            raise error.TestFail(
                    'RTC alarm should increase RTC wakeup_count: %s'
                    % rtc_device)
        if new_sys_wakeup_count == old_sys_wakeup_count:
            raise error.TestFail(
                    'RTC alarm should increase system wakeup_count: %s'
                    % rtc_device)

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can
# be # found in the LICENSE file.

"""Provides utility methods for the Real Time Clock device.
"""

import errno, glob, os


def get_rtc_devices():
    """
    Return a list of all RTC device names on the system.

    The RTC device node will be found at /dev/$NAME.
    """
    return [os.path.basename(rtc) for rtc in glob.glob('/sys/class/rtc/*')]


def get_seconds(utc=True, rtc_device='rtc0'):
    """
    Read the current time out of the RTC
    """
    return int(file('/sys/class/rtc/%s/since_epoch' % rtc_device).readline())


def write_wake_alarm(alarm_time, rtc_device='rtc0'):
    """
    Write a value to the wake alarm
    """
    f = file('/sys/class/rtc/%s/wakealarm' % rtc_device, 'w')
    f.write('%s\n' % str(alarm_time))
    f.close()


def set_wake_alarm(alarm_time, rtc_device='rtc0'):
    """
    Set the hardware RTC-based wake alarm to 'alarm_time'.
    """
    try:
        write_wake_alarm(alarm_time, rtc_device)
    except IOError as (errnum, strerror):
        if errnum != errno.EBUSY:
            raise
        write_wake_alarm('clear', rtc_device)
        write_wake_alarm(alarm_time, rtc_device)

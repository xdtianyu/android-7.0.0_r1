# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module contains some commonly used time conversion function.

import datetime
import time


# This format is used to parse datetime value in MySQL database and should not
# be modified.
TIME_FMT = '%Y-%m-%d %H:%M:%S'
TIME_FMT_MICRO = '%Y-%m-%d %H:%M:%S.%f'

def time_string_to_datetime(time_string, handle_type_error=False):
    """Convert a string of time to a datetime object.

    The format of date string must match '%Y-%m-%d %H:%M:%S' or
    '%Y-%m-%d %H:%M:%S.%f'.

    @param time_string: String of date, e.g., 2014-12-05 15:32:45
    @param handle_type_error: Set to True to prevent the method raise
            TypeError if given time_string is corrupted. Default is False.

    @return: A datetime object with time of the given date string.

    """
    try:
        try:
            return datetime.datetime.strptime(time_string, TIME_FMT)
        except ValueError:
            return datetime.datetime.strptime(time_string, TIME_FMT_MICRO)
    except TypeError:
        if handle_type_error:
            return None
        else:
            raise


def date_string_to_epoch_time(date_string):
    """Parse a date time string into seconds since the epoch.

    @param date_string: A string, formatted according to `TIME_FMT`.

    @return The number of seconds since the UNIX epoch, as a float.

    """
    return time.mktime(time.strptime(date_string, TIME_FMT))


def epoch_time_to_date_string(epoch_time, fmt_string=TIME_FMT):
    """Convert epoch time (float) to a human readable date string.

    @param epoch_time The number of seconds since the UNIX epoch, as
                      a float.
    @param fmt_string: A string describing the format of the datetime
        string output.

    @returns: string formatted in the following way: "yyyy-mm-dd hh:mm:ss"
    """
    if epoch_time:
        return datetime.datetime.fromtimestamp(
                int(epoch_time)).strftime(fmt_string)
    return None


def to_epoch_time(value):
    """Convert the given value to epoch time.

    Convert the given value to epoch time if it is a datetime object or a string
    can be converted to datetime object.
    If the given value is a number, this function assume the value is a epoch
    time value, and returns the value itself.

    @param value: A datetime object or a number.
    @returns: epoch time if value is datetime.datetime,
              otherwise returns the value.
    @raise ValueError: If value is not a datetime object or a number.
    """
    if isinstance(value, basestring):
        value = time_string_to_datetime(value)
    if isinstance(value, datetime.datetime):
        return time.mktime(value.timetuple()) + 0.000001 * value.microsecond
    if not isinstance(value, int) and not isinstance(value, float):
        raise ValueError('Value should be a datetime object, string or a '
                         'number. Unexpected value: %s.' % value)
    return value

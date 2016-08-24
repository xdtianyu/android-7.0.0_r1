# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error


def get_datetime_float(host):
    """
    Returns host's system time in seconds since epoch.

    @param host: an Autotest host object.
    @returns a float, timestamp since epoch in <seconds>.<nanoseconds>.

    @raises TestError: if error reading datetime or converting its value
                       from string to float.
    """
    r = host.run('date +%s.%N')
    if r.exit_status > 0:
        err = ('Error reading datetime from capturer (%r): %r' %
               (r.exit_status, r.stderr))
        raise error.TestError(err)
    try:
        return float(r.stdout)
    except ValueError as e:
        raise error.TestError('Error converting datetime string: %r', e)

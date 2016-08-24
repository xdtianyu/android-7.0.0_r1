# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


_DEFAULT_PING_HOST = 'www.google.com'
_DEFAULT_PING_COUNT = 4
_DEFAULT_PING_TIMEOUT = 4


class brillo_PingTest(test.test):
    """Ping an Internet host."""
    version = 1

    def run_once(self, host=None, ping_host=_DEFAULT_PING_HOST,
                 ping_count=_DEFAULT_PING_COUNT,
                 ping_timeout=_DEFAULT_PING_TIMEOUT):
        """Pings an Internet host with given timeout and count values.

        @param host: A host object representing the DUT.
        @param ping_host: The Internet host to ping.
        @param ping_count: The number of pings to attempt. The test passes if
                           we get at least one reply.
        @param ping_timeout: The number of seconds to wait for a reply.

        @raise TestFail: The test failed.
        """
        cmd = 'ping -q -c %s -W %s %s' % (ping_count, ping_timeout, ping_host)
        try:
            host.run(cmd)
        except error.AutoservRunError:
            raise error.TestFail(
                    'Failed to ping %s in %d seconds on all %d attempts' %
                    (ping_host, ping_timeout, ping_count))

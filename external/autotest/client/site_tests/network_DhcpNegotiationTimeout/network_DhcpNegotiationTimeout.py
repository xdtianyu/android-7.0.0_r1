# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.cros import dhcp_test_base

class network_DhcpNegotiationTimeout(dhcp_test_base.DhcpTestBase):
    """The DHCP Negotiation Timeout class.

    Sets up a virtual ethernet pair, stops the DHCP server on the pair,
    restarts shill, and waits for DHCP to timeout.

    After the timeout interval, checks if the same shill process is
    running. If not, report failure.

    """
    SHILL_DHCP_TIMEOUT_SECONDS = 30


    @staticmethod
    def get_daemon_pid(daemon_name):
        """
        Get the PID of a running daemon that is managed by upstart.

        Query upstart for the PID of |daemon_name|, and return the PID.
        If the daemon is unknown, or not running, raise an exception.

        @return The PID as an integer.

        """
        cmd_result = \
            utils.run("status %s" % daemon_name, ignore_status=True)
        if cmd_result.stdout.find("start/running") != -1:
            # Example: "shill start/running, process 445"
            return int(cmd_result.stdout.split()[3])
        else:
            if len(cmd_result.stdout):
                logging.debug("upstart stdout is %s", cmd_result.stdout)
            if len(cmd_result.stderr):
                logging.debug("upstart stderr is %s", cmd_result.stderr)
            raise error.TestFail('Failed to get pid of %s' % daemon_name)


    def test_body(self):
        """Test main loop."""
        self.server.stop()
        utils.run("restart shill")
        start_pid = self.get_daemon_pid("shill")

        time.sleep(self.SHILL_DHCP_TIMEOUT_SECONDS + 2)
        end_pid = self.get_daemon_pid("shill")
        if end_pid != start_pid:
            raise error.TestFail("shill restarted (probably crashed)")

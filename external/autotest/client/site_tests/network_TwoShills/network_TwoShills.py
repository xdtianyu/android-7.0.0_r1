# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

class network_TwoShills(test.test):
    """Test that only one shill runs at a time"""
    SHILL_EXIT_TIMEOUT_SECONDS = 10
    version = 1


    @staticmethod
    def is_shill_running():
        """
        Check if shill is running.

        @return True or False.

        """
        cmd_result = utils.run("status shill", ignore_status=True)
        return cmd_result.stdout.find("start/running") != -1


    @staticmethod
    def get_default_netdev():
        """
        Get the name of the network device with the default route.

        @return A string such as "eth0" or "wlan0".

        """
        cmd_result = utils.run(
            "ip route show default match 0/0 | awk '{print $5}'")
        return cmd_result.stdout


    def run_once(self):
        """Test main loop."""
        if not self.is_shill_running():
            raise error.TestFail("shill not running at start")

        default_netdev = self.get_default_netdev()
        if len(default_netdev) < 1:
            raise error.TestFail("unable to determine default network device")

        try:
            # Run shill, expecting it to abort quickly. If the new
            # process does not exit within the allotted time,
            # base_utils.run() will kill the new process
            # explicitly. (First with SIGTERM, then SIGKILL.)
            cmd_result = utils.run(
                "shill --foreground --device-black-list=%s" % default_netdev,
                timeout=self.SHILL_EXIT_TIMEOUT_SECONDS,
                ignore_status = True)
        except error.CmdTimeoutError:
            raise error.TestFail("shill did not exit within %d seconds" %
                                 self.SHILL_EXIT_TIMEOUT_SECONDS)

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros import dhcpv6_test_base

class network_Dhcpv6Basic(dhcpv6_test_base.Dhcpv6TestBase):
    """
    Tests DHCPv6 lease negotiation process.
    """

    def test_body(self):
        """The main body for this test."""
        self.check_dhcpv6_config()

#!/usr/bin/python
#
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.server.cros import dnsname_mangler

HOST = 'chromeos1-row1-rack1-host1'
ROUTER = 'chromeos1-row1-rack1-host1-router'
ATTENUATOR = 'chromeos1-row1-rack1-host1-attenuator'
TESTER = 'chromeos1-row1-rack1-host1-router'

HOST_FROM_OUTSIDE_LAB = HOST + '.cros'
ROUTER_FROM_OUTSIDE_LAB = ROUTER + '.cros'
ATTENUATOR_FROM_OUTSIDE_LAB = ATTENUATOR + '.cros'
TESTER_FROM_OUTSIDE_LAB = TESTER + '.cros'


class DnsnameMangerUnittest(unittest.TestCase):
    """Check that we're correctly mangling DNS names."""


    def testRouterNamesCorrect(self):
        """Router names should look like <dut_dns_name>-router[.cros]"""
        self.assertEquals(ROUTER, dnsname_mangler.get_router_addr(HOST))
        self.assertEquals(
                ROUTER_FROM_OUTSIDE_LAB,
                dnsname_mangler.get_router_addr(HOST_FROM_OUTSIDE_LAB))


    def testAttenuatorNamesCorrect(self):
        """Router names should look like <dut_dns_name>-attenuator[.cros]"""
        self.assertEquals(ATTENUATOR, dnsname_mangler.get_attenuator_addr(HOST))
        self.assertEquals(
                ATTENUATOR_FROM_OUTSIDE_LAB,
                dnsname_mangler.get_attenuator_addr(HOST_FROM_OUTSIDE_LAB))


    def testTesterNamesCorrect(self):
        """Router names should look like <dut_dns_name>-router[.cros]"""
        self.assertEquals(TESTER, dnsname_mangler.get_tester_addr(HOST))
        self.assertEquals(
                TESTER_FROM_OUTSIDE_LAB,
                dnsname_mangler.get_tester_addr(HOST_FROM_OUTSIDE_LAB))


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/python
#
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.client.common_lib.cros import dbus_send

EXAMPLE_SHILL_GET_PROPERTIES_OUTPUT = \
"""method return sender=:1.12 -> dest=:1.37 reply_serial=2
   array [
      dict entry(
         string "ActiveProfile"
         variant             string "/profile/default"
      )
      dict entry(
         string "ArpGateway"
         variant             boolean true
      )
      dict entry(
         string "AvailableTechnologies"
         variant             array [
               string "ethernet"
            ]
      )
      dict entry(
         string "CheckPortalList"
         variant             string "''"
      )
      dict entry(
         string "ConnectedTechnologies" variant             array [
               string "ethernet"
            ]
      )
      dict entry(
         string "ConnectionState"
         variant             string "online"
      )
      dict entry(
         string "Country"
         variant             string ""
      )
      dict entry(
         string "DefaultService"
         variant             object path "/service/2"
      )
      dict entry(
         string "DefaultTechnology"
         variant             string "ethernet"
      )
      dict entry(
         string "Devices"
         variant             array [
               object path "/device/eth0"
               object path "/device/eth1"
            ]
      )
      dict entry(
         string "DisableWiFiVHT"
         variant             boolean false
      )
      dict entry(
         string "EnabledTechnologies"
         variant             array [
               string "ethernet"
            ]
      )
      dict entry(
         string "HostName"
         variant             string ""
      )
      dict entry(
         string "IgnoredDNSSearchPaths"
         variant             string "gateway.2wire.net"
      )
      dict entry(
         string "LinkMonitorTechnologies"
         variant             string "wifi"
      )
      dict entry(
         string "NoAutoConnectTechnologies"
         variant             string ""
      )
      dict entry(
         string "OfflineMode"
         variant             boolean false
      )
      dict entry(
         string "PortalCheckInterval"
         variant             int32 30
      )
      dict entry(
         string "PortalURL"
         variant             string "http://www.gstatic.com/generate_204"
      )
      dict entry(
         string "Profiles"
         variant             array [
               object path "/profile/default"
            ]
      )
      dict entry(
         string "ProhibitedTechnologies"
         variant             string ""
      )
      dict entry(
         string "ServiceCompleteList"
         variant             array [
               object path "/service/2"
               object path "/service/1"
               object path "/service/0"
            ]
      )
      dict entry(
         string "ServiceWatchList"
         variant             array [
               object path "/service/2"
            ]
      )
      dict entry(
         string "Services"
         variant             array [
               object path "/service/2"
            ]
      )
      dict entry(
         string "State"
         variant             string "online"
      )
      dict entry(
         string "UninitializedTechnologies"
         variant             array [
            ]
      )
      dict entry(
         string "WakeOnLanEnabled"
         variant             boolean true
      )
   ]
"""

PARSED_SHILL_GET_PROPERTIES_OUTPUT = {
    'ActiveProfile': '/profile/default',
    'ArpGateway': True,
    'AvailableTechnologies': ['ethernet'],
    'CheckPortalList': "''",
    'ConnectedTechnologies': ['ethernet'],
    'ConnectionState': 'online',
    'Country': '',
    'DefaultService': '/service/2',
    'DefaultTechnology': 'ethernet',
    'Devices': ['/device/eth0', '/device/eth1'],
    'DisableWiFiVHT': False,
    'EnabledTechnologies': ['ethernet'],
    'HostName': '',
    'IgnoredDNSSearchPaths': 'gateway.2wire.net',
    'LinkMonitorTechnologies': 'wifi',
    'NoAutoConnectTechnologies': '',
    'OfflineMode': False,
    'PortalCheckInterval': 30,
    'PortalURL': 'http://www.gstatic.com/generate_204',
    'Profiles': ['/profile/default'],
    'ProhibitedTechnologies': '',
    'ServiceCompleteList': ['/service/2', '/service/1', '/service/0'],
    'ServiceWatchList': ['/service/2'],
    'Services': ['/service/2'],
    'State': 'online',
    'UninitializedTechnologies': [],
    'WakeOnLanEnabled': True,
}

EXAMPLE_AVAHI_GET_STATE_OUTPUT = \
"""method return sender=:1.30 -> dest=:1.40 reply_serial=2
   int32 2
"""

class DBusSendTest(unittest.TestCase):
    """Check that we're correctly parsing dbus-send output."""


    def testAvahiGetState(self):
        """Test that extremely simple input works."""
        token_stream = dbus_send._build_token_stream(
                EXAMPLE_AVAHI_GET_STATE_OUTPUT.splitlines()[1:])
        parsed_output = dbus_send._parse_value(token_stream)
        assert parsed_output == 2, 'Actual == %r' % parsed_output


    def testShillManagerGetProperties(self):
        """Test that we correctly parse fairly complex output.

        We could simply write expected == actual, but this lends
        itself to debugging a little more.

        """
        token_stream = dbus_send._build_token_stream(
                EXAMPLE_SHILL_GET_PROPERTIES_OUTPUT.splitlines()[1:])
        parsed_output = dbus_send._parse_value(token_stream)
        for k,v in PARSED_SHILL_GET_PROPERTIES_OUTPUT.iteritems():
            assert k in parsed_output, '%r not in parsed output' % k
            actual_v = parsed_output.pop(k)
            assert actual_v == v, 'Expected %r, got %r' % (v, actual_v)

        assert len(parsed_output) == 0, ('Got extra parsed output: %r' %
                                         parsed_output)

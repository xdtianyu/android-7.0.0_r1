# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.cros import shill_temporary_profile
from autotest_lib.client.cros.networking import wifi_proxy

class network_BasicProfileProperties(test.test):
    """Test that shill's DBus properties for profiles work."""

    version = 1

    PROFILE_NAME = 'test'
    PROFILE_PROPERTY_NAME = 'Name'
    PROFILE_PROPERTY_ENTRIES = 'Entries'


    @staticmethod
    def get_field_from_properties(properties, field):
        """Get a field from a dictionary of properties.

        Raises an exception on failure.

        @param properties dict of properties (presumably from some
                DBus object).
        @param field string key to search for in |properties|.
        @return value of properties[field].

        """
        if not field in properties:
            raise error.TestFail('No %s field in properties %r?' %
                                 (field, properties))

        return properties[field]


    def run_once(self):
        """Test body."""
        shill = wifi_proxy.WifiProxy.get_proxy()
        with shill_temporary_profile.ShillTemporaryProfile(
                shill.manager, profile_name=self.PROFILE_NAME):
            profiles = shill.get_profiles()
            logging.info('Got profiles %r', profiles)
            # The last profile should be the one we just created.
            profile = profiles[-1]
            profile_properties = shill.dbus2primitive(
                    profile.GetProperties(utf8_strings=True))
            logging.debug('Profile properties: %r.', profile_properties)
            profile_name = self.get_field_from_properties(
                    profile_properties, self.PROFILE_PROPERTY_NAME)
            if profile_name != self.PROFILE_NAME:
                raise error.TestFail('Found unexpected top profile with name '
                                     '%r.' % profile_name)

            entries = shill.dbus2primitive(self.get_field_from_properties(
                    profile_properties, self.PROFILE_PROPERTY_ENTRIES))
            logging.info('Found entries: %r', entries)
            ethernet_if = interface.Interface.get_connected_ethernet_interface()
            mac = ethernet_if.mac_address.replace(':', '').lower()
            ethernet_entry_key = 'ethernet_%s' % mac
            if not ethernet_entry_key in entries:
                raise error.TestFail('Missing ethernet entry %s from profile.' %
                                     ethernet_entry_key)

            entry = profile.GetEntry(ethernet_entry_key)

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import sys

from autotest_lib.client.bin import test
from autotest_lib.client.cros import shill_temporary_profile
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking import wifi_proxy

class network_WiFiInvalidParameters(test.test):
    """Test that shill will reject invalid WiFi service configurations.

    In particular, this test checks shill's behavior with invalid SSIDs,
    WEP keys, and WPA passphrases.

    """
    version = 1
    unique_counter = 1


    SSID_TOO_LONG = ('MaxLengthSSID' * 8)[:33]
    SSID_TOO_SHORT = ''

    WPA_PASSPHRASE_TOO_LONG = ('x123456789' * 8)[:64]
    WPA_PASSPHRASE_TOO_SHORT= 'x123456'
    WPA_PMK_TOO_LONG = ('0123456789' * 8)[:65]


    @staticmethod
    def get_ssid():
        """@return string a unique SSID."""
        reserved_value = network_WiFiInvalidParameters.unique_counter
        network_WiFiInvalidParameters.unique_counter += 1
        return 'an ssid%d' % reserved_value


    def check_bad_ssids(self, shill):
        """Assert that shill will reject attempts connect to invalid SSIDs.

        @param shill ShillProxy object representing shill.

        """
        bad_ssids = [self.SSID_TOO_LONG, self.SSID_TOO_SHORT]
        for ssid in bad_ssids:
            config_params = {shill.SERVICE_PROPERTY_TYPE: 'wifi',
                             shill.SERVICE_PROPERTY_SSID: ssid,
                             shill.SERVICE_PROPERTY_SECURITY_CLASS: 'none'}
            try:
                accepted = True
                shill.configure_service(config_params)
            except dbus.exceptions.DBusException, e:
                accepted = False
                if (e.get_dbus_name() !=
                        'org.chromium.flimflam.Error.InvalidNetworkName'):
                    raise error.TestFail('Got an unexpected exception from '
                                         'shill: %s: %s.' %
                                         (e.get_dbus_name(),
                                          e.get_dbus_message()))
            if accepted:
                raise error.TestFail('Expected shill to stop us, but it let us '
                                     'configure a bad SSID "%r"' % ssid)


    def check_bad_wep_keys(self, shill):
        """Assert that shill will reject attempts to use invalid WEP keys.

        @param shill ShillProxy object representing shill.

        """
        LONG_HEX_STRING = '0123456789abcdef' * 8
        BAD_PREFIXES = ('4', '001', '-22', 'a', )
        bad_keys = []
        # Generate a bunch of hex keys.  Some of these are good ASCII keys, but
        # we'll fix this in a moment.
        for index in range(4):
            for length in range(0, 10) + range(11, 26) + range(27, 30):
                bad_keys.append('%d:%s' % (index, LONG_HEX_STRING[:length]))
                bad_keys.append('%d:0x%s' % (index, LONG_HEX_STRING[:length]))
        # |bad_keys| contains all bad hex keys, but some of those hex keys
        # could be interpretted as valid ASCII keys.  Lets take those out.
        good_keys = []
        for key in bad_keys:
            # 5 and 13 length keys are just ASCII keys.  7 and 15 are ASCII keys
            # with the key index prefix.
            if len(key) in (5, 7, 13, 15):
                good_keys.append(key)
        map(bad_keys.remove, good_keys)
        # Now add some keys that are bad because the prefix is no good.
        for valid_key in (LONG_HEX_STRING[:10], LONG_HEX_STRING[:26]):
            for prefix in BAD_PREFIXES:
                bad_keys.append('%s:%s' % (prefix, valid_key))
                bad_keys.append('%s:0x%s' % (prefix, valid_key))
        for valid_key in ('wep40', 'wep104is13len'):
            for prefix in BAD_PREFIXES:
                bad_keys.append('%s:%s' % (prefix, valid_key))
        for key in bad_keys:
            config_params = {shill.SERVICE_PROPERTY_TYPE: 'wifi',
                             shill.SERVICE_PROPERTY_SSID: self.get_ssid(),
                             shill.SERVICE_PROPERTY_PASSPHRASE: key,
                             shill.SERVICE_PROPERTY_SECURITY_CLASS: 'wep'}
            try:
                accepted = True
                shill.configure_service(config_params)
            except dbus.exceptions.DBusException, e:
                accepted = False
                if (e.get_dbus_name() !=
                        'org.chromium.flimflam.Error.InvalidPassphrase'):
                    raise error.TestFail('Got an unexpected exception from '
                                         'shill: %s: %s.' %
                                         (e.get_dbus_name(),
                                          e.get_dbus_message()))

            if accepted:
                raise error.TestFail('Expected shill to stop us, but it let us '
                                     'configure a bad WEP key: %s' % key)

        for key in good_keys:
            config_params = {shill.SERVICE_PROPERTY_TYPE: 'wifi',
                             shill.SERVICE_PROPERTY_SSID: self.get_ssid(),
                             shill.SERVICE_PROPERTY_PASSPHRASE: key,
                             shill.SERVICE_PROPERTY_SECURITY_CLASS: 'wep'}
            try:
                shill.configure_service(config_params)
            except:
                logging.error('%r', sys.exc_info())
                raise error.TestFail('shill should let us use a WEP key '
                                     'like: %s' % key)


    def check_bad_wpa_passphrases(self, shill):
        """Assert that shill will reject invalid WPA passphrases.

        @param shill ShillProxy object representing shill.

        """
        bad_passphrases = [self.WPA_PASSPHRASE_TOO_LONG,
                           self.WPA_PASSPHRASE_TOO_SHORT,
                           self.WPA_PMK_TOO_LONG]
        for psk in bad_passphrases:
            for security in ('rsn', 'wpa', 'psk'):
                config_params = {
                    shill.SERVICE_PROPERTY_TYPE: 'wifi',
                    shill.SERVICE_PROPERTY_SSID: self.get_ssid(),
                    shill.SERVICE_PROPERTY_PASSPHRASE: psk,
                    shill.SERVICE_PROPERTY_SECURITY_RAW: security}
                try:
                    accepted = True
                    shill.configure_service(config_params)
                except dbus.exceptions.DBusException, e:
                    accepted = False
                    if (e.get_dbus_name() !=
                            'org.chromium.flimflam.Error.InvalidPassphrase'):
                        raise error.TestFail('Got an unexpected exception from '
                                             'shill: %s: %s.' %
                                             (e.get_dbus_name(),
                                              e.get_dbus_message()))
                if accepted:
                    raise error.TestFail('Expected shill to stop us, but it '
                                         'let us configure a bad passphrase: '
                                         '%s' % psk)


    def run_once(self):
        """Test body."""
        shill = wifi_proxy.WifiProxy.get_proxy()
        with shill_temporary_profile.ShillTemporaryProfile(shill.manager):
            self.check_bad_ssids(shill)
            self.check_bad_wep_keys(shill)
            self.check_bad_wpa_passphrases(shill)

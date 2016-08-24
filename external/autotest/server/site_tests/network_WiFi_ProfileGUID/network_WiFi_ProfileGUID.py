# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_ProfileGUID(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we can connect to router configured in various ways."""
    version = 1

    TEST_GUID = '01234'
    TEST_PASSWORD0 = 'chromeos0'
    TEST_PASSWORD1 = 'chromeos1'
    TEST_PASSWORD2 = 'chromeos2'
    SERVICE_PROPERTY_GUID = 'GUID'
    STATE_TRANSITION_TIMEOUT_SECONDS = 15


    def _assert_connection(self, ssid):
        """Assert that shill connects to |ssid| after a scan.

        @param ssid: string name of network we expect to connect to.

        """
        # Request a scan, this should goad shill into action.
        self.context.client.scan(frequencies=[], ssids=[])
        result = self.context.client.wait_for_service_states(
                ssid, ['ready', 'online', 'portal'],
                self.STATE_TRANSITION_TIMEOUT_SECONDS)
        success, state, time = result
        if not success:
            logging.error('ERROR!')
            raise error.TestFail('Failed to connect to %s in %f seconds (%r).' %
                                 (ssid, time, state))


    def _assert_guid_value(self, ssid, expected_guid, expect_missing=False):
        """Assert that a service's GUID field has a particular value.

        @param ssid: string name of WiFi network corresponding to the service.
        @param expected_guid: string expected value of the GUID on the service.
        @param expect_missing: boolean True if we expect an empty GUID value.

        """
        properties = self.context.client.shill.get_service_properties(ssid)
        real_guid = properties.get(self.SERVICE_PROPERTY_GUID, '')
        logging.debug('Got service properties: %r.', properties)
        if expect_missing and real_guid:
            raise error.TestFail('Expected GUID property to be missing.')

        if not expect_missing and real_guid != expected_guid:
            raise error.TestFail('Expected GUID value of %r, but got %r.' %
                                 (expected_guid, real_guid))


    def run_once(self):
        """Sets up a router, connects to it, pings it, and repeats."""
        CIPHER_CCMP = xmlrpc_security_types.WPAConfig.CIPHER_CCMP
        WPA_MODE = xmlrpc_security_types.WPAConfig.MODE_PURE_WPA
        get_ap_config = lambda ssid, password: hostap_config.HostapConfig(
                ssid=ssid, channel=1,
                security_config=xmlrpc_security_types.WPAConfig(
                        psk=password,
                        wpa_mode=WPA_MODE,
                        wpa_ciphers=[CIPHER_CCMP]))
        get_client_config = lambda ap_config: \
                xmlrpc_datatypes.AssociationParameters(
                        ssid=self.context.router.get_ssid(),
                        security_config=ap_config.security_config,
                        guid=self.TEST_GUID,
                        autoconnect=True)
        ap_config = get_ap_config(None, self.TEST_PASSWORD0)
        self.context.configure(ap_config)
        assoc_params = get_client_config(ap_config)
        self.context.client.shill.configure_wifi_service(assoc_params)
        self._assert_connection(assoc_params.ssid)
        # Expect the GUID property to be set.
        self._assert_guid_value(assoc_params.ssid, assoc_params.guid)
        if not self.context.client.shill.delete_entries_for_ssid(
                assoc_params.ssid):
            raise error.TestFail('Failed to delete profile entry for %s' %
                                 assoc_params.ssid)

        # GUID property should be missing, since we don't have an entry.
        self._assert_guid_value(assoc_params.ssid, assoc_params.guid,
                                expect_missing=True)

        # Change the password on the AP, do everything again.
        ap_config = get_ap_config(assoc_params.ssid, self.TEST_PASSWORD1)
        self.context.configure(ap_config)
        assoc_params = get_client_config(ap_config)
        self.context.client.shill.configure_wifi_service(assoc_params)
        self._assert_connection(assoc_params.ssid)
        # Expect the GUID property to be set.
        self._assert_guid_value(assoc_params.ssid, assoc_params.guid)
        # Change the security configuration again.
        ap_config = get_ap_config(assoc_params.ssid, self.TEST_PASSWORD2)
        self.context.configure(ap_config)
        # Connect again, but do so by configuring the existing entry.
        # We'll address it by its GUID here.
        if not self.context.client.shill.configure_service_by_guid(
                xmlrpc_datatypes.ConfigureServiceParameters(
                        assoc_params.guid, autoconnect=True,
                        passphrase=self.TEST_PASSWORD2)):
            raise error.TestFail('Failed to configure service by GUID.')

        self._assert_connection(assoc_params.ssid)

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_HiddenRemains(wifi_cell_test_base.WiFiCellTestBase):
    """Check that shill preserves hidden network settings after connect."""

    version = 1

    SERVICE_PROPERTY_HIDDEN_SSID = 'WiFi.HiddenSSID'


    def check_hidden(self, ssid, should_be_hidden):
        """Asserts that the network with |ssid| is a hidden network in shill.

        Implicitly, we assert that shill already has an entry for the given
        network.

        @param ssid string name of network to make assertions about.
        @param should_be_hidden bool True iff service should be marked as
                a hidden SSID.

        """
        logging.info('Checking that %s has hidden=%r.', ssid, should_be_hidden)
        service_properties = self.context.client.shill.get_service_properties(
                ssid)
        if service_properties is None:
            raise error.TestFail('Unable to retrieve properties for service '
                                 '%s.' % ssid)

        logging.debug(service_properties)
        is_hidden = service_properties[self.SERVICE_PROPERTY_HIDDEN_SSID]
        if is_hidden != should_be_hidden:
            raise error.TestFail('Expected hidden=%r, but found hidden=%r.' %
                                 (should_be_hidden, is_hidden))

        logging.info('Service had the expected hidden value.')


    def run_once(self):
        """Test body."""
        ap_configs = [hostap_config.HostapConfig(
                              ssid='a visible network',
                              frequency=2437,
                              mode=hostap_config.HostapConfig.MODE_11G),
                      hostap_config.HostapConfig(
                              hide_ssid=True,
                              ssid='a hidden network',
                              frequency=2437,
                              mode=hostap_config.HostapConfig.MODE_11G)]
        for ap_config in ap_configs:
            self.context.configure(ap_config)
            client_config = xmlrpc_datatypes.AssociationParameters(
                    ssid=self.context.router.get_ssid(),
                    is_hidden=ap_config.hide_ssid)
            self.context.assert_connect_wifi(client_config)
            self.context.assert_ping_from_dut()
            # Check that shill's opinion of our hidden-ness is correct.
            self.check_hidden(self.context.router.get_ssid(),
                              ap_config.hide_ssid is True)
            self.context.client.shill.disconnect(self.context.router.get_ssid())

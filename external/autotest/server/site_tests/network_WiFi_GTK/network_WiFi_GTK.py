# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.network import arping_runner
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_GTK(wifi_cell_test_base.WiFiCellTestBase):
    """Tests that a DUT can continue receiving and sending broadcast traffic.

    This test sets up an AP with an artificially small GTK and GMK rekey
    periods, so that we can test our ability to receive and correctly interpret
    rekeys.

    """
    version = 1
    ARPING_COUNT = 20
    GTK_REKEY_PERIOD = 5
    GMK_REKEY_PERIOD = 7


    def run_once(self):
        """Test body."""
        wpa_config = xmlrpc_security_types.WPAConfig(
                psk='chromeos',
                wpa_mode=xmlrpc_security_types.WPAConfig.MODE_MIXED_WPA,
                wpa_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_TKIP,
                             xmlrpc_security_types.WPAConfig.CIPHER_CCMP],
                wpa2_ciphers=[xmlrpc_security_types.WPAConfig.CIPHER_CCMP],
                use_strict_rekey=True,
                wpa_gtk_rekey_period=self.GTK_REKEY_PERIOD,
                wpa_gmk_rekey_period=self.GMK_REKEY_PERIOD)
        ap_config = hostap_config.HostapConfig(
                frequency=2412,
                mode=hostap_config.HostapConfig.MODE_11G,
                security_config=wpa_config)
        client_conf = xmlrpc_datatypes.AssociationParameters(
                security_config=wpa_config)
        self.context.configure(ap_config)
        client_conf.ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(client_conf)
        # Sanity check ourselves with some unicast pings.
        self.context.assert_ping_from_dut()
        # Now check that network traffic goes through.
        if (not self.check_client_can_recv_broadcast_traffic() or
                not self.check_client_can_send_broadcast_traffic()):
            raise error.TestFail('Not all arping passes were successful.')

        self.context.client.shill.disconnect(client_conf.ssid)
        self.context.router.deconfig()


    def check_client_can_recv_broadcast_traffic(self):
        """@return True iff the client can receive server broadcast packets."""
        logging.info('Checking that broadcast traffic is received by DUT.')
        runner = arping_runner.ArpingRunner(self.context.get_wifi_host(),
                                            self.context.get_wifi_if())
        if not self.context.client.wifi_ip:
            raise error.TestFail('Tried to arping client, but client has no '
                                 'suitable IP address')

        arping_result = runner.arping(self.context.client.wifi_ip,
                                      count=self.ARPING_COUNT)
        if not arping_result.was_successful():
            logging.error('arping from server failed: %r', arping_result)
            return False

        logging.info('arping from server passed: %r', arping_result)
        return True


    def check_client_can_send_broadcast_traffic(self):
        """@return True iff the server can receive client broadcast packets."""
        logging.info('Checking that broadcast traffic may be sent by the DUT.')
        runner = arping_runner.ArpingRunner(self.context.client.host,
                                            self.context.client.wifi_if)
        arping_result = runner.arping(self.context.get_wifi_addr(),
                                      count=self.ARPING_COUNT)
        if not arping_result.was_successful():
            logging.error('arping from client failed: %r', arping_result)
            return False

        logging.info('arping from client passed: %r', arping_result)
        return True

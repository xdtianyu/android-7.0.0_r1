# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.server.cros.network import wifi_client


class network_WiFi_DisconnectReason(wifi_cell_test_base.WiFiCellTestBase):
    """Verify the client disconnects from an AP and read (but not verify)
    the supplicant DisconnectReason for various scenarios."""
    version = 1

    INITIAL_CHANNEL = 64
    ALT_CHANNEL = 6
    CHANNEL_SWITCH_ATTEMPTS = 5
    CHANNEL_SWITCH_WAIT_TIME_SEC = 3

    def run_once(self, disconnect_trigger, req_capabilities=[]):
        """Sets up a router, connects to it, pings it and disables it to trigger
        disconnect."""
        configuration = hostap_config.HostapConfig(
                channel=self.INITIAL_CHANNEL,
                mode=hostap_config.HostapConfig.MODE_11A,
                spectrum_mgmt_required=True)
        self.context.router.require_capabilities(req_capabilities)
        self.context.configure(configuration)

        if site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP in req_capabilities:
            # prep alternate Access Point
            alt_ap_config = hostap_config.HostapConfig(
                    channel=self.ALT_CHANNEL,
                    mode=hostap_config.HostapConfig.MODE_11N_MIXED)
            self.context.configure(alt_ap_config, multi_interface=True)
            alt_assoc_params = xmlrpc_datatypes.AssociationParameters()
            alt_assoc_params.ssid = self.context.router.get_ssid(instance=1)

        assoc_params = xmlrpc_datatypes.AssociationParameters()
        assoc_params.ssid = self.context.router.get_ssid(instance=0)
        self.context.assert_connect_wifi(assoc_params)
        self.context.assert_ping_from_dut()

        with self.context.client.assert_disconnect_event():
            if disconnect_trigger == 'AP gone':
                self.context.router.deconfig()
            elif disconnect_trigger == 'deauth client':
                self.context.router.deauth_client(self.context.client.wifi_mac)
            elif disconnect_trigger == 'AP send channel switch':
                for attempt in range(self.CHANNEL_SWITCH_ATTEMPTS):
                    self.context.router.send_management_frame_on_ap(
                            'channel_switch',
                            self.ALT_CHANNEL)
                    time.sleep(self.CHANNEL_SWITCH_WAIT_TIME_SEC)
            elif disconnect_trigger == 'switch AP':
                self.context.assert_connect_wifi(alt_assoc_params)
            elif disconnect_trigger == 'disable client wifi':
                self.context.client.set_device_enabled(
                        self.context.client.wifi_if, False)
            else:
                raise error.TestError('unknown test mode: %s' % disconnect_trigger)
            time.sleep(wifi_client.DISCONNECT_WAIT_TIME_SECONDS)

        disconnect_reasons = self.context.client.get_disconnect_reasons()
        if disconnect_reasons is None or len(disconnect_reasons) == 0:
            raise error.TestFail('supplicant DisconnectReason not logged')
        for entry in disconnect_reasons:
            logging.info("DisconnectReason: %s", entry);

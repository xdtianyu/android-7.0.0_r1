# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_LinkMonitorFailure(wifi_cell_test_base.WiFiCellTestBase):
    """Test how a DUT behaves when the network link disappears.

    Connects a DUT to an AP, then silently change the gateway IP on the AP
    to simulate network link disappearance. Determine the time the DUT take
    to detect link failure and the time for the subsequent reassociation
    request.

    """

    version = 1

    # Passive link monitor takes 25 seconds to fail, active link monitor
    # takes upto 50 seconds to fail (unicast ARP failures doesn't count since
    # unicast ARP gateway support is not established).
    LINK_FAILURE_MAX_SECONDS = 80
    REASSOCIATE_TIMEOUT_SECONDS = 10

    def run_once(self):
        """Body of the test."""
        # Establish a connection with an AP.
        ap_config = hostap_config.HostapConfig(channel=1)
        self.context.configure(ap_config)
        ssid = self.context.router.get_ssid()
        client_config = xmlrpc_datatypes.AssociationParameters(ssid=ssid)
        self.context.assert_connect_wifi(client_config)
        self.context.assert_ping_from_dut()

        # Restart local server with a different address index. This will
        # simulate the disappearance of the network link from the client's
        # point of view.
        logging.info("Restart local server with different address")
        self.context.router.change_server_address_index()
        with self.context.client.iw_runner.get_event_logger() as logger:
            logger.start()
            # wait for the timeout seconds for link failure and reassociation
            # to complete.
            time.sleep(self.LINK_FAILURE_MAX_SECONDS +
                       self.REASSOCIATE_TIMEOUT_SECONDS)
            logger.stop()

            # Link failure detection time.
            link_failure_time = logger.get_time_to_disconnected()
            if (link_failure_time is None or
                link_failure_time > self.LINK_FAILURE_MAX_SECONDS):
                raise error.TestFail(
                        'Failed to detect link failure within given timeout')
            logging.info('Link failure detection time: %.2f seconds',
                         link_failure_time)

            # Reassociation time.
            reassociate_time = logger.get_reassociation_time()
            if (reassociate_time is None or
                reassociate_time > self.REASSOCIATE_TIMEOUT_SECONDS):
                raise error.TestFail(
                        'Failed to reassociate within given timeout')
            logging.info('Reassociate time: %.2f seconds', reassociate_time)

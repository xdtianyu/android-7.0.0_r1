# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import test

DUT_CONNECTION_RETRIES = 3

class network_WiFi_ChaosLongConnect(test.test):
    """ Dynamic Chaos test to hold connection to AP. """

    version = 1


    def run_once(self, capturer, capturer_frequency, capturer_ht_type, host,
                 assoc_params, client, debug_info=None,
                 conn_worker=None):
        """ Main entry function for autotest.

        @param capturer: a packet capture device
        @param capturer_frequency: integer channel frequency in MHz.
        @param capturer_ht_type: string specifier of channel HT type.
        @param host: an Autotest host object, DUT.
        @param assoc_params: an AssociationParameters object.
        @param client: WiFiClient object
        @param debug_info: a string of additional info to display on failure
        @param conn_worker: ConnectionWorkerAbstract or None, to run extra
            work after successful connection.

        """

        results = []

        client.shill.disconnect(assoc_params.ssid)
        if not client.shill.init_test_network_state():
            raise error.TestError('Failed to set up isolated test context '
                    'profile.')

        capturer.start_capture(capturer_frequency, ht_type=capturer_ht_type)
        try:
            success = False
            for i in range(DUT_CONNECTION_RETRIES):
                logging.info('Connecting DUT (try: %d) to AP', (i+1))
                host.syslog('Connection attempt %d' % (i+1))
                assoc_result = xmlrpc_datatypes.deserialize(
                        client.shill.connect_wifi(assoc_params))
                success = assoc_result.success
                if not success:
                    logging.info('Connection attempt of DUT failed try %d;'
                                 ' reason: %s',
                                 (i+1), assoc_result.failure_reason)
                    continue
                else:
                    logging.info('DUT connected to the AP')
                    if conn_worker is not None:
                        conn_worker.run(client)
                    break

            if not success:
                msg = str('DUT failed to connect to the AP in %d tries:\n%s\n'
                          'With the assoc_params:\n%s\n Debug info: %s\n '
                          'DUT MAC: %s' %
                          (DUT_CONNECTION_RETRIES, pprint.pformat(results),
                          assoc_params, debug_info, client.wifi_mac))
                raise error.TestError(msg)
        finally:
            filename = str('connect_try_%s.trc' %
                               ('success' if success else 'fail'))
            capturer.stop_capture(save_dir=self.outputdir,
                                  save_filename=filename)
            client.shill.disconnect(assoc_params.ssid)
            client.shill.clean_profiles()

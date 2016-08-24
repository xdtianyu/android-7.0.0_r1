# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server import test


class network_WiFi_ChaosConnectDisconnect(test.test):
    """ Dynamic Chaos test to connect and disconnect to an AP. """

    version = 1


    def run_once(self, capturer, capturer_frequency, capturer_ht_type,
                 host, assoc_params, client, tries, debug_info=None):
        """ Main entry function for autotest.

        @param capturer: a packet capture device
        @param capturer_frequency: integer channel frequency in MHz.
        @param capturer_ht_type: string specifier of channel HT type.
        @param host: an Autotest host object, DUT.
        @param assoc_params: an AssociationParameters object.
        @param client: WiFiClient object
        @param tries: an integer, number of connection attempts.
        @param debug_info: a string of additional info to display on failure

        """

        results = []

        for i in range(1, tries + 1):
            client.shill.disconnect(assoc_params.ssid)
            if not client.shill.init_test_network_state():
                return 'Failed to set up isolated test context profile.'

            capturer.start_capture(capturer_frequency, ht_type=capturer_ht_type)
            try:
                success = False
                logging.info('Connection attempt %d', i)
                host.syslog('Connection attempt %d' % i)
                start_time = host.run("date '+%FT%T.%N%:z'").stdout.strip()
                assoc_result = xmlrpc_datatypes.deserialize(
                        client.shill.connect_wifi(assoc_params))
                end_time = host.run("date '+%FT%T.%N%:z'").stdout.strip()
                success = assoc_result.success
                if not success:
                    logging.info('Connection attempt %d failed; reason: %s',
                                 i, assoc_result.failure_reason)
                    results.append(
                            {'try' : i,
                             'error' : assoc_result.failure_reason,
                             'start_time': start_time,
                             'end_time': end_time})
                else:
                    logging.info('Connection attempt %d passed', i)
            finally:
                filename = str('connect_try_%d_%s.trc' % (i,
                               ('success' if success else 'fail')))
                capturer.stop_capture(save_dir=self.outputdir,
                                      save_filename=filename)
                client.shill.disconnect(assoc_params.ssid)
                client.shill.clean_profiles()

        if len(results) > 0:
            # error.TestError doesn't handle the formatting inline, doing it
            # here so it is clearer to read in the status.log file.
            msg = str('Failed on the following attempts:\n%s\n'
                      'With the assoc_params:\n%s\n'
                      'Debug info: %s\nDUT MAC:%s' %
                      (pprint.pformat(results), assoc_params, debug_info,
                       client.wifi_mac))
            raise error.TestFail(msg)
        logging.debug('Debug info: %s', debug_info)

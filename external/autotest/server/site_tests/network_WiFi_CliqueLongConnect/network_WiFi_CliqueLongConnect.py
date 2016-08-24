# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros.clique_lib import clique_dut_control


class network_WiFi_CliqueLongConnect(test.test):
    """ Dynamic Clique test to connect and disconnect to an AP. """

    version = 1


    def run_once(self, capturer, capturer_frequency, capturer_ht_type,
                 dut_pool, assoc_params_list, tries, debug_info,
                 conn_workers):
        """ Main entry function for autotest.

        @param capturer: a packet capture device
        @param capturer_frequency: integer channel frequency in MHz.
        @param capturer_ht_type: string specifier of channel HT type.
        @param dut_pool: the DUT pool to be used for the test. It is a 2D list
                         of DUTObjects.
        @param assoc_params_list: a list of AssociationParameters objects.
        @param tries: an integer, number of connection attempts.
        @param debug_info: a string of additional info to display on failure
        @param conn_workers: List of ConnectionWorkerAbstract objects, to
                             run extra work after successful connection.
        """
        # We need 2 sets in the pool for this test.
        if len(dut_pool) != 2:
            raise error.TestFail("Incorrect DUT pool configuration.")
        # We need 2 AP's in the pool for this test.
        if len(assoc_params_list) != 2:
            raise error.TestFail("Incorrect AP pool configuration.")
        # We need 2 connection workers in the pool for this test.
        if len(conn_workers) != 2:
            raise error.TestFail("Incorrect connection worker configuration.")

        # Both DUT sets are performing long connects.
        dut_role_classes = [clique_dut_control.DUTRoleConnectDuration,
                            clique_dut_control.DUTRoleConnectDuration]

        test_params = { 'capturer': capturer,
                        'capturer_frequency': capturer_frequency,
                        'capturer_ht_type': capturer_ht_type,
                        'debug_info': debug_info }
        error_results = clique_dut_control.execute_dut_pool(
                dut_pool, dut_role_classes, assoc_params_list, conn_workers,
                test_params)
        if error_results:
            logging.debug('Debug info: %s', debug_info)
            raise error.TestFail("Failed test. Error Results: %s" %
                                 str(error_results))

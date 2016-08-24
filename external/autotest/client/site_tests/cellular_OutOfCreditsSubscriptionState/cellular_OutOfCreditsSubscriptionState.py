# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.networking import cellular_proxy
from autotest_lib.client.cros.networking import pm_proxy
from autotest_lib.client.cros.networking import shill_context

SHORT_TIMEOUT = 10


class cellular_OutOfCreditsSubscriptionState(test.test):
    """
    This test verifies that shill out-of-credits behavior works properly based
    on the modem subscription state.

    """
    version = 1

    def _initialize_modem(self, subscription_state):
        # Simulate an Altair 3100 modem since that modem supports subscription
        # state information.
        self.test_env.shill.disable_modem_for_test_setup()
        # TODO(thieule): Set the modem model using the pseudomodem testing
        # interface (crbug.com/343258).
        self.modem.iface_properties.Set(
                mm1_constants.I_MODEM,
                mm1_constants.MM_MODEM_PROPERTY_NAME_PLUGIN,
                'Altair LTE')
        self.pseudomm.iface_testing.SetSubscriptionState(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN,
                subscription_state)
        self.test_env.shill.manager.EnableTechnology(
                cellular_proxy.CellularProxy.TECHNOLOGY_CELLULAR)
        # Wait for a registered state.
        self.modem.wait_for_states([mm1_constants.MM_MODEM_STATE_REGISTERED,
                                    mm1_constants.MM_MODEM_STATE_CONNECTED])


    def _is_out_of_credits(self, cellular_service):
        properties = cellular_service.GetProperties(utf8_strings=True)
        return properties[cellular_proxy.CellularProxy.
                          DEVICE_PROPERTY_OUT_OF_CREDITS]


    def _test_provisioned(self):
        logging.info('Initialize modem with provisioned state')
        self._initialize_modem(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_PROVISIONED)
        logging.info('Verify out-of-credits is not set in cellular service')
        cellular_service = \
                self.test_env.shill.wait_for_cellular_service_object()
        if self._is_out_of_credits(cellular_service):
            error_msg = 'Service marked as out-of-credits when it ' \
                        'should not be.'
            logging.error(error_msg)
            raise error.TestFail(error_msg)


    def _test_out_of_credits_at_start(self):
        logging.info('Initialize modem with out-of-credits state')
        self._initialize_modem(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_OUT_OF_DATA)
        logging.info('Verify out-of-credits is set in cellular service')
        cellular_service = \
                self.test_env.shill.wait_for_cellular_service_object()
        if not self._is_out_of_credits(cellular_service):
            error_msg = 'Service not marked out-of-credits when it ' \
                        'should be.'
            logging.error(error_msg)
            raise error.TestFail(error_msg)


    def _test_out_of_credits_while_connected(self):
        logging.info('Initialize modem with provisioned state')
        self._initialize_modem(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_PROVISIONED)
        cellular_service = \
                self.test_env.shill.wait_for_cellular_service_object()
        logging.info('Mark modem as out-of-credits')
        self.pseudomm.iface_testing.SetSubscriptionState(
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_UNKNOWN,
                mm1_constants.MM_MODEM_3GPP_SUBSCRIPTION_STATE_OUT_OF_DATA)
        logging.info('Verify out-of-credits set in cellular service')
        try:
            utils.poll_for_condition(
                    lambda: self._is_out_of_credits(cellular_service),
                    exception=error.TestFail('Service failed to be marked as '
                                             'out-of-credits.'),
                    timeout=SHORT_TIMEOUT)
        except error.TestFail as e:
            logging.error(repr(e))
            raise e


    def run_once(self):
        """Calls by autotest to run this test."""
        self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': '3GPP'},))
        with self.test_env, shill_context.ServiceAutoConnectContext(
                self.test_env.shill.find_cellular_service_object, False):
            self.pseudomm = pm_proxy.PseudoMMProxy.get_proxy()
            self.modem = self.pseudomm.get_modem()

            tests = [self._test_provisioned,
                     self._test_out_of_credits_at_start,
                     self._test_out_of_credits_while_connected]

            for test in tests:
                test()

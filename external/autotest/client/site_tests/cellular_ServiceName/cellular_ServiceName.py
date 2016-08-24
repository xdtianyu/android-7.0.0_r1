# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm1_constants
from autotest_lib.client.cros.cellular import test_environment
from autotest_lib.client.cros.cellular.pseudomodem import modem_3gpp
from autotest_lib.client.cros.cellular.pseudomodem import modem_cdma
from autotest_lib.client.cros.cellular.pseudomodem import sim

TEST_MODEMS_MODULE_PATH = __file__

# Use valid carrier info since shill looks this up in its database.
TEST_3GPP_HOME_CARRIER = 'Orange'
TEST_3GPP_HOME_CARRIER_MCC = '232'
TEST_3GPP_HOME_CARRIER_MNC = '05'
TEST_3GPP_ROAMING_CARRIER = 'T-Mobile'
TEST_3GPP_ROAMING_OPERATOR_CODE = '23203'
TEST_CDMA_CARRIER = 'Test Network'
TEST_CDMA_SID = 99998

class TestModemRoaming(modem_3gpp.Modem3gpp):
    """
    Test modem that registers with a roaming network.

    """
    def __init__(self):
        roaming_networks = [modem_3gpp.Modem3gpp.GsmNetwork(
                operator_long=TEST_3GPP_ROAMING_CARRIER,
                operator_short=TEST_3GPP_ROAMING_CARRIER,
                operator_code=TEST_3GPP_ROAMING_OPERATOR_CODE,
                status=mm1_constants.
                        MM_MODEM_3GPP_NETWORK_AVAILABILITY_AVAILABLE,
                access_technology=mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_LTE)]
        modem_3gpp.Modem3gpp.__init__(self, roaming_networks=roaming_networks)


    def RegisterWithNetwork(
        self, operator_id='', return_cb=None, raise_cb=None):
        """ Overriden from superclass. """
        logging.info('Force modem to register with roaming network |%s| '
                     'instead of |%s|',
                     TEST_3GPP_ROAMING_OPERATOR_CODE, operator_id)
        modem_3gpp.Modem3gpp.RegisterWithNetwork(
                self, TEST_3GPP_ROAMING_OPERATOR_CODE, return_cb, raise_cb)


class TestSIM(sim.SIM):
    """
    Test SIM with a specific carrier name that the tests below are expecting.

    """
    def __init__(self):
        carrier = sim.SIM.Carrier()
        carrier.mcc = TEST_3GPP_HOME_CARRIER_MCC
        carrier.mnc = TEST_3GPP_HOME_CARRIER_MNC
        carrier.operator_name = TEST_3GPP_HOME_CARRIER
        carrier.operator_id = carrier.mcc + carrier.mnc
        sim.SIM.__init__(self, carrier,
                         mm1_constants.MM_MODEM_ACCESS_TECHNOLOGY_LTE)


class TestCdmaModem(modem_cdma.ModemCdma):
    """
    Test modem that simulates a CDMA modem.

    """
    def __init__(self):
        network = modem_cdma.ModemCdma.CdmaNetwork(sid=TEST_CDMA_SID)
        modem_cdma.ModemCdma.__init__(self, home_network=network)


class cellular_ServiceName(test.test):
    """
    Verifies that shill reports the correct service name depending on the SIM
    provider information and the network registration status.

    """
    version = 1

    def _verify_service_name(self, expected_name):
        """
        Verifies the service name is as expected.

        @param expected_name: Service name that is expected.
        @raises error.TestFail() if the service name and expected name does not
                match.

        """
        cellular_service = \
                self.test_env.shill.wait_for_cellular_service_object()
        service_name = cellular_service.GetProperties()['Name']
        if service_name != expected_name:
            raise error.TestFail('Expected service name: |%s|, '
                                 'actual service name: |%s|' %
                                 (expected_name, service_name))
        logging.info('Successfully verified service name |%s|',
                     expected_name)


    def _test_3gpp_no_roaming(self):
        """
        Checks the service name when the SIM and the network is the same
        carrier.

        """
        logging.info('Testing service name for 3GPP no roaming')
        self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': '3GPP',
                                'test-module': TEST_MODEMS_MODULE_PATH,
                                'test-sim-class': 'TestSIM'},))
        with self.test_env:
            self._verify_service_name(TEST_3GPP_HOME_CARRIER)


    def _test_3gpp_roaming(self):
        """
        Checks the service name when roaming.

        The service name while roaming should be (per 3GPP TS 31.102 and
        annex A of 122.101):
                <home provider> | <serving operator>

        """
        logging.info('Testing service name for 3GPP roaming')
        self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': '3GPP',
                                'test-module': TEST_MODEMS_MODULE_PATH,
                                'test-modem-class': 'TestModemRoaming',
                                'test-sim-class': 'TestSIM'},))
        with self.test_env:
            expected_name = (TEST_3GPP_HOME_CARRIER + ' | ' +
                             TEST_3GPP_ROAMING_CARRIER)
            self._verify_service_name(expected_name)


    def _test_cdma(self):
        """ Checks the service name for a CDMA network. """
        logging.info('Testing service name for CDMA')
        self.test_env = test_environment.CellularPseudoMMTestEnvironment(
                pseudomm_args=({'family': 'CDMA',
                                'test-module': TEST_MODEMS_MODULE_PATH,
                                'test-modem-class': 'TestCdmaModem'},))
        with self.test_env:
            self._verify_service_name(TEST_CDMA_CARRIER)


    def run_once(self):
        tests = [self._test_3gpp_no_roaming,
                 self._test_3gpp_roaming,
                 self._test_cdma]

        for test in tests:
            test()

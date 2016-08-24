# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import attenuator_controller
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

ATTENUATION_STEP = 4
FINAL_ATTENUATION = 90
ATTENUATORS_PER_PHY = 2

LOW_POWER_SIGNAL = -75

class AttenuatorInfo(object):
    """Contains debug information about an attenuator."""

    def __init__(self):
        self.attenuator_zeros = False
        self.zeroed_scan_signal = None
        self.allows_connection = False
        self._zeroed_linked_signal = None


    @property
    def zeroed_linked_signal(self):
        """Returns the linked signal as a float."""
        return self._zeroed_linked_signal


    @zeroed_linked_signal.setter
    def zeroed_linked_signal(self, value):
        """Sets the linked signal to a float.

        @param value: the linked signal as a float

        """
        if (self._zeroed_linked_signal is None or
            value > self._zeroed_linked_signal):
            self._zeroed_linked_signal = value


    def healthy_attenuator(self):
        """Returns True if the attenuator looks good; False otherwise."""
        if (not self.allows_connection and
            self.zeroed_scan_signal is None):
            return False
        elif not self.attenuator_zeros:
            return False
        if (self.zeroed_scan_signal < LOW_POWER_SIGNAL and
            self.zeroed_linked_signal < LOW_POWER_SIGNAL):
            return False
        return True


class network_WiFi_VerifyAttenuator(wifi_cell_test_base.WiFiCellTestBase):
    """Test that all connected attenuators are functioning correctly."""
    version = 1


    def _refresh_ap_ssids(self, frequency):
        """Start up new APs, with unique SSIDs.

        Doing this before each connection attempt in the test prevents
        spillover from previous connection attempts interfering with
        our intentions.

        @param frequency: int WiFi frequency to configure the APs on.

        """
        ap_config = hostap_config.HostapConfig(
                frequency=frequency,
                mode=hostap_config.HostapConfig.MODE_11N_PURE)
        self.context.router.deconfig_aps()
        self._all_ssids = list()
        for i in range(self.num_phys):
            self.context.configure(ap_config, multi_interface=True)
            self._all_ssids.append(self.context.router.get_ssid(instance=i))


    def _get_phy_num_for_instance(self, instance):
        """Get the phy number corresponding to a hostapd instance.

        @param instance: int hostapd instance to test against.
        @return int phy number corresponding to that AP (e.g.
                for phy0 return 0).

        """
        phy = self.context.router.get_hostapd_phy(instance)
        if not phy.startswith('phy'):
            raise error.TestError('Unexpected phy name %s' % phy)

        return int(phy[3:])


    def _wait_for_good_signal_levels(self, ssid, attenuator_info):
        """Verify the desired SSID is available with a good signal.

        @param ssid: the ssid as a string
        @param attenuator_info: dictionary with information about the
                                current attenuator

        @returns an updated attenuator_info dictionary

        """
        # In practice it has been observed that going from max attuation
        # to 0 attenuation may take several scans until the signal is what
        # is desirable.
        for i in range(5):
            scan_result = self._client_iw_runner.wait_for_scan_result(
                self._client_if, ssids=[ssid], timeout_seconds=10)
            if scan_result is None or len(scan_result) == 0:
                # Device is busy or not results at this time, try again
                continue
            for network in scan_result:
                if network.ssid == ssid and network.signal < LOW_POWER_SIGNAL:
                    logging.info('WARNING: Signal strength is less than '
                                 'optimal (%f) consider re-calibrating or '
                                 'check the conductive cabling.',
                                 network.signal)
                    attenuator_info.zeroed_scan_signal = network.signal
                    return attenuator_info
                elif network.ssid == ssid and network.signal > LOW_POWER_SIGNAL:
                    logging.info('Scan found an acceptable signal strength %f',
                                 network.signal)
                    attenuator_info.zeroed_scan_signal = network.signal
                    return attenuator_info
        raise error.TestError('The desired SSID is not visible, the '
                              'attenuator may be stuck or broken. '
                              'OR the AP is in a bad state or is '
                              'bad, try swapping.')


    def _verify_attenuator(self, ap_num, frequency_mhz, attenuator_num):
        """Verify that each phy has two attenuators controlling its signal.

        @param ap_num: int hostapd instance to test against.
        @param frequency_mhz: int frequency of the AP.
        @param attenuator_num: int attenuator num controlling one antenna on
                the AP.

        @return AttenuatorInfo object

        """
        logging.info('Verifying attenuator functionality')
        ai = AttenuatorInfo()
        # Remove knowledge of previous networks from shill.
        self.context.client.shill.init_test_network_state()
        # Isolate the client entirely.
        self.context.attenuator.set_variable_attenuation(
                attenuator_controller.MAX_VARIABLE_ATTENUATION)
        logging.info('Removing variable attenuation for attenuator=%d',
                     attenuator_num)
        # But allow one antenna on this phy.
        self.context.attenuator.set_variable_attenuation(
                0, attenuator_num=attenuator_num)
        client_conf = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid(instance=ap_num))

        logging.info('Waiting for client signal levels to settle.')
        ai = self._wait_for_good_signal_levels(client_conf.ssid, ai)
        logging.info('Connecting to %s', client_conf.ssid)
        assoc_result = xmlrpc_datatypes.deserialize(
                self.context.client.shill.connect_wifi(client_conf))
        if not assoc_result.success:
            logging.error('Failed to connect to AP %d on attenuator %d',
                          ap_num, attenuator_num)
            return ai
        ai.allows_connection = True
        ai.zeroed_linked_signal = self.context.client.wifi_signal_level
        logging.info('Connected successfully')
        start_atten = self.context.attenuator.get_minimal_total_attenuation()
        for atten in range(start_atten,
                           min(start_atten + 20, FINAL_ATTENUATION),
                           ATTENUATION_STEP):
            self.context.attenuator.set_total_attenuation(
                    atten, frequency_mhz, attenuator_num=attenuator_num)
            time.sleep(2)
            logging.info('Attenuator %d signal at attenuation=%d is %d dBm.',
                         attenuator_num, atten,
                         self.context.client.wifi_signal_level)
        return ai


    def _debug_phy_attenuator_correspondence(self, visible_ssid, hidden_ssid):
        """Verify that the non-attenuated SSID is the only one that is visble.

        If everything is working correctly then all the DUT should see is one
        SSID that is not the one which is attenuated.  Here are the different
        possible failure scenarios:
            - Two network_<blah> SSIDs are visible, both with a strong signal
              (something greater than -80 dBm) means the rainbow cables on the
              attenuation rig are backwards.
            - Two network_<blah> SSIDs are visble, the one which should be
              hidden is visible with something less than -80 dBm means one
              of the attenuators is broken.
            - The attenuated SSID is the only visible one, means that rainbow
              cables are in the wrong order.
            - The visible SSID is not seen, means that both attenuators are
              stuck at max attenuation or there is a cabling problem.

        @param visible_ssid: string of the SSID that should be visible.
        @param hidden_ssid: string of the SSID that should be hidden

        """
        scan_result = self._client_iw_runner.wait_for_scan_result(
                self._client_if, ssids=[visible_ssid, hidden_ssid])
        if scan_result is None or len(scan_result) == 0:
            raise error.TestFail('No visible SSIDs. Check cables, the '
                                 'attenuators may be stuck')
        elif (len(scan_result) == 1 and scan_result[0].ssid == hidden_ssid):
            raise error.TestFail('The wrong network is visible, the rainbow '
                                 'cables are in the wrong order.')
        elif len(scan_result) > 1:
            for network in scan_result:
                if (network.ssid == hidden_ssid):
                    # The SSID that should be hidden from the DUT is not,
                    # along with what is presumably the network that should
                    # be visible. Check the signal strength.
                    if network.signal > LOW_POWER_SIGNAL:
                        raise error.TestFail('Two SSIDs are visible, the '
                                             'rainbow cables may be '
                                             'connected backwards.')
                    else:
                        logging.warning('The attenuated SSID is visible with '
                                        'very low power (%f), the attenuator '
                                        'may be broken, or this is ghost '
                                        'signal; will attempt to connect',
                                        network.signal)


    def _verify_phy_attenuator_correspondence(self, instance):
        """Verify that we cannot connect to a phy when it is attenuated.

        Check that putting maximum attenuation on the attenuators expected
        to gate a particular phy produces the expected result.  We should
        be unable to connect to the corresponding SSID.

        @param instance: int hostapd instance to verify corresponds to
                a particular 2 attenuators.

        """
        logging.info('Verifying attenuator correspondence')
        # Turn up all attenuation.
        self.context.attenuator.set_variable_attenuation(
                attenuator_controller.MAX_VARIABLE_ATTENUATION)
        # Turn down attenuation for phys other than the instance we're
        # interested in.
        for other_instance in [x for x in range(self.num_phys)
                                 if x != instance]:
            other_phy_num = self._get_phy_num_for_instance(other_instance)
            for attenuator_offset in range(ATTENUATORS_PER_PHY):
                attenuator_num = (other_phy_num * ATTENUATORS_PER_PHY +
                                  attenuator_offset)
                self.context.attenuator.set_variable_attenuation(
                        0, attenuator_num=attenuator_num)
                # The other SSID should be available.
                self._debug_phy_attenuator_correspondence(
                    self.context.router.get_ssid(instance=other_instance),
                    self.context.router.get_ssid(instance=instance))
        # We should be unable to connect.
        client_conf = xmlrpc_datatypes.AssociationParameters(
                ssid=self.context.router.get_ssid(instance=instance),
                expect_failure=True)
        self.context.assert_connect_wifi(client_conf)


    def run_once(self):
        """For each PHY on a router, for 2 and 5 Ghz bands on a PHY:

        1) Set up an AP on the PHY.
        2) Walk the attenuators from low to high attenuations.
        3) Measure AP signal as attenuation increases.
        4) Tester should manually inspect that signal levels decrease linearly
           and are consistent from attenuator to attenuator.

        """
        # Create some re-usable client objects
        self._client_iw_runner = self.context.client.iw_runner
        self._client_if = self.context.client.wifi_if

        # Verify the client cell is clean
        scan_result = self._client_iw_runner.scan(self._client_if)
        if scan_result and len(scan_result) > 0:
            raise error.TestError('SSIDs found, the cell is not closed or '
                                  'is not cabled correctly.')

        attenuators_info = list()
        self.num_phys = len(self.context.router.iw_runner.list_phys())
        # Pick channels other than the calibrated ones.
        for frequency in (2447, 5660):
            for instance in range(self.num_phys):
                if self.num_phys > 1:
                    self._refresh_ap_ssids(frequency)
                    self._verify_phy_attenuator_correspondence(instance)
                phy_num = self._get_phy_num_for_instance(instance)
                for attenuator_offset in range(ATTENUATORS_PER_PHY):
                    attenuator_num = (phy_num * ATTENUATORS_PER_PHY +
                                      attenuator_offset)
                    self._refresh_ap_ssids(frequency)
                    attenuator_info = self._verify_attenuator(
                            instance, frequency, attenuator_num)
                    attenuators_info.append(attenuator_info)

        for info in attenuators_info:
            if info.healthy_attenuator is False:
                raise error.TestFail('One or more attenuators are broken!')

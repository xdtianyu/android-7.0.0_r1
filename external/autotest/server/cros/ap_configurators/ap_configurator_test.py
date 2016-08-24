# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit test for ap_configurator."""

import os
import sys
import unittest

# Define autotest_lib MAGIC!
sys.path.append(os.path.join(
                os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
from utils import common

from autotest_lib.server.cros import host_lock_manager
import ap_batch_locker
import ap_spec


class ConfiguratorTest(unittest.TestCase):
    """This test needs to be run against the UI interface of a real AP.

    The purpose of this test is to act as a basic acceptance test when
    developing a new AP configurator class.  Use this to make sure all core
    functionality is implemented.

    This test does not verify that everything works for ALL APs. It only
    tests against the AP specified below in AP_SPEC.

    Launch this unit test from outside chroot:
      $ cd ~/chromeos/src/third_party/autotest/files
      $ python utils/unittest_suite.py \
        server.cros.ap_configurators.ap_configurator_test --debug

    To run a single test, from outside chroot, e.g.
      $ cd ~/chromeos/src/third_party/autotest/files/\
           server/cros/ap_configurators
      $ python -m unittest ap_configurator_test.ConfiguratorTest.test_ssid
    """

    # Enter the hostname of the AP to test against
    AP_SPEC = ap_spec.APSpec(hostnames=['chromeos3-row4-rack1-host9'])

    # Do not actually power up the AP, assume it is on.
    OVERRIDE_POWER = True

    @classmethod
    def setUpClass(self):
        lock_manager = host_lock_manager.HostLockManager()
        self.batch_locker = ap_batch_locker.ApBatchLocker(lock_manager,
                            self.AP_SPEC, hostname_matching_only=True)
        ap_batch = self.batch_locker.get_ap_batch(batch_size=1)
        if not ap_batch:
            raise RuntimeError('Unable to lock AP %r' % self.AP_SPEC)
        self.ap = ap_batch[0]
        # Use a development webdriver server
        self.ap.webdriver_port = 9516
        if not self.OVERRIDE_POWER:
            print('Powering up the AP (this may take a minute...)')
            self.ap._power_up_router()
        else:
            print('Assuming AP is not, skipping power on.')
            self.ap.router_on = True


    @classmethod
    def tearDownClass(self):
        if self.batch_locker:
            self.batch_locker.unlock_aps()
        if not self.OVERRIDE_POWER:
            self.ap._power_down_router()


    def setUp(self):
        # All tests have to have a band pre-set.
        bands = self.ap.get_supported_bands()
        self.ap.set_band(bands[0]['band'])
        self.ap.apply_settings()


    def disabled_security_on_all_bands(self):
        """Disables security on all available bands."""
        for band in self.ap.get_supported_bands():
            self.ap.set_band(band['band'])
            self.ap.set_security_disabled()
            self.ap.apply_settings()


    def return_non_n_mode_pair(self):
        """Returns a mode and band that do not contain wireless mode N.

        Wireless N does not support several wifi security modes.  In order
        to test they can be configured that makes it easy to select an
        available compatible mode.
        """
        # Make this return something that does not contain N
        return_dict = {}
        for mode in self.ap.get_supported_modes():
            return_dict['band'] = mode['band']
            for mode_type in mode['modes']:
                if (mode_type & ap_spec.MODE_N) != ap_spec.MODE_N:
                    return_dict['mode'] = mode_type
                else:
                    raise RuntimeError('No modes without MODE_N')
        return return_dict


    def test_make_no_changes(self):
        """Test saving with no changes doesn't throw an error."""
        # Set to a known state.
        self.ap.set_radio(enabled=True)
        self.ap.apply_settings()
        # Set the same setting again.
        self.ap.set_radio(enabled=True)
        self.ap.apply_settings()


    def test_radio(self):
        """Test we can adjust the radio setting."""
        self.ap.set_radio(enabled=True)
        self.ap.apply_settings()
        self.ap.set_radio(enabled=False)
        self.ap.apply_settings()


    def test_channel(self):
        """Test adjusting the channel."""
        supported_bands = self.ap.get_supported_bands()
        for band in supported_bands:
            self.ap.set_band(band['band'])
            # Set to the second available channel
            self.ap.set_channel(band['channels'][1])
            self.ap.apply_settings()


    def test_visibility(self):
        """Test adjusting the visibility."""
        if not self.ap.is_visibility_supported():
            return
        self.ap.set_visibility(False)
        self.ap.apply_settings()
        self.ap.set_visibility(True)
        self.ap.apply_settings()


    def test_ssid(self):
        """Test setting the SSID."""
        bands_info = self.ap.get_supported_bands()
        self.assertTrue(bands_info, msg='Invalid band sent.')
        ssid = 'ssid2'
        for bands in bands_info:
            band = bands['band']
            if band == ap_spec.BAND_5GHZ:
                ssid = 'ssid5'
            self.ap.set_band(band)
            self.ap.set_ssid(ssid)
            self.ap.apply_settings()
        self.assertEqual(ssid, self.ap.ssid)


    def test_band(self):
        """Test switching the band."""
        self.ap.set_band(ap_spec.BAND_2GHZ)
        self.ap.apply_settings()
        self.ap.set_band(ap_spec.BAND_5GHZ)
        self.ap.apply_settings()


    def test_switching_bands_and_change_settings(self):
        """Test switching between bands and change settings for each band."""
        bands_info = self.ap.get_supported_bands()
        self.assertTrue(bands_info, msg='Invalid band sent.')
        bands_set = [d['band'] for d in bands_info]
        for band in bands_set:
            self.ap.set_band(band)
            self.ap.set_ssid('pqrstu_' + band)
            if self.ap.is_visibility_supported():
                self.ap.set_visibility(True)
            if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
                self.ap.set_security_wep('test2',
                                         ap_spec.WEP_AUTHENTICATION_OPEN)
            self.ap.apply_settings()


    def test_invalid_security(self):
        """Test an exception is thrown for an invalid configuration."""
        self.disabled_security_on_all_bands()
        for mode in self.ap.get_supported_modes():
            if not ap_spec.MODE_N in mode['modes']:
                return
        if not self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
            return
        self.ap.set_mode(ap_spec.MODE_N)
        self.ap.set_security_wep('77777', ap_spec.WEP_AUTHENTICATION_OPEN)
        try:
            self.ap.apply_settings()
        except RuntimeError, e:
            self.ap.driver.close()
            message = str(e)
            if message.find('no handler was specified') != -1:
                self.fail('Subclass did not handle an alert.')
            return
        self.fail('An exception should have been thrown but was not.')


    def test_security_wep(self):
        """Test configuring WEP security."""
        if not self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
            return
        for mode in self.ap.get_supported_modes():
            self.ap.set_band(mode['band'])
            for mode_type in mode['modes']:
                if mode_type & ap_spec.MODE_N != ap_spec.MODE_N:
                    self.ap.set_mode(mode_type)
                    self.ap.set_security_wep('45678',
                                             ap_spec.WEP_AUTHENTICATION_OPEN)
                    self.ap.apply_settings()
                    self.ap.set_security_wep('90123',
                                             ap_spec.WEP_AUTHENTICATION_SHARED)
                    self.ap.apply_settings()


    def test_priority_sets(self):
        """Test that commands are run in the right priority."""
        self.ap.set_radio(enabled=False)
        if self.ap.is_visibility_supported():
            self.ap.set_visibility(True)
        self.ap.set_ssid('prioritytest')
        self.ap.apply_settings()


    def test_security_and_general_settings(self):
        """Test updating settings that are general and security related."""
        self.disabled_security_on_all_bands()
        try:
            good_pair = self.return_non_n_mode_pair()
            self.ap.set_radio(enabled=False)
            self.ap.set_band(good_pair['band'])
            self.ap.set_mode(good_pair['mode'])
        except RuntimeError:
            # AP does not support modes without MODE_N
            return
        if self.ap.is_visibility_supported():
            self.ap.set_visibility(True)
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
            self.ap.set_security_wep('88888', ap_spec.WEP_AUTHENTICATION_OPEN)
        self.ap.set_ssid('secgentest')
        self.ap.apply_settings()


    def test_modes(self):
        """Tests switching modes."""
        # Some security settings won't work with some modes
        self.ap.set_security_disabled()
        self.ap.apply_settings()
        modes_info = self.ap.get_supported_modes()
        self.assertTrue(modes_info,
                        msg='Returned an invalid mode list.  Is this method'
                        ' implemented?')
        for band_modes in modes_info:
            self.ap.set_band(band_modes['band'])
            for mode in band_modes['modes']:
                self.ap.set_mode(mode)
                self.ap.apply_settings()


    def test_modes_with_band(self):
        """Tests switching modes that support adjusting the band."""
        # Different bands and security options conflict.  Disable security for
        # this test.
        self.disabled_security_on_all_bands()
        # Check if we support self.kModeN across multiple bands
        modes_info = self.ap.get_supported_modes()
        n_bands = []
        for band_modes in modes_info:
            if ap_spec.MODE_N in band_modes['modes']:
                n_bands.append(band_modes['band'])
        if len(n_bands) > 1:
            for n_band in n_bands:
                self.ap.set_mode(ap_spec.MODE_N, band=n_band)
                self.ap.apply_settings()


    def test_fast_cycle_security(self):
        """Mini stress for changing security settings rapidly."""
        self.disabled_security_on_all_bands()
        self.ap.set_radio(enabled=True)
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
            self.ap.set_security_wep('77777', ap_spec.WEP_AUTHENTICATION_OPEN)
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_DISABLED):
            self.ap.set_security_disabled()
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WPAPSK):
            self.ap.set_security_wpapsk(ap_spec.SECURITY_TYPE_WPAPSK,
                                        'qwertyuiolkjhgfsdfg')
        self.ap.apply_settings()


    def test_cycle_security(self):
        """Test switching between different security settings."""
        self.disabled_security_on_all_bands()
        try:
            good_pair = self.return_non_n_mode_pair()
            self.ap.set_radio(enabled=True)
            self.ap.set_band(good_pair['band'])
            self.ap.set_mode(good_pair['mode'])
        except RuntimeError:
            # AP does not support modes without MODE_N
            return
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
            self.ap.set_security_wep('77777', ap_spec.WEP_AUTHENTICATION_OPEN)
        self.ap.apply_settings()
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_DISABLED):
            self.ap.set_security_disabled()
        self.ap.apply_settings()
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WPA2PSK):
            self.ap.set_security_wpapsk(ap_spec.SECURITY_TYPE_WPA2PSK,
                                        'qwertyuiolkjhgfsdfg')
        self.ap.apply_settings()


    def test_actions_when_radio_disabled(self):
        """Test making changes when the radio is disabled."""
        self.disabled_security_on_all_bands()
        try:
            good_pair = self.return_non_n_mode_pair()
            self.ap.set_radio(enabled=False)
            self.ap.set_band(good_pair['band'])
            self.ap.set_mode(good_pair['mode'])
        except RuntimeError:
            # AP does not support modes without MODE_N
            return
        self.ap.apply_settings()
        if self.ap.is_security_mode_supported(ap_spec.SECURITY_TYPE_WEP):
            self.ap.set_security_wep('77777', ap_spec.WEP_AUTHENTICATION_OPEN)
        self.ap.set_radio(enabled=False)
        self.ap.apply_settings()


    def test_configuring_with_ap_spec(self):
        """Test configuring the AP using an APSpec."""
        spec = ap_spec.APSpec()
        self.ap.set_using_ap_spec(spec)
        self.ap.apply_settings()


    def test_power_cycle_router(self):
        """Test powering the ap down and back up again."""
        self.ap.power_cycle_router_up()


if __name__ == '__main__':
    unittest.main()

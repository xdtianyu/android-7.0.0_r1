#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/ap_configurators/ap_configurator_factory.py.
"""

import mox
import unittest

import common

from autotest_lib.server.cros.ap_configurators import \
    ap_configurator_config
from autotest_lib.server.cros.ap_configurators import \
    ap_configurator_factory
from autotest_lib.server.cros.ap_configurators import \
    ap_spec


class APConfiguratorFactoryTest(mox.MoxTestBase):
    """Unit tests for ap_configurator_factory.APConfiguratorFactory."""


    class MockAp(object):
        """Mock object used to test _get_aps_with_bands()."""

        def __init__(self, bands_and_channels=[], bands_and_modes=[],
                     supported_securities=[], visibility_supported=False,
                     host_name='mock_ap',
                     configurator_type=ap_spec.CONFIGURATOR_ANY):
            """Constructor.

            @param bands_and_channels: a list of dicts of strings, e.g.
                [{'band': self.ap_config.BAND_2GHZ, 'channels': [5]},
                 {'band': self.ap_config.BAND_5GHZ, 'channels': [48]}]
            @param bands_and_modes: a list of dicts of strings, e.g.
                [{'band': self.ap_config.BAND_2GHZ,
                  'modes': [self.ap_config.MODE_B]},
                 {'band': self.ap_config.BAND_5GHZ,
                  'modes': [self.ap_config.MODE_G]}]
            @param supported_securities: a list of integers.
            @param visibility_supported: a boolean
            """
            self.bands_and_channels = bands_and_channels
            self.bands_and_modes = bands_and_modes
            self.supported_securities = supported_securities
            self.visibility_supported = visibility_supported
            self.host_name = host_name
            self.channel = None
            self.configurator_type = configurator_type


        def get_supported_bands(self):
            """@returns supported bands and channels."""
            return self.bands_and_channels


        def get_supported_modes(self):
            """@returns supported bands and modes."""
            return self.bands_and_modes


        def is_security_mode_supported(self, security):
            """Checks if security is supported.

            @param security: an integer, security method.
            @returns a boolean, True iff security is supported.
            """
            return security in self.supported_securities


        def is_visibility_supported(self):
            """Returns if visibility is supported."""
            return self.visibility_supported


        def host_name(self):
            """Returns the host name of the AP."""
            return self.host_name


        def set_using_ap_spec(self, ap_spec):
            """Sets a limited numberof setting of the AP.

            @param ap_spec: APSpec object
            """
            self.channel = ap_spec.channel


        def configurator_type(self):
            """Returns the configurator type."""
            return self.configurator_type


        def get_channel(self):
            """Returns the channel."""
            return self.channel


    def setUp(self):
        """Initialize."""
        super(APConfiguratorFactoryTest, self).setUp()
        self.factory = ap_configurator_factory.APConfiguratorFactory()
        # ap_config is used to fetch constants such as bands, modes, etc.
        self.ap_config = ap_configurator_config.APConfiguratorConfig()


    """New tests that cover the new ap_spec use case."""
    def _build_ap_test_inventory(self):
        # AP1 supports 2.4GHz band, all modes, and all securities.
        self.mock_ap1 = self.MockAp(
            bands_and_channels=[{'band': ap_spec.BAND_2GHZ,
                                 'channels': ap_spec.VALID_2GHZ_CHANNELS}],
            bands_and_modes=[{'band': ap_spec.BAND_2GHZ,
                              'modes': ap_spec.VALID_2GHZ_MODES}],
            supported_securities=ap_spec.VALID_SECURITIES,
            host_name='chromeos3-row2-rack1-host1',
            configurator_type=ap_spec.CONFIGURATOR_STATIC
            )
        # AP2 supports 2.4 and 5 GHz, all modes, open system, and visibility.
        self.mock_ap2 = self.MockAp(
            bands_and_channels=[{'band': ap_spec.BAND_2GHZ,
                                 'channels': ap_spec.VALID_2GHZ_CHANNELS},
                                {'band': ap_spec.BAND_5GHZ,
                                 'channels': ap_spec.VALID_5GHZ_CHANNELS}],
            bands_and_modes=[{'band': ap_spec.BAND_2GHZ,
                              'modes': ap_spec.VALID_2GHZ_MODES},
                             {'band': ap_spec.BAND_5GHZ,
                              'modes': ap_spec.VALID_5GHZ_MODES}],
            supported_securities=[ap_spec.SECURITY_TYPE_DISABLED],
            visibility_supported=True,
            configurator_type=ap_spec.CONFIGURATOR_DYNAMIC,
            host_name='chromeos3-row2-rack1-host2',
            )
        # AP3 supports 2.4GHz band, all modes, all securities, and is not
        # in the lab.
        self.mock_ap3 = self.MockAp(
            bands_and_channels=[{'band': ap_spec.BAND_2GHZ,
                                 'channels': ap_spec.VALID_2GHZ_CHANNELS}],
            bands_and_modes=[{'band': ap_spec.BAND_2GHZ,
                              'modes': ap_spec.VALID_2GHZ_MODES}],
            supported_securities=ap_spec.VALID_SECURITIES,
            host_name='chromeos3-row7-rack1-host2',
            configurator_type=ap_spec.CONFIGURATOR_STATIC
            )
        self.factory.ap_list = [self.mock_ap1, self.mock_ap2, self.mock_ap3]


    def testGetApConfigurators_WithBandAPSpec(self):
        """Test with a band only specified AP Spec"""
        self._build_ap_test_inventory()

        spec = ap_spec.APSpec(band=ap_spec.BAND_2GHZ)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap1, self.mock_ap2].sort(), actual.sort())

        spec = ap_spec.APSpec(band=ap_spec.BAND_5GHZ)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap2], actual)


    def testGetAPConfigurators_WithModeAPSpec(self):
        """Test with a mode only specified AP Spec"""
        self._build_ap_test_inventory()

        spec = ap_spec.APSpec(mode=ap_spec.DEFAULT_2GHZ_MODE)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap1, self.mock_ap2].sort(), actual.sort())

        spec = ap_spec.APSpec(mode=ap_spec.DEFAULT_5GHZ_MODE)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap2], actual)


    def testGetAPConfigurators_WithSecurityAPSpec(self):
        """Test with a security only specified AP Spec"""
        self._build_ap_test_inventory()
        spec = ap_spec.APSpec(security=ap_spec.SECURITY_TYPE_WPAPSK)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap1], actual)


    def testGetAPConfigurators_WithVisibilityAPSpec(self):
        """Test with a visibility specified AP Spec."""
        self._build_ap_test_inventory()

        spec = ap_spec.APSpec(visible=True)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap1, self.mock_ap2].sort(), actual.sort())

        spec = ap_spec.APSpec(band=ap_spec.BAND_5GHZ, visible=False)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap2], actual)


    def testGetAPConfigurators_ByHostName(self):
        """Test obtaining a list of APs by hostname."""
        self._build_ap_test_inventory()

        spec = ap_spec.APSpec(hostnames=['chromeos3-row2-rack1-host1'])
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap1], actual)

        spec = ap_spec.APSpec(hostnames=['chromeos3-row2-rack1-host1',
                                         'chromeos3-row2-rack1-host2'])
        actual = self.factory.get_ap_configurators_by_spec(spec=spec)
        self.assertEquals([self.mock_ap1, self.mock_ap2].sort(), actual.sort())


    def testGetAndPreConfigureAPConfigurators(self):
        """Test preconfiguring APs."""
        self._build_ap_test_inventory()

        # Pick something that is not the default channel.
        channel = ap_spec.VALID_5GHZ_CHANNELS[-1]
        spec = ap_spec.APSpec(channel=channel)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec,
                                                           pre_configure=True)
        self.assertEquals([self.mock_ap2], actual)
        self.assertEquals(actual[0].get_channel(), channel)


    def testGetAPConfigurators_ByType(self):
        """Test obtaining configurators by type."""
        self._build_ap_test_inventory()

        spec = ap_spec.APSpec(configurator_type=ap_spec.CONFIGURATOR_STATIC)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec,
                                                           pre_configure=True)
        self.assertEquals([self.mock_ap1], actual)


    def testGetAPConfigurators_ByLab(self):
        """Test obtaining configurators by location relative to the lab."""
        self._build_ap_test_inventory()

        spec = ap_spec.APSpec(lab_ap=False)
        actual = self.factory.get_ap_configurators_by_spec(spec=spec,
                                                           pre_configure=True)
        self.assertEquals([self.mock_ap3], actual)


if __name__ == '__main__':
    unittest.main()

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/ap_configurators/ap_spec.py.
"""

import unittest

from autotest_lib.server.cros.ap_configurators import \
    ap_spec

class APSpecTest(unittest.TestCase):
    """Unit test for the ap_spec object."""

    def test_default_creation(self):
        """Test building a default ap_spec object."""
        spec = ap_spec.APSpec()
        self.assertEquals(spec.visible, True)
        self.assertEquals(spec.security, ap_spec.DEFAULT_SECURITY_TYPE)
        self.assertEquals(spec.band, ap_spec.DEFAULT_BAND)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_2GHZ_MODE)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)
        self.assertIsNone(spec.password)


    def test_only_set_band_2ghz(self):
        """Test setting only the band to 2GHz."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_2GHZ)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_2GHZ_MODE)


    def test_only_set_band_5ghz(self):
        """Test setting only the band to 5GHz."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_5GHZ)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_5GHZ_CHANNEL)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_5GHZ_MODE)


    def test_only_set_mode_2ghz(self):
        """Test setting only a 2GHz mode."""
        spec = ap_spec.APSpec(mode=ap_spec.MODE_B)
        self.assertEquals(spec.band, ap_spec.DEFAULT_BAND)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)


    def test_only_set_mode_5ghz(self):
        """Test setting only a 5GHz mode."""
        spec = ap_spec.APSpec(mode=ap_spec.MODE_A)
        self.assertEquals(spec.band, ap_spec.BAND_5GHZ)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_5GHZ_CHANNEL)


    def test_only_set_mode_n(self):
        """Test setting the mode to N."""
        spec = ap_spec.APSpec(mode=ap_spec.MODE_N)
        self.assertEquals(spec.band, ap_spec.DEFAULT_BAND)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)


    def test_only_set_channel_2ghz(self):
        """Test setting only a 2GHz channel."""
        spec = ap_spec.APSpec(channel=ap_spec.DEFAULT_2GHZ_CHANNEL)
        self.assertEquals(spec.band, ap_spec.BAND_2GHZ)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_2GHZ_MODE)


    def test_only_set_channel_5ghz(self):
        """Test setting only a 5GHz channel."""
        spec = ap_spec.APSpec(channel=ap_spec.DEFAULT_5GHZ_CHANNEL)
        self.assertEquals(spec.band, ap_spec.BAND_5GHZ)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_5GHZ_MODE)


    def test_set_band_and_mode_2ghz(self):
        """Test setting the band and mode to valid 2GHz values."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_2GHZ, mode=ap_spec.MODE_G)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)


    def test_set_band_and_mode_5ghz(self):
        """Test setting the band and mode to valid 5GHz values."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_5GHZ, mode=ap_spec.MODE_A)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_5GHZ_CHANNEL)


    def test_set_band_mode_and_channel_2ghz(self):
        """Test setting the band and channel to valid 2GHz values."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_2GHZ, mode=ap_spec.MODE_N,
                              channel=ap_spec.DEFAULT_2GHZ_CHANNEL)
        self.assertNotEquals(spec.mode, ap_spec.DEFAULT_5GHZ_MODE)


    def test_set_band_mode_and_channel_5ghz(self):
        """Test setting the band and channel to valid 5GHz value."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_5GHZ, mode=ap_spec.MODE_N,
                              channel=ap_spec.DEFAULT_5GHZ_CHANNEL)
        self.assertNotEquals(spec.mode, ap_spec.DEFAULT_2GHZ_MODE)


    def test_set_security_psk_default(self):
        """Test setting security to WPAPSK."""
        spec = ap_spec.APSpec(security=ap_spec.SECURITY_TYPE_WPAPSK)
        self.assertEquals(spec.visible, True)
        self.assertEquals(spec.security, ap_spec.SECURITY_TYPE_WPAPSK)
        self.assertEquals(spec.band, ap_spec.DEFAULT_BAND)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_2GHZ_MODE)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)


    def test_set_security_and_visibility(self):
        """Test setting visibility to hidden and security to WPAPSK."""
        spec = ap_spec.APSpec(visible=False,
                              security=ap_spec.SECURITY_TYPE_WPAPSK)
        self.assertEquals(spec.visible, False)
        self.assertEquals(spec.security, ap_spec.SECURITY_TYPE_WPAPSK)
        self.assertEquals(spec.band, ap_spec.DEFAULT_BAND)
        self.assertEquals(spec.mode, ap_spec.DEFAULT_2GHZ_MODE)
        self.assertEquals(spec.channel, ap_spec.DEFAULT_2GHZ_CHANNEL)
        self.assertIsNotNone(spec.password)


    def test_invalid_mode_and_band(self):
        """Test setting mode and band to non-compatible settings."""
        self.assertRaises(ValueError, ap_spec.APSpec,
                          band=ap_spec.BAND_2GHZ, mode=ap_spec.MODE_A)


    def test_invalid_channel_and_band(self):
        """Test setting channel and band to non-compatible settings."""
        self.assertRaises(ValueError, ap_spec.APSpec,
                          band=ap_spec.BAND_5GHZ, channel=1)


    def test_invalid_mode_and_channel(self):
        """Test setting mode and channel to non-compatible settings."""
        self.assertRaises(ValueError, ap_spec.APSpec,
                          mode=ap_spec.MODE_G, channel=153)


    def test_invalid_values(self):
        """Test passing invalid values to an ap_spec object."""
        self.assertRaises(ValueError, ap_spec.APSpec, band='foo')

        self.assertRaises(ValueError, ap_spec.APSpec, mode=0x3)

        self.assertRaises(ValueError, ap_spec.APSpec, channel=84)

        self.assertRaises(ValueError, ap_spec.APSpec, security='foo')


    def test_mode_string_generation(self):
        """Test a set of mode constants a generates a human readable string."""
        mode = ap_spec.mode_string_for_mode(ap_spec.MODE_B | ap_spec.MODE_G)
        self.assertEquals('b/g', mode)

        mode = ap_spec.mode_string_for_mode(ap_spec.MODE_B | ap_spec.MODE_G |
                                            ap_spec.MODE_N)
        self.assertEquals('b/g/n', mode)

        mode = ap_spec.mode_string_for_mode(ap_spec.MODE_A)
        self.assertEquals('a', mode)


    def test_mode_n_on_both_bands(self):
        """Test that band is maintained when setting a mode N spec."""
        spec = ap_spec.APSpec(band=ap_spec.BAND_5GHZ, mode=ap_spec.MODE_N)
        self.assertEquals(spec.band, ap_spec.BAND_5GHZ)
        self.assertEquals(spec.mode, ap_spec.MODE_N)
        spec = ap_spec.APSpec(band=ap_spec.BAND_2GHZ, mode=ap_spec.MODE_N)
        self.assertEquals(spec.band, ap_spec.BAND_2GHZ)
        self.assertEquals(spec.mode, ap_spec.MODE_N)


if __name__ == '__main__':
    unittest.main()

# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class APConfiguratorConfig(object):
    """Parameters to configure a APConfigurator."""

    BAND_2GHZ = '2.4GHz'
    BAND_5GHZ = '5GHz'

    # List of valid 802.11 protocols (modes).
    MODE_A = 0x00001
    MODE_B = 0x00010
    MODE_G = 0x00100
    MODE_N = 0x01000
    MODE_AC = 0x10000
    MODE_AUTO = 0x100000
    MODE_M = 0x0111  # Used for standard maintenance
    MODE_D = 0x1011  # International roaming extensions
    SECURITY_TYPE_DISABLED = 'disabled'
    SECURITY_TYPE_WEP = 'wep'
    SECURITY_TYPE_WPAPSK = 'wpa-psk'
    SECURITY_TYPE_WPA2PSK = 'wpa2-psk'

    WEP_AUTHENTICATION_OPEN = 'open'
    WEP_AUTHENTICATION_SHARED = 'shared'
    # List of valid bands.
    VALID_BANDS = [BAND_2GHZ, BAND_5GHZ]

    # List of valid modes.
    VALID_MODES = [MODE_A, MODE_AC, MODE_AUTO, MODE_B, MODE_D, MODE_G, MODE_M,
                   MODE_N]


    # List of valid securities.
    VALID_SECURITIES = [SECURITY_TYPE_DISABLED,
                        SECURITY_TYPE_WEP,
                        SECURITY_TYPE_WPAPSK,
                        SECURITY_TYPE_WPA2PSK]


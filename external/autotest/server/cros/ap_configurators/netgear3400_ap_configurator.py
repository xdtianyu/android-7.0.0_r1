# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import netgear_WNDR_dual_band_configurator
import ap_spec


class Netgear3400APConfigurator(
        netgear_WNDR_dual_band_configurator.NetgearDualBandAPConfigurator):
    """Base class for Netgear WNDR 3400 v2 and 3700 v3 dual band routers."""

    def is_security_mode_supported(self, security_mode):
        """Returns if the supported security modes.

        @param security_mode: the security mode to check against
        @return True if the security_mode is supported by the router

        """
        return security_mode in (ap_spec.SECURITY_TYPE_DISABLED,
                                 ap_spec.SECURITY_TYPE_WPAPSK,
                                 ap_spec.SECURITY_TYPE_WEP)

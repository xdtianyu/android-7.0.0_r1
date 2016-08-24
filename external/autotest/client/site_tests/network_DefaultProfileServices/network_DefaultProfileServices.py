# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking import shill_context
from autotest_lib.client.cros.networking import shill_proxy

class network_DefaultProfileServices(test.test):
    """The Default Profile Services class.

    Wipe the default profile, start shill, configure a service, restart
    shill, and check that the service exists.

    The service name is chosen such that it is unlikely match any SSID
    that is present over-the-air.

    """
    DEFAULT_PROFILE_PATH = '/var/cache/shill/default.profile'
    OUR_SSID = 'org.chromium.DfltPrflSrvcsTest'
    version = 1


    def run_once(self):
        """Test main loop."""
        with shill_context.stopped_shill():
            os.remove(self.DEFAULT_PROFILE_PATH)
        shill = shill_proxy.ShillProxy.get_proxy()
        if shill is None:
            raise error.TestFail('Could not connect to shill')

        shill.manager.PopAllUserProfiles()
        path = shill.configure_service({
                shill.SERVICE_PROPERTY_TYPE: 'wifi',
                shill.SERVICE_PROPERTY_MODE: 'managed',
                shill.SERVICE_PROPERTY_SSID: self.OUR_SSID,
                shill.SERVICE_PROPERTY_HIDDEN: True,
                shill.SERVICE_PROPERTY_SECURITY_CLASS: 'none',
                })

        with shill_context.stopped_shill():
            # We don't actually need to do anything while shill is
            # stopped. We just want shill to be restarted.
            pass
        shill = shill_proxy.ShillProxy.get_proxy()
        if shill is None:
            raise error.TestFail('Could not connect to shill')

        shill.manager.PopAllUserProfiles()
        service = shill.find_object('AnyService',
                                    {'Name': self.OUR_SSID})
        if not service:
            raise error.TestFail('Network not found after restart.')

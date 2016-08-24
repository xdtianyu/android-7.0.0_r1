# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.networking import shill_context
from autotest_lib.client.cros.networking import shill_proxy

class network_DefaultProfileCreation(test.test):
    """The Default Profile Creation class.

    Wipe the default profile, start shill, and check that a default
    profile has been created.

    Test that the default profile contains default values for properties
    that should have them.

    """
    DEFAULT_PROFILE_PATH = '/var/cache/shill/default.profile'
    EXPECTED_SETTINGS = [
        # From DefaultProfile::LoadManagerProperties
        'CheckPortalList=ethernet,wifi,cellular',
        'IgnoredDNSSearchPaths=gateway.2wire.net',
        'LinkMonitorTechnologies=wifi',
        'PortalURL=http://www.gstatic.com/generate_204',
        'PortalCheckInterval=30',
        ]
    PROFILE_LOAD_TIMEOUT_SECONDS = 5
    version = 1


    def run_once(self):
        """Test main loop."""
        with shill_context.stopped_shill():
            try:
                os.remove(self.DEFAULT_PROFILE_PATH)
            except OSError as e:
                if e.errno != errno.ENOENT:
                    raise e
        shill = shill_proxy.ShillProxy.get_proxy()
        start_time = time.time()
        profile = None
        while time.time() - start_time < self.PROFILE_LOAD_TIMEOUT_SECONDS:
            if shill.get_profiles():
                with open(self.DEFAULT_PROFILE_PATH) as f:
                    profile = f.read()
                    if profile:
                        break

            time.sleep(1)
        else:
            if profile is None:
                raise error.TestFail('shill should load a profile within '
                                     '%d seconds.' %
                                     self.PROFILE_LOAD_TIMEOUT_SECONDS)
            else:
                raise error.TestFail('shill profile is still empty after '
                                     '%d seconds.' %
                                     self.PROFILE_LOAD_TIMEOUT_SECONDS)

        logging.info('Profile contents after %d seconds:\%s',
                     time.time() - start_time, profile)
        for setting in self.EXPECTED_SETTINGS:
            if setting not in profile:
                logging.error('Did not find setting %s', setting)
                logging.error('Full profile contents are:\n%s', profile)
                raise error.TestFail('Missing setting(s) in default profile.')

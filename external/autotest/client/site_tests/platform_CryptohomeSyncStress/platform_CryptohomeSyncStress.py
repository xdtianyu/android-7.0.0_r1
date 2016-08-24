# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, string
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import login

class platform_CryptohomeSyncStress(test.test):
    version = 1

    def run_once(self, username, password):
        # log in. don't use UITest because it uses its own auth and
        # DNS servers, and we need to do real login and chrome sync
        login.attempt_login(username, password)

        # make sure cryptohome is mounted
        bus = dbus.SystemBus()
        proxy = bus.get_object('org.chromium.Cryptohome',
                               '/org/chromium/Cryptohome')
        cryptohome = dbus.Interface(proxy, 'org.chromium.CryptohomeInterface')

        ismounted = cryptohome.IsMounted()
        if not ismounted:
            raise error.TestFail('Cryptohome failed to mount.')

        self.job.set_state('client_fail', False)

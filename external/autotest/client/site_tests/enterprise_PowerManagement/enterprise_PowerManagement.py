# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.cros import enterprise_base
from autotest_lib.client.bin import utils


class enterprise_PowerManagement(enterprise_base.EnterpriseTest):
    """Verify the power management policy setting."""
    version = 1


    def setup(self):
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')


    def initialize(self):
        self.import_dmserver(self.srcdir)
        super(enterprise_PowerManagement, self).initialize()


    def _setup_lock_policy(self):
        """Setup policy to lock screen in 10 seconds of idle time."""
        self._screen_lock_delay = 10
        screen_lock_policy = '{ "AC": %d }' % (self._screen_lock_delay*1000)

        policy_blob = """{
            "google/chromeos/user": {
                "mandatory": {
                    "ScreenLockDelays": %s
                }
            },
            "managed_users": [ "*" ],
            "policy_user": "%s",
            "current_key_index": 0,
            "invalidation_source": 16,
            "invalidation_name": "test_policy"
        }""" % (json.dumps(screen_lock_policy), self.USERNAME)

        self.setup_policy(policy_blob)


    def _setup_logout_policy(self):
        """Setup policy to logout in 10 seconds of idle time."""
        self._screen_logout_delay = 10
        idle_settings_policy = '''{
            "AC": {
                "Delays": {
                    "ScreenDim": 2000,
                    "ScreenOff": 3000,
                    "IdleWarning": 4000,
                    "Idle": %d
                 },
                 "IdleAction": "Logout"
            }
        }''' % (self._screen_logout_delay*1000)

        policy_blob = """{
            "google/chromeos/user": {
                "mandatory": {
                    "PowerManagementIdleSettings": %s
                }
            },
            "managed_users": [ "*" ],
            "policy_user": "%s",
            "current_key_index": 0,
            "invalidation_source": 16,
            "invalidation_name": "test_policy"
        }""" % (json.dumps(idle_settings_policy), self.USERNAME)

        self.setup_policy(policy_blob)


    def run_once(self):
        """Run the power management policy tests."""
        self._setup_lock_policy()
        with self.create_chrome(autotest_ext=True) as cr:
            utils.poll_for_condition(
                lambda: cr.login_status['isScreenLocked'],
                exception=error.TestFail('User is not locked'),
                timeout=self._screen_lock_delay*2,
                sleep_interval=1,
                desc='Expects to find Chrome locked.')

        self._setup_logout_policy()
        with self.create_chrome() as cr:
            utils.poll_for_condition(
                lambda: not cryptohome.is_vault_mounted(user=self.USERNAME,
                                                        allow_fail=True),
                exception=error.TestFail('User is not logged out'),
                timeout=self._screen_logout_delay*2,
                sleep_interval=1,
                desc='Expects to find user logged out.')


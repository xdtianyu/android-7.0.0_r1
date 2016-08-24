# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome


class telemetry_LoginTest(test.test):
    """This is a client side Telemetry Login Test."""
    version = 1


    def _get_login_status(self, cr):
        login_status = cr.login_status
        if not login_status:
            raise error.TestFail('Failed to get LoginStatus')

        if type(login_status) != dict:
            raise error.TestFail('LoginStatus type mismatch %r'
                                 % type(login_status))

        is_regular_user = login_status['isRegularUser']
        is_guest = login_status['isGuest']
        email = login_status['email']
        if is_regular_user == is_guest:
            raise error.TestFail('isRegularUser == isGuest')
        return (is_regular_user, email)


    def run_once(self):
        """Test chrome login status.

        This test uses telemetry via chrome.py to log in as a regular user,
        and then checks chrome login status via the private extension api
        autotestPrivate.loginStatus.
        """
        with chrome.Chrome(logged_in=True, autotest_ext=True) as cr:
            (is_regular_user, email) = self._get_login_status(cr)
            if not is_regular_user:
                raise error.TestFail('isRegularUser should be True')
            if email != cr.username:
                raise error.TestFail('user email mismatch %s' % email)

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject, hashlib, os
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome, session_manager
from autotest_lib.client.cros import constants, cryptohome, ownership


class login_OwnershipNotRetaken(test.test):
    """Subsequent logins after the owner must not clobber the owner's key."""
    version = 2

    _TEST_USER = 'example@chromium.org'
    _TEST_PASS = 'testme'
    _TEST_GAIAID = '7583'


    def initialize(self):
        super(login_OwnershipNotRetaken, self).initialize()
        # Start clean, wrt ownership and the desired user.
        ownership.restart_ui_to_clear_ownership_files()

        bus_loop = DBusGMainLoop(set_as_default=True)
        self._cryptohome_proxy = cryptohome.CryptohomeProxy(bus_loop)


    def run_once(self):
        listener = session_manager.OwnershipSignalListener(gobject.MainLoop())
        listener.listen_for_new_key_and_policy()
        # Sign in. Sign out happens automatically when cr goes out of scope.
        with chrome.Chrome(clear_enterprise_policy=False) as cr:
            listener.wait_for_signals(desc='Owner settings written to disk.')

        key = open(constants.OWNER_KEY_FILE, 'rb')
        hash = hashlib.md5(key.read())
        key.close()
        mtime = os.stat(constants.OWNER_KEY_FILE).st_mtime

        # Sign in/sign out as a second user.
        with chrome.Chrome(clear_enterprise_policy=False,
                           username=self._TEST_USER,
                           password=self._TEST_PASS,
                           gaia_id=self._TEST_GAIAID) as cr:
            pass

        # Checking mtime to see if key file was touched during second sign in.
        if os.stat(constants.OWNER_KEY_FILE).st_mtime > mtime:
            raise error.TestFail("Owner key was touched on second login!")

        # Sanity check.
        key2 = open(constants.OWNER_KEY_FILE, 'rb')
        hash2 = hashlib.md5(key2.read())
        key2.close()
        if hash.hexdigest() != hash2.hexdigest():
            raise error.TestFail("Owner key was touched on second login!")


    def cleanup(self):
        self._cryptohome_proxy.remove(self._TEST_USER)
        super(login_OwnershipNotRetaken, self).cleanup()

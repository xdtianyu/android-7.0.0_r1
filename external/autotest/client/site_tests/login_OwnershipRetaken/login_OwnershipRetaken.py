# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject, os
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import policy, session_manager
from autotest_lib.client.cros import constants, cros_ui, cryptohome, ownership


class login_OwnershipRetaken(test.test):
    """"Ensure that ownership is re-taken upon loss of owner's cryptohome."""
    version = 1

    _tempdir = None
    _got_new_key = False
    _got_new_policy = False

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')


    def initialize(self):
        super(login_OwnershipRetaken, self).initialize()
        # Start clean, wrt ownership and the desired user.
        ownership.restart_ui_to_clear_ownership_files()

        bus_loop = DBusGMainLoop(set_as_default=True)
        self._cryptohome_proxy = cryptohome.CryptohomeProxy(bus_loop)
        self._cryptohome_proxy.remove(ownership.TESTUSER)

        self._sm = session_manager.connect(bus_loop)


    def run_once(self):
        pkey = ownership.known_privkey()
        pubkey = ownership.known_pubkey()

        # Pre-configure some owner settings, including initial key.
        poldata = policy.build_policy_data(self.srcdir,
                                           owner=ownership.TESTUSER,
                                           guests=False,
                                           new_users=True,
                                           roaming=True,
                                           whitelist=(ownership.TESTUSER,
                                                      'a@b.c'),
                                           proxies={ 'proxy_mode': 'direct' })
        policy_string = policy.generate_policy(self.srcdir,
                                               pkey,
                                               pubkey,
                                               poldata)
        policy.push_policy_and_verify(policy_string, self._sm)

        # grab key, ensure that it's the same as the known key.
        if (utils.read_file(constants.OWNER_KEY_FILE) != pubkey):
            raise error.TestFail('Owner key should not have changed!')

        # Start a new session, which will trigger the re-taking of ownership.
        listener = session_manager.OwnershipSignalListener(gobject.MainLoop())
        listener.listen_for_new_key_and_policy()
        self._cryptohome_proxy.mount(ownership.TESTUSER,
                                     ownership.TESTPASS,
                                     create=True)
        if not self._sm.StartSession(ownership.TESTUSER, ''):
            raise error.TestError('Could not start session for owner')

        listener.wait_for_signals(desc='Re-taking of ownership complete.')

        # grab key, ensure that it's different than known key
        if (utils.read_file(constants.OWNER_KEY_FILE) == pubkey):
            raise error.TestFail('Owner key should have changed!')

        # RetrievePolicy, check sig against new key, check properties
        retrieved_policy = self._sm.RetrievePolicy(byte_arrays=True)
        if retrieved_policy is None:
            raise error.TestError('Policy not found')
        policy.compare_policy_response(self.srcdir,
                                       retrieved_policy,
                                       owner=ownership.TESTUSER,
                                       guests=False,
                                       new_users=True,
                                       roaming=True,
                                       whitelist=(ownership.TESTUSER, 'a@b.c'),
                                       proxies={ 'proxy_mode': 'direct' })


    def cleanup(self):
        if self._tempdir: self._tempdir.clean()
        cros_ui.restart()
        self._cryptohome_proxy.remove(ownership.TESTUSER)
        super(login_OwnershipRetaken, self).cleanup()

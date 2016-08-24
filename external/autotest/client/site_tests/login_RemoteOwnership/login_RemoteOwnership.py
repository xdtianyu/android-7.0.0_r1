# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, random, string, os
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import policy, session_manager
from autotest_lib.client.cros import cros_ui, cryptohome, ownership


class login_RemoteOwnership(test.test):
    """Tests to ensure that the Ownership API can be used, as an
       enterprise might, to set device policies.
    """

    version = 1

    def setup(self):
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')


    def initialize(self):
        # Start with a clean slate wrt ownership
        ownership.restart_ui_to_clear_ownership_files()
        super(login_RemoteOwnership, self).initialize()

        bus_loop = DBusGMainLoop(set_as_default=True)
        self._cryptohome_proxy = cryptohome.CryptohomeProxy(bus_loop)
        self._sm = session_manager.connect(bus_loop)


    def run_once(self):
        # Initial policy setup.
        poldata = policy.build_policy_data(self.srcdir)
        priv = ownership.known_privkey()
        pub = ownership.known_pubkey()
        policy.push_policy_and_verify(
            policy.generate_policy(self.srcdir, priv, pub, poldata), self._sm)

        # Force re-key the device
        (priv, pub) = ownership.pairgen_as_data()
        policy.push_policy_and_verify(
            policy.generate_policy(self.srcdir, priv, pub, poldata), self._sm)

        # Rotate key gracefully.
        self.username = (''.join(random.sample(string.ascii_lowercase,6)) +
                         "@foo.com")
        password = ''.join(random.sample(string.ascii_lowercase,6))
        self._cryptohome_proxy.remove(self.username)
        self._cryptohome_proxy.mount(self.username, password, create=True)

        (new_priv, new_pub) = ownership.pairgen_as_data()

        if not self._sm.StartSession(self.username, ''):
            raise error.TestFail('Could not start session for random user')

        policy.push_policy_and_verify(
            policy.generate_policy(self.srcdir,
                                   key=new_priv,
                                   pubkey=new_pub,
                                   policy=poldata,
                                   old_key=priv),
            self._sm)

        try:
            self._sm.StopSession('')
        except error.TestError as e:
            logging.error(str(e))
            raise error.TestFail('Could not stop session for random user')


    def cleanup(self):
        # Best effort to bounce the UI, which may be up or down.
        cros_ui.stop(allow_fail=True)
        self._cryptohome_proxy.remove(self.username)
        cros_ui.start(allow_fail=True, wait_for_login_prompt=False)
        super(login_RemoteOwnership, self).cleanup()

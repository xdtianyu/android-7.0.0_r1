# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, grp, os, pwd, stat
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import policy, session_manager
from autotest_lib.client.cros import cros_ui, cryptohome, ownership


class login_UserPolicyKeys(test.test):
    """Verifies that, after user policy is pushed, the user policy key winds
       up stored in the right place.
    """
    version = 1

    def _can_read(self, uid, gid, info):
        """Returns true if uid or gid can read a file with the info stat."""
        if uid == info.st_uid:
            return info.st_mode & stat.S_IRUSR
        if gid == info.st_gid:
            return info.st_mode & stat.S_IRGRP
        return info.st_mode & stat.S_IROTH


    def _can_execute(self, uid, gid, info):
        """Returns true if uid or gid can execute a file with the info stat."""
        if uid == info.st_uid:
            return info.st_mode & stat.S_IXUSR
        if gid == info.st_gid:
            return info.st_mode & stat.S_IXGRP
        return info.st_mode & stat.S_IXOTH


    def _verify_key_file(self, key_file):
        """Verifies that the key file has been created and is readable."""
        if not os.path.isfile(key_file):
            raise error.TestFail('%s does not exist!' % key_file)
        # And is readable by chronos.
        chronos_uid = pwd.getpwnam('chronos').pw_uid
        chronos_gid = grp.getgrnam('chronos').gr_gid
        info = os.stat(key_file)
        if not stat.S_ISREG(info.st_mode):
            raise error.TestFail('%s is not a regular file' % key_file)
        if not self._can_read(chronos_uid, chronos_gid, info):
            raise error.TestFail('chronos can\' read %s, mode is %s' %
                                 (key_file, oct(info.st_mode)))
        # All the parent directories must be executable by chronos.
        current = key_file
        parent = os.path.dirname(current)
        while current != parent:
            current = parent
            parent = os.path.dirname(parent)
            info = os.stat(current)
            mode = stat.S_IMODE(info.st_mode)
            if not self._can_execute(chronos_uid, chronos_gid, info):
                raise error.TestFail('chronos can\'t execute %s, mode is %s' %
                                     (current, oct(info.st_mode)))


    def setup(self):
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')


    def initialize(self):
        super(login_UserPolicyKeys, self).initialize()
        self._bus_loop = DBusGMainLoop(set_as_default=True)
        self._cryptohome_proxy = cryptohome.CryptohomeProxy(self._bus_loop)

        # Clear the user's vault, to make sure the test starts without any
        # policy or key lingering around. At this stage the session isn't
        # started and there's no user signed in.
        ownership.restart_ui_to_clear_ownership_files()
        self._cryptohome_proxy.remove(ownership.TESTUSER)


    def run_once(self):
        # Mount the vault, connect to session_manager and start the session.
        self._cryptohome_proxy.mount(ownership.TESTUSER,
                                     ownership.TESTPASS,
                                     create=True)
        sm = session_manager.connect(self._bus_loop)
        if not sm.StartSession(ownership.TESTUSER, ''):
            raise error.TestError('Could not start session')

        # No policy stored yet.
        retrieved_policy = sm.RetrievePolicyForUser(ownership.TESTUSER,
                                                    byte_arrays=True)
        if retrieved_policy:
            raise error.TestError('session_manager already has user policy!')

        # And no user key exists.
        key_file = ownership.get_user_policy_key_filename(ownership.TESTUSER)
        if os.path.exists(key_file):
            raise error.TestFail('%s exists before storing user policy!' %
                                 key_file)

        # Now store a policy. This is building a device policy protobuf, but
        # that's fine as far as the session_manager is concerned; it's the
        # outer PolicyFetchResponse that contains the public_key.
        public_key = ownership.known_pubkey()
        private_key = ownership.known_privkey()
        policy_data = policy.build_policy_data(self.srcdir)
        policy_response = policy.generate_policy(self.srcdir,
                                                 private_key,
                                                 public_key,
                                                 policy_data)
        try:
          result = sm.StorePolicyForUser(ownership.TESTUSER,
                                         dbus.ByteArray(policy_response))
          if not result:
              raise error.TestFail('Failed to store user policy')
        except dbus.exceptions.DBusException, e:
          raise error.TestFail('Failed to store user policy', e)

        # The policy key should have been created now.
        self._verify_key_file(key_file)

        # Restart the ui; the key should be deleted.
        self._cryptohome_proxy.unmount(ownership.TESTUSER)
        cros_ui.restart()
        if os.path.exists(key_file):
            raise error.TestFail('%s exists after restarting ui!' %
                                 key_file)

        # Starting a new session will restore the key that was previously
        # stored. Reconnect to the session_manager, since the restart killed it.
        self._cryptohome_proxy.mount(ownership.TESTUSER,
                                     ownership.TESTPASS,
                                     create=True)
        sm = session_manager.connect(self._bus_loop)
        if not sm.StartSession(ownership.TESTUSER, ''):
            raise error.TestError('Could not start session after restart')
        self._verify_key_file(key_file)


    def cleanup(self):
        cros_ui.restart()
        self._cryptohome_proxy.remove(ownership.TESTUSER)
        super(login_UserPolicyKeys, self).cleanup()

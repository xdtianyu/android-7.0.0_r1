# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import gobject, os, shutil
from dbus.mainloop.glib import DBusGMainLoop

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome, session_manager
from autotest_lib.client.cros import constants, cros_ui, ownership


def set_block_devmode(value):
    try:
        utils.system('crossystem block_devmode=%d' % (1 if value else 0))
    except error.CmdError, e:
        raise error.TestError('Failed to run crossystem: %s' % e)


def get_block_devmode():
    try:
        return utils.system_output('crossystem block_devmode') == '1'
    except error.CmdError, e:
        raise error.TestError('Failed to run crossystem: %s' % e)


class platform_SessionManagerBlockDevmodeSetting(test.test):
    """Verifies that session_manager updates the block_devmode flag to be in
    sync with the corresponding device setting."""
    version = 1

    def initialize(self):
        super(platform_SessionManagerBlockDevmodeSetting, self).initialize()
        ownership.restart_ui_to_clear_ownership_files()
        self._bus_loop = DBusGMainLoop(set_as_default=True)


    def run_once(self):
        try:
            if utils.system_output('crossystem mainfw_type') == 'nonchrome':
                raise error.TestNAError(
                    'State key generation only works on Chrome OS hardware')
        except error.CmdError, e:
            raise error.TestError('Failed to run crossystem: %s' % e)

        # Make sure that the flag sticks when there is no owner.
        set_block_devmode(True)
        cros_ui.restart()
        cros_ui.stop()
        if not get_block_devmode():
            raise error.TestFail("Flag got reset for non-owned device.")

        # Test whether the flag gets reset when taking ownership.
        listener = session_manager.OwnershipSignalListener(gobject.MainLoop())
        listener.listen_for_new_key_and_policy()
        with chrome.Chrome() as cr:
            listener.wait_for_signals(desc='Ownership files written to disk.')
            if get_block_devmode():
                raise error.TestFail(
                    "Flag not clear after ownership got established.")

        # Put a new owner key and policy blob in place, the latter of which
        # specifies block_devmode=true.
        cros_ui.stop(allow_fail=True)
        shutil.copyfile(
            os.path.join(self.bindir, 'owner.key'), constants.OWNER_KEY_FILE)
        shutil.copyfile(
            os.path.join(self.bindir, 'policy_block_devmode_enabled'),
            constants.SIGNED_POLICY_FILE)
        cros_ui.start()
        if not get_block_devmode():
            raise error.TestFail(
                "Flag not set after starting with policy enabled.")

        # Send a new policy blob to session_manager that disables block_devmode.
        listener.listen_for_new_policy()
        with open(os.path.join(self.bindir,
                  'policy_block_devmode_disabled')) as f:
            session_manager_proxy = session_manager.connect(self._bus_loop)
            session_manager_proxy.StorePolicy(f.read())
        listener.wait_for_signals(desc='Policy updated.')

        if get_block_devmode():
            raise error.TestFail(
                "Flag set after updating policy to clear flag.")

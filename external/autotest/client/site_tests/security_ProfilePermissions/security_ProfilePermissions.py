# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import pwd
import stat

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants, cryptohome


class security_ProfilePermissions(test.test):
    """Check permissions of files of logged in and guest user."""
    version = 1
    _HOMEDIR_MODE = 0710

    def initialize(self, logged_in):
        self._logged_in = logged_in

    def check_owner_mode(self, path, expected_owner, expected_mode):
        """
        Checks if the file/directory at 'path' is owned by 'expected_owner'
        with permissions matching 'expected_mode'.
        Returns True if they match, else False.
        Logs any mismatches to logging.error.

        @param path: file path to test.
        @param expected_owner: expected owner of the file.
        @param expected_mode: expected permission mode of the file.

        """
        s = os.stat(path)
        actual_owner = pwd.getpwuid(s.st_uid).pw_name
        actual_mode = stat.S_IMODE(s.st_mode)
        if (expected_owner != actual_owner or
            expected_mode != actual_mode):
            logging.error("%s - Expected %s:%s, saw %s:%s",
                         path, expected_owner, oct(expected_mode),
                         actual_owner, oct(actual_mode))
            return False
        else:
            return True


    def run_once(self):
        with chrome.Chrome(logged_in=self._logged_in) as cr:
            username = (cr.username if self._logged_in
                                    else cryptohome.GUEST_USER_NAME)

            """Check permissions within cryptohome for anything too permissive.
            """
            passes = []

            homepath = "/home/chronos"
            passes.append(self.check_owner_mode(homepath, "chronos", 0755))

            user_mountpt = cryptohome.user_path(username)
            passes.append(self.check_owner_mode(user_mountpt, "chronos",
                                                self._HOMEDIR_MODE))

            # TODO(benchan): Refactor the following code to use some helper
            # functions instead of find commands.

            # An array of shell commands, each representing a test that
            # passes if it emits no output. The first test is the main one.
            # In general, writable by anyone else is bad, as is owned by
            # anyone else. Any exceptions to that are pruned out of the
            # first test and checked individually by subsequent tests.
            cmds = [
                ('find -L "%s" -path "%s" -o '
                 # Avoid false-positives on SingletonLock, SingletonCookie, etc.
                 ' \\( -name "Singleton*" -a -type l \\) -o '
                 ' -path "%s/user" -prune -o '
                 ' -path "%s/Downloads" -prune -o '
                 ' -path "%s/flimflam" -prune -o '
                 ' -path "%s/shill" -prune -o '
                 ' -path "%s/.chaps" -prune -o '
                 ' -path "%s/u-*" -prune -o '
                 ' -path "%s/crash" -prune -o '
                 ' \\( -perm /022 -o \\! -user chronos \\) -ls') %
                (homepath, homepath, homepath, user_mountpt, user_mountpt,
                user_mountpt, user_mountpt, homepath, homepath),
                # /home/chronos/user and /home/chronos/user/Downloads are owned
                # by the chronos-access group and with a group execute
                # permission.
                'find -L "%s" -maxdepth 0 \\( \\! -perm 710 '
                '-o \\! -user chronos -o \\! -group chronos-access \\) -ls' %
                user_mountpt,
                'find -L "%s/Downloads" -maxdepth 0 \\( \\! -perm 710 '
                '-o \\! -user chronos -o \\! -group chronos-access \\) -ls' %
                user_mountpt,
                'find -L "%s/flimflam" \\( -perm /077 -o \\! -user root \\) -ls'
                % user_mountpt,
                'find -L "%s/shill" \\( -perm /077 -o \\! -user root \\) -ls' %
                user_mountpt,
                'find -L "%s/.chaps -name auth_data_salt -prune -o '
                '\\! -user chaps -o \\! -group chronos-access -o -perm /027 -ls'
                % user_mountpt,
                'find -L "%s/.chaps -name auth_data_salt -a '
                '\\( \\! -user root -o -perm /077 \\) -ls' % user_mountpt,
            ]

            for cmd in cmds:
                cmd_output = utils.system_output(cmd, ignore_status=True)
                if cmd_output:
                    passes.append(False)
                    logging.error(cmd_output)

            # This next section only applies if we have a real vault mounted
            # (ie, not a BWSI tmpfs).
            if cryptohome.is_vault_mounted(
                    username,
                    device_regex=constants.CRYPTOHOME_DEV_REGEX_REGULAR_USER):
                # Also check the permissions of the underlying vault and
                # supporting directory structure.
                vaultpath = cryptohome.get_mounted_vault_devices(username)[0]

                passes.append(self.check_owner_mode(vaultpath, "root", 0700))
                passes.append(self.check_owner_mode(vaultpath + "/../master.0",
                                                    "root", 0600))
                passes.append(self.check_owner_mode(vaultpath + "/../",
                                                    "root", 0700))
                passes.append(self.check_owner_mode(vaultpath + "/../../",
                                                    "root", 0700))

            if False in passes:
                raise error.TestFail(
                    'Bad permissions found on cryptohome files')

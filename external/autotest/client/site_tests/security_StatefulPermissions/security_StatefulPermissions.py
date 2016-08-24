# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pwd
import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_StatefulPermissions(test.test):
    """
    Report all unexpected writable paths in the /mnt/stateful_partition
    tree.
    """
    version = 1
    _STATEFUL_ROOT = "/mnt/stateful_partition"

    # Note that chronos permissions in /home are covered in greater detail
    # by 'security_ProfilePermissions'.
    _masks_byuser = {"adm": [],
                     "attestation": ["/unencrypted/preserve/attestation.epb"],
                     "avfs": [],
                     "bin": [],
                     "bluetooth": ["/encrypted/var/lib/bluetooth"],
                     "chaps": ["/encrypted/var/lib/chaps"],
                     "chronos": ["/encrypted/chronos",
                                 "/encrypted/var/cache/app_pack",
                                 "/encrypted/var/cache/device_local_account_component_policy",
                                 "/encrypted/var/cache/device_local_account_extensions",
                                 "/encrypted/var/cache/device_local_account_external_policy_data",
                                 "/encrypted/var/cache/echo",
                                 "/encrypted/var/cache/external_cache",
                                 "/encrypted/var/cache/shared_extensions",
                                 "/encrypted/var/cache/touch_trial/selection",
                                 "/encrypted/var/lib/cromo",
                                 "/encrypted/var/lib/opencryptoki",
                                 "/encrypted/var/lib/timezone",
                                 "/encrypted/var/lib/Synaptics/chronos.1000",
                                 "/encrypted/var/log/chrome",
                                 "/encrypted/var/log/connectivity.bak",
                                 "/encrypted/var/log/connectivity.log",
                                 "/encrypted/var/log/metrics",
                                 "/encrypted/var/minidumps",
                                 "/home/user"],
                     "chronos-access": [],
                     "cras": [],
                     "cros-disks": [],
                     "daemon": [],
                     "debugd": [],
                     "dhcp": ["/encrypted/var/lib/dhcpcd"],
                     "input": [],
                     "ipsec": [],
                     "lp": [],
                     "messagebus": [],
                     "mtp": [],
                     "news": [],
                     "nobody": [],
                     "ntfs-3g": [],
                     "openvpn": [],
                     "portage": ["/encrypted/var/log/emerge.log"],
                     "power": ["/encrypted/var/lib/power_manager",
                               "/encrypted/var/log/power_manager"],
                     "pkcs11": [],
                     "root": None,
                     "sshd": [],
                     "syslog": ["/encrypted/var/log"],
                     "tcpdump": [],
                     "tlsdate": [],
                     "tss": ["/var/lib/tpm"],
                     "uucp": [],
                     "wpa": [],
                     "xorg": ["/encrypted/var/lib/xkb",
                              "/encrypted/var/log/xorg",
                              "/encrypted/var/log/Xorg.0.log"]
                    }


    def systemwide_exclusions(self):
        """Returns a list of paths that are only present on test images
        and therefore should be excluded from all 'find' commands.
        """
        paths = []

        # 'preserve/log' is test-only.
        paths.append("/unencrypted/preserve/log")

        # Cover up Portage noise.
        paths.append("/encrypted/var/cache/edb")
        paths.append("/encrypted/var/lib/gentoo")
        paths.append("/encrypted/var/log/portage")

        # Cover up Autotest noise.
        paths.append("/dev_image")
        paths.append("/var_overlay")

        return paths


    def generate_prune_arguments(self, prunelist):
        """Returns a command-line fragment to make 'find' exclude the entries
        in |prunelist|.

        @param prunelist: list of paths to ignore
        """
        fragment = "-path STATEFUL_ROOT%s -prune -o"
        fragments = [fragment % path for path in prunelist]
        return " ".join(fragments)


    def generate_find(self, user, prunelist):
        """
        Generates the "find" command that spits out all files in stateful
        writable by a given user, with the given list of directories removed.

        @param user: report writable paths owned by this user
        @param prunelist: list of paths to ignore
        """
        if prunelist is None:
            return "true" # return a no-op shell command, e.g. for root.

        # Exclude world-writeable stuff.
        # '/var/lib/metrics/uma-events' is world-writeable: crbug.com/198054.
        prunelist.append("/encrypted/var/lib/metrics/uma-events")
        # '/run/lock' is world-writeable.
        prunelist.append("/encrypted/var/lock")
        # '/var/log/asan' should be world-writeable: crbug.com/453579
        prunelist.append("/encrypted/var/log/asan")

        # Add system-wide exclusions.
        prunelist.extend(self.systemwide_exclusions())

        cmd = "find STATEFUL_ROOT "
        cmd += self.generate_prune_arguments(prunelist)
        # Note that we don't "prune" all of /var/tmp's contents, just mask
        # the dir itself. Any contents are still interesting.
        cmd += " -path STATEFUL_ROOT/encrypted/var/tmp -o "
        cmd += " -writable -ls -o -user %s -ls 2>/dev/null" % user
        return cmd


    def expected_owners(self):
        """Returns the set of file/directory owners expected in stateful."""
        # In other words, this is basically the users mentioned in
        # tests_byuser, except for any expected to actually own zero files.
        # Currently, there's no exclusions.
        return set(self._masks_byuser.keys())


    def observed_owners(self):
        """Returns the set of file/directory owners present in stateful."""

        cmd = "find STATEFUL_ROOT "
        cmd += self.generate_prune_arguments(self.systemwide_exclusions())
        cmd += " -printf '%u\\n' | sort -u"
        return set(self.subst_run(cmd).splitlines())


    def owners_lacking_coverage(self):
        """
        Determines the set of owners not covered by any of the
        per-owner tests implemented in this class. Returns
        a set of usernames (possibly the empty set).
        """
        return self.observed_owners().difference(self.expected_owners())


    def log_owned_files(self, owner):
        """
        Sends information about all files in the stateful partition
        owned by a given owner to the standard logging facility.

        @param owner: paths owned by this user will be reported
        """
        cmd = "find STATEFUL_ROOT -user %s -ls" % owner
        cmd_output = self.subst_run(cmd)
        logging.error(cmd_output)


    def subst_run(self, cmd, stateful_root=_STATEFUL_ROOT):
        """
        Replace "STATEFUL_ROOT" with the actual stateful partition path.

        @param cmd: string containing the command to examine
        @param stateful_root: path used to replace "STATEFUL_ROOT"
        """
        cmd = cmd.replace("STATEFUL_ROOT", stateful_root)
        return utils.system_output(cmd, ignore_status=True)


    def run_once(self):
        """
        Accounts for the contents of the stateful partition
        piece-wise, inspecting the level of access which can
        be obtained by each of the privilege levels (usernames)
        used in CrOS.

        The test passes if each of the owner-specific sub-tests pass,
        and if there are no files unaccounted for (i.e., no unexpected
        file-owners for which we have no tests.)
        """
        testfail = False

        unexpected_owners = self.owners_lacking_coverage()
        if unexpected_owners:
            testfail = True
            for o in unexpected_owners:
                self.log_owned_files(o)

        # Now run the sub-tests.
        for user, mask in self._masks_byuser.items():
            cmd = self.generate_find(user, mask)

            try:
                pwd.getpwnam(user)
            except KeyError, err:
                logging.warning('Skipping missing user: %s', err)
                continue

            # The 'EOF' below helps us distinguish 2 types of failures.
            # We have to use ignore_status=True because many of these
            # find-based tests will exit(nonzero) to signal that they
            # encountered inaccessible directories, which we expect by-design.
            # This creates ambiguity as to whether empty output means
            # the test ran, and passed, or the su failed. Including an
            # expected 'EOF' output disambiguates these cases.
            cmd = "su -s /bin/sh -c '%s;echo EOF' %s" % (cmd, user)
            cmd_output = self.subst_run(cmd)

            if not cmd_output:
                # we never got 'EOF', su failed
                testfail = True
                logging.error("su failed while attempting to run:")
                logging.error(cmd)
                logging.error("[Got %s]", cmd_output)
            elif not re.search("^\s*EOF\s*$", cmd_output):
                # we got test failures before 'EOF'
                testfail = True
                logging.error("Test for '%s' found unexpected files:\n%s",
                              user, cmd_output)

        if testfail:
            raise error.TestFail("Unexpected files/perms in stateful")

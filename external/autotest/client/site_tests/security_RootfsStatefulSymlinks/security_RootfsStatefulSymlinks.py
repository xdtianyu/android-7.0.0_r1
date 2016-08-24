# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import grp
import json
import logging
import pwd
import os
import stat

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class security_RootfsStatefulSymlinks(test.test):
    version = 1
    _BAD_DESTINATIONS = [
                         '*/var/*', '*/home/*', '*/stateful_partition/*',
                         '*/usr/local/*'
                        ]

    def load_baseline(self):
        bfile = open(os.path.join(self.bindir, 'baseline'))
        baseline = json.loads(bfile.read())
        bfile.close()
        return baseline


    def validate_attributes(self, link, expectations):
        """
        Given a symlink, validate that the file it points to
        matches all of the expected properties (owner, group, mode).
        Returns True if all expections are met, False otherwise.
        """
        destination = os.readlink(link)
        if destination != expectations['destination']:
            logging.error(
                "Expected '%s' to point to '%s', but it points to '%s'",
                link, expectations['destination'], destination)
            logging.error(utils.system_output("ls -ald '%s'" % destination))
            return False

        # By this point, we know it points to the right place, but we
        # need to determine if the destination exists (and, if not, if
        # that's permitted by "can_dangle": true in the baseline.
        if not os.path.exists(destination):
            logging.warning("'%s' points to '%s', but it's dangling",
                            link, destination)
            return expectations['can_dangle']

        # It exists, it's the right path, so check the permissions.
        s = os.stat(destination)
        owner = pwd.getpwuid(s.st_uid).pw_name
        group = grp.getgrgid(s.st_gid).gr_name
        mode = oct(stat.S_IMODE(s.st_mode))
        if (owner == expectations['owner'] and
            group == expectations['group'] and
            mode == expectations['mode']):
            return True
        else:
            logging.error("'%s': expected %s:%s %s, saw %s:%s %s",
                          destination, expectations['owner'],
                          expectations['group'], expectations['mode'],
                          owner, group, mode)
            return False


    def run_once(self):
        """
        Find any symlinks that point from the rootfs into
        "bad destinations" (e.g., stateful partition). Validate
        that any approved cases meet with all expectations, and
        that there are no unexpected additional such links found.
        """
        baseline = self.load_baseline()
        test_pass = True

        clauses = ["-lname '%s'" % i for i in self._BAD_DESTINATIONS]
        cmd = 'find / -xdev %s' % ' -o '.join(clauses)
        cmd_output = utils.system_output(cmd, ignore_status=True)

        links_seen = set(cmd_output.splitlines())
        for link in links_seen:
            # Check if this link is in the baseline. If not, test fails.
            if not link in baseline:
                logging.error("No baseline entry for '%s'", link)
                logging.error(utils.system_output("ls -ald '%s'" % link))
                test_pass = False
                continue
            # If it is, proceed to validate other attributes (where it points,
            # permissions of what it points to, etc).
            file_pass = self.validate_attributes(link, baseline[link])
            test_pass = test_pass and file_pass

        # The above will have flagged any links for which we had no baseline.
        # Warn (but do not trigger failure) when we have baseline entries
        # which we did not find on the system.
        expected_set = set(baseline.keys())
        diff = expected_set.difference(links_seen)
        if diff:
            logging.warning("Warning, possible stale baseline entries:")
            for d in diff:
                logging.warning(d)

        if not test_pass:
            raise error.TestFail("Baseline mismatch")


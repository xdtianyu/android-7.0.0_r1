# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
import os.path

class security_ReservedPrivileges(test.test):
    version = 1

    def reserved_commands(self, command_list):
        process_list = []
        for line in command_list:
            items = line.split()
            # We don't care about defunct processes for purposes of this test,
            # so skip to the next process if we encounter one.
            if '<defunct>' in items:
                continue

            # There are n items in the list.  The first is the command, all of
            # the remaining are either the different users or groups.  They
            # must all match, if they don't we collect it.
            matches = [i for i,owners in enumerate(items) if owners == items[1]]
            # We do < because some processes have the same name as their owner.
            # in that case the number of items will equal the number of matches
            if (len(matches) < (len(items) - 1)):
                process_list.append(items[0])
        return set(process_list)


    def load_baseline(self, bltype):
        # Figure out path to baseline file, by looking up our own path
        path = os.path.abspath(__file__)
        path = os.path.join(os.path.dirname(path), 'baseline.%s' % bltype)
        if (os.path.isfile(path) == False):
            return set([])
        baseline_file = open(path)
        baseline_data = baseline_file.read()
        baseline_set = set(baseline_data.splitlines())
        baseline_file.close()
        return baseline_set


    def run_once(self, owner_type='user'):
        """
        Do a find on the system for commands with reserved privileges and
        compare against baseline.  Fail if these do not match.
        """

        # Find the max column width needed to represent user and group names
        # in ps outoupt.
        usermax = utils.system_output("cut -d: -f1 /etc/passwd | wc -L",
                                      ignore_status=True)
        usermax = max(int(usermax), 8)

        groupmax = utils.system_output("cut -d: -f1 /etc/group | wc -L",
                                       ignore_status=True)
        groupmax = max(int(groupmax), 8)

        if (owner_type == 'user'):
            command = ('ps --no-headers -eo '\
                       'comm:16,euser:%d,ruser:%d,suser:%d,fuser:%d' %
                       (usermax, usermax, usermax, usermax))
        else:
            command = ('ps --no-headers -eo comm:16,rgroup:%d,group:%d' %
                       (groupmax, groupmax))

        command_output = utils.system_output(command, ignore_status=True)
        output_lines = command_output.splitlines()

        dump_file = open(os.path.join(self.resultsdir, "ps_output"), 'w')
        for line in output_lines:
            dump_file.write(line.strip() + "\n")

        dump_file.close()

        observed_set = self.reserved_commands(output_lines)
        baseline_set = self.load_baseline(owner_type)

        # If something in the observed set is not
        # covered by the baseline...
        diff = observed_set.difference(baseline_set)
        if len(diff) > 0:
            for command in diff:
                logging.error('Unexpected command: %s' % command)

        # Or, things in baseline are missing from the system:
        diff2 = baseline_set.difference(observed_set)
        if len(diff2) > 0:
            for command in diff2:
                logging.error('Missing command: %s' % command)

        if (len(diff) + len(diff2)) > 0:
            raise error.TestFail('Baseline mismatch')

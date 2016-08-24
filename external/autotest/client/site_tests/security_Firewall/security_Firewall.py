# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros.tendo import webservd_helper


class security_Firewall(test.test):
    """Tests that rules in iptables/ip6tables match our expectations exactly."""
    version = 1


    @staticmethod
    def get_firewall_settings(executable):
        rules = utils.system_output("%s -S" % executable)
        return set([line.strip() for line in rules.splitlines()])


    def load_baseline(self, baseline_filename):
        """The baseline file lists the rules that we expect.

        @param baseline_filename: string name of file containing relevant rules.
        """
        baseline_path = os.path.join(self.bindir, baseline_filename)
        with open(baseline_path) as f:
            return set([line.strip() for line in f.readlines()])


    def dump_rules(self, rules, executable):
        """Store actual rules in results/ for future use.

        Leaves a list of iptables/ip6tables rules in the results dir
        so that we can update the baseline file if necessary.

        @param rules: list of string containing rules we found on the board.
        @param executable: 'iptables' for IPv4 or 'ip6tables' for IPv6.
        """
        outf = open(os.path.join(self.resultsdir, "%s_rules" % executable), 'w')
        for rule in rules:
            outf.write(rule + "\n")

        outf.close()


    @staticmethod
    def log_error_rules(rules, message):
        """Log a set of rules and the problem with those rules.

        @param rules: list of string containing rules we have issues with.
        @param message: string detailing what our problem with the rules is.
        """
        rules_str = ", ".join(["'%s'" % rule for rule in rules])
        logging.error("%s: %s", message, rules_str)


    def run_once(self):
        """Matches found and expected iptables/ip6tables rules.
        Fails only when rules are missing.
        """

        failed = False
        for executable in ["iptables", "ip6tables"]:
            baseline = self.load_baseline("baseline.%s" % executable)
            # TODO(wiley) Remove when we get per-board baselines (crbug.com/406013)
            webserv_rules = self.load_baseline("baseline.webservd")
            if webservd_helper.webservd_is_running():
                baseline.update(webserv_rules)
            current = self.get_firewall_settings(executable)

            # Save to results dir
            self.dump_rules(current, executable)

            missing_rules = baseline - current
            extra_rules = current - baseline

            if len(missing_rules) > 0:
                failed = True
                self.log_error_rules(missing_rules,
                                     "Missing %s rules" % executable)

            if len(extra_rules) > 0:
                # TODO(zqiu): implement a way to verify per-interface rules
                # that are created dynamically.
                self.log_error_rules(extra_rules, "Extra %s rules" % executable)

        if failed:
            raise error.TestFail("Mismatched firewall rules")

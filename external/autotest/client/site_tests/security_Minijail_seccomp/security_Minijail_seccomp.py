# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from collections import namedtuple

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

Jail = namedtuple("Jail", "user policy nnp")

class security_Minijail_seccomp(test.test):
    version = 1


    def setup(self):
        os.chdir(self.srcdir)
        utils.make('clean')
        utils.make()


    def run_test(self, exe, args, jail, expected_ret, pretty_msg):
        cmdline = '/sbin/minijail0'

        if jail.user:
            cmdline += ' -u %s' % jail.user

        if jail.nnp:
            cmdline += ' -n'

        cmdline += ' -S %s/%s %s/%s' % (self.bindir, jail.policy,
                                        self.bindir, exe)

        if len(args) > 0:
            cmdline += ' %s' % ' '.join(args)

        logging.info("Command line: " + cmdline)
        ret = utils.system(cmdline, ignore_status=True)

        if ret != expected_ret:
            logging.error("ret: %d, expected: %d" % (ret, expected_ret))
            raise error.TestFail(pretty_msg)


    def run_once(self):
        privdrop_policy = "policy-privdrop_" + utils.get_arch_userspace()

        case_ok = ("ok", [],
                   Jail(None, "policy", nnp=False),
                   0, "Allowed system calls failed")
        case_block_privdrop = ("ok", [],
                               Jail("chronos", "policy", nnp=False),
                               253, "Blocked priv-drop system calls succeeded")
        case_allow_privdrop = ("ok", [],
                               Jail("chronos", privdrop_policy, nnp=False),
                               0, "Allowed system calls failed")
        case_no_new_privs = ("ok", [],
                             Jail("chronos", "policy", nnp=True),
                             0, "Allowed system calls failed")
        case_fail = ("fail", [],
                     Jail(None, "policy", nnp=False),
                     253, "Blocked system calls succeeded")

        case_arg_equals_ok = ("open", ["0"],
                              Jail(None, "policy-rdonly", nnp=False),
                              0, "Allowing system calls via args == failed")
        case_arg_equals_fail = ("open", ["1"],
                                Jail(None, "policy-rdonly", nnp=False),
                                253, "Blocking system calls via args == failed")
        case_arg_flags_ok = ("open", ["1"],
                             Jail(None, "policy-wronly", nnp=False),
                             0, "Allowing system calls via args & failed")
        case_arg_flags_ok = ("open", ["2"],
                             Jail(None, "policy-wronly", nnp=False),
                             253, "Blocking system calls via args & failed")

        for case in [case_ok, case_block_privdrop, case_allow_privdrop,
                     case_no_new_privs, case_fail,
                     case_arg_equals_ok, case_arg_equals_fail,
                     case_arg_flags_ok]:
            self.run_test(*case)

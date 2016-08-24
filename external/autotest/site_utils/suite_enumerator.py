#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool for enumerating the tests in a given suite.

Given an autotest root directory and a suite name (e.g., bvt, regression), this
tool will print out the name of each test in that suite, one per line.

Example:
$ ./site_utils/suite_enumerator.py -a /usr/local/autotest bvt 2>/dev/null
login_LoginSuccess
logging_CrashSender
login_BadAuthentication

This is intended for use only with Chrome OS test suits that leverage the
dynamic suite infrastructure in server/cros/dynamic_suite.py.
"""

import logging
import optparse, os, sys

# Silence messages relating to imports of missing, unneeded
# modules.
logging.basicConfig(level=logging.INFO)

import common
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros.dynamic_suite.suite import Suite

def parse_options():
    """Parse command line for arguments including autotest directory, suite
    name, if to list stable tests only, and if to list all available suites.
    """
    usage = "usage: %prog [options] suite_name"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-a', '--autotest_dir', dest='autotest_dir',
                      default=os.path.abspath(
                          os.path.join(os.path.dirname(__file__),
                                       os.pardir)),
                      help='Directory under which to search for tests.'\
                           ' (e.g. /usr/local/autotest)')
    parser.add_option('-l', '--listall',
                      action='store_true', default=False,
                      help='Print a listing of all suites. Ignores all args.')
    options, args = parser.parse_args()
    return parser, options, args


def main():
    """Entry point to run the suite enumerator command."""
    parser, options, args = parse_options()
    if options.listall:
        if args:
            print 'Cannot use suite_name with --listall'
            parser.print_help()
    elif not args or len(args) != 1:
        parser.print_help()
        return

    fs_getter = Suite.create_fs_getter(options.autotest_dir)
    devserver = dev_server.ImageServer('')
    if options.listall:
        for suite in Suite.list_all_suites('', devserver, fs_getter):
            print suite
        return

    suite = Suite.create_from_name(args[0], {}, '', devserver, fs_getter)
    # If in test list, print firmware_FAFTSetup before other tests
    # NOTE: the test.name value can be *different* from the directory
    # name that appears in test.path
    PRETEST_LIST = ['firmware_FAFTSetup',]
    for test in filter(lambda test: test.name in \
                              PRETEST_LIST, suite.stable_tests()):
        print test.path
    for test in filter(lambda test: test.name not in \
                       PRETEST_LIST, suite.stable_tests()):
        print test.path

    # Check if test_suites/control.suite_name exists.
    control_path = os.path.join(options.autotest_dir, 'test_suites',
                                'control.' + args[0])
    if not os.path.exists(control_path):
        print >> sys.stderr, ('Warning! control file is missing: %s' %
                              control_path)


if __name__ == "__main__":
    sys.exit(main())

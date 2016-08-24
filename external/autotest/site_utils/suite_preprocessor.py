#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Deprecated tool for preprocessing tests to determine their DEPENDENCIES.
"""

import optparse, os, sys
import common


def parse_options():
    """Parse command line arguments."""
    parser = optparse.OptionParser()
    parser.add_option('-a', '--autotest_dir', dest='autotest_dir',
                      default=os.path.abspath(
                          os.path.join(os.path.dirname(__file__), '..')),
                      help="Directory under which to search for tests."\
                           " (e.g. /usr/local/autotest).  Defaults to '..'")
    parser.add_option('-o', '--output_file', dest='output_file',
                      default=None,
                      help='File into which to write collected test info.'\
                           '  Defaults to stdout.')
    options, _ = parser.parse_args()
    return options


def main():
    """Main function."""
    options = parse_options()

    test_deps = {}

    if options.output_file:
        with open(options.output_file, 'w') as file_obj:
            file_obj.write('%r' % test_deps)
    else:
        print '%r' % test_deps


if __name__ == "__main__":
    sys.exit(main())

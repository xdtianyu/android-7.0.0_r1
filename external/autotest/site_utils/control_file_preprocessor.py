#!/usr/bin/python
#
# Copyright (c) 20123 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool for preprocessing control files to build a suite to control files map.

Given an autotest root directory, this tool will bucket tests accroding to
their suite.Data will be written to stdout (or, optionally a file), eg:

{'suite1': ['path/to/test1/control', 'path/to/test2/control'],
 'suite2': ['path/to/test4/control', 'path/to/test5/control']}

This is intended for use only with Chrome OS test suites that leverage the
dynamic suite infrastructure in server/cros/dynamic_suite.py. It is invoked
at build time to generate said suite to control files map, which dynamic_suite
consults at run time to determine which tests belong to a suite.
"""


import collections, json, os, sys

import common
from autotest_lib.server.cros.dynamic_suite import suite
from autotest_lib.site_utils import suite_preprocessor


# A set of SUITES that we choose not to preprocess as they might have tests
# added later.
SUITE_BLACKLIST = set(['au'])


def _get_control_files_to_process(autotest_dir):
    """Find all control files in autotest_dir that have 'SUITE='

    @param autotest_dir: The directory to search for control files.
    @return: All control files in autotest_dir that have a suite attribute.
    """
    fs_getter = suite.Suite.create_fs_getter(autotest_dir)
    predicate = lambda t: hasattr(t, 'suite')
    return suite.Suite.find_and_parse_tests(fs_getter, predicate,
                                            add_experimental=True)


def get_suite_control_files(autotest_dir):
    """
    Partition all control files in autotest_dir based on suite.

    @param autotest_dir: Directory to walk looking for control files.
    @return suite_control_files: A dictionary mapping suite->[control files]
                                 as described in this files docstring.
    @raise ValueError: If autotest_dir doesn't exist.
    """
    if not os.path.exists(autotest_dir):
      raise ValueError('Could not find directory: %s, failed to map suites to'
                       ' their control files.' % autotest_dir)

    autotest_dir = autotest_dir.rstrip('/')
    suite_control_files = collections.defaultdict(list)

    for test in _get_control_files_to_process(autotest_dir):
        for suite_name in suite.Suite.parse_tag(test.suite):
            if suite_name in SUITE_BLACKLIST:
                continue

            suite_control_files[suite_name].append(
                test.path.replace('%s/' % autotest_dir, ''))
    return suite_control_files


def main():
    """
    Main function.
    """
    options = suite_preprocessor.parse_options()

    suite_control_files = get_suite_control_files(options.autotest_dir)
    if options.output_file:
        with open(options.output_file, 'w') as file_obj:
            json.dump(suite_control_files, file_obj)
    else:
        print suite_control_files


if __name__ == '__main__':
    sys.exit(main())

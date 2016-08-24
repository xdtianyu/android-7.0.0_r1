#!/usr/bin/python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Utility code adapted from test_importer.py for test doc generation.

These routines are modified versions of those in test_importer.py. If the
docgen code is ever merged into Autotest, this code should be factored out
of test_importer.py and combined with this.
"""

import fnmatch
import os

import common
from autotest_lib.client.common_lib import control_data


def GetTestsFromFS(parent_dir, logger):
    """
    Find control files in file system and load a list with their info.

    @param parent_dir: directory to search recursively.
    @param logger: Python logger for logging.

    @return dictionary of the form: tests[file_path] = parsed_object
    """
    tests = {}
    tests_src = {}
    for root, dirnames, filenames in os.walk(parent_dir):
        for filename in fnmatch.filter(filenames, 'control*'):
            test_name = os.path.basename(root)
            if test_name[:5].lower() == 'suite' or '.svn' in filename:
                continue
            full_name = os.path.join(root, filename)
            try:
                found_test = control_data.parse_control(full_name,
                                                        raise_warnings=True)
                tests[test_name] = ''
                tests_src[test_name] = parent_dir
            except control_data.ControlVariableException, e:
                logger.warn("Skipping %s\n%s", full_name, e)
            except Exception, e:
                logger.error("Bad %s\n%s", full_name, e)
    return tests, tests_src



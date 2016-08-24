# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import os

import common


def CheckControlFileExistance(tasks):
    """
    Make sure that for any task that schedules a suite, that
    test_suites/control.<suite> exists. this prevents people from accidentally
    adding a suite to suite_scheduler.ini but not adding an actual suite
    control file, thus resulting in their suite not running and the lab team
    getting lots of email

    @param tasks The list of tasks to check.
    @return 0 if no missing control files are found
            1 if there are at least one missing control files
    """
    corrections = False

    for task in tasks:
        suite_path = os.path.join(common.autotest_dir,
                                  'test_suites', 'control.'+task.suite)
        if not os.path.exists(suite_path):
            corrections = True
            logging.warning("No suite control file for %s", task.suite)

    return 1 if corrections else 0

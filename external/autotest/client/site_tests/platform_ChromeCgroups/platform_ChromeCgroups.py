# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome

CGROUP_DIR = '/sys/fs/cgroup/cpu/chrome_renderers'
FG_CGROUP_DIR = os.path.join(CGROUP_DIR, 'foreground')
BG_CGROUP_DIR = os.path.join(CGROUP_DIR, 'background')

class platform_ChromeCgroups(test.test):
    version = 1

    def _get_cgroup_tasks(self, cgroup_dir):
        """
        Returns the set of tasks in a cgroup.

        @param cgroup_dir Directory containing the cgroup.
        """
        task_path = os.path.join(cgroup_dir, 'tasks')
        task_file = open(task_path)
        if not task_file:
            raise error.TestError('failed to open %s' % task_path)
        tasks = set(line.rstrip() for line in task_file.readlines())
        task_file.close()
        logging.info('tasks in cgroup %s: %s', cgroup_dir, ','.join(tasks))
        return tasks

    def run_once(self):
        """
        Check that the chrome_renderers cgroups are created and that tasks
        are placed in them.
        """
        with chrome.Chrome() as cr:
            # Make sure the cgroup directories actually exist.
            if not os.path.isdir(CGROUP_DIR):
                raise error.TestFail('chrome_renderers cgroup does not exist')
            if not os.path.isdir(FG_CGROUP_DIR):
                raise error.TestFail('foreground cgroup does not exist')
            if not os.path.isdir(BG_CGROUP_DIR):
                raise error.TestFail('background cgroup does not exist')

            # Open up two tabs in the same window. One should be in the foreground
            # while the other is in the background.
            tab1 = cr.browser.tabs[0]
            tab1.Navigate('about:blank')
            tab1.WaitForDocumentReadyStateToBeComplete()
            tab2 = cr.browser.tabs.New()
            tab2.Navigate('chrome:system')
            tab2.WaitForDocumentReadyStateToBeComplete()

            # Make sure the foreground and background cgroups are non-empty.
            if not self._get_cgroup_tasks(FG_CGROUP_DIR):
                raise error.TestFail('no tasks in foreground cgroup')
            if not self._get_cgroup_tasks(BG_CGROUP_DIR):
                raise error.TestFail('no tasks in background cgroup')

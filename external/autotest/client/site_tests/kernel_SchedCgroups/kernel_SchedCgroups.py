#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, tempfile

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error


class kernel_SchedCgroups(test.test):
    """
    Verify scheduler cgroups
    """
    version = 1


    def cleanup(self):
        utils.system('umount %s' % self._tmpdir)
        utils.system('rm -rf %s' % self._tmpdir)


    def run_once(self):
        self._tmpdir = tempfile.mkdtemp()
        utils.system('mount -t cgroup cgroup %s -o cpu' % self._tmpdir)
        utils.system('mkdir -p -m 0777 %s/test' % self._tmpdir)
        self.assert_(os.path.isfile('%s/test/tasks' % self._tmpdir))

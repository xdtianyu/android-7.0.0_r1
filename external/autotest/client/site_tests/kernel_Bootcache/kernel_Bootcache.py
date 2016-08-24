#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, optparse, os, shutil, re, string
from autotest_lib.client.bin import utils, test

class kernel_Bootcache(test.test):
    """Run the boot cache test
    """
    version = 1
    Bin = '/usr/local/opt/punybench/bin/'


    def initialize(self):
        self.results = []
        self.job.drop_caches_between_iterations = True


    def _run(self, cmd, args):
        """Run a puny test

        Prepends the path to the puny benchmark bin.

        Args:
          cmd: command to be run
          args: arguments for the command
        """
        result = utils.system_output(
            os.path.join(self.Bin, cmd) + ' ' + args)
        logging.debug(result)
        return result


    def run_once(self, args=[]):
        """Run the boot cache test.
        """
        self._run('bootcachetest', "")

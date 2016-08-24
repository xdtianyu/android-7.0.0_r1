#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import tempfile

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class kernel_PerfEventRename(test.test):
    """
    Test perf's ability to deal with processes which have changed their name
    after perf record starts.  Older versions of perf lost their data about
    which executables or libraries were mapped so all the samples would end
    up being called unknown and this would make the profiles useless.

    Good output:
        97.43%        149  foobar_name  perf-rename-test   [.] 0x000006f8
    Bad output:
        96.54%        140  foobar_name  [unknown]          [.] 0x777046f3
    """
    version = 1
    executable = 'perf-rename-test'

    # This runs during the build process
    def setup(self):
        os.chdir(self.srcdir)
        utils.make(self.executable)

    def run_once(self):
        # the rename program runs a crc loop for a while to ensure that we get
        # a good number of samples
        loops = 10 * 1000 * 1000
        rename_name = 'foobar_name'
        (data_tmpfile, data_name) = tempfile.mkstemp(
            prefix='perfEventRename_data.', dir='/tmp')

        utils.system('perf record -o %s %s %s %s' %
                     (data_name, os.path.join(self.srcdir,
                                              self.executable),
                      rename_name, loops),
                     timeout=60)
        report = utils.system_output('perf report -n -i %s' % (data_name))
        logging.debug('report output: %s' % report)
        os.unlink(data_name)

        for line in report.splitlines():
            # The first several lines of output should be comments with '#'
            if re.match('^#', line):
                continue

            stuff = line.split()
            # there's a slight risk that we might get an unknown sample
            # somewhere in the mix, and I've seen this more often on some
            # platforms where there is high "skid" or imprecision.
            # So we'll mitigate that by only checking the first line after
            # the comments, which should be in the crc loop and the majority
            # of samples
            if stuff[3] == '[unknown]':
                logging.info('bad report entry: %s' % line)
                raise error.TestFail
            else:
                return True


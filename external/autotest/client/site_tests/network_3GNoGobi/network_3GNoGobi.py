# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


GOBI_FILES = ['/usr/lib/libQCWWAN2k.so']
GOBI_DIRS = ['/opt/Qualcomm']


class network_3GNoGobi(test.test):
    version = 1


    def GobiDirs(self):
        # Return a list of non-empty gobi directories.
        return [d for d in GOBI_DIRS if os.path.exists(d) and os.listdir(d)]


    def GobiFiles(self):
        # Return a list of all Gobi files
        return [f for f in GOBI_FILES if os.path.exists(f)]


    def run_once(self):
        # Look in the file system to make sure there are no gobi
        # related files.
        files = self.GobiFiles()
        if files:
            raise error.TestError('Found files: %s' %
                                  ', '.join(files))

        dirs = self.GobiDirs()
        if dirs:
            raise error.TestError('Found non-empty directories: %s' %
                                  ', '.join(dirs))

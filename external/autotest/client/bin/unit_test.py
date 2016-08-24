# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import utils

class unit_test(test.test):
    """
    Unit test should simply subclass this test which handles everything.
    """
    version = 1
    preserve_srcdir = True


    def setup(self):
      os.chdir(self.srcdir)
      utils.make('clean')
      utils.make('all')
      
      self.job.setup_dep(['gtest'])

    def run_once(self):
        dep ='gtest'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)
      
        # Run the unit test, gather the results and place the gcda files for
        # code coverage in the results directory.
        
        os.chdir(self.srcdir)
        result = utils.run('LD_LIBRARY_PATH=' + dep_dir + 
                           ' GCOV_PREFIX=' + self.resultsdir +
                           ' GCOV_PREFIX_STRIP=9999 ./unit_test > ' +
                           self.resultsdir + '/unit_test_result.txt')
        logging.debug(result.stderr)
        logging.info('result: ' + self.resultsdir + '/unit_test_result.txt')
        
    def cleanup(self):
        # This is a hack - we should only need to copy back the .gcda file but
        # we don't know how to access the source on the server. So copy 
        # everything back.
        
        os.chdir(self.srcdir)
        utils.run('cp * ' + self.resultsdir)

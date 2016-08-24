# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import shutil

from autotest_lib.client.common_lib import utils
from autotest_lib.server import autotest, test

def gen_gcov_report(report, files):
    results = {}

    for f in files:
        escf = re.escape(f)
        match = re.search("File '.*%s'\nLines executed:([0-9.]+)%%" % escf, 
                          report)
        if match:
            # simple replace to make a valid identifier
            key = f.replace('/', '_').replace('.', '_')
            results[key] = float(match.group(1))

    return results

class unit_test_server(test.test):
    version = 1

    def run_once(self, host=None):
        self.client = host

        # Collect the gcov by running a client side test
        client_at = autotest.Autotest(self.client)
        client_at.run_test(self.client_test)
        
    def postprocess(self):
        logging.info('UnitTestServer: postprocess %s' %
                     self.client.hostname)
        
        # Get the result director of the client
        results_dir = os.path.join(self.outputdir, self.client_test, 'results/')
        
        # Execute gcov on the result
        os.chdir(results_dir)
        report = utils.system_output('gcov ' + ''.join(self.test_files))
        
        # Filter report for the files of interest
        filtered = gen_gcov_report(report, self.test_files)
        
        # Promote the client test keyval as our own
        src = os.path.join(self.outputdir, self.client_test, 'results/keyval')
        dst = os.path.join(self.resultsdir, 'keyval')
        if os.path.exists(src):
            shutil.copy(src, dst)
        else:
            logging.warning('Unable to locate %s' % src)
        
        # Append the coverage report
        self.write_perf_keyval(filtered)

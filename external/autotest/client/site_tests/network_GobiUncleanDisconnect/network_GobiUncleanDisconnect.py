# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

import subprocess

class network_GobiUncleanDisconnect(test.test):
    version = 1

    def unclean_disconnect(self, iteration):
        rv = subprocess.call(['/usr/bin/libgobi3k/open-abort'])
        print 'Iteration %d: %d' % (iteration, rv)
        if rv != -9:
            # subprocess communicates kills by returning the signal value,
            # negated.
            raise error.TestFail('Unexpected exit status %d' % rv)

    def run_once(self, iterations=10):
        for iteration in xrange(iterations):
            self.unclean_disconnect(iteration)

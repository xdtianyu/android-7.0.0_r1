# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import test

from autotest_lib.client.common_lib import utils

class platform_OpenSSLActual(test.test):
    version = 1

    def curl(self, rest):
        base = '/usr/bin/curl -sSIo /dev/null'
        out = utils.system_output('%s %s' % (base, rest))
        print out

    def run_once(self):
        self.curl('https://www.google.com')
        self.curl('--capath /var/empty https://www.google.com; [ $? != 0 ]')

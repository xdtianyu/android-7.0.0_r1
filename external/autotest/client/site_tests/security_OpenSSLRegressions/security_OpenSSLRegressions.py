# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

OPENSSL = '/usr/bin/openssl'
VERIFY = OPENSSL + ' verify'

class security_OpenSSLRegressions(test.test):
    version = 1

    def verify(self):
        r = os.system('%s %s' % (VERIFY, self.cert))
        return r

    def run_once(self, opts=None):
        self.cert = '%s/cert.pem' % self.srcdir

        # Checking for openssl-0.9.8r-verify-retcode.patch (see
        # chromiumos-overlay:2ea51e44669062977689ff09a43ac8438f55673f).
        if self.verify() == 0:
            raise error.TestFail('Verify returned zero on error.')

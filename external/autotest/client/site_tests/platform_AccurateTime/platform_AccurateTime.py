# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, math, re
import subprocess
import time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_logging

OPENSSL = '/usr/bin/openssl'
TLSDATE = '/usr/bin/tlsdate'

class platform_AccurateTime(test.test):
    version = 1

    def serve(self):
        self.ca = '%s/ca.pem' % self.srcdir
        self.cert = '%s/cert.pem' % self.srcdir
        self.key = '%s/cert.key' % self.srcdir
        self.server = subprocess.Popen([OPENSSL, 's_server', '-www',
                                        '-CAfile', self.ca, '-cert', self.cert,
                                        '-key', self.key, '-port', '4433'])
        time.sleep(1)

    def tlsdate(self):
        proc = subprocess.Popen([TLSDATE, '-H', '127.0.0.1', '-p', '4433',
                                 '-C', self.srcdir,
                                 '-nv'], stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        (out,err) = proc.communicate()
        return err

    def run_once(self):
        self.serve()
        out = self.tlsdate()
        print out
        try:
            if 'verification passed' not in out:
                raise error.TestFail('ssl did not verify')
            if 'difference is about' not in out:
                raise error.TestFail('no time delta found')
        finally:
            self.server.terminate()

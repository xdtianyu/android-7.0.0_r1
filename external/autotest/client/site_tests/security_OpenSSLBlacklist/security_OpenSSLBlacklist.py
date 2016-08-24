# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

OPENSSL = '/usr/bin/openssl'
VERIFY = OPENSSL + ' verify'

class security_OpenSSLBlacklist(test.test):
    version = 1

    def verify(self, blacklist='/dev/null'):
        r = os.system('OPENSSL_BLACKLIST_PATH=%s %s -CAfile %s %s' %
            (blacklist, VERIFY, self.ca, self.cert))
        return r == 0

    def fetch(self, blacklist='/dev/null'):
        r = os.system('OPENSSL_BLACKLIST_PATH=%s curl --cacert %s -o /dev/null '
                      'https://127.0.0.1:4433/' % (blacklist, self.ca))
        return r == 0

    def run_once(self, opts=None):
        self.blacklists = [
            '%s/sha256_blacklist' % self.srcdir,
            '%s/sha1_blacklist' % self.srcdir,
            '%s/serial_blacklist' % self.srcdir,
        ]
        self.bogus_blacklist = '%s/bogus_blacklist' % self.srcdir
        self.ca = '%s/ca.pem' % self.srcdir
        self.cert = '%s/cert.pem' % self.srcdir
        self.key = '%s/cert.key' % self.srcdir

        if not self.verify():
            raise error.TestFail('Certificate does not verify normally.')
        for b in self.blacklists:
            if self.verify(b):
                raise error.TestFail('Certificate verified with %s' % b)
        if not self.verify(self.bogus_blacklist):
            raise error.TestFail('Certificate does not verify with nonempty blacklist.')

        # Fire up an openssl s_server and have curl fetch from it
        server = subprocess.Popen([OPENSSL, 's_server', '-www',
                                   '-CAfile', self.ca, '-cert', self.cert,
                                   '-key', self.key, '-port', '4433'])
        try:
            # Need to wait for openssl to be ready to talk to us
            utils.poll_for_condition(
                self.fetch,
                error.TestFail('Fetch without blacklist fails.'))
            for b in self.blacklists:
                if self.fetch(b):
                    raise error.TestFail('Fetched with %s' % b)
        finally:
            server.terminate()

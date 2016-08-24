# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import requests

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class brillo_WebservdSanity(test.test):
    """Verify that webservd delegates requests to clients."""
    version = 1

    def run_once(self, host=None):
        """Body of the test."""
        host.adb_run('forward tcp:8998 tcp:80')
        host.run('start webservd_tclient')
        r = requests.get('http://localhost:8998/webservd-test-client/ping')
        if r.status_code != 200:
            raise error.TestFail('Expected successful http request but '
                                 'status=%d' % r.status_code)
        if r.text != 'Still alive, still alive!\n':
            raise error.TestFail('Unexpected response: %s' % r.text)

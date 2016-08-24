# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import tempfile
from autotest_lib.client.bin import test
from autotest_lib.client.cros import cryptohome
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import chrome


class login_GaiaLogin(test.test):
    """Sign into production gaia using Telemetry."""
    version = 1


    _USERNAME = 'powerloadtest@gmail.com'
    # TODO(achuith): Get rid of this when crbug.com/358427 is fixed.
    _USERNAME_DISPLAY = 'power.loadtest@gmail.com'
    _PLTP_URL = 'https://sites.google.com/a/chromium.org/dev/chromium-os' \
                '/testing/power-testing/pltp/pltp'

    def run_once(self):
        with tempfile.NamedTemporaryFile() as pltp:
            file_utils.download_file(self._PLTP_URL, pltp.name)
            self._password = pltp.read().rstrip()

        with chrome.Chrome(gaia_login=True, username=self._USERNAME,
                                            password=self._password) as cr:
            if not cryptohome.is_vault_mounted(user=self._USERNAME):
                raise error.TestFail('Expected to find a mounted vault for %s'
                                     % self._USERNAME)
            tab = cr.browser.tabs.New()
            # TODO(achuith): Use a better signal of being logged in, instead of
            # parsing accounts.google.com.
            tab.Navigate('http://accounts.google.com')
            tab.WaitForDocumentReadyStateToBeComplete()
            res = tab.EvaluateJavaScript('''
                    var res = '',
                        divs = document.getElementsByTagName('div');
                    for (var i = 0; i < divs.length; i++) {
                        res = divs[i].textContent;
                        if (res.search('%s') > 1) {
                            break;
                        }
                    }
                    res;
            ''' % self._USERNAME_DISPLAY)
            if not res:
                raise error.TestFail('No references to %s on accounts page.'
                                     % self._USERNAME_DISPLAY)
            tab.Close()

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import ap_constants
from autotest_lib.server import test

class network_WiFi_ChaosConfigFailure(test.test):
    """ Test to grab debugging info about chaos configuration falures. """

    version = 1


    def run_once(self, ap, error_string):
        """ Main entry function for autotest.

        There are three pieces of information we want to grab:
          1.) Screenshot at the point of failure
          2.) Screenshots of all pages
          3.) Stack trace of failure

        @param ap: an APConfigurator object
        @param error_string: String with the Configurator error description

        """
        if ap_constants.AP_CONFIG_FAIL in error_string:
            ap.debug_last_failure(self.outputdir)

        ap.debug_full_state(self.outputdir)

        if ap_constants.AP_PDU_DOWN in error_string:
            raise error.TestError('The AP was not configured because the PDU '
                                  'is down. See the ERROR log for more details.'
                                  '\n%s', ap.name)

        if ap_constants.AP_CONFIG_FAIL in error_string:
            raise error.TestError('The AP was not configured correctly. Please '
                                  'see the ERROR log for more details.\n%s',
                                  ap.name)
        elif ap_constants.AP_SECURITY_MISMATCH in error_string:
            raise error.TestError('The AP was not configured with correct '
                                  'security. Please check screenshots to '
                                  'debug.\n%s', ap.name)
        elif ap_constants.WORK_CLI_CONNECT_FAIL in error_string:
            raise error.TestError('Work client was not able to connect to '
                                  'the AP. Please check screenshots to '
                                  'debug.\n%s', ap.name)
        else:
            raise error.TestError('The SSID %s was not found in the scan. '
                                  'Check the screenshots to debug.\n%s',
                                  ap.ssid, ap.name)

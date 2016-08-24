# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import iw_runner


class network_WlanRegulatory(test.test):
    """
    Ensure the "crda" tool works, and can be successfully triggered by
    the kernel from the "iw" userspace utility to change the system
    regulatory domain.
    """
    version = 1
    REGULATORY_DATABASE = '/usr/lib/crda/regulatory.bin'

    def get_regulatory_domains(self):
        """Get the list or regulatory domains in the DUT's database."""
        return utils.system_output('regdbdump %s | grep country | '
                                   'sed -e s/^country.// -e s/:.*//' %
                                   self.REGULATORY_DATABASE).split()


    def assert_set_regulatory_domain(self, regdomain):
        """Set the system regulatory domain, then assert that it is correct.

        @param regdomain string 2-letter country code of the regulatory
            domain to set.

        """
        logging.info('Using iw to set regulatory domain to %s', regdomain)
        self._iw.set_regulatory_domain(regdomain)

        # It takes time for the kernel to invoke udev, which will in turn
        # invoke CRDA.  Since this is asynchronous with the exit of the
        # "iw" utility, we must wait a while.
        time.sleep(1)

        current_regdomain = self._iw.get_regulatory_domain()
        if current_regdomain != regdomain:
            raise error.TestFail('Expected iw to set regdomain %s but got %s' %
                                 (regdomain, current_regdomain))


    def run_once(self):
        """Test main loop"""
        self._iw = iw_runner.IwRunner()
        initial_regdomain = self._iw.get_regulatory_domain()
        logging.info('Initial regulatory domain is %s', initial_regdomain)

        domain_list = self.get_regulatory_domains()
        if not domain_list:
            raise error.TestFail('Did not get a domain list from the database')

        for domain in domain_list:
            self.assert_set_regulatory_domain(domain)
        self.assert_set_regulatory_domain(initial_regdomain)

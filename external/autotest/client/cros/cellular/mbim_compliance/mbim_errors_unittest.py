# Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import unittest

import common
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


class MBIMComplianceAssertionErrorTestCase(unittest.TestCase):
    """ Test MBIMComplianceAsertionError construction. """

    def test_correct_assertion_code(self):
        """ Constructs an error with a valid assertion id. """
        self.assertRaises(
                mbim_errors.MBIMComplianceAssertionError,
                mbim_errors.log_and_raise,
                mbim_errors.MBIMComplianceAssertionError,
                'mbim1.0:3.2.1#1')


    def test_correct_assertion_code_and_error_string(self):
        """ Constructs an error with a valid assertion id and extra string. """
        self.assertRaises(
                mbim_errors.MBIMComplianceAssertionError,
                mbim_errors.log_and_raise,
                mbim_errors.MBIMComplianceAssertionError,
                'mbim1.0:3.2.1#1',
                'Some error')


    def test_incorrect_assertion_code(self):
        """ Constructs an error with and invalid assertion id. """
        self.assertRaises(
                mbim_errors.MBIMComplianceFrameworkError,
                mbim_errors.log_and_raise,
                mbim_errors.MBIMComplianceAssertionError,
                'wrong_id_obviously')


    def test_generic_assertion_error(self):
        """ Constructs a generic error. """
        self.assertRaises(
                mbim_errors.MBIMComplianceGenericAssertionError,
                mbim_errors.log_and_raise,
                mbim_errors.MBIMComplianceGenericAssertionError,
                'some generic error')


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()

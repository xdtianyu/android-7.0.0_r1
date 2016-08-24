#!/usr/bin/python3.4
#
#   Copyright 2015 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts import asserts
from acts import base_test

import mock_controller

class IntegrationTest(base_test.BaseTestClass):

    def setup_class(self):
        self.register_controller(mock_controller)

    def test_hello_world(self):
        asserts.assert_equal(self.user_params["icecream"], 42)
        asserts.assert_equal(self.user_params["extra_param"], "haha")
        self.log.info("This is a bare minimal test to make sure the basic ACTS"
                      "test flow works.")
        asserts.explicit_pass("Hello World")
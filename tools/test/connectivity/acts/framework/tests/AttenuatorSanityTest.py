#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import random
from acts.base_test import BaseTestClass

CONSERVATIVE_MAX_ATTEN_VALUE = 10
MIN_ATTEN_VALUE = 0

class AttenuatorSanityTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_attenuator_validation",
            "test_attenuator_get_max_value",
        )
        self.number_of_iteration = 2

    def test_attenuator_validation(self):
        """Validate attenuator set and get APIs works fine.
        """
        for atten in self.attenuators:
            self.log.info("Attenuator: {}".format(atten))
            try:
                atten_max_value = atten.get_max_atten()
            except ValueError as e:
                self.log.error(e)
                self.log.info("Using conservative max value.")
                atten_max_value = CONSERVATIVE_MAX_ATTEN_VALUE

            atten_value_list = [MIN_ATTEN_VALUE, atten_max_value]
            for i in range(0, self.number_of_iteration):
                atten_value_list.append(int(random.uniform(0,atten_max_value)))

            for atten_val in atten_value_list:
                self.log.info("Set atten to {}".format(atten_val))
                atten.set_atten(atten_val)
                current_atten = int(atten.get_atten())
                self.log.info("Current atten = {}".format(current_atten))
                assert atten_val == current_atten, "Setting attenuator failed."

        return True

    def test_attenuator_get_max_value(self):
        """Validate attenuator get_max_atten APIs works fine.
        """
        for atten in self.attenuators:
            try:
                atten_max_value = atten.get_max_atten()
            except ValueError as e:
                self.log.error(e)
                return False
        return True

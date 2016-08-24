#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

import unittest

from acts import records
from acts import signals

class ActsRecordsTest(unittest.TestCase):
    """This test class tests the implementation of classes in acts.records.
    """

    def setUp(self):
        self.tn = "test_name"
        self.details = "Some details about the test execution."
        self.float_extra = 12345.56789
        self.json_extra = {"ha": "whatever"}

    def verify_record(self, record, result, details, extras):
        # Verify each field.
        self.assertEqual(record.test_name, self.tn)
        self.assertEqual(record.result, result)
        self.assertEqual(record.details, details)
        self.assertEqual(record.extras, extras)
        self.assertTrue(record.begin_time, "begin time should not be empty.")
        self.assertTrue(record.end_time, "end time should not be empty.")
        # UID is not used at the moment, should always be None.
        self.assertIsNone(record.uid)
        # Verify to_dict.
        d = {}
        d[records.TestResultEnums.RECORD_NAME] = self.tn
        d[records.TestResultEnums.RECORD_RESULT] = result
        d[records.TestResultEnums.RECORD_DETAILS] = details
        d[records.TestResultEnums.RECORD_EXTRAS] = extras
        d[records.TestResultEnums.RECORD_BEGIN_TIME] = record.begin_time
        d[records.TestResultEnums.RECORD_END_TIME] = record.end_time
        d[records.TestResultEnums.RECORD_UID] = None
        d[records.TestResultEnums.RECORD_CLASS] = None
        d[records.TestResultEnums.RECORD_EXTRA_ERRORS] = {}
        actual_d = record.to_dict()
        self.assertDictEqual(actual_d, d)
        # Verify that these code paths do not cause crashes and yield non-empty
        # results.
        self.assertTrue(str(record), "str of the record should not be empty.")
        self.assertTrue(repr(record), "the record's repr shouldn't be empty.")
        self.assertTrue(record.json_str(), ("json str of the record should "
                         "not be empty."))

    """ Begin of Tests """
    def test_result_record_pass_none(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        record.test_pass()
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_PASS,
                           details=None,
                           extras=None)

    def test_result_record_pass_with_float_extra(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        s = signals.TestPass(self.details, self.float_extra)
        record.test_pass(s)
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_PASS,
                           details=self.details,
                           extras=self.float_extra)

    def test_result_record_pass_with_json_extra(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        s = signals.TestPass(self.details, self.json_extra)
        record.test_pass(s)
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_PASS,
                           details=self.details,
                           extras=self.json_extra)

    def test_result_record_fail_none(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        record.test_fail()
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_FAIL,
                           details=None,
                           extras=None)

    def test_result_record_fail_with_float_extra(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        s = signals.TestFailure(self.details, self.float_extra)
        record.test_fail(s)
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_FAIL,
                           details=self.details,
                           extras=self.float_extra)

    def test_result_record_fail_with_json_extra(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        s = signals.TestFailure(self.details, self.json_extra)
        record.test_fail(s)
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_FAIL,
                           details=self.details,
                           extras=self.json_extra)

    def test_result_record_skip_none(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        record.test_skip()
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_SKIP,
                           details=None,
                           extras=None)

    def test_result_record_skip_with_float_extra(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        s = signals.TestSkip(self.details, self.float_extra)
        record.test_skip(s)
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_SKIP,
                           details=self.details,
                           extras=self.float_extra)

    def test_result_record_skip_with_json_extra(self):
        record = records.TestResultRecord(self.tn)
        record.test_begin()
        s = signals.TestSkip(self.details, self.json_extra)
        record.test_skip(s)
        self.verify_record(record=record,
                           result=records.TestResultEnums.TEST_RESULT_SKIP,
                           details=self.details,
                           extras=self.json_extra)

if __name__ == "__main__":
   unittest.main()
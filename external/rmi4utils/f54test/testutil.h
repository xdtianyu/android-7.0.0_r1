/*
 * Copyright (C) 2014 Satoshi Noguchi
 * Copyright (C) 2014 Synaptics Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TESTUTIL_H_
#define _TESTUTIL_H_

enum update_error {
	TEST_SUCCESS				= 0,
	TEST_FAIL,
	TEST_FAIL_TIMEOUT,
	TEST_FAIL_NO_FUNCTION_01,
	TEST_FAIL_NO_FUNCTION_54,
	TEST_FAIL_NO_FUNCTION_55,
	TEST_FAIL_QUERY_BASIC_PROPERTIES,
	TEST_FAIL_READ_F54_QUERIES,
	TEST_FAIL_READ_F54_CONTROLS,
	TEST_FAIL_SCAN_PDT,
	TEST_FAIL_READ_DEVICE_STATUS,
	TEST_FAIL_READ_F01_CONTROL_0,
	TEST_FAIL_WRITE_F01_CONTROL_0,
	TEST_FAIL_TIMEOUT_WAITING_FOR_ATTN,
	TEST_FAIL_INVALID_PARAMETER,
	TEST_FAIL_MEMORY_ALLOCATION,
};

const char * test_err_to_string(int err);

unsigned long extract_long(const unsigned char *data);
unsigned short extract_short(const unsigned char *data);
const char * StripPath(const char * path, ssize_t size);

#endif // _TESTUTIL_H_

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

#include <sys/types.h>

#include "testutil.h"

const char *test_error_str[] = {
	"success",							// TEST_SUCCESS
	"failed",							// TEST_FAIL
	"timeout",							// TEST_FAIL_TIMEOUT
	"failed to find F01 on device",					// TEST_FAIL_NO_FUNCTION_01
	"failed to find F54 on device",					// TEST_FAIL_NO_FUNCTION_54
	"failed to find F55 on device",					// TEST_FAIL_NO_FUNCTION_55
	"failed to query the basic properties in F01",			// TEST_FAIL_QUERY_BASIC_PROPERTIES
	"failed to read F54 query registers",				// TEST_FAIL_READ_F54_QUERIES
	"failed to read F54 control registers",				// TEST_FAIL_READ_F54_CONTROLS
	"failed to scan the PDT",					// TEST_FAIL_SCAN_PDT
	"failed to read the device status",				// TEST_FAIL_READ_DEVICE_STATUS
	"failed to read F01 control 0 register",			// TEST_FAIL_READ_F01_CONTROL_0
	"failed to write F01 control 0 register",			// TEST_FAIL_WRITE_F01_CONTROL_0
	"timeout waiting for attn",					// TEST_FAIL_TIMEOUT_WAITING_FOR_ATTN
	"invalid parameter",						// TEST_FAIL_INVALID_PARAMETER
	"memory allocation failure",					// TEST_FAIL_MEMORY_ALLOCATION
};

const char * test_err_to_string(int err)
{
	return test_error_str[err];
}

unsigned long extract_long(const unsigned char *data)
{
	return (unsigned long)data [0]
		+ (unsigned long)data [1] * 0x100
		+ (unsigned long)data [2] * 0x10000
		+ (unsigned long)data [3] * 0x1000000;
}

unsigned short extract_short(const unsigned char *data)
{
	return (unsigned long)data [0]
		+ (unsigned long)data [1] * 0x100;
}

const char * StripPath(const char * path, ssize_t size)
{
	int i;
	const char * str;

	for (i = size - 1, str = &path[size - 1]; i > 0; --i, --str)
		if (path[i - 1] == '/')
			break;

	return str;
}

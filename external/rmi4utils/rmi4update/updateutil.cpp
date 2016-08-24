/*
 * Copyright (C) 2014 Andrew Duggan
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

#include "updateutil.h"

const char *update_error_str[] = {
	"success",							// UPDATE_SUCCESS
	"failed",							// UPDATE_FAIL
	"timeout",							// UPDATE_FAIL_TIMEOUT
	"invalid firmware image",					// UPDATE_FAIL_VERIFY_IMAGE
	"checksum does not match image",				// UPDATE_FAIL_VERIFY_CHECKSUM
	"image firmware size does not match device",			// UPDATE_FAIL_VERIFY_FIRMWARE_SIZE
	"image config size does not match device",			// UPDATE_FAIL_VERIFY_CONFIG_SIZE
	"image version is unsupported",					// UPDATE_FAIL_UNSUPPORTED_IMAGE_VERSION
	"failed to find F01 on device",					// UPDATE_FAIL_NO_FUNCTION_01
	"failed to find F34 on device",					// UPDATE_FAIL_NO_FUNCTION_34
	"failed to query the basic properties in F01",			// UPDATE_FAIL_QUERY_BASIC_PROPERTIES
	"failed to read F34 query registers",				// UPDATE_FAIL_READ_F34_QUERIES
	"failed to read the bootloader id",				// UPDATE_FAIL_READ_BOOTLOADER_ID
	"failed to read F34 control registers",				// UPDATE_FAIL_READ_F34_CONTROLS
	"failed to write the bootloader id",				// UPDATE_FAIL_WRITE_BOOTLOADER_ID
	"failed to enable flash programming",				// UPDATE_FAIL_ENABLE_FLASH_PROGRAMMING
	"failed to reach idle state",					// UPDATE_FAIL_NOT_IN_IDLE_STATE
	"programming is not enabled",					// UPDATE_FAIL_PROGRAMMING_NOT_ENABLED
	"failed to scan the PDT",					// UPDATE_FAIL_SCAN_PDT
	"failed to read the device status",				// UPDATE_FAIL_READ_DEVICE_STATUS
	"device not in the bootloader after enabling programming",	// UPDATE_FAIL_DEVICE_NOT_IN_BOOTLOADER
	"failed to read F01 control 0 register",			// UPDATE_FAIL_READ_F01_CONTROL_0
	"failed to write F01 control 0 register",			// UPDATE_FAIL_WRITE_F01_CONTROL_0
	"failed to write initial zeros",				// UPDATE_FAIL_WRITE_INITIAL_ZEROS
	"failed to write block",					// UPDATE_FAIL_WRITE_BLOCK
	"failed to write the flash command",				// UPDATE_FAIL_WRITE_FLASH_COMMAND
	"timeout waiting for attn",					// UPDATE_FAIL_TIMEOUT_WAITING_FOR_ATTN
	"failed to write erase all command",				// UPDATE_FAIL_ERASE_ALL
	"the firmware image is older then the firmware on the device",	// UPDATE_FAIL_FIRMWARE_IMAGE_IS_OLDER
	"invalid parameter",						// UPDATE_FAIL_INVALID_PARAMETER
	"failed to open firmware image file",				// UPDATE_FAIL_OPEN_FIRMWARE_IMAGE
};

const char * update_err_to_string(int err)
{
	return update_error_str[err];
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
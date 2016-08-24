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

#ifndef _UPDATEUTIL_H_
#define _UPDATEUTIL_H_

enum update_error {
	UPDATE_SUCCESS				= 0,
	UPDATE_FAIL,
	UPDATE_FAIL_TIMEOUT,
	UPDATE_FAIL_VERIFY_IMAGE,
	UPDATE_FAIL_VERIFY_CHECKSUM,
	UPDATE_FAIL_VERIFY_FIRMWARE_SIZE,
	UPDATE_FAIL_VERIFY_CONFIG_SIZE,
	UPDATE_FAIL_UNSUPPORTED_IMAGE_VERSION,
	UPDATE_FAIL_NO_FUNCTION_01,
	UPDATE_FAIL_NO_FUNCTION_34,
	UPDATE_FAIL_QUERY_BASIC_PROPERTIES,
	UPDATE_FAIL_READ_F34_QUERIES,
	UPDATE_FAIL_READ_BOOTLOADER_ID,
	UPDATE_FAIL_READ_F34_CONTROLS,
	UPDATE_FAIL_WRITE_BOOTLOADER_ID,
	UPDATE_FAIL_ENABLE_FLASH_PROGRAMMING,
	UPDATE_FAIL_NOT_IN_IDLE_STATE,
	UPDATE_FAIL_PROGRAMMING_NOT_ENABLED,
	UPDATE_FAIL_SCAN_PDT,
	UPDATE_FAIL_READ_DEVICE_STATUS,
	UPDATE_FAIL_DEVICE_NOT_IN_BOOTLOADER,
	UPDATE_FAIL_READ_F01_CONTROL_0,
	UPDATE_FAIL_WRITE_F01_CONTROL_0,
	UPDATE_FAIL_WRITE_INITIAL_ZEROS,
	UPDATE_FAIL_WRITE_BLOCK,
	UPDATE_FAIL_WRITE_FLASH_COMMAND,
	UPDATE_FAIL_TIMEOUT_WAITING_FOR_ATTN,
	UPDATE_FAIL_ERASE_ALL,
	UPDATE_FAIL_FIRMWARE_IMAGE_IS_OLDER,
	UPDATE_FAIL_INVALID_PARAMETER,
	UPDATE_FAIL_OPEN_FIRMWARE_IMAGE,
};

const char * update_err_to_string(int err);

unsigned long extract_long(const unsigned char *data);
unsigned short extract_short(const unsigned char *data);
const char * StripPath(const char * path, ssize_t size);

#endif // _UPDATEUTIL_H_
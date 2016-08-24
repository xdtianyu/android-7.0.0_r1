/*
 * Copyright (C) 2012 - 2014 Andrew Duggan
 * Copyright (C) 2012 - 2014 Synaptics Inc
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

#include <iostream>
#include <fstream>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "firmware_image.h"

using namespace std;

unsigned long FirmwareImage::Checksum(unsigned short * data, unsigned long len)
{
	unsigned long checksum = 0xFFFFFFFF;
	unsigned long lsw = checksum & 0xFFFF;
	unsigned long msw = checksum >> 16;

	while (len--) {
		lsw += *data++;
		msw += lsw;
		lsw = (lsw & 0xffff) + (lsw >> 16);
		msw = (msw & 0xffff) + (msw >> 16);
	}

	checksum = msw << 16 | lsw;

	return checksum;
}

int FirmwareImage::Initialize(const char * filename)
{
	if (!filename)
		return UPDATE_FAIL_INVALID_PARAMETER;

	ifstream ifsFile(filename, ios::in|ios::binary|ios::ate);
	if (!ifsFile)
		return UPDATE_FAIL_OPEN_FIRMWARE_IMAGE;

	ifsFile.seekg(0, ios::end);
	m_imageSize = ifsFile.tellg();
	if (m_imageSize < 0)
		return UPDATE_FAIL_OPEN_FIRMWARE_IMAGE;

	m_memBlock = new unsigned char[m_imageSize];
	ifsFile.seekg(0, ios::beg);
	ifsFile.read((char*)m_memBlock, m_imageSize);

	if (m_imageSize < 0x100)
		return UPDATE_FAIL_VERIFY_IMAGE;

	m_checksum = extract_long(&m_memBlock[RMI_IMG_CHECKSUM_OFFSET]);

	unsigned long imageSizeMinusChecksum = m_imageSize - 4;
	if ((imageSizeMinusChecksum % 2) != 0)
		/*
		 * Since the header size is fixed and the firmware is
		 * in 16 byte blocks a valid image size should always be
		 * divisible by 2.
		 */
		return UPDATE_FAIL_VERIFY_IMAGE;

	unsigned long calculated_checksum = Checksum((uint16_t *)&(m_memBlock[4]),
		imageSizeMinusChecksum >> 1);

	if (m_checksum != calculated_checksum) {
		fprintf(stderr, "Firmware image checksum verification failed, saw 0x%08lX, calculated 0x%08lX\n",
			m_checksum, calculated_checksum);
		return UPDATE_FAIL_VERIFY_CHECKSUM;
	}

	m_io = m_memBlock[RMI_IMG_IO_OFFSET];
	m_bootloaderVersion = m_memBlock[RMI_IMG_BOOTLOADER_VERSION_OFFSET];
	m_firmwareSize = extract_long(&m_memBlock[RMI_IMG_IMAGE_SIZE_OFFSET]);
	m_configSize = extract_long(&m_memBlock[RMI_IMG_CONFIG_SIZE_OFFSET]);
	if (m_io == 1) {
		m_firmwareBuildID = extract_long(&m_memBlock[RMI_IMG_FW_BUILD_ID_OFFSET]);
		m_packageID = extract_long(&m_memBlock[RMI_IMG_PACKAGE_ID_OFFSET]);
	}
	memcpy(m_productID, &m_memBlock[RMI_IMG_PRODUCT_ID_OFFSET], RMI_PRODUCT_ID_LENGTH);
	m_productID[RMI_PRODUCT_ID_LENGTH] = 0;
	m_productInfo = extract_short(&m_memBlock[RMI_IMG_PRODUCT_INFO_OFFSET]);

	m_firmwareData = &m_memBlock[RMI_IMG_FW_OFFSET];
	m_configData = &m_memBlock[RMI_IMG_FW_OFFSET + m_firmwareSize];

	switch (m_bootloaderVersion) {
		case 2:
			m_lockdownSize = RMI_IMG_LOCKDOWN_V2_SIZE;
			m_lockdownData = &m_memBlock[RMI_IMG_LOCKDOWN_V2_OFFSET];
			break;
		case 3:
		case 4:
			m_lockdownSize = RMI_IMG_LOCKDOWN_V3_SIZE;
			m_lockdownData = &m_memBlock[RMI_IMG_LOCKDOWN_V3_OFFSET];
			break;
		case 5:
		case 6:
			m_lockdownSize = RMI_IMG_LOCKDOWN_V5_SIZE;
			m_lockdownData = &m_memBlock[RMI_IMG_LOCKDOWN_V5_OFFSET];
			break;
		default:
			return UPDATE_FAIL_UNSUPPORTED_IMAGE_VERSION;
	}

	fprintf(stdout, "Firmware Header:\n");
	PrintHeaderInfo();

	return UPDATE_SUCCESS;
}

void FirmwareImage::PrintHeaderInfo()
{
	fprintf(stdout, "Checksum:\t\t0x%lx\n", m_checksum);
	fprintf(stdout, "Firmware Size:\t\t%ld\n", m_firmwareSize);
	fprintf(stdout, "Config Size:\t\t%ld\n", m_configSize);
	fprintf(stdout, "Lockdown Size:\t\t%ld\n", m_lockdownSize);
	fprintf(stdout, "Firmware Build ID:\t%ld\n", m_firmwareBuildID);
	fprintf(stdout, "Package ID:\t\t%d\n", m_packageID);
	fprintf(stdout, "Bootloader Version:\t%d\n", m_bootloaderVersion);
	fprintf(stdout, "Product ID:\t\t%s\n", m_productID);
	fprintf(stdout, "Product Info:\t\t%d\n", m_productInfo);
	fprintf(stdout, "\n");
}

int FirmwareImage::VerifyImageMatchesDevice(unsigned long deviceFirmwareSize,
						unsigned long deviceConfigSize)
{
	if (m_firmwareSize != deviceFirmwareSize) {
		fprintf(stderr, "Firmware image size verfication failed, size in image %ld did "
			"not match device size %ld\n", m_firmwareSize, deviceFirmwareSize);
		return UPDATE_FAIL_VERIFY_FIRMWARE_SIZE;
	}

	if (m_configSize != deviceConfigSize) {
		fprintf(stderr, "Firmware image size verfication failed, size in image %ld did "
			"not match device size %ld\n", m_firmwareSize, deviceConfigSize);
		return UPDATE_FAIL_VERIFY_CONFIG_SIZE;
	}
	
	return UPDATE_SUCCESS;
}

FirmwareImage::~FirmwareImage()
{
	delete [] m_memBlock;
	m_memBlock = NULL;
}

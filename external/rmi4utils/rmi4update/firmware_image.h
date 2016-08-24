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

#ifndef _FIRMWAREIMAGE_H_
#define _FIRMWAREIMAGE_H_

#include "rmidevice.h"
#include "updateutil.h"

#define RMI_IMG_CHECKSUM_OFFSET			0
#define RMI_IMG_IO_OFFSET			0x06
#define RMI_IMG_BOOTLOADER_VERSION_OFFSET	0x07
#define RMI_IMG_IMAGE_SIZE_OFFSET		0x08
#define RMI_IMG_CONFIG_SIZE_OFFSET		0x0C
#define RMI_IMG_PACKAGE_ID_OFFSET		0x1A
#define RMI_IMG_FW_BUILD_ID_OFFSET		0x50

#define RMI_IMG_PRODUCT_ID_OFFSET		0x10
#define RMI_IMG_PRODUCT_INFO_OFFSET		0x1E

#define RMI_IMG_FW_OFFSET			0x100

#define RMI_IMG_LOCKDOWN_V2_OFFSET		0xD0
#define RMI_IMG_LOCKDOWN_V2_SIZE		0x30

#define RMI_IMG_LOCKDOWN_V3_OFFSET		0xC0
#define RMI_IMG_LOCKDOWN_V3_SIZE		0x40

#define RMI_IMG_LOCKDOWN_V5_OFFSET		0xB0
#define RMI_IMG_LOCKDOWN_V5_SIZE		0x50

class FirmwareImage
{
public:
	FirmwareImage() : m_firmwareBuildID(0), m_packageID(0), m_firmwareData(NULL), m_configData(NULL), m_lockdownData(NULL),
				m_memBlock(NULL)
	{}
	int Initialize(const char * filename);
	int VerifyImageMatchesDevice(unsigned long deviceFirmwareSize,
					unsigned long deviceConfigSize);
	unsigned char * GetFirmwareData() { return m_firmwareData; }
	unsigned char * GetConfigData() { return m_configData; }
	unsigned char * GetLockdownData() { return m_lockdownData; }
	unsigned long GetFirmwareSize() { return m_firmwareSize; }
	unsigned long GetConfigSize() { return m_configSize; }
	unsigned long GetLockdownSize() { return m_lockdownSize; }
	unsigned long GetFirmwareID() { return m_firmwareBuildID; }
	bool HasIO() { return m_io; }
	~FirmwareImage();

private:
	unsigned long Checksum(unsigned short * data, unsigned long len);
	void PrintHeaderInfo();

private:
	unsigned long m_checksum;
	unsigned long m_firmwareSize;
	unsigned long m_configSize;
	unsigned long m_lockdownSize;
	long m_imageSize;
	unsigned long m_firmwareBuildID;
	unsigned short m_packageID;
	unsigned char m_bootloaderVersion;
	unsigned char m_io;
	char m_productID[RMI_PRODUCT_ID_LENGTH + 1];
	unsigned short m_productInfo;

	unsigned char * m_firmwareData;
	unsigned char * m_configData;
	unsigned char * m_lockdownData;
	unsigned char * m_memBlock;
};

#endif // _FIRMWAREIMAGE_H_

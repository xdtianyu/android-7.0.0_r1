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
 
#ifndef _RMI4UPDATE_H_
#define _RMI4UPDATE_H_

#include "rmidevice.h"
#include "firmware_image.h"

#define RMI_BOOTLOADER_ID_SIZE		2

class RMI4Update
{
public:
	RMI4Update(RMIDevice & device, FirmwareImage & firmwareImage) : m_device(device), 
			m_firmwareImage(firmwareImage), m_writeBlockWithCmd(true)
	{}
	int UpdateFirmware(bool force = false, bool performLockdown = false);

private:
	int DisableNonessentialInterupts();
	int FindUpdateFunctions();
	int ReadF34Queries();
	int ReadF34Controls();
	int WriteBootloaderID();
	int EnterFlashProgramming();
	int WriteBlocks(unsigned char *block, unsigned short count, unsigned char cmd);
	int WaitForIdle(int timeout_ms, bool readF34OnSucess = true);
	int GetFirmwareSize() { return m_blockSize * m_fwBlockCount; }
	int GetConfigSize() { return m_blockSize * m_configBlockCount; }

private:
	RMIDevice & m_device;
	FirmwareImage & m_firmwareImage;

	RMIFunction m_f01;
	RMIFunction m_f34;

	unsigned char m_deviceStatus;
	unsigned char m_bootloaderID[RMI_BOOTLOADER_ID_SIZE];
	bool m_writeBlockWithCmd;

	/* F34 Controls */
	unsigned char m_f34Command;
	unsigned char m_f34Status;
	bool m_programEnabled;

	/* F34 Query */
	bool m_hasNewRegmap;
	bool m_unlocked;
	bool m_hasConfigID;
	unsigned short m_blockSize;
	unsigned short m_fwBlockCount;
	unsigned short m_configBlockCount;

	unsigned short m_f34StatusAddr;
};

#endif // _RMI4UPDATE_H_

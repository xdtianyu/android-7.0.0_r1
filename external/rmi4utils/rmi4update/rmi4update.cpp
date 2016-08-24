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

#include <alloca.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "rmi4update.h"

#define RMI_F34_QUERY_SIZE		7
#define RMI_F34_HAS_NEW_REG_MAP		(1 << 0)
#define RMI_F34_IS_UNLOCKED		(1 << 1)
#define RMI_F34_HAS_CONFIG_ID		(1 << 2)
#define RMI_F34_BLOCK_SIZE_OFFSET	1
#define RMI_F34_FW_BLOCKS_OFFSET	3
#define RMI_F34_CONFIG_BLOCKS_OFFSET	5

#define RMI_F34_BLOCK_SIZE_V1_OFFSET	0
#define RMI_F34_FW_BLOCKS_V1_OFFSET	0
#define RMI_F34_CONFIG_BLOCKS_V1_OFFSET	2

#define RMI_F34_BLOCK_DATA_OFFSET	2
#define RMI_F34_BLOCK_DATA_V1_OFFSET	1

#define RMI_F34_COMMAND_MASK		0x0F
#define RMI_F34_STATUS_MASK		0x07
#define RMI_F34_STATUS_SHIFT		4
#define RMI_F34_ENABLED_MASK		0x80

#define RMI_F34_COMMAND_V1_MASK		0x3F
#define RMI_F34_STATUS_V1_MASK		0x3F
#define RMI_F34_ENABLED_V1_MASK		0x80

#define RMI_F34_WRITE_FW_BLOCK        0x02
#define RMI_F34_ERASE_ALL             0x03
#define RMI_F34_WRITE_LOCKDOWN_BLOCK  0x04
#define RMI_F34_WRITE_CONFIG_BLOCK    0x06
#define RMI_F34_ENABLE_FLASH_PROG     0x0f

#define RMI_F34_ENABLE_WAIT_MS 300
#define RMI_F34_ERASE_WAIT_MS (5 * 1000)
#define RMI_F34_IDLE_WAIT_MS 500

/* Most recent device status event */
#define RMI_F01_STATUS_CODE(status)		((status) & 0x0f)
/* Indicates that flash programming is enabled (bootloader mode). */
#define RMI_F01_STATUS_BOOTLOADER(status)	(!!((status) & 0x40))
/* The device has lost its configuration for some reason. */
#define RMI_F01_STATUS_UNCONFIGURED(status)	(!!((status) & 0x80))

/*
 * Sleep mode controls power management on the device and affects all
 * functions of the device.
 */
#define RMI_F01_CTRL0_SLEEP_MODE_MASK	0x03

#define RMI_SLEEP_MODE_NORMAL		0x00
#define RMI_SLEEP_MODE_SENSOR_SLEEP	0x01
#define RMI_SLEEP_MODE_RESERVED0	0x02
#define RMI_SLEEP_MODE_RESERVED1	0x03

/*
 * This bit disables whatever sleep mode may be selected by the sleep_mode
 * field and forces the device to run at full power without sleeping.
 */
#define RMI_F01_CRTL0_NOSLEEP_BIT	(1 << 2)

int RMI4Update::UpdateFirmware(bool force, bool performLockdown)
{
	struct timespec start;
	struct timespec end;
	long long int duration_us = 0;
	int rc;
	const unsigned char eraseAll = RMI_F34_ERASE_ALL;

	rc = FindUpdateFunctions();
	if (rc != UPDATE_SUCCESS)
		return rc;

	rc = m_device.QueryBasicProperties();
	if (rc < 0)
		return UPDATE_FAIL_QUERY_BASIC_PROPERTIES;

	if (!force && m_firmwareImage.HasIO()) {
		if (m_firmwareImage.GetFirmwareID() <= m_device.GetFirmwareID()) {
			fprintf(stderr, "Firmware image (%ld) is not newer then the firmware on the device (%ld)\n",
				m_firmwareImage.GetFirmwareID(), m_device.GetFirmwareID());
			rc = UPDATE_FAIL_FIRMWARE_IMAGE_IS_OLDER;
			return rc;
		}
	}

	fprintf(stdout, "Device Properties:\n");
	m_device.PrintProperties();

	rc = DisableNonessentialInterupts();
	if (rc != UPDATE_SUCCESS)
		return rc;

	rc = ReadF34Queries();
	if (rc != UPDATE_SUCCESS)
		return rc;

	rc = m_firmwareImage.VerifyImageMatchesDevice(GetFirmwareSize(), GetConfigSize());
	if (rc != UPDATE_SUCCESS)
		return rc;

	rc = EnterFlashProgramming();
	if (rc != UPDATE_SUCCESS) {
		fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
		goto reset;
	}

	if (performLockdown && m_unlocked) {
		if (m_firmwareImage.GetLockdownData()) {
			fprintf(stdout, "Writing lockdown...\n");
			clock_gettime(CLOCK_MONOTONIC, &start);
			rc = WriteBlocks(m_firmwareImage.GetLockdownData(),
					m_firmwareImage.GetLockdownSize() / 0x10,
					RMI_F34_WRITE_LOCKDOWN_BLOCK);
			if (rc != UPDATE_SUCCESS) {
				fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
				goto reset;
			}
			clock_gettime(CLOCK_MONOTONIC, &end);
			duration_us = diff_time(&start, &end);
			fprintf(stdout, "Done writing lockdown, time: %lld us.\n", duration_us);
		}

		rc = EnterFlashProgramming();
		if (rc != UPDATE_SUCCESS) {
			fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
			goto reset;
		}
		
	}

	rc = WriteBootloaderID();
	if (rc != UPDATE_SUCCESS) {
		fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
		goto reset;
	}

	fprintf(stdout, "Erasing FW...\n");
	clock_gettime(CLOCK_MONOTONIC, &start);
	rc = m_device.Write(m_f34StatusAddr, &eraseAll, 1);
	if (rc != 1) {
		fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(UPDATE_FAIL_ERASE_ALL));
		rc = UPDATE_FAIL_ERASE_ALL;
		goto reset;
	}

	rc = WaitForIdle(RMI_F34_ERASE_WAIT_MS);
	if (rc != UPDATE_SUCCESS) {
		fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
		goto reset;
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	duration_us = diff_time(&start, &end);
	fprintf(stdout, "Erase complete, time: %lld us.\n", duration_us);

	if (m_firmwareImage.GetFirmwareData()) {
		fprintf(stdout, "Writing firmware...\n");
		clock_gettime(CLOCK_MONOTONIC, &start);
		rc = WriteBlocks(m_firmwareImage.GetFirmwareData(), m_fwBlockCount,
						RMI_F34_WRITE_FW_BLOCK);
		if (rc != UPDATE_SUCCESS) {
			fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
			goto reset;
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		duration_us = diff_time(&start, &end);
		fprintf(stdout, "Done writing FW, time: %lld us.\n", duration_us);
	}

	if (m_firmwareImage.GetConfigData()) {
		fprintf(stdout, "Writing configuration...\n");
		clock_gettime(CLOCK_MONOTONIC, &start);
		rc = WriteBlocks(m_firmwareImage.GetConfigData(), m_configBlockCount,
				RMI_F34_WRITE_CONFIG_BLOCK);
		if (rc != UPDATE_SUCCESS) {
			fprintf(stderr, "%s: %s\n", __func__, update_err_to_string(rc));
			goto reset;
		}
		clock_gettime(CLOCK_MONOTONIC, &end);
		duration_us = diff_time(&start, &end);
		fprintf(stdout, "Done writing config, time: %lld us.\n", duration_us);
	}

reset:
	m_device.Reset();
	m_device.RebindDriver();
	return rc;

}

int RMI4Update::DisableNonessentialInterupts()
{
	int rc;
	unsigned char interruptEnabeMask = m_f34.GetInterruptMask() | m_f01.GetInterruptMask();

	rc = m_device.Write(m_f01.GetControlBase() + 1, &interruptEnabeMask, 1);
	if (rc != 1)
		return rc;

	return UPDATE_SUCCESS;
}

int RMI4Update::FindUpdateFunctions()
{
	if (0 > m_device.ScanPDT())
		return UPDATE_FAIL_SCAN_PDT;

	if (!m_device.GetFunction(m_f01, 0x01))
		return UPDATE_FAIL_NO_FUNCTION_01;

	if (!m_device.GetFunction(m_f34, 0x34))
		return UPDATE_FAIL_NO_FUNCTION_34;

	return UPDATE_SUCCESS;
}

int RMI4Update::ReadF34Queries()
{
	int rc;
	unsigned char idStr[3];
	unsigned char buf[8];
	unsigned short queryAddr = m_f34.GetQueryBase();
	unsigned short f34Version = m_f34.GetFunctionVersion();
	unsigned short querySize;

	if (f34Version == 0x1)
		querySize = 8;
	else
		querySize = 2;

	rc = m_device.Read(queryAddr, m_bootloaderID, RMI_BOOTLOADER_ID_SIZE);
	if (rc != RMI_BOOTLOADER_ID_SIZE)
		return UPDATE_FAIL_READ_BOOTLOADER_ID;

	if (f34Version == 0x1)
		++queryAddr;
	else
		queryAddr += querySize;

	if (f34Version == 0x1) {
		rc = m_device.Read(queryAddr, buf, 1);
		if (rc != 1)
			return UPDATE_FAIL_READ_F34_QUERIES;

		m_hasNewRegmap = buf[0] & RMI_F34_HAS_NEW_REG_MAP;
		m_unlocked = buf[0] & RMI_F34_IS_UNLOCKED;;
		m_hasConfigID = buf[0] & RMI_F34_HAS_CONFIG_ID;

		++queryAddr;

		rc = m_device.Read(queryAddr, buf, 2);
		if (rc != 2)
			return UPDATE_FAIL_READ_F34_QUERIES;

		m_blockSize = extract_short(buf + RMI_F34_BLOCK_SIZE_V1_OFFSET);

		++queryAddr;

		rc = m_device.Read(queryAddr, buf, 8);
		if (rc != 8)
			return UPDATE_FAIL_READ_F34_QUERIES;

		m_fwBlockCount = extract_short(buf + RMI_F34_FW_BLOCKS_V1_OFFSET);
		m_configBlockCount = extract_short(buf + RMI_F34_CONFIG_BLOCKS_V1_OFFSET);
	} else {
		rc = m_device.Read(queryAddr, buf, RMI_F34_QUERY_SIZE);
		if (rc != RMI_F34_QUERY_SIZE)
			return UPDATE_FAIL_READ_F34_QUERIES;

		m_hasNewRegmap = buf[0] & RMI_F34_HAS_NEW_REG_MAP;
		m_unlocked = buf[0] & RMI_F34_IS_UNLOCKED;;
		m_hasConfigID = buf[0] & RMI_F34_HAS_CONFIG_ID;
		m_blockSize = extract_short(buf + RMI_F34_BLOCK_SIZE_OFFSET);
		m_fwBlockCount = extract_short(buf + RMI_F34_FW_BLOCKS_OFFSET);
		m_configBlockCount = extract_short(buf + RMI_F34_CONFIG_BLOCKS_OFFSET);
	}

	idStr[0] = m_bootloaderID[0];
	idStr[1] = m_bootloaderID[1];
	idStr[2] = 0;

	fprintf(stdout, "F34 bootloader id: %s (%#04x %#04x)\n", idStr, m_bootloaderID[0],
		m_bootloaderID[1]);
	fprintf(stdout, "F34 has config id: %d\n", m_hasConfigID);
	fprintf(stdout, "F34 unlocked:      %d\n", m_unlocked);
	fprintf(stdout, "F34 new reg map:   %d\n", m_hasNewRegmap);
	fprintf(stdout, "F34 block size:    %d\n", m_blockSize);
	fprintf(stdout, "F34 fw blocks:     %d\n", m_fwBlockCount);
	fprintf(stdout, "F34 config blocks: %d\n", m_configBlockCount);
	fprintf(stdout, "\n");

	if (f34Version == 0x1)
		m_f34StatusAddr = m_f34.GetDataBase() + 2;
	else
		m_f34StatusAddr = m_f34.GetDataBase() + RMI_F34_BLOCK_DATA_OFFSET + m_blockSize;

	return UPDATE_SUCCESS;
}

int RMI4Update::ReadF34Controls()
{
	int rc;
	unsigned char buf[2];

	if (m_f34.GetFunctionVersion() == 0x1) {
		rc = m_device.Read(m_f34StatusAddr, buf, 2);
		if (rc != 2)
			return UPDATE_FAIL_READ_F34_CONTROLS;

		m_f34Command = buf[0] & RMI_F34_COMMAND_V1_MASK;
		m_f34Status = buf[1] & RMI_F34_STATUS_V1_MASK;
		m_programEnabled = !!(buf[1] & RMI_F34_ENABLED_MASK);

	} else {
		rc = m_device.Read(m_f34StatusAddr, buf, 1);
		if (rc != 1)
			return UPDATE_FAIL_READ_F34_CONTROLS;

		m_f34Command = buf[0] & RMI_F34_COMMAND_MASK;
		m_f34Status = (buf[0] >> RMI_F34_STATUS_SHIFT) & RMI_F34_STATUS_MASK;
		m_programEnabled = !!(buf[0] & RMI_F34_ENABLED_MASK);
	}
	
	return UPDATE_SUCCESS;
}

int RMI4Update::WriteBootloaderID()
{
	int rc;
	int blockDataOffset = RMI_F34_BLOCK_DATA_OFFSET;

	if (m_f34.GetFunctionVersion() == 0x1)
		blockDataOffset = RMI_F34_BLOCK_DATA_V1_OFFSET;

	rc = m_device.Write(m_f34.GetDataBase() + blockDataOffset,
				m_bootloaderID, RMI_BOOTLOADER_ID_SIZE);
	if (rc != RMI_BOOTLOADER_ID_SIZE)
		return UPDATE_FAIL_WRITE_BOOTLOADER_ID;

	return UPDATE_SUCCESS;
}

int RMI4Update::EnterFlashProgramming()
{
	int rc;
	unsigned char f01Control_0;
	const unsigned char enableProg = RMI_F34_ENABLE_FLASH_PROG;

	rc = WriteBootloaderID();
	if (rc != UPDATE_SUCCESS)
		return rc;

	fprintf(stdout, "Enabling flash programming.\n");
	rc = m_device.Write(m_f34StatusAddr, &enableProg, 1);
	if (rc != 1)
		return UPDATE_FAIL_ENABLE_FLASH_PROGRAMMING;

	Sleep(RMI_F34_ENABLE_WAIT_MS);
	m_device.RebindDriver();
	rc = WaitForIdle(0);
	if (rc != UPDATE_SUCCESS)
		return UPDATE_FAIL_NOT_IN_IDLE_STATE;

	if (!m_programEnabled)
		return UPDATE_FAIL_PROGRAMMING_NOT_ENABLED;

	fprintf(stdout, "Programming is enabled.\n");
	rc = FindUpdateFunctions();
	if (rc != UPDATE_SUCCESS)
		return rc;

	rc = m_device.Read(m_f01.GetDataBase(), &m_deviceStatus, 1);
	if (rc != 1)
		return UPDATE_FAIL_READ_DEVICE_STATUS;

	if (!RMI_F01_STATUS_BOOTLOADER(m_deviceStatus))
		return UPDATE_FAIL_DEVICE_NOT_IN_BOOTLOADER;

	rc = ReadF34Queries();
	if (rc != UPDATE_SUCCESS)
		return rc;

	rc = m_device.Read(m_f01.GetControlBase(), &f01Control_0, 1);
	if (rc != 1)
		return UPDATE_FAIL_READ_F01_CONTROL_0;

	f01Control_0 |= RMI_F01_CRTL0_NOSLEEP_BIT;
	f01Control_0 = (f01Control_0 & ~RMI_F01_CTRL0_SLEEP_MODE_MASK) | RMI_SLEEP_MODE_NORMAL;

	rc = m_device.Write(m_f01.GetControlBase(), &f01Control_0, 1);
	if (rc != 1)
		return UPDATE_FAIL_WRITE_F01_CONTROL_0;

	return UPDATE_SUCCESS;
}

int RMI4Update::WriteBlocks(unsigned char *block, unsigned short count, unsigned char cmd)
{
	int blockNum;
	unsigned char zeros[] = { 0, 0 };
	int rc;
	unsigned short addr;
	unsigned char *blockWithCmd = (unsigned char *)alloca(m_blockSize + 1);

	if (m_f34.GetFunctionVersion() == 0x1)
		addr = m_f34.GetDataBase() + RMI_F34_BLOCK_DATA_V1_OFFSET;
	else
		addr = m_f34.GetDataBase() + RMI_F34_BLOCK_DATA_OFFSET;

	rc = m_device.Write(m_f34.GetDataBase(), zeros, 2);
	if (rc != 2)
		return UPDATE_FAIL_WRITE_INITIAL_ZEROS;

	for (blockNum = 0; blockNum < count; ++blockNum) {
		if (m_writeBlockWithCmd) {
			memcpy(blockWithCmd, block, m_blockSize);
			blockWithCmd[m_blockSize] = cmd;

			rc = m_device.Write(addr, blockWithCmd, m_blockSize + 1);
			if (rc != m_blockSize + 1) {
				fprintf(stderr, "failed to write block %d\n", blockNum);
				return UPDATE_FAIL_WRITE_BLOCK;
			}
		} else {
			rc = m_device.Write(addr, block, m_blockSize);
			if (rc != m_blockSize) {
				fprintf(stderr, "failed to write block %d\n", blockNum);
				return UPDATE_FAIL_WRITE_BLOCK;
			}

			rc = m_device.Write(m_f34StatusAddr, &cmd, 1);
			if (rc != 1) {
				fprintf(stderr, "failed to write command for block %d\n", blockNum);
				return UPDATE_FAIL_WRITE_FLASH_COMMAND;
			}
		}

		rc = WaitForIdle(RMI_F34_IDLE_WAIT_MS, !m_writeBlockWithCmd);
		if (rc != UPDATE_SUCCESS) {
			fprintf(stderr, "failed to go into idle after writing block %d\n", blockNum);
			return UPDATE_FAIL_NOT_IN_IDLE_STATE;
		}

		block += m_blockSize;
	}

	return UPDATE_SUCCESS;
}

/*
 * This is a limited implementation of WaitForIdle which assumes WaitForAttention is supported
 * this will be true for HID, but other protocols will need to revert polling. Polling
 * is not implemented yet.
 */
int RMI4Update::WaitForIdle(int timeout_ms, bool readF34OnSucess)
{
	int rc = 0;
	struct timeval tv;

	if (timeout_ms > 0) {
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		rc = m_device.WaitForAttention(&tv, m_f34.GetInterruptMask());
		if (rc == -ETIMEDOUT)
			/*
			 * If for some reason we are not getting attention reports for HID devices
			 * then we can still continue after the timeout and read F34 status
			 * but if we have to wait for the timeout to ellapse everytime then this
			 * will be slow. If this message shows up a lot then something is wrong
			 * with receiving attention reports and that should be fixed.
			 */
			fprintf(stderr, "Timed out waiting for attn report\n");
	}

	if (rc <= 0 || readF34OnSucess) {
		rc = ReadF34Controls();
		if (rc != UPDATE_SUCCESS)
			return rc;

		if (!m_f34Status && !m_f34Command) {
			if (!m_programEnabled) {
				fprintf(stderr, "Bootloader is idle but program_enabled bit isn't set.\n");
				return UPDATE_FAIL_PROGRAMMING_NOT_ENABLED;
			} else {
				return UPDATE_SUCCESS;
			}
		}

		fprintf(stderr, "ERROR: Waiting for idle status.\n");
		fprintf(stderr, "Command: %#04x\n", m_f34Command);
		fprintf(stderr, "Status:  %#04x\n", m_f34Status);
		fprintf(stderr, "Enabled: %d\n", m_programEnabled);
		fprintf(stderr, "Idle:    %d\n", !m_f34Command && !m_f34Status);

		return UPDATE_FAIL_NOT_IN_IDLE_STATE;
	}

	return UPDATE_SUCCESS;
}

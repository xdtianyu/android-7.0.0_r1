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

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "rmidevice.h"

#define RMI_DEVICE_PDT_ENTRY_SIZE		6
#define RMI_DEVICE_PAGE_SELECT_REGISTER		0xFF
#define RMI_DEVICE_MAX_PAGE			0xFF
#define RMI_DEVICE_PAGE_SIZE			0x100
#define RMI_DEVICE_PAGE_SCAN_START		0x00e9
#define RMI_DEVICE_PAGE_SCAN_END		0x0005
#define RMI_DEVICE_F01_BASIC_QUERY_LEN		11
#define RMI_DEVICE_F01_QRY5_YEAR_MASK		0x1f
#define RMI_DEVICE_F01_QRY6_MONTH_MASK		0x0f
#define RMI_DEVICE_F01_QRY7_DAY_MASK		0x1f

#define RMI_DEVICE_F01_QRY1_HAS_LTS		(1 << 2)
#define RMI_DEVICE_F01_QRY1_HAS_SENSOR_ID	(1 << 3)
#define RMI_DEVICE_F01_QRY1_HAS_CHARGER_INP	(1 << 4)
#define RMI_DEVICE_F01_QRY1_HAS_ADJ_DOZE	(1 << 5)
#define RMI_DEVICE_F01_QRY1_HAS_ADJ_DOZE_HOFF	(1 << 6)
#define RMI_DEVICE_F01_QRY1_HAS_PROPS_2		(1 << 7)

#define RMI_DEVICE_F01_LTS_RESERVED_SIZE	19

#define RMI_DEVICE_F01_QRY42_DS4_QUERIES	(1 << 0)
#define RMI_DEVICE_F01_QRY42_MULTI_PHYS		(1 << 1)

#define RMI_DEVICE_F01_QRY43_01_PACKAGE_ID     (1 << 0)
#define RMI_DEVICE_F01_QRY43_01_BUILD_ID       (1 << 1)

#define PACKAGE_ID_BYTES			4
#define BUILD_ID_BYTES				3

#define RMI_F01_CMD_DEVICE_RESET	1
#define RMI_F01_DEFAULT_RESET_DELAY_MS	100

int RMIDevice::SetRMIPage(unsigned char page)
{
	int rc;

	if (m_page == page)
		return 0;

	m_page = page;
	rc = Write(RMI_DEVICE_PAGE_SELECT_REGISTER, &page, 1);
	if (rc < 0 || rc < 1) {
		m_page = -1;
		return rc;
	}
	return 0;
}

int RMIDevice::QueryBasicProperties()
{
	int rc;
	unsigned char basicQuery[RMI_DEVICE_F01_BASIC_QUERY_LEN];
	unsigned short queryAddr;
	unsigned char infoBuf[4];
	unsigned short prodInfoAddr;
	RMIFunction f01;

	SetRMIPage(0x00);

	if (GetFunction(f01, 1)) {
		queryAddr = f01.GetQueryBase();

		rc = Read(queryAddr, basicQuery, RMI_DEVICE_F01_BASIC_QUERY_LEN);
		if (rc < 0 || rc < RMI_DEVICE_F01_BASIC_QUERY_LEN) {
			fprintf(stderr, "Failed to read the basic query: %s\n", strerror(errno));
			return rc;
		}
		m_manufacturerID = basicQuery[0];
		m_hasLTS = basicQuery[1] & RMI_DEVICE_F01_QRY1_HAS_LTS;
		m_hasSensorID = basicQuery[1] & RMI_DEVICE_F01_QRY1_HAS_SENSOR_ID;
		m_hasAdjustableDoze = basicQuery[1] & RMI_DEVICE_F01_QRY1_HAS_ADJ_DOZE;
		m_hasAdjustableDozeHoldoff = basicQuery[1] & RMI_DEVICE_F01_QRY1_HAS_ADJ_DOZE_HOFF;
		m_hasQuery42 = basicQuery[1] & RMI_DEVICE_F01_QRY1_HAS_PROPS_2;
		m_firmwareVersionMajor = basicQuery[2];
		m_firmwareVersionMinor = basicQuery[3];

		snprintf(m_dom, sizeof(m_dom), "20%02d/%02d/%02d",
				basicQuery[5] & RMI_DEVICE_F01_QRY5_YEAR_MASK,
		 		basicQuery[6] & RMI_DEVICE_F01_QRY6_MONTH_MASK,
		 		basicQuery[7] & RMI_DEVICE_F01_QRY7_DAY_MASK);

		queryAddr += 11;
		rc = Read(queryAddr, m_productID, RMI_PRODUCT_ID_LENGTH);
		if (rc < 0 || rc < RMI_PRODUCT_ID_LENGTH) {
			fprintf(stderr, "Failed to read the product id: %s\n", strerror(errno));
			return rc;
		}
		m_productID[RMI_PRODUCT_ID_LENGTH] = '\0';

		prodInfoAddr = queryAddr + 6;
		queryAddr += 10;

		if (m_hasLTS)
			++queryAddr;

		if (m_hasSensorID) {
			rc = Read(queryAddr++, &m_sensorID, 1);
			if (rc < 0 || rc < 1) {
				fprintf(stderr, "Failed to read sensor id: %s\n", strerror(errno));
				return rc;
			}
		}

		if (m_hasLTS)
			queryAddr += RMI_DEVICE_F01_LTS_RESERVED_SIZE;

		if (m_hasQuery42) {
			rc = Read(queryAddr++, infoBuf, 1);
			if (rc < 0 || rc < 1) {
				fprintf(stderr, "Failed to read query 42: %s\n", strerror(errno));
				return rc;
			}

			m_hasDS4Queries = infoBuf[0] & RMI_DEVICE_F01_QRY42_DS4_QUERIES;
			m_hasMultiPhysical = infoBuf[0] & RMI_DEVICE_F01_QRY42_MULTI_PHYS;
		}

		if (m_hasDS4Queries) {
			rc = Read(queryAddr++, &m_ds4QueryLength, 1);
			if (rc < 0 || rc < 1) {
				fprintf(stderr, "Failed to read DS4 query length: %s\n", strerror(errno));
				return rc;
			}
		}

		for (int i = 1; i <= m_ds4QueryLength; ++i) {
			unsigned char val;
			rc = Read(queryAddr++, &val, 1);
			if (rc < 0 || rc < 1) {
				fprintf(stderr, "Failed to read F01 Query43.%02d: %s\n", i, strerror(errno));
				continue;
			}

			switch(i) {
				case 1:
					m_hasPackageIDQuery = val & RMI_DEVICE_F01_QRY43_01_PACKAGE_ID;
					m_hasBuildIDQuery = val & RMI_DEVICE_F01_QRY43_01_BUILD_ID;
					break;
				case 2:
				case 3:
				default:
					break;
			}
		}

		if (m_hasPackageIDQuery) {
			rc = Read(prodInfoAddr++, infoBuf, PACKAGE_ID_BYTES);
			if (rc >= PACKAGE_ID_BYTES) {
				unsigned short *val = (unsigned short *)infoBuf;
				m_packageID = *val;
				val = (unsigned short *)(infoBuf + 2);
				m_packageRev = *val;
			}
		}

		if (m_hasBuildIDQuery) {
			rc = Read(prodInfoAddr, infoBuf, BUILD_ID_BYTES);
			if (rc >= BUILD_ID_BYTES) {
				unsigned short *val = (unsigned short *)infoBuf;
				m_buildID = *val;
				m_buildID += infoBuf[2] * 65536;
			}
		}
	}
	return 0;
}

void RMIDevice::PrintProperties()
{
	fprintf(stdout, "manufacturerID:\t\t%d\n", m_manufacturerID);
	fprintf(stdout, "Has LTS?:\t\t%d\n", m_hasLTS);
	fprintf(stdout, "Has Sensor ID?:\t\t%d\n", m_hasSensorID);
	fprintf(stdout, "Has Adjustable Doze?:\t%d\n", m_hasAdjustableDoze);
	fprintf(stdout, "Has Query 42?:\t\t%d\n", m_hasQuery42);
	fprintf(stdout, "Date of Manufacturer:\t%s\n", m_dom);
	fprintf(stdout, "Product ID:\t\t%s\n", m_productID);
	fprintf(stdout, "Firmware Version:\t%d.%d\n", m_firmwareVersionMajor, m_firmwareVersionMinor);
	fprintf(stdout, "Package ID:\t\t%d\n", m_packageID);
	fprintf(stdout, "Package Rev:\t\t%d\n", m_packageRev);
	fprintf(stdout, "Build ID:\t\t%ld\n", m_buildID);
	fprintf(stdout, "Sensor ID:\t\t%d\n", m_sensorID);
	fprintf(stdout, "Has DS4 Queries?:\t%d\n", m_hasDS4Queries);
	fprintf(stdout, "Has Multi Phys?:\t%d\n", m_hasMultiPhysical);
	fprintf(stdout, "\n");
}

int RMIDevice::Reset()
{
	int rc;
	RMIFunction f01;
	const unsigned char deviceReset = RMI_F01_CMD_DEVICE_RESET;

	if (!GetFunction(f01, 1))
		return -1;

	fprintf(stdout, "Resetting...\n");
	rc = Write(f01.GetCommandBase(), &deviceReset, 1);
	if (rc < 0 || rc < 1)
		return rc;

	rc = Sleep(RMI_F01_DEFAULT_RESET_DELAY_MS);
	if (rc < 0)
		return -1;
	fprintf(stdout, "Reset completed.\n");
	return 0;
}

bool RMIDevice::GetFunction(RMIFunction &func, int functionNumber)
{
	std::vector<RMIFunction>::iterator funcIter;

	for (funcIter = m_functionList.begin(); funcIter != m_functionList.end(); ++funcIter) {
		if (funcIter->GetFunctionNumber() == functionNumber) {
			func = *funcIter;
			return true;
		}
	}
	return false;
}

void RMIDevice::PrintFunctions()
{
	std::vector<RMIFunction>::iterator funcIter;

	for (funcIter = m_functionList.begin(); funcIter != m_functionList.end(); ++funcIter)
		fprintf(stdout, "0x%02x (%d) (%d) (0x%x): 0x%02x 0x%02x 0x%02x 0x%02x\n",
				funcIter->GetFunctionNumber(), funcIter->GetFunctionVersion(),
				funcIter->GetInterruptSourceCount(),
				funcIter->GetInterruptMask(),
				funcIter->GetDataBase(),
				funcIter->GetControlBase(), funcIter->GetCommandBase(),
				funcIter->GetQueryBase());
}

int RMIDevice::ScanPDT(int endFunc, int endPage)
{
	int rc;
	unsigned int page;
	unsigned int maxPage;
	unsigned int addr;
	unsigned char entry[RMI_DEVICE_PDT_ENTRY_SIZE];
	unsigned int interruptCount = 0;

	maxPage = (unsigned int)((endPage < 0) ? RMI_DEVICE_MAX_PAGE : endPage);

	m_functionList.clear();

	for (page = 0; page < maxPage; ++page) {
		unsigned int page_start = RMI_DEVICE_PAGE_SIZE * page;
		unsigned int pdt_start = page_start + RMI_DEVICE_PAGE_SCAN_START;
		unsigned int pdt_end = page_start + RMI_DEVICE_PAGE_SCAN_END;
		bool found = false;

		SetRMIPage(page);

		for (addr = pdt_start; addr >= pdt_end; addr -= RMI_DEVICE_PDT_ENTRY_SIZE) {
			rc = Read(addr, entry, RMI_DEVICE_PDT_ENTRY_SIZE);
			if (rc < 0 || rc < RMI_DEVICE_PDT_ENTRY_SIZE) {
				fprintf(stderr, "Failed to read PDT entry at address (0x%04x)\n", addr);
				return rc;
			}
			
			RMIFunction func(entry, page_start, interruptCount);
			if (func.GetFunctionNumber() == 0)
				break;

			m_functionList.push_back(func);
			interruptCount += func.GetInterruptSourceCount();
			found = true;

			if (func.GetFunctionNumber() == endFunc)
				return 0;
		}

		if (!found && (endPage < 0))
			break;
	}

	m_numInterruptRegs = (interruptCount + 7) / 8;

	return 0;
}

bool RMIDevice::InBootloader()
{
	RMIFunction f01;
	if (GetFunction(f01, 0x01)) {
		int rc;
		unsigned char status;

		rc = Read(f01.GetDataBase(), &status, 1);
		if (rc < 0 || rc < 1)
			return true;

		return !!(status & 0x40);
	}
	return true;
}

long long diff_time(struct timespec *start, struct timespec *end)
{
	long long diff;
	diff = (end->tv_sec - start->tv_sec) * 1000 * 1000;
	diff += (end->tv_nsec - start->tv_nsec) / 1000;
	return diff;
}

int Sleep(int ms)
{
	struct timespec ts;
	struct timespec rem;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000 * 1000;
	for (;;) {
		if (nanosleep(&ts, &rem) == 0) {
			break;
		} else {
			if (errno == EINTR) {
				ts = rem;
				continue;
			}
			return -1;
		}
	}
	return 0;
}

void print_buffer(const unsigned char *buf, unsigned int len)
{
	for (unsigned int i = 0; i < len; ++i) {
		fprintf(stdout, "0x%02X ", buf[i]);
		if (i % 8 == 7)
			fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");
}

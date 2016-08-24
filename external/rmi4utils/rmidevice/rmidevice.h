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

#ifndef _RMIDEVICE_H_
#define _RMIDEVICE_H_

#include <cstddef>
#include <vector>

#include "rmifunction.h"

#define RMI_PRODUCT_ID_LENGTH		10

#define RMI_INTERUPT_SOURCES_ALL_MASK	0xFFFFFFFF

class RMIDevice
{
public:
	RMIDevice() : m_functionList(), m_sensorID(0), m_bCancel(false), m_bytesPerReadRequest(0), m_page(-1)
	{}
	virtual ~RMIDevice() {}
	virtual int Open(const char * filename) = 0;
	virtual int Read(unsigned short addr, unsigned char *data,
				unsigned short len) = 0;
	virtual int Write(unsigned short addr, const unsigned char *data,
				 unsigned short len) = 0;
	virtual int SetMode(int mode) { return -1; /* Unsupported */ }
	virtual int WaitForAttention(struct timeval * timeout = NULL,
			unsigned int source_mask = RMI_INTERUPT_SOURCES_ALL_MASK) = 0;
	virtual int GetAttentionReport(struct timeval * timeout, unsigned int source_mask,
					unsigned char *buf, unsigned int *len)
	{ return -1; /* Unsupported */ }
	virtual void Close() = 0;
	virtual void Cancel() { m_bCancel = true; }
	virtual void RebindDriver() = 0;

	unsigned long GetFirmwareID() { return m_buildID; }
	int GetFirmwareVersionMajor() { return m_firmwareVersionMajor; }
	int GetFirmwareVersionMinor() { return m_firmwareVersionMinor; }
	virtual int QueryBasicProperties();
	
	int SetRMIPage(unsigned char page);
	
	int ScanPDT(int endFunc = 0, int endPage = -1);
	void PrintProperties();
	virtual void PrintDeviceInfo() = 0;
	int Reset();

	bool InBootloader();

	bool GetFunction(RMIFunction &func, int functionNumber);
	void PrintFunctions();

	void SetBytesPerReadRequest(int bytes) { m_bytesPerReadRequest = bytes; }

	unsigned int GetNumInterruptRegs() { return m_numInterruptRegs; }

protected:
	std::vector<RMIFunction> m_functionList;
	unsigned char m_manufacturerID;
	bool m_hasLTS;
	bool m_hasSensorID;
	bool m_hasAdjustableDoze;
	bool m_hasAdjustableDozeHoldoff;
	bool m_hasQuery42;
	char m_dom[11];
	unsigned char m_productID[RMI_PRODUCT_ID_LENGTH + 1];
	unsigned short m_packageID;
	unsigned short m_packageRev;
	unsigned long m_buildID;
	unsigned char m_sensorID;
	unsigned long m_boardID;

	int m_firmwareVersionMajor;
	int m_firmwareVersionMinor;

	bool m_hasDS4Queries;
	bool m_hasMultiPhysical;

	unsigned char m_ds4QueryLength;

	bool m_hasPackageIDQuery;
	bool m_hasBuildIDQuery;

	bool m_bCancel;
	int m_bytesPerReadRequest;
	int m_page;

	unsigned int m_numInterruptRegs;
 };

/* Utility Functions */
long long diff_time(struct timespec *start, struct timespec *end);
int Sleep(int ms);
void print_buffer(const unsigned char *buf, unsigned int len);
#endif /* _RMIDEVICE_H_ */

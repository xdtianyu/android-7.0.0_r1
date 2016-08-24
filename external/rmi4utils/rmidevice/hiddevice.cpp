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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
#include <signal.h>
#include <stdlib.h>

#include "hiddevice.h"

#define RMI_WRITE_REPORT_ID                 0x9 // Output Report
#define RMI_READ_ADDR_REPORT_ID             0xa // Output Report
#define RMI_READ_DATA_REPORT_ID             0xb // Input Report
#define RMI_ATTN_REPORT_ID                  0xc // Input Report
#define RMI_SET_RMI_MODE_REPORT_ID          0xf // Feature Report

enum rmi_hid_mode_type {
	HID_RMI4_MODE_MOUSE                     = 0,
	HID_RMI4_MODE_ATTN_REPORTS              = 1,
	HID_RMI4_MODE_NO_PACKED_ATTN_REPORTS    = 2,
};

enum hid_report_type {
	HID_REPORT_TYPE_UNKNOWN			= 0x0,
	HID_REPORT_TYPE_INPUT			= 0x81,
	HID_REPORT_TYPE_OUTPUT			= 0x91,
	HID_REPORT_TYPE_FEATURE			= 0xb1,
};

#define HID_RMI4_REPORT_ID			0
#define HID_RMI4_READ_INPUT_COUNT		1
#define HID_RMI4_READ_INPUT_DATA		2
#define HID_RMI4_READ_OUTPUT_ADDR		2
#define HID_RMI4_READ_OUTPUT_COUNT		4
#define HID_RMI4_WRITE_OUTPUT_COUNT		1
#define HID_RMI4_WRITE_OUTPUT_ADDR		2
#define HID_RMI4_WRITE_OUTPUT_DATA		4
#define HID_RMI4_FEATURE_MODE			1
#define HID_RMI4_ATTN_INTERUPT_SOURCES		1
#define HID_RMI4_ATTN_DATA			2

#define SYNAPTICS_VENDOR_ID			0x06cb

int HIDDevice::Open(const char * filename)
{
	int rc;
	int desc_size;

	if (!filename)
		return -EINVAL;

	m_fd = open(filename, O_RDWR);
	if (m_fd < 0)
		return -1;

	memset(&m_rptDesc, 0, sizeof(m_rptDesc));
	memset(&m_info, 0, sizeof(m_info));

	rc = ioctl(m_fd, HIDIOCGRDESCSIZE, &desc_size);
	if (rc < 0)
		return rc;
	
	m_rptDesc.size = desc_size;
	rc = ioctl(m_fd, HIDIOCGRDESC, &m_rptDesc);
	if (rc < 0)
		return rc;
	
	rc = ioctl(m_fd, HIDIOCGRAWINFO, &m_info);
	if (rc < 0)
		return rc;

	if (m_info.vendor != SYNAPTICS_VENDOR_ID) {
		errno = -ENODEV;
		return -1;
	}

	ParseReportSizes();

	m_inputReport = new unsigned char[m_inputReportSize]();
	if (!m_inputReport) {
		errno = -ENOMEM;
		return -1;
	}

	m_outputReport = new unsigned char[m_outputReportSize]();
	if (!m_outputReport) {
		errno = -ENOMEM;
		return -1;
	}

	m_readData = new unsigned char[m_inputReportSize]();
	if (!m_readData) {
		errno = -ENOMEM;
		return -1;
	}

	m_attnData = new unsigned char[m_inputReportSize]();
	if (!m_attnData) {
		errno = -ENOMEM;
		return -1;
	}

	m_deviceOpen = true;

	rc = SetMode(HID_RMI4_MODE_ATTN_REPORTS);
	if (rc)
		return -1;

	return 0;
}

void HIDDevice::ParseReportSizes()
{
	bool isVendorSpecific = false;
	bool isReport = false;
	int totalReportSize = 0;
	int reportSize = 0;
	int reportCount = 0;
	enum hid_report_type hidReportType = HID_REPORT_TYPE_UNKNOWN;

	for (unsigned int i = 0; i < m_rptDesc.size; ++i) {
		if (isVendorSpecific) {
			if (m_rptDesc.value[i] == 0x85 || m_rptDesc.value[i] == 0xc0) {
				if (isReport) {
					// finish up data on the previous report
					totalReportSize = (reportSize * reportCount) >> 3;

					switch (hidReportType) {
						case HID_REPORT_TYPE_INPUT:
							m_inputReportSize = totalReportSize + 1;
							break;
						case HID_REPORT_TYPE_OUTPUT:
							m_outputReportSize = totalReportSize + 1;
							break;
						case HID_REPORT_TYPE_FEATURE:
							m_featureReportSize = totalReportSize + 1;
							break;
						case HID_REPORT_TYPE_UNKNOWN:
						default:
							break;
					}
				}

				// reset values for the new report
				totalReportSize = 0;
				reportSize = 0;
				reportCount = 0;
				hidReportType = HID_REPORT_TYPE_UNKNOWN;

				if (m_rptDesc.value[i] == 0x85)
					isReport = true;
				else
					isReport = false;

				if (m_rptDesc.value[i] == 0xc0)
					isVendorSpecific = false;
			}

			if (isReport) {
				if (m_rptDesc.value[i] == 0x75) {
					if (i + 1 >= m_rptDesc.size)
						return;
					reportSize = m_rptDesc.value[++i];
					continue;
				}

				if (m_rptDesc.value[i] == 0x95) {
					if (i + 1 >= m_rptDesc.size)
						return;
					reportCount = m_rptDesc.value[++i];
					continue;
				}

				if (m_rptDesc.value[i] == HID_REPORT_TYPE_INPUT)
					hidReportType = HID_REPORT_TYPE_INPUT;

				if (m_rptDesc.value[i] == HID_REPORT_TYPE_OUTPUT)
					hidReportType = HID_REPORT_TYPE_OUTPUT;

				if (m_rptDesc.value[i] == HID_REPORT_TYPE_FEATURE) {
					hidReportType = HID_REPORT_TYPE_FEATURE;
				}
			}
		}

		if (i + 2 >= m_rptDesc.size)
			return;
		if (m_rptDesc.value[i] == 0x06 && m_rptDesc.value[i + 1] == 0x00
						&& m_rptDesc.value[i + 2] == 0xFF) {
			isVendorSpecific = true;
			i += 2;
		}
	}
}

int HIDDevice::Read(unsigned short addr, unsigned char *buf, unsigned short len)
{
	ssize_t count;
	size_t bytesReadPerRequest;
	size_t bytesInDataReport;
	size_t totalBytesRead;
	size_t bytesPerRequest;
	size_t bytesWritten;
	size_t bytesToRequest;
	int reportId;
	int rc;

	if (!m_deviceOpen)
		return -1;

	if (m_bytesPerReadRequest)
		bytesPerRequest = m_bytesPerReadRequest;
	else
		bytesPerRequest = len;

	for (totalBytesRead = 0; totalBytesRead < len; totalBytesRead += bytesReadPerRequest) {
		count = 0;
		if ((len - totalBytesRead) < bytesPerRequest)
			bytesToRequest = len % bytesPerRequest;
		else
			bytesToRequest = bytesPerRequest;

		if (m_outputReportSize < HID_RMI4_READ_OUTPUT_COUNT + 2) {
			return -1;
		}
		m_outputReport[HID_RMI4_REPORT_ID] = RMI_READ_ADDR_REPORT_ID;
		m_outputReport[1] = 0; /* old 1 byte read count */
		m_outputReport[HID_RMI4_READ_OUTPUT_ADDR] = addr & 0xFF;
		m_outputReport[HID_RMI4_READ_OUTPUT_ADDR + 1] = (addr >> 8) & 0xFF;
		m_outputReport[HID_RMI4_READ_OUTPUT_COUNT] = bytesToRequest  & 0xFF;
		m_outputReport[HID_RMI4_READ_OUTPUT_COUNT + 1] = (bytesToRequest >> 8) & 0xFF;

		m_dataBytesRead = 0;

		for (bytesWritten = 0; bytesWritten < m_outputReportSize; bytesWritten += count) {
			m_bCancel = false;
			count = write(m_fd, m_outputReport + bytesWritten,
					m_outputReportSize - bytesWritten);
			if (count < 0) {
				if (errno == EINTR && m_deviceOpen && !m_bCancel)
					continue;
				else
					return count;
			}
			break;
		}

		bytesReadPerRequest = 0;
		while (bytesReadPerRequest < bytesToRequest) {
			rc = GetReport(&reportId);
			if (rc > 0 && reportId == RMI_READ_DATA_REPORT_ID) {
				if (static_cast<ssize_t>(m_inputReportSize) <
				    std::max(HID_RMI4_READ_INPUT_COUNT,
					     HID_RMI4_READ_INPUT_DATA))
					return -1;
				bytesInDataReport = m_readData[HID_RMI4_READ_INPUT_COUNT];
				if (bytesInDataReport > bytesToRequest
				    || bytesReadPerRequest + bytesInDataReport > len)
					return -1;
				memcpy(buf + bytesReadPerRequest, &m_readData[HID_RMI4_READ_INPUT_DATA],
					bytesInDataReport);
				bytesReadPerRequest += bytesInDataReport;
				m_dataBytesRead = 0;
			}
		}
		addr += bytesPerRequest;
	}

	return totalBytesRead;
}

int HIDDevice::Write(unsigned short addr, const unsigned char *buf, unsigned short len)
{
	ssize_t count;

	if (!m_deviceOpen)
		return -1;

	if (static_cast<ssize_t>(m_outputReportSize) <
	    HID_RMI4_WRITE_OUTPUT_DATA + len)
		return -1;
	m_outputReport[HID_RMI4_REPORT_ID] = RMI_WRITE_REPORT_ID;
	m_outputReport[HID_RMI4_WRITE_OUTPUT_COUNT] = len;
	m_outputReport[HID_RMI4_WRITE_OUTPUT_ADDR] = addr & 0xFF;
	m_outputReport[HID_RMI4_WRITE_OUTPUT_ADDR + 1] = (addr >> 8) & 0xFF;
	memcpy(&m_outputReport[HID_RMI4_WRITE_OUTPUT_DATA], buf, len);

	for (;;) {
		m_bCancel = false;
		count = write(m_fd, m_outputReport, m_outputReportSize);
		if (count < 0) {
			if (errno == EINTR && m_deviceOpen && !m_bCancel)
				continue;
			else
				return count;
		}
		return len;
	}
}

int HIDDevice::SetMode(int mode)
{
	int rc;
	char buf[2];

	if (!m_deviceOpen)
		return -1;

	buf[0] = 0xF;
	buf[1] = mode;
	rc = ioctl(m_fd, HIDIOCSFEATURE(2), buf);
	if (rc < 0) {
		perror("HIDIOCSFEATURE");
		return rc;
	}

	return 0;
}

void HIDDevice::Close()
{
	if (!m_deviceOpen)
		return;

	SetMode(HID_RMI4_MODE_MOUSE);
	m_deviceOpen = false;
	close(m_fd);
	m_fd = -1;

	delete[] m_inputReport;
	m_inputReport = NULL;
	delete[] m_outputReport;
	m_outputReport = NULL;
	delete[] m_readData;
	m_readData = NULL;
	delete[] m_attnData;
	m_attnData = NULL;
}

int HIDDevice::WaitForAttention(struct timeval * timeout, unsigned int source_mask)
{
	return GetAttentionReport(timeout, source_mask, NULL, NULL);
}

int HIDDevice::GetAttentionReport(struct timeval * timeout, unsigned int source_mask,
					unsigned char *buf, unsigned int *len)
{
	int rc = 0;
	int reportId;

	// Assume the Linux implementation of select with timeout set to the
	// time remaining.
	while (!timeout || (timeout->tv_sec != 0 || timeout->tv_usec != 0)) {
		rc = GetReport(&reportId, timeout);
		if (rc > 0) {
			if (reportId == RMI_ATTN_REPORT_ID) {
				// If a valid buffer is passed in then copy the data from
				// the attention report into it. If the buffer is
				// too small simply set *len to 0 to indicate nothing
				// was copied. Some callers won't care about the contents
				// of the report so failing to copy the data should not return
				// an error.
				if (buf && len) {
					if (*len >= m_inputReportSize) {
						*len = m_inputReportSize;
						memcpy(buf, m_attnData, *len);
					} else {
						*len = 0;
					}
				}

				if (m_inputReportSize < HID_RMI4_ATTN_INTERUPT_SOURCES + 1)
					return -1;

				if (source_mask & m_attnData[HID_RMI4_ATTN_INTERUPT_SOURCES])
					return rc;
			}
		} else {
			return rc;
		}
	}

	return rc;
}

int HIDDevice::GetReport(int *reportId, struct timeval * timeout)
{
	ssize_t count = 0;
	fd_set fds;
	int rc;

	if (!m_deviceOpen)
		return -1;

	if (m_inputReportSize < HID_RMI4_REPORT_ID + 1)
		return -1;

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(m_fd, &fds);

		rc = select(m_fd + 1, &fds, NULL, NULL, timeout);
		if (rc == 0) {
			return -ETIMEDOUT;
		} else if (rc < 0) {
			if (errno == EINTR && m_deviceOpen && !m_bCancel)
				continue;
			else
				return rc;
		} else if (rc > 0 && FD_ISSET(m_fd, &fds)) {
			size_t offset = 0;
			for (;;) {
				m_bCancel = false;
				count = read(m_fd, m_inputReport + offset, m_inputReportSize - offset);
				if (count < 0) {
					if (errno == EINTR && m_deviceOpen && !m_bCancel)
						continue;
					else
						return count;
				}
				offset += count;
				if (offset == m_inputReportSize)
					break;
			}
			count = offset;
		}
		break;
	}

	if (reportId)
		*reportId = m_inputReport[HID_RMI4_REPORT_ID];

	if (m_inputReport[HID_RMI4_REPORT_ID] == RMI_ATTN_REPORT_ID) {
		if (static_cast<ssize_t>(m_inputReportSize) < count)
			return -1;
		memcpy(m_attnData, m_inputReport, count);
	} else if (m_inputReport[HID_RMI4_REPORT_ID] == RMI_READ_DATA_REPORT_ID) {
		if (static_cast<ssize_t>(m_inputReportSize) < count)
			return -1;
		memcpy(m_readData, m_inputReport, count);
		m_dataBytesRead = count;
	}
	return 1;
}

void HIDDevice::PrintReport(const unsigned char *report)
{
	int i;
	int len = 0;
	const unsigned char * data;
	int addr = 0;

	switch (report[HID_RMI4_REPORT_ID]) {
		case RMI_WRITE_REPORT_ID:
			len = report[HID_RMI4_WRITE_OUTPUT_COUNT];
			data = &report[HID_RMI4_WRITE_OUTPUT_DATA];
			addr = (report[HID_RMI4_WRITE_OUTPUT_ADDR] & 0xFF)
				| ((report[HID_RMI4_WRITE_OUTPUT_ADDR + 1] & 0xFF) << 8);
			fprintf(stdout, "Write Report:\n");
			fprintf(stdout, "Address = 0x%02X\n", addr);
			fprintf(stdout, "Length = 0x%02X\n", len);
			break;
		case RMI_READ_ADDR_REPORT_ID:
			addr = (report[HID_RMI4_READ_OUTPUT_ADDR] & 0xFF)
				| ((report[HID_RMI4_READ_OUTPUT_ADDR + 1] & 0xFF) << 8);
			len = (report[HID_RMI4_READ_OUTPUT_COUNT] & 0xFF)
				| ((report[HID_RMI4_READ_OUTPUT_COUNT + 1] & 0xFF) << 8);
			fprintf(stdout, "Read Request (Output Report):\n");
			fprintf(stdout, "Address = 0x%02X\n", addr);
			fprintf(stdout, "Length = 0x%02X\n", len);
			return;
			break;
		case RMI_READ_DATA_REPORT_ID:
			len = report[HID_RMI4_READ_INPUT_COUNT];
			data = &report[HID_RMI4_READ_INPUT_DATA];
			fprintf(stdout, "Read Data Report:\n");
			fprintf(stdout, "Length = 0x%02X\n", len);
			break;
		case RMI_ATTN_REPORT_ID:
			fprintf(stdout, "Attention Report:\n");
			len = 28;
			data = &report[HID_RMI4_ATTN_DATA];
			fprintf(stdout, "Interrupt Sources: 0x%02X\n", 
				report[HID_RMI4_ATTN_INTERUPT_SOURCES]);
			break;
		default:
			fprintf(stderr, "Unknown Report: ID 0x%02x\n", report[HID_RMI4_REPORT_ID]);
			return;
	}

	fprintf(stdout, "Data:\n");
	for (i = 0; i < len; ++i) {
		fprintf(stdout, "0x%02X ", data[i]);
		if (i % 8 == 7) {
			fprintf(stdout, "\n");
		}
	}
	fprintf(stdout, "\n\n");
}

// Print protocol specific device information
void HIDDevice::PrintDeviceInfo()
{
	fprintf(stdout, "HID device info:\nBus: %s Vendor: 0x%04x Product: 0x%04x\n",
		m_info.bustype == BUS_I2C ? "I2C" : "USB", m_info.vendor, m_info.product);
	fprintf(stdout, "Report sizes: input: %ld output: %ld\n", (unsigned long)m_inputReportSize,
		(unsigned long)m_outputReportSize);
}

bool WriteDeviceNameToFile(const char * file, const char * str)
{
	int fd;
	ssize_t size;

	fd = open(file, O_WRONLY);
	if (fd < 0)
		return false;

	for (;;) {
		size = write(fd, str, strlen(str));
		if (size < 0) {
			if (errno == EINTR)
				continue;

			return false;
		}
		break;
	}

	return close(fd) == 0 && size == static_cast<ssize_t>(strlen(str));
}

void HIDDevice::RebindDriver()
{
	int bus = m_info.bustype;
	int vendor = m_info.vendor;
	int product = m_info.product;
	std::string hidDeviceName;
	std::string transportDeviceName;
	std::string driverPath;
	std::string bindFile;
	std::string unbindFile;
	std::string hidrawFile;
	struct stat stat_buf;
	int rc;
	int i;

	Close();

	if (!LookupHidDeviceName(bus, vendor, product, hidDeviceName)) {
		fprintf(stderr, "Failed to find HID device name for the specified device: bus (0x%x) vendor: (0x%x) product: (0x%x)\n",
			bus, vendor, product);
		return;
	}

	if (!FindTransportDevice(bus, hidDeviceName, transportDeviceName, driverPath)) {
		fprintf(stderr, "Failed to find the transport device / driver for %s\n", hidDeviceName.c_str());
		return;
	}

	bindFile = driverPath + "bind";
	unbindFile = driverPath + "unbind";

	if (!WriteDeviceNameToFile(unbindFile.c_str(), transportDeviceName.c_str())) {
		fprintf(stderr, "Failed to unbind HID device %s: %s\n",
			transportDeviceName.c_str(), strerror(errno));
		return;
	}

	if (!WriteDeviceNameToFile(bindFile.c_str(), transportDeviceName.c_str())) {
		fprintf(stderr, "Failed to bind HID device %s: %s\n",
			transportDeviceName.c_str(), strerror(errno));
		return;
	}

	// The hid device id has changed since this is now a new hid device. Now we have to look up the new name.
	if (!LookupHidDeviceName(bus, vendor, product, hidDeviceName)) {
		fprintf(stderr, "Failed to find HID device name for the specified device: bus (0x%x) vendor: (0x%x) product: (0x%x)\n",
			bus, vendor, product);
		return;
	}

	if (!FindHidRawFile(hidDeviceName, hidrawFile)) {
		fprintf(stderr, "Failed to find the hidraw device file for %s\n", hidDeviceName.c_str());
		return;
	}

	for (i = 0; i < 200; i++) {
		rc = stat(hidrawFile.c_str(), &stat_buf);
		if (!rc)
			break;
		Sleep(5);
	}

	rc = Open(hidrawFile.c_str());
	if (rc)
		fprintf(stderr, "Failed to open device (%s) during rebind: %d: errno: %s (%d)\n",
				hidrawFile.c_str(), rc, strerror(errno), errno);
}

bool HIDDevice::FindTransportDevice(int bus, std::string & hidDeviceName,
			std::string & transportDeviceName, std::string & driverPath)
{
	std::string devicePrefix = "/sys/bus/";
	std::string devicePath;
	struct dirent * devicesDirEntry;
	DIR * devicesDir;
	struct dirent * devDirEntry;
	DIR * devDir;
	bool deviceFound = false;
	ssize_t sz;

	if (bus == BUS_I2C) {
		devicePrefix += "i2c/";
		driverPath = devicePrefix + "drivers/i2c_hid/";
	} else {
		devicePrefix += "usb/";
		driverPath = devicePrefix + "drivers/usbhid/";
	}
	devicePath = devicePrefix + "devices/";

	devicesDir = opendir(devicePath.c_str());
	if (!devicesDir)
		return false;

	while((devicesDirEntry = readdir(devicesDir)) != NULL) {
		if (devicesDirEntry->d_type != DT_LNK)
			continue;

		char buf[PATH_MAX];

		sz = readlinkat(dirfd(devicesDir), devicesDirEntry->d_name, buf, PATH_MAX);
		if (sz < 0)
			continue;

		buf[sz] = 0;

		std::string fullLinkPath = devicePath + buf;
		devDir = opendir(fullLinkPath.c_str());
		if (!devDir) {
			fprintf(stdout, "opendir failed\n");
			continue;
		}

		while ((devDirEntry = readdir(devDir)) != NULL) {
			if (!strcmp(devDirEntry->d_name, hidDeviceName.c_str())) {
				transportDeviceName = devicesDirEntry->d_name;
				deviceFound = true;
				break;
			}
		}
		closedir(devDir);

		if (deviceFound)
			break;
	}
	closedir(devicesDir);

	return deviceFound;
}

bool HIDDevice::LookupHidDeviceName(int bus, int vendorId, int productId, std::string & deviceName)
{
	bool ret = false;
	struct dirent * devDirEntry;
	DIR * devDir;
	char devicePrefix[15];

	snprintf(devicePrefix, 15, "%04X:%04X:%04X", bus, vendorId, productId);

	devDir = opendir("/sys/bus/hid/devices");
	if (!devDir)
		return false;

	while ((devDirEntry = readdir(devDir)) != NULL) {
		if (!strncmp(devDirEntry->d_name, devicePrefix, 14)) {
			deviceName = devDirEntry->d_name;
			ret = true;
			break;
		}
	}
	closedir(devDir);

	return ret;
}

bool HIDDevice::FindHidRawFile(std::string & deviceName, std::string & hidrawFile)
{
	bool ret = false;
	char hidrawDir[PATH_MAX];
	struct dirent * devDirEntry;
	DIR * devDir;

	snprintf(hidrawDir, PATH_MAX, "/sys/bus/hid/devices/%s/hidraw", deviceName.c_str());

	devDir = opendir(hidrawDir);
	if (!devDir)
		return false;

	while ((devDirEntry = readdir(devDir)) != NULL) {
		if (!strncmp(devDirEntry->d_name, "hidraw", 6)) {
			hidrawFile = std::string("/dev/") + devDirEntry->d_name;
			ret = true;
			break;
		}
	}
	closedir(devDir);

	return ret;
}

/*
 * Copyright (C) 2013 - 2014 Andrew Duggan
 * Copyright (C) 2013 - 2014 Synaptics Inc
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <getopt.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
#include <signal.h>
#include <stdlib.h>

#include "hiddevice.h"

#define RMI4UPDATE_GETOPTS      "hp:ir:w:foambde"

 enum rmihidtool_cmd {
	RMIHIDTOOL_CMD_INTERACTIVE,
	RMIHIDTOOL_CMD_READ,
	RMIHIDTOOL_CMD_WRITE,
	RMIHIDTOOL_CMD_FW_ID,
	RMIHIDTOOL_CMD_PROPS,
	RMIHIDTOOL_CMD_ATTN,
	RMIHIDTOOL_CMD_PRINT_FUNCTIONS,
	RMIHIDTOOL_CMD_REBIND_DRIVER,
	RMIHIDTOOL_CMD_PRINT_DEVICE_INFO,
	RMIHIDTOOL_CMD_RESET_DEVICE,
};

static int report_attn = 0;
static RMIDevice * g_device = NULL;

void print_help(const char *prog_name)
{
	fprintf(stdout, "Usage: %s [OPTIONS] DEVICEFILE\n", prog_name);
	fprintf(stdout, "\t-h, --help\t\t\t\tPrint this message\n");
	fprintf(stdout, "\t-p, --protocol [protocol]\t\tSet which transport prototocl to use.\n");
	fprintf(stdout, "\t-i, --interactive\t\t\tRun in interactive mode.\n");
	fprintf(stdout, "\t-r, --read [address] [length]\t\tRead registers starting at the address.\n");
	fprintf(stdout, "\t-r, --write [address] [length] [data]\tWrite registers starting at the address.\n");
	fprintf(stdout, "\t-f, --firmware-id\t\t\tPrint the firmware id\n");
	fprintf(stdout, "\t-o, --props\t\t\t\tPrint device properties\n");
	fprintf(stdout, "\t-a, --attention\t\t\t\tPrint attention reports until control + c\n");
	fprintf(stdout, "\t-m, --print-functions\t\t\tPrint RMI4 functions for the device.\n");
	fprintf(stdout, "\t-b, --rebind-driver\t\t\tRebind the driver to force an update of device properties.\n");
	fprintf(stdout, "\t-d, --device-info\t\t\tPrint protocol specific information about the device.\n");
	fprintf(stdout, "\t-e, --reset-device\t\t\tReset the device.\n");
}

void print_cmd_usage()
{
	fprintf(stdout, "Commands:\n");
	fprintf(stdout, "s [0,1,2]: Set RMIMode\n");
	fprintf(stdout, "r address size: read size bytes from address\n");
	fprintf(stdout, "w address { values }: write bytes to address\n");
	fprintf(stdout, "a: Wait for attention\n");
	fprintf(stdout, "q: quit\n");
}

int find_token(char * input, char * result, size_t result_len, char ** endpp)
{
	int i = 0;
	char * start = input;
	char * end;

	while (input[i] == ' ') {
		++start;
		++i;
	}

	while (input[i] != '\0') {
		if (input[++i] == ' ')
			break;
	}
	end = &input[i];

	if (start == end)
		return 0;

	*endpp = end;
	if (static_cast<ssize_t>(result_len) < end - start + 1)
		return 0;
	strncpy(result, start, end - start);
	result[end - start] = '\0';

	return 1;
}

void interactive(RMIDevice * device, unsigned char *report)
{
	char token[256];
	char * start;
	char * end;
	int rc;

	for (;;) {
		fprintf(stdout, "\n");
		print_cmd_usage();
		char input[256];

		if (fgets(input, 256, stdin)) {
			memset(token, 0, 256);

			if (input[0] == 's') {
				start = input + 2;
				find_token(start, token, sizeof(token), &end);
				int mode = strtol(token, NULL, 0);
				if (mode >= 0 && mode <= 2) {
					if (device->SetMode(mode)) {
						fprintf(stderr, "Set RMI Mode to: %d\n", mode);
					} else {
						fprintf(stderr, "Set RMI Mode FAILED!\n");
						continue;
					}
				}
			} else if (input[0] == 'r') {
				start = input + 2;
				find_token(start, token, sizeof(token), &end);
				start = end + 1;
				unsigned int addr = strtol(token, NULL, 0);
				find_token(start, token, sizeof(token), &end);
				start = end + 1;
				unsigned int len = strtol(token, NULL, 0);
				fprintf(stdout, "Address = 0x%02x Length = %d\n", addr, len);

				memset(report, 0, 256);
				rc = device->Read(addr, report, len);
				if (rc < 0)
					fprintf(stderr, "Failed to read report: %d\n", rc);
				print_buffer(report, len);
			} else if (input[0] == 'w') {
				int index = 0;
				start = input + 2;
				find_token(start, token, sizeof(token), &end);
				start = end + 1;
				unsigned int addr = strtol(token, NULL, 0);
				unsigned int len = 0;

				memset(report, 0, 256);
				while (find_token(start, token, sizeof(token), &end)) {
					start = end;
					report[index++] = strtol(token, NULL, 0);
					++len;
				}

				if (device->Write(addr, report, len) < 0) {
					fprintf(stderr, "Failed to Write Report\n");
					continue;
				}
			} else if (input[0] == 'a') {
				unsigned int bytes = 256;
				device->GetAttentionReport(NULL,
						RMI_INTERUPT_SOURCES_ALL_MASK,
						report, &bytes);
				print_buffer(report, bytes);
			} else if (input[0] == 'q') {
				return;
			} else {
				print_cmd_usage();
			}
		}
	}
}

static void cleanup(int status)
{
	if (report_attn) {
		report_attn = 0;
		if (g_device)
			g_device->Cancel();
	} else {
		exit(0);
	}
}

int main(int argc, char ** argv)
{
	int rc;
	struct sigaction sig_cleanup_action;
	int opt;
	int index;
	RMIDevice *device;
	const char *protocol = "HID";
	unsigned char report[256];
	char token[256];
	static struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"protocol", 1, NULL, 'p'},
		{"interactive", 0, NULL, 'i'},
		{"read", 1, NULL, 'r'},
		{"write", 1, NULL, 'w'},
		{"firmware-id", 0, NULL, 'f'},
		{"props", 0, NULL, 'o'},
		{"attention", 0, NULL, 'a'},
		{"print-functions", 0, NULL, 'm'},
		{"rebind-driver", 0, NULL, 'b'},
		{"device-info", 0, NULL, 'd'},
		{"reset-device", 0, NULL, 'e'},
		{0, 0, 0, 0},
	};
	enum rmihidtool_cmd cmd = RMIHIDTOOL_CMD_INTERACTIVE;
	unsigned int addr = 0;
	unsigned int len = 0;
	char * data = NULL;
	char * start;
	char * end;
	int i = 0;

	memset(&sig_cleanup_action, 0, sizeof(struct sigaction));
	sig_cleanup_action.sa_handler = cleanup;
	sig_cleanup_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sig_cleanup_action, NULL);

	while ((opt = getopt_long(argc, argv, RMI4UPDATE_GETOPTS, long_options, &index)) != -1) {
		switch (opt) {
			case 'h':
				print_help(argv[0]);
				return 0;
			case 'p':
				protocol = optarg;
				break;
			case 'i':
				cmd = RMIHIDTOOL_CMD_INTERACTIVE;
				break;
			case 'r':
				cmd = RMIHIDTOOL_CMD_READ;
				addr = strtol(optarg, NULL, 0);
				len = strtol(argv[optind++], NULL, 0);
				break;
			case 'w':
				cmd = RMIHIDTOOL_CMD_WRITE;
				addr = strtol(optarg, NULL, 0);
				data = argv[optind++];
				break;
			case 'f':
				cmd = RMIHIDTOOL_CMD_FW_ID;
				break;
			case 'o':
				cmd = RMIHIDTOOL_CMD_PROPS;
				break;
			case 'a':
				cmd = RMIHIDTOOL_CMD_ATTN;
				break;
			case 'm':
				cmd = RMIHIDTOOL_CMD_PRINT_FUNCTIONS;
				break;
			case 'b':
				cmd = RMIHIDTOOL_CMD_REBIND_DRIVER;
				break;
			case 'd':
				cmd = RMIHIDTOOL_CMD_PRINT_DEVICE_INFO;
				break;
			case 'e':
				cmd = RMIHIDTOOL_CMD_RESET_DEVICE;
				break;
			default:
				print_help(argv[0]);
				return 0;
				break;

		}
	}

	if (!strncasecmp("hid", protocol, 3)) {
		device = new HIDDevice();
	} else {
		fprintf(stderr, "Invalid Protocol: %s\n", protocol);
		return -1;
	}

	if (optind >= argc) {
		print_help(argv[0]);
		return -1;
	}

	rc = device->Open(argv[optind++]);
	if (rc) {
		fprintf(stderr, "%s: failed to initialize rmi device (%d): %s\n", argv[0], errno,
			strerror(errno));
		return 1;
	}

	g_device = device;

	switch (cmd) {
		case RMIHIDTOOL_CMD_READ:
			memset(report, 0, sizeof(report));
			rc = device->Read(addr, report, len);
			if (rc < 0)
				fprintf(stderr, "Failed to read report: %d\n", rc);

			print_buffer(report, len);
			break;
		case RMIHIDTOOL_CMD_WRITE:
			i = 0;
			start = data;
			memset(report, 0, sizeof(report));
			while (find_token(start, token, sizeof(token), &end)) {
				start = end;
				report[i++] = (unsigned char)strtol(token, NULL, 0);
				++len;
			}

			if (device->Write(addr, report, len) < 0) {
				fprintf(stderr, "Failed to Write Report\n");
				return -1;
			}
			break;
		case RMIHIDTOOL_CMD_FW_ID:
			device->ScanPDT();
			device->QueryBasicProperties();
			fprintf(stdout, "firmware id: %lu\n", device->GetFirmwareID());
			break;
		case RMIHIDTOOL_CMD_PROPS:
			device->ScanPDT();
			device->QueryBasicProperties();
			device->PrintProperties();
			break;
		case RMIHIDTOOL_CMD_ATTN:
			report_attn = 1;
			while(report_attn) {
				unsigned int bytes = 256;
				rc = device->GetAttentionReport(NULL,
						RMI_INTERUPT_SOURCES_ALL_MASK,
						report, &bytes);
				if (rc > 0) {
					print_buffer(report, bytes);
					fprintf(stdout, "\n");
				}
			}
			break;
		case RMIHIDTOOL_CMD_PRINT_FUNCTIONS:
			device->ScanPDT();
			device->PrintFunctions();
			break;
		case RMIHIDTOOL_CMD_REBIND_DRIVER:
			device->RebindDriver();
			break;
		case RMIHIDTOOL_CMD_PRINT_DEVICE_INFO:
			device->PrintDeviceInfo();
			break;
		case RMIHIDTOOL_CMD_RESET_DEVICE:
			device->ScanPDT();
			device->Reset();
			break;
		case RMIHIDTOOL_CMD_INTERACTIVE:
		default:
			interactive(device, report);
			break;
	}

	device->Close();

	return 0;
}

/*
 * Copyright (C) 2014 Satoshi Noguchi
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
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <signal.h>

#include "hiddevice.h"
#include "f54test.h"
#include "display.h"

#define F54TEST_GETOPTS	"hd:r:cn"

static bool stopRequested;

void printHelp(const char *prog_name)
{
	fprintf(stdout, "Usage: %s [OPTIONS]\n", prog_name);
	fprintf(stdout, "\t-h, --help\tPrint this message\n");
	fprintf(stdout, "\t-d, --device\thidraw device file associated with the device being tested.\n");
	fprintf(stdout, "\t-r, --report_type\tReport type.\n");
	fprintf(stdout, "\t-c, --continuous\tContinuous mode.\n");
	fprintf(stdout, "\t-n, --no_reset\tDo not reset after the report.\n");
}

int RunF54Test(const char * deviceFile, f54_report_types reportType, bool continuousMode, bool noReset)
{
	int rc;
	HIDDevice rmidevice;
	Display * display;

	if (continuousMode)
	{
		display = new AnsiConsole();
	}
	else
	{
		display = new Display();
	}

	display->Clear();

	rc = rmidevice.Open(deviceFile);
	if (rc)
		return rc;

	F54Test f54Test(rmidevice, *display);

	rc = f54Test.Prepare(reportType);
	if (rc)
		return rc;

	stopRequested = false;

	do {
		rc = f54Test.Run();
	}
	while (continuousMode && !stopRequested);

	if (!noReset)
		rmidevice.Reset();

	rmidevice.Close();

	delete display;

	return rc;
}

void SignalHandler(int p_signame)
{
	stopRequested = true;
}

int main(int argc, char **argv)
{
	int rc = 0;
	int opt;
	int index;
	char *deviceName = NULL;
	static struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{"device", 1, NULL, 'd'},
		{"report_type", 1, NULL, 'r'},
		{"continuous", 0, NULL, 'c'},
		{"no_reset", 0, NULL, 'n'},
		{0, 0, 0, 0},
	};
	struct dirent * devDirEntry;
	DIR * devDir;
	f54_report_types reportType = F54_16BIT_IMAGE;
	bool continuousMode = false;
	bool noReset = false;

	while ((opt = getopt_long(argc, argv, F54TEST_GETOPTS, long_options, &index)) != -1) {
		switch (opt) {
			case 'h':
				printHelp(argv[0]);
				return 0;
			case 'd':
				deviceName = optarg;
				break;
			case 'r':
				reportType = (f54_report_types)strtol(optarg, NULL, 0);
				break;
			case 'c':
				continuousMode = true;
				break;
			case 'n':
				noReset = true;
				break;
			default:
				break;

		}
	}

	if (continuousMode)
	{
		signal(SIGHUP, SignalHandler);
		signal(SIGINT, SignalHandler);
		signal(SIGTERM, SignalHandler);
	}

	if (deviceName) {
		rc = RunF54Test(deviceName, reportType, continuousMode, noReset);
		if (rc)
			return rc;

		return rc;
	} else {
		char rawDevice[PATH_MAX];
		char deviceFile[PATH_MAX];
		bool found = false;

		devDir = opendir("/dev");
		if (!devDir)
			return -1;

		while ((devDirEntry = readdir(devDir)) != NULL) {
			if (strstr(devDirEntry->d_name, "hidraw")) {
				strncpy(rawDevice, devDirEntry->d_name, PATH_MAX);
				snprintf(deviceFile, PATH_MAX, "/dev/%s", devDirEntry->d_name);
				rc = RunF54Test(deviceFile, reportType, continuousMode, noReset);
				if (rc != 0) {
					continue;
				} else {
					found = true;
					break;
				}
			}
		}
		closedir(devDir);

		if (!found)
			return rc;
	}

	return 0;
}

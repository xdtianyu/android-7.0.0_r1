# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


# Take inventory of these processes:
INVENTORY_PROCS = (
    'apmanager',
    'bluetoothtbd',
    'firewalld',
    'mediaserver',
    'metricsd',
    'metrics_collector',
    'nativepowerman',
    'peripheralman',
    'sensorservice',
    'shill',
    'update_engine',
    'weaved',
    'webservd',
    'wpa_supplicant',
)
# Using this command:
PROCRANK_CMD = 'procrank'

# And these shared libs:
INVENTORY_LIBS = (
    'libweaved.so',
    'libbrillo.so',
    'libbase.so',
    'libweave.so',
    'libchrome.so',
    'libbrillo-http.so',
    'libwebserv.so',
)
# Using this command:
LIBRANK_CMD = 'librank'


class brillo_ProcLibMemoryInventory(test.test):
    """Report memory sizes of Brillo daemons and libraries."""
    version = 1


    def parse_mem_str(self, str):
        """Return a string (presumably digits) with the trailing "K" removed

        @param str: a digit string with trailing "K" from procrank/librank

        """
        if str.endswith('K'):
            return int(str[:-1])
        else:
            return None


    def report_procs(self, host):
        """Report Unique Set Size (USS) for various Brillo processes.

        Unique Set Size is the amount of physical memory in use that
        is unique to the process (not shared with one or more other
        processes).  This is retrieved via the procrank command.

        @param host: a host object representing the DUT.

        @raise TestError: Something went wrong while trying to execute the test.

        """
        procrank_output = host.run_output(PROCRANK_CMD).splitlines()
        proc_uss_map = {}
        total_proc_uss = None

        for line in procrank_output:
            line_tokens = line.split()

            #   0         1        2        3        4    5
            #   PID       Vss      Rss      Pss      Uss  cmdline
            #   257    13468K    5564K    2005K    1492K  /system/bin/weaved
            if len(line_tokens) == 6:
                pid, vss_str, rss_str, pss_str, uss_str, cmdline = line_tokens
                path, daemon = os.path.split(cmdline)
                uss_kbytes = self.parse_mem_str(uss_str)
                if path == '/system/bin' and uss_kbytes is not None:
                    proc_uss_map[daemon] = uss_kbytes

            #                           0        1       2
            #                           ------   ------  ------
            #                           36708K   29196K  TOTAL
            elif len(line_tokens) == 3 and line_tokens[2] == 'TOTAL':
                total_proc_uss = self.parse_mem_str(line_tokens[1])

        if total_proc_uss == None:
            raise error.TestError('no total memory found in procrank output')

        self.write_perf_keyval({'total-proc-uss': total_proc_uss})

        for proc in INVENTORY_PROCS:
            if proc in proc_uss_map:
                self.write_perf_keyval({proc + '-uss': proc_uss_map[proc]})
            else:
                logging.info('Failed to find procrank results for ' + proc);
                self.write_perf_keyval({proc + '-uss': 0})


    def report_libs(self, host):
        """Report Resident Set Size (RSS) for various Brillo shared libs.

        Resident Set Size is the amount of physical memory in use for the
        library (some of which may be shared by multiple processes).  This is
        retrieved via the librank command.

        @param host: a host object representing the DUT.

        @raise TestError: Something went wrong while trying to execute the test.

        """
        librank_output = host.run_output(LIBRANK_CMD).splitlines()
        lib_rss = {}
        total_lib_rss = 0

        for line in librank_output:
            line_tokens = line.split()

            #  0                                           1
            #  RSStot      VSS      RSS      PSS      USS  Name/PID
            #    959K                                      /system/lib/libc.so
            if len(line_tokens) == 2:
                rss_str, lib_pathname = line_tokens;
                path, lib_name = os.path.split(lib_pathname)
                rss_kbytes = self.parse_mem_str(rss_str)
                if path == '/system/lib' and rss_kbytes is not None:
                    lib_rss[lib_name] = rss_kbytes
                    total_lib_rss += rss_kbytes

        if total_lib_rss == 0:
            raise error.TestError('no Brillo libraries found in librank output')

        self.write_perf_keyval({'total-lib-rss': total_lib_rss})

        for lib in INVENTORY_LIBS:
            if lib in lib_rss:
                self.write_perf_keyval({lib + '-rss': lib_rss[lib]})
            else:
                logging.info('Failed to find librank results for ' + lib)
                self.write_perf_keyval({lib + '-rss': 0})


    def run_once(self, host):
        """Run the Brillo memory size inventory test.

        @param host: a host object representing the DUT.

        """
        self.report_procs(host)
        self.report_libs(host)

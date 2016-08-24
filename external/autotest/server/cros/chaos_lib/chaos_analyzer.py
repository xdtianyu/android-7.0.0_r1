#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re

import chaos_capture_analyzer
import chaos_log_analyzer

class ChaosTestInfo(object):
    """ Class to gather the relevant test information from a folder. """

    MESSAGES_FILE_NAME = "messages"
    NET_LOG_FILE_NAME = "net.log"
    TEST_DEBUG_LOG_FILE_END = "DEBUG"
    SYSINFO_FOLDER_NAME_END = "sysinfo"
    TEST_DEBUG_FOLDER_NAME_END = "debug"

    def __init__(self, dir_name, file_names, failures_only):
        """
        Gathers all the relevant Chaos test results from a given folder.

        @param dir: Folder to check for test results.
        @param files: Files present in the folder found during os.walk.
        @param failures_only: Flag to indicate whether to analyze only
                              failure test attempts.

        """
        self._meta_info = None
        self._traces = []
        self._message_log = None
        self._net_log = None
        self._test_debug_log = None
        for file_name in file_names:
            if file_name.endswith('.trc'):
                basename = os.path.basename(file_name)
                if 'success' in basename and failures_only:
                    continue
                self._traces.append(os.path.join(dir_name, file_name))
        if self._traces:
            for root, dir_name, file_names in os.walk(dir_name):
                # Now get the log files from the sysinfo, debug folder
                if root.endswith(self.SYSINFO_FOLDER_NAME_END):
                    # There are multiple copies of |messages| file under
                    # sysinfo tree. We only want the one directly in sysinfo.
                    for file_name in file_names:
                        if file_name == self.MESSAGES_FILE_NAME:
                            self._message_log = os.path.join(root, file_name)
                    for root, dir_name, file_names in os.walk(root):
                        for file_name in file_names:
                            if file_name == self.NET_LOG_FILE_NAME:
                                self._net_log = os.path.join(root, file_name)
                if root.endswith(self.TEST_DEBUG_FOLDER_NAME_END):
                    for root, dir_name, file_names in os.walk(root):
                        for file_name in file_names:
                            if file_name.endswith(self.TEST_DEBUG_LOG_FILE_END):
                                self._test_debug_log = (
                                        os.path.join(root, file_name))
                                self._parse_meta_info(
                                        os.path.join(root, file_name))

    def _parse_meta_info(self, file):
        dut_mac_prefix ='\'DUT\': '
        ap_bssid_prefix ='\'AP Info\': '
        ap_ssid_prefix ='\'SSID\': '
        self._meta_info = {}
        with open(file) as infile:
            for line in infile.readlines():
                line = line.strip()
                if line.startswith(dut_mac_prefix):
                    dut_mac = line[len(dut_mac_prefix):].rstrip()
                    self._meta_info['dut_mac'] = (
                        dut_mac.replace('\'', '').replace(',', ''))
                if line.startswith(ap_ssid_prefix):
                    ap_ssid = line[len(ap_ssid_prefix):].rstrip()
                    self._meta_info['ap_ssid'] = (
                        ap_ssid.replace('\'', '').replace(',', ''))
                if line.startswith(ap_bssid_prefix):
                    debug_info = self._parse_debug_info(line)
                    if debug_info:
                        self._meta_info.update(debug_info)

    def _parse_debug_info(self, line):
        # Example output:
        #'AP Info': "{'2.4 GHz MAC Address': '84:1b:5e:e9:74:ee', \n
        #'5 GHz MAC Address': '84:1b:5e:e9:74:ed', \n
        #'Controller class': 'Netgear3400APConfigurator', \n
        #'Hostname': 'chromeos3-row2-rack2-host12', \n
        #'Router name': 'wndr 3700 v3'}",
        debug_info = line.replace('\'', '')
        address_label = 'Address: '
        bssids = []
        for part in debug_info.split(','):
            address_index = part.find(address_label)
            if address_index >= 0:
                address = part[(address_index+len(address_label)):]
                if address != 'N/A':
                    bssids.append(address)
        if not bssids:
            return None
        return { 'ap_bssids': bssids }

    def _is_meta_info_valid(self):
        return ((self._meta_info is not None) and
                ('dut_mac' in self._meta_info) and
                ('ap_ssid' in self._meta_info) and
                ('ap_bssids' in self._meta_info))

    @property
    def traces(self):
        """Returns the trace files path in test info."""
        return self._traces

    @property
    def message_log(self):
        """Returns the message log path in test info."""
        return self._message_log

    @property
    def net_log(self):
        """Returns the net log path in test info."""
        return self._net_log

    @property
    def test_debug_log(self):
        """Returns the test debug log path in test info."""
        return self._test_debug_log

    @property
    def bssids(self):
        """Returns the BSSID of the AP in test info."""
        return self._meta_info['ap_bssids']

    @property
    def ssid(self):
        """Returns the SSID of the AP in test info."""
        return self._meta_info['ap_ssid']

    @property
    def dut_mac(self):
        """Returns the MAC of the DUT in test info."""
        return self._meta_info['dut_mac']

    def is_valid(self, packet_capture_only):
        """
        Checks if the given folder contains a valid Chaos test results.

        @param packet_capture_only: Flag to indicate whether to analyze only
                                    packet captures.

        @return True if valid chaos results are found; False otherwise.

        """
        if packet_capture_only:
            return ((self._is_meta_info_valid()) and
                    (bool(self._traces)))
        else:
            return ((self._is_meta_info_valid()) and
                    (bool(self._traces)) and
                    (bool(self._message_log)) and
                    (bool(self._net_log)))


class ChaosLogger(object):
    """ Class to log the analysis to the given output file. """

    LOG_SECTION_DEMARKER = "--------------------------------------"

    def __init__(self, output):
        self._output = output

    def log_to_output_file(self, log_msg):
        """
        Logs the provided string to the output file.

        @param log_msg: String to print to the output file.

        """
        self._output.write(log_msg + "\n")

    def log_start_section(self, section_description):
        """
        Starts a new section in the output file with demarkers.

        @param log_msg: String to print in section description.

        """
        self.log_to_output_file(self.LOG_SECTION_DEMARKER)
        self.log_to_output_file(section_description)
        self.log_to_output_file(self.LOG_SECTION_DEMARKER)


class ChaosAnalyzer(object):
    """ Main Class to analyze the chaos test output from a given folder. """

    LOG_OUTPUT_FILE_NAME_FORMAT = "chaos_analyzer_try_%s.log"
    TRACE_FILE_ATTEMPT_NUM_RE = r'\d+'

    def _get_attempt_number_from_trace(self, trace):
        file_name = os.path.basename(trace)
        return re.search(self.TRACE_FILE_ATTEMPT_NUM_RE, file_name).group(0)

    def _get_all_test_infos(self, dir_name, failures_only, packet_capture_only):
        test_infos = []
        for root, dir, files in os.walk(dir_name):
            test_info = ChaosTestInfo(root, files, failures_only)
            if test_info.is_valid(packet_capture_only):
                test_infos.append(test_info)
        if not test_infos:
            print "Did not find any valid test info!"
        return test_infos

    def analyze(self, input_dir_name=None, output_dir_name=None,
                failures_only=False, packet_capture_only=False):
        """
        Starts the analysis of the Chaos test logs and packet capture.

        @param input_dir_name: Directory which contains the chaos test results.
        @param output_dir_name: Directory to which the chaos analysis is output.
        @param failures_only: Flag to indicate whether to analyze only
                              failure test attempts.
        @param packet_capture_only: Flag to indicate whether to analyze only
                                    packet captures.

        """
        for test_info in self._get_all_test_infos(input_dir_name, failures_only,
                                                  packet_capture_only):
            for trace in test_info.traces:
                attempt_num = self._get_attempt_number_from_trace(trace)
                trace_dir_name = os.path.dirname(trace)
                print "Analyzing attempt number: " + attempt_num + \
                      " from folder: " + os.path.abspath(trace_dir_name)
                # Store the analysis output in the respective log folder
                # itself unless there is an explicit output directory
                # specified in which case we prepend the |testname_| to the
                # output analysis file name.
                output_file_name = (
                        self.LOG_OUTPUT_FILE_NAME_FORMAT % (attempt_num))
                if not output_dir_name:
                    output_dir = trace_dir_name
                else:
                    output_dir = output_dir_name
                    output_file_name = "_".join([trace_dir_name,
                                                 output_file_name])
                output_file_path = (
                        os.path.join(output_dir, output_file_name))
                try:
                    with open(output_file_path, "w") as output_file:
                         logger = ChaosLogger(output_file)
                         protocol_analyzer = (
                                chaos_capture_analyzer.ChaosCaptureAnalyzer(
                                        test_info.bssids, test_info.ssid,
                                        test_info.dut_mac, logger))
                         protocol_analyzer.analyze(trace)
                         if not packet_capture_only:
                             with open(test_info.message_log, "r") as message_log, \
                                  open(test_info.net_log, "r") as net_log:
                                  log_analyzer = (
                                         chaos_log_analyzer.ChaosLogAnalyzer(
                                                message_log, net_log, logger))
                                  log_analyzer.analyze(attempt_num)
                except IOError as e:
                    print 'Operation failed: %s!' % e.strerror


def main():
    # By default the script parses all the logs places under the current
    # directory and places the analyzed output for each set of logs in their own
    # respective directories.
    parser = argparse.ArgumentParser(description='Analyze Chaos logs.')
    parser.add_argument('-f', '--failures-only', action='store_true',
                        help='analyze only failure logs.')
    parser.add_argument('-p', '--packet-capture-only', action='store_true',
                        help='analyze only packet captures.')
    parser.add_argument('-i', '--input-dir', action='store', default='.',
                        help='process the logs from directory.')
    parser.add_argument('-o', '--output-dir', action='store',
                        help='output the analysis to directory.')
    args = parser.parse_args()
    chaos_analyzer = ChaosAnalyzer()
    chaos_analyzer.analyze(input_dir_name=args.input_dir,
                           output_dir_name=args.output_dir,
                           failures_only=args.failures_only,
                           packet_capture_only=args.packet_capture_only)

if __name__ == "__main__":
    main()


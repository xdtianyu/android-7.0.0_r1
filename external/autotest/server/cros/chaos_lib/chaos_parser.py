# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import copy
import csv
import logging
import os
import re
import shutil

CONNECT_FAIL = object()
CONFIG_FAIL = object()
RESULTS_DIR = '/tmp/chaos'


class ChaosParser(object):
    """Defines a parser for chaos test results"""

    def __init__(self, results_dir, create_file, print_config_failures):
        """ Constructs a parser interface.

        @param results_dir: complete path to restuls directory for a chaos test.
        @param create_file: True to create csv files; False otherwise.
        @param print_config_failures: True to print the config info to stdout;
                                      False otherwise.

        """
        self._test_results_dir = results_dir
        self._create_file = create_file
        self._print_config_failures = print_config_failures


    def convert_set_to_string(self, set_list):
        """Converts a set to a single string.

        @param set_list: a set to convert

        @returns a string, which is all items separated by the word 'and'

        """
        return_string = str()
        for i in set_list:
            return_string += str('%s and ' % i)
        return return_string[:-5]


    def create_csv(self, filename, data_list):
        """Creates a file in .csv format.

        @param filename: name for the csv file
        @param data_list: a list of all the info to write to a file

        """
        if not os.path.exists(RESULTS_DIR):
            os.mkdir(RESULTS_DIR)
        try:
            path = os.path.join(RESULTS_DIR, filename + '.csv')
            with open(path, 'wb') as f:
                writer = csv.writer(f)
                writer.writerow(data_list)
                logging.info('Created CSV file %s', path)
        except IOError as e:
            logging.error('File operation failed with %s: %s', e.errno,
                           e.strerror)
            return


    def get_ap_name(self, line):
        """Gets the router name from the string passed.

        @param line: Test ERROR string from chaos status.log

        @returns the router name or brand.

        """
        router_info = re.search('Router name: ([\w\s]+)', line)
        return router_info.group(1)


    def get_ap_mode_chan_freq(self, ssid):
        """Gets the AP band from ssid using channel.

        @param ssid: A valid chaos test SSID as a string

        @returns the AP band, mode, and channel.

        """
        channel_security_info = ssid.split('_')
        channel_info = channel_security_info[-2]
        mode = channel_security_info[-3]
        channel = int(re.split('(\d+)', channel_info)[1])
        # TODO Choose if we want to keep band, we never put it in the
        # spreadsheet and is currently unused.
        if channel in range(1, 15):
            band = '2.4GHz'
        else:
            band = '5GHz'
        return {'mode': mode.upper(), 'channel': channel,
                'band': band}


    def generate_percentage_string(self, passed_tests, total_tests):
        """Creates a pass percentage string in the formation x/y (zz%)

        @param passed_tests: int of passed tests
        @param total_tests: int of total tests

        @returns a formatted string as described above.

        """
        percent = float(passed_tests)/float(total_tests) * 100
        percent_string = str(int(round(percent))) + '%'
        return str('%d/%d (%s)' % (passed_tests, total_tests, percent_string))


    def parse_keyval(self, filepath):
        """Parses the 'keyvalue' file to get device details.

        @param filepath: the complete path to the keyval file

        @returns a board with device name and OS version.

        """
        # Android information does not exist in the keyfile, add temporary
        # information into the dictionary.  crbug.com/570408
        lsb_dict = {'board': 'unknown',
                    'version': 'unknown'}
        f = open(filepath, 'r')
        for line in f:
            line = line.split('=')
            if 'RELEASE_BOARD' in line[0]:
                lsb_dict = {'board':line[1].rstrip()}
            elif 'RELEASE_VERSION' in line[0]:
                lsb_dict['version'] = line[1].rstrip()
            else:
                continue
        f.close()
        return lsb_dict


    def parse_status_log(self, board, os_version, security, status_log_path):
        """Parses the entire status.log file from chaos test for test failures.
           and creates two CSV files for connect fail and configuration fail
           respectively.

        @param board: the board the test was run against as a string
        @param os_version: the version of ChromeOS as a string
        @param security: the security used during the test as a string
        @param status_log_path: complete path to the status.log file

        """
        # Items that can have multiple values
        modes = list()
        channels = list()
        test_fail_aps = list()
        static_config_failures = list()
        dynamic_config_failures = list()
        kernel_version = ""
        fw_version = ""
        f = open(status_log_path, 'r')
        total = 0
        for line in f:
            line = line.strip()
            if line.startswith('START\tnetwork_WiFi'):
               # Do not count PDU failures in total tests run.
               if 'PDU' in line:
                   continue
               total += 1
            elif 'kernel_version' in line:
                kernel_version = re.search('[\d.]+', line).group(0)
            elif 'firmware_version' in line:
               fw_version = re.search('firmware_version\': \'([\w\s:().]+)',
                                      line).group(1)
            elif line.startswith('ERROR') or line.startswith('FAIL'):
                title_info = line.split()
                if 'reboot' in title_info:
                    continue
                # Get the hostname for the AP that failed configuration.
                if 'PDU' in title_info[1]:
                    continue
                else:
                    # Get the router name, band for the AP that failed
                    # connect.
                    if 'Config' in title_info[1]:
                        failure_type = CONFIG_FAIL
                    else:
                        failure_type = CONNECT_FAIL

                    if (failure_type == CONFIG_FAIL and
                        'chromeos' in title_info[1]):
                        ssid = title_info[1].split('.')[1].split('_')[0]
                    else:
                        ssid_info = title_info[1].split('.')
                        ssid = ssid_info[1]
                        network_dict = self.get_ap_mode_chan_freq(ssid)
                        modes.append(network_dict['mode'])
                        channels.append(network_dict['channel'])

                    # Security mismatches and Ping failures are not connect
                    # failures.
                    if (('Ping command' in line or 'correct security' in line)
                        or failure_type == CONFIG_FAIL):
                        if 'StaticAPConfigurator' in line:
                            static_config_failures.append(ssid)
                        else:
                            dynamic_config_failures.append(ssid)
                    else:
                        test_fail_aps.append(ssid)
            elif ('END GOOD' in line and ('ChaosConnectDisconnect' in line or
                                          'ChaosLongConnect' in line)):
                    test_name = line.split()[2]
                    ssid = test_name.split('.')[1]
                    network_dict = self.get_ap_mode_chan_freq(ssid)
                    modes.append(network_dict['mode'])
                    channels.append(network_dict['channel'])
            else:
                continue

        config_pass = total - (len(dynamic_config_failures) +
                               len(static_config_failures))
        config_pass_string = self.generate_percentage_string(config_pass,
                                                             total)
        connect_pass = config_pass - len(test_fail_aps)
        connect_pass_string = self.generate_percentage_string(connect_pass,
                                                              config_pass)

        base_csv_list = [board, os_version, fw_version, kernel_version,
                         self.convert_set_to_string(set(modes)),
                         self.convert_set_to_string(set(channels)),
                         security]

        static_config_csv_list = copy.deepcopy(base_csv_list)
        static_config_csv_list.append(config_pass_string)
        static_config_csv_list.extend(static_config_failures)

        dynamic_config_csv_list = copy.deepcopy(base_csv_list)
        dynamic_config_csv_list.append(config_pass_string)
        dynamic_config_csv_list.extend(dynamic_config_failures)

        connect_csv_list = copy.deepcopy(base_csv_list)
        connect_csv_list.append(connect_pass_string)
        connect_csv_list.extend(test_fail_aps)

        print('Connect failure for security: %s' % security)
        print ','.join(connect_csv_list)
        print('\n')

        if self._print_config_failures:
            config_files = [('Static', static_config_csv_list),
                            ('Dynamic', dynamic_config_csv_list)]
            for config_data in config_files:
                self.print_config_failures(config_data[0], security,
                                           config_data[1])

        if self._create_file:
            self.create_csv('chaos_WiFi_dynamic_config_fail.' + security,
                            dynamic_config_csv_list)
            self.create_csv('chaos_WiFi_static_config_fail.' + security,
                            static_config_csv_list)
            self.create_csv('chaos_WiFi_connect_fail.' + security,
                            connect_csv_list)


    def print_config_failures(self, config_type, security, config_csv_list):
        """Prints out the configuration failures.

        @param config_type: string describing the configurator type
        @param security: the security type as a string
        @param config_csv_list: list of the configuration failures

        """
        # 8 because that is the lenth of the base list
        if len(config_csv_list) <= 8:
            return
        print('%s config failures for security: %s' % (config_type, security))
        print ','.join(config_csv_list)
        print('\n')


    def traverse_results_dir(self, path):
        """Walks through the results directory and get the pathnames for the
           status.log and the keyval files.

        @param path: complete path to a specific test result directory.

        @returns a dict with absolute pathnames for the 'status.log' and
                'keyfile' files.

        """
        status = None
        keyval = None

        for root, dir_name, file_name in os.walk(path):
            for name in file_name:
                current_path = os.path.join(root, name)
                if name == 'status.log' and not status:
                       status = current_path
                elif name == 'keyval' and ('param-debug_info' in
                                           open(current_path).read()):
                    # This is a keyval file for a single test and not a suite.
                    keyval = os.path.join(root, name)
                    break
                else:
                    continue
        if not keyval:
            raise Exception('Did Chaos tests complete successfully? Rerun tests'
                            ' with missing results.')
        return {'status_file': status, 'keyval_file': keyval}


    def parse_results_dir(self):
        """Parses each result directory.

        For each results directory created by test_that, parse it and
        create summary files.

        """
        if os.path.exists(RESULTS_DIR):
            shutil.rmtree(RESULTS_DIR)
        test_processed = False
        for results_dir in os.listdir(self._test_results_dir):
            if 'results' in results_dir:
                path = os.path.join(self._test_results_dir, results_dir)
                test = results_dir.split('.')[1]
                status_key_dict = self.traverse_results_dir(path)
                status_log_path = status_key_dict['status_file']
                lsb_info = self.parse_keyval(status_key_dict['keyval_file'])
                if test is not None:
                    self.parse_status_log(lsb_info['board'],
                                          lsb_info['version'],
                                          test,
                                          status_log_path)
                    test_processed = True
        if not test_processed:
            raise RuntimeError('chaos_parse: Did not find any results directory'
                               'to process')


def main():
    """Main function to call the parser."""
    logging.basicConfig(level=logging.INFO)
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-d', '--directory', dest='dir_name',
                            help='Pathname to results generated by test_that',
                            required=True)
    arg_parser.add_argument('--create_file', dest='create_file',
                            action='store_true', default=False)
    arg_parser.add_argument('--print_config_failures',
                            dest='print_config_failures',
                            action='store_true',
                            default=False)
    arguments = arg_parser.parse_args()
    if not arguments.dir_name:
        raise RuntimeError('chaos_parser: No directory name supplied. Use -h'
                           ' for help')
    if not os.path.exists(arguments.dir_name):
        raise RuntimeError('chaos_parser: Invalid directory name supplied.')
    parser = ChaosParser(arguments.dir_name, arguments.create_file,
                         arguments.print_config_failures)
    parser.parse_results_dir()


if __name__ == '__main__':
    main()

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Mosys(FirmwareTest):
    """
    Mosys commands test for Firmware values.

    Execute
    a. mosys -k smbios info bios
    b. mosys -k ec info
    c. mosys platform name
    d. mosys eeprom map
    e. mosys platform vendor
    f. mosys -k pd info

    """
    version = 1


    def initialize(self, host, cmdline_args, dev_mode=False):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        super(firmware_Mosys, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('dev' if dev_mode else 'normal')
        # a list contain failed execution.
        self.failed_command = []
        # Get a list of available mosys commands.
        lines = self.run_cmd('mosys help')
        self.command_list = []
        cmdlist_start = False
        for line in lines:
            if cmdlist_start:
                cmdlst = re.split('\s+', line)
                if len(cmdlst) > 2:
                    self.command_list.append(cmdlst[1])
            elif 'Commands:' in line:
                cmdlist_start = True
        logging.info('Availabe commands: %s', ' '.join(self.command_list))

    def run_cmd(self, command):
        """
        Log and execute command and return the output.

        @param command: Command to executeon device.
        @returns the output of command.

        """
        logging.info('Execute %s', command)
        output = self.faft_client.system.run_shell_command_get_output(command)
        logging.info('Output %s', output)
        return output

    def check_ec_version(self, command, exp_ec_version):
        """
        Compare output of 'ectool version' for the current firmware
        copy to exp_ec_version.

        @param command: command string
        @param exp_ec_version: The exepected EC version string.

        """
        if self.faft_client.system.has_host():
            lines = self.run_cmd('fwtool ec version')
        else:
            lines = self.run_cmd('ectool version')
        fwcopy_pattern = re.compile('Firmware copy: (.*)$')
        ver_pattern = re.compile('(R[OW]) version:    (.*)$')
        version = {}
        for line in lines:
            ver_matched = ver_pattern.match(line)
            if ver_matched:
                version[ver_matched.group(1)] = ver_matched.group(2)
            fwcopy_matched = fwcopy_pattern.match(line)
            if fwcopy_matched:
                fwcopy = fwcopy_matched.group(1)
        if fwcopy in version:
            actual_version = version[fwcopy]
            logging.info('Expected ec version %s actual_version %s',
                         exp_ec_version, actual_version)
            if exp_ec_version != actual_version:
               self._tag_failure(command)
        else:
            self._tag_failure(command)
            logging.error('Failed to locate version from ectool')

    def check_pd_version(self, command, exp_pd_version):
        """
        Compare output of 'ectool --dev 1 version' for the current PD firmware
        copy to exp_pd_version.

        @param command: command string
        @param exp_pd_version: The exepected PD version string.

        """
        lines = self.run_cmd('ectool --dev 1 version')
        fwcopy_pattern = re.compile('Firmware copy: (.*)$')
        ver_pattern = re.compile('(R[OW]) version:    (.*)$')
        version = {}
        for line in lines:
            ver_matched = ver_pattern.match(line)
            if ver_matched:
                version[ver_matched.group(1)] = ver_matched.group(2)
            fwcopy_matched = fwcopy_pattern.match(line)
            if fwcopy_matched:
                fwcopy = fwcopy_matched.group(1)
        if fwcopy in version:
            actual_version = version[fwcopy]
            logging.info('Expected pd version %s actual_version %s',
                         exp_pd_version, actual_version)
            if exp_pd_version != actual_version:
               self._tag_failure(command)
        else:
            self._tag_failure(command)
            logging.error('Failed to locate version from ectool')

    def check_lsb_info(self, command, fieldname, exp_value):
        """
        Comapre output of fieldname in /etc/lsb-release to exp_value.

        @param command: command string
        @param fieldname: field name in lsd-release file.
        @param exp_value: expected value for fieldname

        """
        lsb_info = 'cat /etc/lsb-release'
        lines = self.run_cmd(lsb_info)
        pattern = re.compile(fieldname + '=(.*)$')
        for line in lines:
            matched = pattern.match(line)
            if matched:
                actual = matched.group(1)
                logging.info('Expected %s %s actual %s',
                             fieldname, exp_value, actual)
                # Some board will have prefix.  Example nyan_big for big.
                if exp_value.lower() in actual.lower():
                  return
        self._tag_failure(command)

    def check_adb_devices(self, command, fieldname, exp_value):
        """
        Compare output of fieldname in adb devices -l to exp_value.

        @param command: command string
        @param fieldname: field name from adb devices -l output.
        @param exp_value: expected value for fieldname

        """
        device_info = 'adb devices -l'
        lines = self.faft_client.host.run_shell_command_get_output(device_info)
        logging.info(lines)
        pattern = re.compile(fieldname + ':(\S+)\s+')
        logging.info(pattern)
        for line in lines:
            matched = pattern.search(line)
            if matched:
                actual = matched.group(1)
                logging.info('Expected %s %s actual %s',
                             fieldname, exp_value, actual)
                # Some board will have prefix.  Example nyan_big for big.
                if exp_value.lower() in actual.lower():
                  return
        self._tag_failure(command)

    def _tag_failure(self, cmd):
        self.failed_command.append(cmd)
        logging.error('Execute %s failed', cmd)

    def run_once(self, dev_mode=False):
        # a. mosys -k smbios info bios
        command = 'mosys -k smbios info bios'
        if 'smbios' in self.command_list:
            output = self.run_cmd(command)[0]
            p = re.compile('vendor="coreboot" version="(.*)"'
                           ' release_date="[/0-9]+" size="[0-9]+ KB"')
            v = p.match(output)
            if not v:
              self._tag_failure(command)
            version = v.group(1)
            if not self.checkers.crossystem_checker({'fwid': version}):
              self._tag_failure(command)
        else:
            logging.warning('Skip "%s", command not available.', command)

        # b. mosys -k ec info
        command = 'mosys -k ec info'
        if self.faft_config.chrome_ec:
          output = self.run_cmd(command)[0]
          p = re.compile('vendor="[a-z]+" name="[ -~]+" fw_version="(.*)"')
          v = p.match(output)
          if v:
             version = v.group(1)
             self.check_ec_version(command, version)
          else:
            self._tag_failure(command)
        else:
          logging.info('Skip "%s", command not available.', command)

        # c. mosys platform name
        command = 'mosys platform name'
        output = self.run_cmd(command)[0]
        if self.faft_client.system.has_host():
            self.check_adb_devices(command, 'product', output)
        else:
            self.check_lsb_info(command, 'CHROMEOS_RELEASE_BOARD', output)

        # d. mosys eeprom map
        command = 'mosys eeprom map|egrep "RW_SHARED|RW_SECTION_[AB]"'
        lines = self.run_cmd(command)
        if len(lines) != 3:
          logging.error('Expect RW_SHARED|RW_SECTION_[AB] got "%s"', lines)
          self._tag_failure(command)
        emap = {'RW_SECTION_A': 0, 'RW_SECTION_B': 0, 'RW_SHARED': 0}
        for line in lines:
            row = line.split(' | ')
            if row[1] in emap:
                emap[row[1]] += 1
            if row[2] == '0x00000000':
                logging.error('Expect non zero but got %s instead(%s)',
                              (row[2], line))
                self._tag_failure(command)
            if row[3] == '0x00000000':
                logging.error('Expect non zero but got %s instead(%s)',
                              (row[3], line))
                self._tag_failure(command)
        # Check that there are one A and one B.
        if emap['RW_SECTION_A'] != 1 or emap['RW_SECTION_B'] != 1:
            logging.error('Missing RW_SECTION A or B, %s', lines)
            self._tag_failure(command)

        # e. mosys platform vendor
        # Output will be GOOGLE until launch, see crosbug/p/29755
        command = 'mosys platform vendor'
        output = self.run_cmd(command)[0]
        p = re.compile('^[-\w\s]+$')
        if not p.match(output):
            logging.error('output is not a string Expect GOOGLE'
                          'or name of maker.')
            self._tag_failure(command)

        # f. mosys -k pd info
        command = 'mosys -k pd info'
        if 'pd' in self.command_list:
          output = self.run_cmd(command)[0]
          p = re.compile('vendor="[a-z]+" name="[ -~]+" fw_version="(.*)"')
          v = p.match(output)
          if v:
             version = v.group(1)
             self.check_pd_version(command, version)
          else:
             self._tag_failure(command)
        else:
          logging.info('Skip "%s", command not available.', command)

        # Add any other mosys commands or tests before this section.
        # empty failed_command indicate all passed.
        if self.failed_command:
          raise error.TestFail('%d commands failed, detail above.  '
                               'Failed commands are "%s"' %
                               (len(self.failed_command),
                               ','.join(self.failed_command)))

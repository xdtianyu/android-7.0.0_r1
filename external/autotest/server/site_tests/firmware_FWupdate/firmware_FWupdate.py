# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import shutil
import tempfile

from chromite.lib import remote_access
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_FWupdate(FirmwareTest):
    """RO+RW firmware update using chromeos-firmware --mode=[recovery|factory]

    Setup Steps:
    1. Check the device is in normal mode for recovery or
       Check the device is in dev mode for factory

    Test Steps:
    2. extract shellball and repack with new bios.bin and ec.bin
    3. run --mode=recovery
    4. reboot

    Verification Steps:
    1. Step 3 should result into a success message
    2. Run crossystem and check fwid and ro_fwid should display the new bios
       firmware version string.
    4. Run ectool version to check ec version. The RO version and RW version
       strings should display new ec firmware strings.
    """

    version = 1

    SHELLBALL_ORG = '/usr/sbin/chromeos-firmwareupdate'
    SHELLBALL_COPY = '/home/root/chromeos-firmwareupdate'

    def initialize(self, host, cmdline_args):
        dict_args = utils.args_to_dict(cmdline_args)
        super(firmware_FWupdate, self).initialize(host, cmdline_args)
        if not set(('new_ec', 'new_bios')).issubset(set(dict_args)):
          raise error.TestError('Missing new_ec and/or new_bios argument')
        self.new_ec = dict_args['new_ec']
        self.new_bios = dict_args['new_bios']
        if not os.path.isfile(self.new_ec) or not os.path.isfile(self.new_bios):
          raise error.TestError('Failed to locate ec or bios file')
        self.new_pd = ''
        if 'new_pd' in dict_args:
          self.new_pd = dict_args['new_pd']
          if not os.path.isfile(self.new_pd):
            raise error.TestError('Failed to locate pd file')
        logging.info('EC=%s BIOS=%s PD=%s',
                     self.new_ec, self.new_bios, self.new_pd)
        self.mode = 'recovery'
        if 'mode' in dict_args:
          self.mode = dict_args['mode']
          if self.mode == 'recovery':
            self.switcher.setup_mode('normal')  # Set device to normal mode
          elif self.mode == 'factory':
            self.switcher.setup_mode('dev')   # Set device to dev mode
          else:
            raise error.TestError('Unknown mode:%s' % self.mode)

    def local_run_cmd(self, command):
        """Execute command on local system.

        @param command: shell command to be executed on local system.
        @returns command output.
        """
        logging.info('Execute %s', command)
        output = utils.system_output(command)
        logging.info('Output %s', output)
        return output

    def dut_run_cmd(self, command):
        """Execute command on DUT.

        @param command: shell command to be executed on DUT.
        @returns command output.
        """
        logging.info('Execute %s', command)
        output = self.faft_client.system.run_shell_command_get_output(command)
        logging.info('Output %s', output)
        return output

    def get_pd_version(self):
        """Get pd firmware version.

        @returns pd firmware version string if available.
        """
        if self.new_pd:
            return self.dut_run_cmd('mosys -k pd info')[0].split('"')[5]
        return ''

    def get_system_setup(self):
        """Get and return DUT system params.

        @returns DUT system params needed for this test.
        """
        return {
          'pd_version': self.get_pd_version(),
          'ec_version': self.faft_client.ec.get_version(),
          'mainfw_type':
            self.faft_client.system.get_crossystem_value('mainfw_type'),
          'ro_fwid':
            self.faft_client.system.get_crossystem_value('ro_fwid'),
          'fwid':
            self.faft_client.system.get_crossystem_value('fwid'),
        }

    def repack_shellball(self, hostname):
        """Repack DUT shellball and replace on DUT.

        @param hostname: hostname of DUT.
        """
        extract_dir = tempfile.mkdtemp(prefix='extract', dir='/tmp')

        self.dut_run_cmd('mkdir %s' % extract_dir)
        self.dut_run_cmd('cp %s %s' % (self.SHELLBALL_ORG, self.SHELLBALL_COPY))
        self.dut_run_cmd('%s --sb_extract %s' % (self.SHELLBALL_COPY,
                                                 extract_dir))

        dut_access = remote_access.RemoteDevice(hostname, username='root')
        self.dut_run_cmd('cp %s %s' % (self.SHELLBALL_ORG, self.SHELLBALL_COPY))

        # Replace bin files.
        target_file = '%s/%s' % (extract_dir, 'ec.bin')
        dut_access.CopyToDevice(self.new_ec, target_file, mode='scp')
        target_file = '%s/%s' % (extract_dir, 'bios.bin')
        dut_access.CopyToDevice(self.new_bios, target_file,  mode='scp')

        if self.new_pd:
          target_file = '%s/%s' % (extract_dir, 'pd.bin')
          dut_access.CopyToDevice(self.new_pd, target_file,  mode='scp')

        self.dut_run_cmd('%s --sb_repack %s' % (self.SHELLBALL_COPY,
                                                extract_dir))

        # Call to "shar" in chromeos-firmwareupdate might fail and the repack
        # ignore failure and exit with 0 status (http://crosbug.com/p/33719).
        # Add additional check to ensure the repack is successful.
        command = 'tail -1 %s' % self.SHELLBALL_COPY
        output = self.dut_run_cmd(command)
        if 'exit 0' not in output:
          raise error.TestError('Failed to repack %s' % self.SHELLBALL_COPY)

    def get_fw_bin_version(self):
        """Get firmwware version from binary file.

        @returns verions for bios, ec, pd
        """
        bios_version = self.local_run_cmd('strings %s|grep Google_|head -1'
                                              % self.new_bios)
        ec_version = self.local_run_cmd('strings %s|head -1' % self.new_ec)
        pd_version = ''
        if self.new_pd:
            pd_version = self.local_run_cmd('strings %s|head -1' % self.new_pd)
        return (bios_version, ec_version, pd_version)

    def run_once(self, host):
        """Run chromeos-firmwareupdate with recovery or factory mode.

        @param host: host to run on
        """
        crossystem_before = self.get_system_setup()
        (bios_version, ec_version, pd_version) = self.get_fw_bin_version()

        # Repack shellball with new ec and bios.
        self.repack_shellball(host.hostname)

        # Flash DUT with new bios/ec.
        command = '%s --mode=%s' % (self.SHELLBALL_COPY, self.mode)
        self.dut_run_cmd(command)
        host.reboot()

        # Extract and verify DUT state.
        crossystem_after = self.get_system_setup()
        logging.info('crossystem BEFORE: %s', crossystem_before)
        logging.info('crossystem AFTER: %s', crossystem_after)
        logging.info('Expects bios %s', bios_version)
        logging.info('Expects ec %s', ec_version)
        logging.info('Expects pd %s', pd_version)
        assert bios_version == crossystem_after['fwid']
        assert bios_version == crossystem_after['ro_fwid']
        assert ec_version == crossystem_after['ec_version']
        assert pd_version == crossystem_after['pd_version']

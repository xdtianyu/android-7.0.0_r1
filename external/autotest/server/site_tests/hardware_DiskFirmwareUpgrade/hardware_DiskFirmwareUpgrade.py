# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, os.path, re
from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, test

class hardware_DiskFirmwareUpgrade(test.test):
    """
    Integrity stress test for storage device
    """
    version = 1

    TEST_NAME='hardware_DiskFirmwareUpgrade'
    TEST_SCRIPT='/usr/sbin/chromeos-disk-firmware-update.sh'
    DEFAULT_LOCATION='/opt/google/disk/firmware'

    _client_install_path = None


    def _exists_on_client(self, f):
        return self._client.run('ls "%s"' % f,
                               ignore_status=True).exit_status == 0

    def _get_model_name(self):
        """ Return the name of an ATA/SCSI device. """
        return self._client.run(
            'cat /sys/block/$(basename $(rootdev -s -d))/device/model').stdout

    def _get_device_name(self):
        """ Return the name of an eMMC device, using cid data."""
        return self._client.run(
            'cat /sys/block/$(basename $(rootdev -s -d))/device/cid | cut -c 7-18').stdout

    def run_once(self, host, disk_fw_packages):
        """
        For every firmware package in disk_fw_packages, we launch the sibbling
        client test if:
        - the script to install the package is present
        - the model of the device present matches the defined model regex.
        We launch the slibbing client test a second time to put the machine
        in a well-known state.

        @param host:     machine to use.
        @param disk_fw_packages: directory of firmare to use and
                         expected return code. See control for details.
        """

        self._client = host
        self._client_at = autotest.Autotest(self._client)
        # First, check if the machine image contains the
        # upgrade script.
        if not self._exists_on_client(self.TEST_SCRIPT):
            raise error.TestNAError('Firmware upgrade not supported')

        # Retrieve model name.
        try:
            model = self._get_model_name()
        except error.AutoservRunError:
            model = self._get_device_name()

        i = 0
        for model_re, package_desc in disk_fw_packages.iteritems():
            if not re.match(model_re, model):
                continue
            for p, results in package_desc.iteritems():
                result_dir = '-'.join([self.TEST_NAME, str(i), p])
                if p.startswith('test_'):
                    self._client_at.run_test(
                            self.TEST_NAME,
                            results_dir=result_dir,
                            disk_firmware_package=self.DEFAULT_LOCATION + '-test',
                            expected_result=results[0],
                            upgrade_required=results[1])
                else:
                    # We are not expecting downloads.
                    self._tmpdir = self._client.get_tmp_dir()
                    self._client.send_file(os.path.join(self.bindir, p),
                                           self._tmpdir)
                    self._client_at.run_test(
                            self.TEST_NAME,
                            results_dir=result_dir,
                            disk_firmware_package=os.path.join(self._tmpdir, p),
                            expected_result=results[0],
                            upgrade_required=results[1])
                result_dir = '-'.join([self.TEST_NAME, str(i), '~base'])
                self._client_at.run_test(
                        self.TEST_NAME,
                        results_dir=result_dir,
                        disk_firmware_package=self.DEFAULT_LOCATION,
                        upgrade_required=results[1])
                i += 1


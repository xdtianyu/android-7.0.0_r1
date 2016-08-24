# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import glob
import logging
import json
import os
import tempfile

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cros_disks import CrosDisksTester
from autotest_lib.client.cros.cros_disks import ExceptionSuppressor
from autotest_lib.client.cros.cros_disks import VirtualFilesystemImage
from autotest_lib.client.cros.cros_disks import DefaultFilesystemTestContent


class CrosDisksFilesystemTester(CrosDisksTester):
    """A tester to verify filesystem support in CrosDisks.
    """
    def __init__(self, test, test_configs):
        super(CrosDisksFilesystemTester, self).__init__(test)
        self._test_configs = test_configs

    def _run_test_config(self, config):
        logging.info('Testing "%s"', config['description'])
        test_mount_filesystem_type = config.get('test_mount_filesystem_type')
        test_mount_options = config.get('test_mount_options')
        is_write_test = config.get('is_write_test', False)

        # Create a virtual filesystem image based on the specified parameters in
        # the test configuration and use it to verify that CrosDisks can
        # recognize and mount the filesystem properly.
        with VirtualFilesystemImage(
                block_size=config['block_size'],
                block_count=config['block_count'],
                filesystem_type=config['filesystem_type'],
                mount_filesystem_type=config.get('mount_filesystem_type'),
                mkfs_options=config.get('mkfs_options')) as image:
            image.format()
            image.mount(options=['sync'])
            test_content = DefaultFilesystemTestContent()

            # If it is not a write test, create the test content on the virtual
            # filesystem so that they can be read later after CrosDisks mounts
            # the filesystem.
            if not is_write_test and not test_content.create(image.mount_dir):
                raise error.TestFail("Failed to create filesystem test content")
            image.unmount()

            device_file = image.loop_device
            self.cros_disks.mount(device_file, test_mount_filesystem_type,
                                  test_mount_options)
            expected_mount_completion = {
                'status': config['expected_mount_status'],
                'source_path': device_file,
            }
            if 'expected_mount_path' in config:
                expected_mount_completion['mount_path'] = \
                    config['expected_mount_path']
            result = self.cros_disks.expect_mount_completion(
                    expected_mount_completion)

            actual_mount_path = result['mount_path']

            # If it is a write test, create the test content on the filesystem
            # after it is mounted by CrosDisks.
            if is_write_test and not test_content.create(actual_mount_path):
                raise error.TestFail("Failed to create filesystem test content")

            if not test_content.verify(actual_mount_path):
                raise error.TestFail("Failed to verify filesystem test content")
            self.cros_disks.unmount(device_file, ['force'])

    def test_using_virtual_filesystem_image(self):
        try:
            for config in self._test_configs:
                self._run_test_config(config)
        except RuntimeError:
            cmd = 'ls -la %s' % tempfile.gettempdir()
            logging.debug(utils.run(cmd))
            raise

    def get_tests(self):
        return [self.test_using_virtual_filesystem_image]


class platform_CrosDisksFilesystem(test.test):
    version = 1

    def run_once(self, *args, **kwargs):
        test_configs = []
        config_file = '%s/%s' % (self.bindir, kwargs['config_file'])
        with open(config_file, 'rb') as f:
            test_configs.extend(json.load(f))

        tester = CrosDisksFilesystemTester(self, test_configs)
        tester.run(*args, **kwargs)

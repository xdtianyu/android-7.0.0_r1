# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import json

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cros_disks import CrosDisksTester
from autotest_lib.client.cros.cros_disks import VirtualFilesystemImage
from autotest_lib.client.cros.cros_disks import DefaultFilesystemTestContent


class CrosDisksFormatTester(CrosDisksTester):
    """A tester to verify format support in CrosDisks.
    """
    def __init__(self, test, test_configs):
        super(CrosDisksFormatTester, self).__init__(test)
        self._test_configs = test_configs

    def _run_test_config(self, config):
        logging.info('Testing "%s"', config['description'])
        filesystem_type = config['filesystem_type']
        format_options = config.get('format_options')
        # Create a zero-filled virtual filesystem image to help stimulate
        # a removable drive.
        with VirtualFilesystemImage(
                block_size=1024,
                block_count=65536,
                filesystem_type=filesystem_type) as image:
            # Attach the zero-filled virtual filesystem image to a loop device
            # without actually formatting it.
            device_file = image.attach_to_loop_device()

            # Format the virtual filesystem image via CrosDisks.
            self.cros_disks.format(device_file, filesystem_type, format_options)
            expected_format_completion = {
                'path': device_file
            }
            if 'expected_format_status' in config:
                expected_format_completion['status'] = \
                        config['expected_format_status']
            result = self.cros_disks.expect_format_completion(
                expected_format_completion)

            if result['status'] == 0:
                # Test creating and verifying content the formatted device.
                logging.info("Test filesystem access on formatted device")
                test_content = DefaultFilesystemTestContent()
                mount_path = image.mount()
                if not test_content.create(mount_path):
                    raise error.TestFail("Failed to create test content")
                if not test_content.verify(mount_path):
                    raise error.TestFail("Failed to verify test content")

    def test_using_virtual_filesystem_image(self):
        for config in self._test_configs:
            self._run_test_config(config)

    def get_tests(self):
        return [self.test_using_virtual_filesystem_image]


class platform_CrosDisksFormat(test.test):
    version = 1

    def run_once(self, *args, **kwargs):
        test_configs = []
        config_file = '%s/%s' % (self.bindir, kwargs['config_file'])
        with open(config_file, 'rb') as f:
            test_configs.extend(json.load(f))

        tester = CrosDisksFormatTester(self, test_configs)
        tester.run(*args, **kwargs)

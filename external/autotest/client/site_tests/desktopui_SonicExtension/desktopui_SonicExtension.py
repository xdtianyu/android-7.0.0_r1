# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import socket
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chromedriver
from autotest_lib.client.cros.video import histogram_verifier

import config_manager
import test_utils

#histogram fields to be verified
MEDIA_GVD_INIT_STATUS = 'Cast.Sender.VideoEncodeAcceleratorInitializeSuccess'
MEDIA_GVD_BUCKET = 1

class desktopui_SonicExtension(test.test):
    """Test loading the sonic extension through chromedriver."""
    version = 1
    wait_time = 5
    dep = 'sonic_extension'

    def _install_sonic_extension(self):
        dep_dir = os.path.join(self.autodir, 'deps', self.dep)
        logging.info('Installing sonic extension into %s', dep_dir)
        self.job.install_pkg(self.dep, 'dep', dep_dir)
        return dep_dir


    def _check_manifest(self, extension_path):
        """Checks the manifest for a public key.

        The sonic extension is an autotest dependency and will get
        installed through install_pkg as a component extension (with
        a public key). Any other method of installation is supported
        too, as long as it has a public key.

        @param extension_path: A path to the directory of the extension
            that contains a manifest.json.

        @raises TestError: If the extension doesn't have a public key.
        """
        manifest_json_file = os.path.join(extension_path, 'manifest.json')
        with open(manifest_json_file, 'r') as f:
            manifest_json = json.loads(f.read())
            if not manifest_json.get('key'):
                raise error.TestError('Not a component extension, cannot '
                                      'proceed with sonic test')


    def initialize(self, test_config, sonic_hostname, sonic_build='00000',
        extension_dir=None):
        """Initialize the test.

        @param extension_dir: Directory of a custom extension.
            If one isn't supplied, the latest ToT extension is
            downloaded and loaded into chromedriver.
        @param live: Use a live url if True. Start a test server
            and server a hello world page if False.
        """
        super(desktopui_SonicExtension, self).initialize()

        if not extension_dir:
            self._extension_dir = self._install_sonic_extension()
        else:
            self._extension_dir = extension_dir
        if not os.path.exists(self._extension_dir):
            raise error.TestError('Failed to install sonic extension.')
        logging.info('extension: %s', self._extension_dir)
        self._check_manifest(self._extension_dir)
        self._test_utils_page = 'e2e_test_utils.html'
        self._test_config = test_config
        self._sonic_hostname = sonic_hostname
        self._sonic_build = sonic_build
        self._settings = config_manager.ConfigurationManager(
                self._test_config).get_config_settings()
        self._test_utils = test_utils.TestUtils()


    def cleanup(self):
        """Clean up the test environment, e.g., stop local http server."""
        super(desktopui_SonicExtension, self).cleanup()

    def _get_run_information(self, driver, settings):
        """Get all the information about the test run.

        @param driver: The webdriver instance of the test
        @param settings: The settings and information about the test
        @return A json that contains all the different information
            about the test run
        """
        information = {}
        if 'machine_name' in settings:
            information['machine_name'] = settings['machine_name']
        else:
            information['machine_name'] = socket.gethostname()
        information['network_profile'] = settings['network_profile']
        information['chrome_version'] = self._test_utils.get_chrome_version(
                driver)
        information['chrome_revision'] = self._test_utils.get_chrome_revision(
                driver)
        information['sonic_build'] = self._sonic_build
        information['video_name'] = settings.get('video_name',
                                                 settings['video_site'])
        information['comments'] = settings['comments']
        return information

    def run_once(self):
        """Run the test code."""
        # TODO: When we've cloned the sonic test repo get these from their
        # test config files.
        logging.info('Starting sonic client test.')
        kwargs = {
            'extension_paths': [self._extension_dir],
            'is_component': True,
            'extra_chrome_flags': [self._settings['extra_flags']],
        }
        with chromedriver.chromedriver(**kwargs) as chromedriver_instance:
            driver = chromedriver_instance.driver
            extension = chromedriver_instance.get_extension(
                    self._extension_dir)
            extension_id = extension.extension_id
            time.sleep(self.wait_time)
            self._test_utils.close_popup_tabs(driver)
            self._test_utils.block_setup_dialog(driver, extension_id)
            test_info = self._get_run_information(driver, self._settings)
            logging.info('Starting tabcast to extension: %s', extension_id)
            self._test_utils.set_mirroring_options(
                    driver, extension_id, self._settings)
            current_tab_handle = driver.current_window_handle
            self._test_utils.start_v2_mirroring_test_utils(
                    driver, extension_id, self._sonic_hostname,
                    self._settings['video_site'],
                    self._settings['full_screen'] == 'on')
            self._test_utils.set_focus_tab(driver, current_tab_handle)
            driver.switch_to_window(current_tab_handle)
            cpu_usage = self._test_utils.cpu_usage_interval(
                    int(self._settings['mirror_duration']))
            self._test_utils.stop_v2_mirroring_test_utils(driver, extension_id)
            crash_id = self._test_utils.upload_v2_mirroring_logs(
                    driver, extension_id)
            test_info['crash_id'] = crash_id
            if self._settings.get('sender_root_dir'):
                cpu_bound = self._test_utils.compute_cpu_utilization(cpu_usage)
                info_json_file = os.path.join(self._settings['sender_root_dir'],
                                              'test_information.json')
                cpu_json_file = os.path.join(
                        self._settings['sender_root_dir'], 'cpu_data.json')
                cpu_bound_json_file = os.path.join(
                        self._settings['sender_root_dir'], 'cpu_bound.json')
                json.dump(test_info, open(info_json_file, 'wb'))
                json.dump(cpu_usage, open(cpu_json_file, 'wb'))
                json.dump(cpu_bound, open(cpu_bound_json_file, 'wb'))
            time.sleep(self.wait_time)
            #To cehck encoder acceleration used while casting
            histogram_verifier.verify(
                 chromedriver_instance.chrome_instance,
                 MEDIA_GVD_INIT_STATUS, MEDIA_GVD_BUCKET)

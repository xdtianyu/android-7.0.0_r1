# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import socket

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.server import afe_utils
from autotest_lib.server import test


_CONFIG = global_config.global_config
# pylint: disable-msg=E1120
_IMAGE_URL_PATTERN = _CONFIG.get_config_value(
        'CROS', 'image_url_pattern', type=str)


class provision_AutoUpdate(test.test):
    """A test that can provision a machine to the correct ChromeOS version."""
    version = 1

    def initialize(self, host, value, force=False, is_test_na=False):
        """Initialize.

        @param host: The host object to update to |value|.
        @param value: The build type and version to install on the host.
        @param force: not used by initialize.
        @param is_test_na: boolean, if True, will simply skip the test
                           and emit TestNAError. The control file
                           determines whether the test should be skipped
                           and passes the decision via this argument. Note
                           we can't raise TestNAError in control file as it won't
                           be caught and handled properly.
        """
        if is_test_na:
            raise error.TestNAError(
                'Test not available for test_that. chroot detected, '
                'you are probably using test_that.')
        # We check value in initialize so that it fails faster.
        if not value:
            raise error.TestFail('No build version specified.')


    @staticmethod
    def log_devserver_match_stats(dut_hostname, devserver_url):
        """Log stats whether host and devserver are in the same subnet.

        @param dut_hostname: Hostname of the dut.
        @param devserver_url: Url to the devserver.
        """
        try:
            devserver_name = dev_server.ImageServer.get_server_name(
                    devserver_url)
            devserver_ip = socket.gethostbyname(devserver_name)
            dut_ip = socket.gethostbyname(dut_hostname)
        except socket.gaierror as e:
            logging.error('Failed to get IP address, error: %s', e)
            return

        # Take the first 2 octets as the indicator of subnet.
        devserver_subnet = '_'.join(devserver_ip.split('.')[0:2])
        dut_subnet = '_'.join(dut_ip.split('.')[0:2])
        if not utils.is_in_same_subnet(devserver_ip, dut_ip, 19):
            counter = ('devserver_mismatch.%s_to_%s' %
                       (devserver_subnet, dut_subnet))
            autotest_stats.Counter(counter).increment()
            counter = 'devserver_mismatch.%s' % devserver_subnet
        else:
            counter = 'devserver_match.%s' % devserver_subnet

        autotest_stats.Counter(counter).increment()


    def run_once(self, host, value, force=False):
        """The method called by the control file to start the test.

        @param host: The host object to update to |value|.
        @param value: The host object to provision with a build corresponding
                      to |value|.
        @param force: True iff we should re-provision the machine regardless of
                      the current image version.  If False and the image
                      version matches our expected image version, no
                      provisioning will be done.

        """
        logging.debug('Start provisioning %s to %s', host, value)
        image = value

        # If the host is already on the correct build, we have nothing to do.
        # Note that this means we're not doing any sort of stateful-only
        # update, and that we're relying more on cleanup to do cleanup.
        # We could just not pass |force_update=True| to |machine_install|,
        # but I'd like the semantics that a provision test 'returns' TestNA
        # if the machine is already properly provisioned.
        if not force and afe_utils.get_build(host) == value:
            # We can't raise a TestNA, as would make sense, as that makes
            # job.run_test return False as if the job failed.  However, it'd
            # still be nice to get this into the status.log, so we manually
            # emit an INFO line instead.
            self.job.record('INFO', None, None,
                            'Host already running %s' % value)
            return

        # We're about to reimage a machine, so we need full_payload and
        # stateful.  If something happened where the devserver doesn't have one
        # of these, then it's also likely that it'll be missing autotest.
        # Therefore, we require the devserver to also have autotest staged, so
        # that the test that runs after this provision finishes doesn't error
        # out because the devserver that its job_repo_url is set to is missing
        # autotest test code.
        # TODO(milleral): http://crbug.com/249426
        # Add an asynchronous staging call so that we can ask the devserver to
        # fetch autotest in the background here, and then wait on it after
        # reimaging finishes or at some other point in the provisioning.
        try:
            ds = dev_server.ImageServer.resolve(image, host.hostname)
            ds.stage_artifacts(image, ['full_payload', 'stateful',
                                       'autotest_packages'])
        except dev_server.DevServerException as e:
            raise error.TestFail(str(e))

        self.log_devserver_match_stats(host.hostname, ds.url())

        url = _IMAGE_URL_PATTERN % (ds.url(), image)

        logging.debug('Installing image')
        try:
            afe_utils.machine_install_and_update_labels(host,
                                                        force_update=True,
                                                        update_url=url,
                                                        force_full_update=force)
        except error.InstallError as e:
            logging.error(e)
            raise error.TestFail(str(e))
        logging.debug('Finished provisioning %s to %s', host, value)

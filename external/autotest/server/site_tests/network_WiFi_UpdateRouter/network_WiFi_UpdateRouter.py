# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import hosts
from autotest_lib.server import site_linux_router
from autotest_lib.server import test
from autotest_lib.server.cros.network import wifi_test_context_manager


# Stable versions come from the builders.
# The builder version is used to build the URL of the corresponding image
# in Google Storage.
# The image version is a line from /etc/lsb-release in the corresponding image.
StableVersion = collections.namedtuple('StableVersion',
                                       ['builder_version', 'release_version'])

class network_WiFi_UpdateRouter(test.test):
    """Updates a router to the most recent stable version.

    This is not a test per se, since it does not test client behavior.  However
    it is advantageous to write this as a test so that we can schedule it to
    run periodically via the same infrastructure we use to run tests.

    Note that this test is very much patterned on provision_AutoUpdate.

    """
    version = 1

    STABLE_VERSIONS = {
        'stumpy': StableVersion('trybot-stumpy-test-ap/R47-7424.0.0-b10',
                                '7424.0.2015_09_03_1514'),
        'panther': StableVersion('trybot-panther-test-ap/R47-7424.0.0-b10',
                                 '7424.0.2015_09_03_1532'),
        'whirlwind': StableVersion('trybot-whirlwind-test-ap/R50-7849.0.0-b13',
                                   '7849.0.2016_01_20_2033')
    }


    def get_release_version(self, host):
        result = host.run('cat /etc/lsb-release')
        for line in result.stdout.splitlines():
            if line.startswith('CHROMEOS_RELEASE_VERSION='):
                return line.split('=', 1)[1]


    def get_update_url(self, ds_url, image):
        CONFIG = global_config.global_config
        # pylint: disable-msg=E1120
        IMAGE_URL_PATTERN = CONFIG.get_config_value(
                'CROS', 'image_url_pattern', type=str)
        return IMAGE_URL_PATTERN % (ds_url, image)


    def warmup(self, raw_cmdline_args):
        """Possibly parse the router hostname from the commandline.

        @param raw_cmdline_args raw input from autotest.

        """
        cmdline_args = utils.args_to_dict(raw_cmdline_args)
        logging.info('Running wifi test with commandline arguments: %r',
                     cmdline_args)
        self._router_hostname_from_cmdline = cmdline_args.get(
                wifi_test_context_manager.WiFiTestContextManager. \
                        CMDLINE_ROUTER_ADDR)


    def run_once(self, host):
        router_hostname = site_linux_router.build_router_hostname(
                client_hostname=host.hostname,
                router_hostname=self._router_hostname_from_cmdline)
        router_host = hosts.create_host(router_hostname)
        board = router_host.get_board().split(':', 1)[1]  # Remove 'board:'
        desired = self.STABLE_VERSIONS.get(board, None)
        if desired is None:
            raise error.TestFail('No stable version found for for router with '
                                 'board=%s.' % board)

        logging.info('Checking whether router is at the latest '
                     'stable version: %s', desired.release_version)
        current_release_version = self.get_release_version(router_host)
        if desired.release_version == current_release_version:
            raise error.TestNAError('%s is already at latest version %s.' %
                                    (router_hostname, desired.release_version))

        logging.info('Updating %s to image %s from %s',
                     router_hostname, desired.release_version,
                     current_release_version)
        logging.info('Staging artifacts.')
        try:
            ds = dev_server.ImageServer.resolve(desired.builder_version)
            ds.stage_artifacts(desired.builder_version,
                               ['full_payload', 'stateful'])
        except dev_server.DevServerException as e:
            logging.error(e)
            raise error.TestFail(str(e))

        url = self.get_update_url(ds.url(), desired.builder_version)
        try:
            router_host.machine_install(force_update=True, update_url=url)
        except error.InstallError as e:
            logging.error(e)
            raise error.TestFail(str(e))

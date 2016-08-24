# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side external display test using the Chameleon board."""

import logging
import os
import random
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.client.cros.chameleon import edid
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_SuspendStress(test.test):
    """Server side external display test.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    external display function of the DUT with DUT being repeatedly
    suspended and resumed.
    """
    version = 1
    DEFAULT_TESTCASE_SPEC = ('HDMI', 1920, 1080)

    # TODO: Allow reading testcase_spec from command line.
    def run_once(self, host, test_mirrored=False, testcase_spec=None,
                 repeat_count=3, suspend_time_range=(5,7)):
        if testcase_spec is None:
            testcase_spec = self.DEFAULT_TESTCASE_SPEC

        test_name = "%s_%dx%d" % testcase_spec
        _, width, height = testcase_spec
        test_resolution = (width, height)

        if not edid.is_edid_supported(host, *testcase_spec):
            raise error.TestFail('Error: EDID is not supported by the platform'
                    ': %s', test_name)

        edid_path = os.path.join(self.bindir, 'test_data', 'edids', test_name)

        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)
        for chameleon_port in finder.iterate_all_ports():
            screen_test = chameleon_screen_test.ChameleonScreenTest(
                    chameleon_port, display_facade, self.outputdir)

            logging.info('Use EDID: %s', test_name)
            with chameleon_port.use_edid_file(edid_path):
                # Keep the original connector name, for later comparison.
                expected_connector = utils.wait_for_value_changed(
                        display_facade.get_external_connector_name,
                        old_value=False)
                logging.info('See the display on DUT: %s', expected_connector)

                if not expected_connector:
                    raise error.TestFail('Error: Failed to see external display'
                            ' (chameleon) from DUT: %s', test_name)

                logging.info('Set mirrored: %s', test_mirrored)
                display_facade.set_mirrored(test_mirrored)
                logging.info('Repeat %d times Suspend and resume', repeat_count)

                count = repeat_count
                while count > 0:
                    count -= 1
                    if test_mirrored:
                        # magic sleep to make nyan_big wake up in mirrored mode
                        # TODO: find root cause
                        time.sleep(6)
                    suspend_time = random.randint(*suspend_time_range)
                    logging.info('Going to suspend, for %d seconds...',
                                 suspend_time)
                    display_facade.suspend_resume(suspend_time)
                    logging.info('Resumed back')

                    message = screen_test.check_external_display_connected(
                            expected_connector)
                    if not message:
                        message = screen_test.test_screen_with_image(
                                test_resolution, test_mirrored)
                    if message:
                        raise error.TestFail(message)

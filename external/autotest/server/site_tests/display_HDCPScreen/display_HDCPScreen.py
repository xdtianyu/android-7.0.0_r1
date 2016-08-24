# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side to enable HDCP and verify screen."""

import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_HDCPScreen(test.test):
    """Server side test to enable HDCP and verify screen.

    This test forces CrOS to enable HDCP and compares screens between CrOS
    and Chameleon.
    """
    version = 1

    TEST_CONFIGS = [
        # (enable_chameleon, request_cros, expected_cros_state,
        #  expected_chameleon_state)
        (True, 'Desired', 'Enabled', True),
        (False, 'Desired', 'Desired', False),
        # TODO: Investigate the case below which was disabled as it failed.
        # Check http://crbug.com/447493
        #(True, 'Undesired', 'Undesired', False),
        (False, 'Undesired', 'Undesired', False),
    ]

    DURATION_UNPLUG_FOR_HDCP = 1
    TIMEOUT_HDCP_SWITCH = 10

    def run_once(self, host, test_mirrored=False):
        if host.get_architecture() != 'arm':
            raise error.TestNAError('HDCP is not supported on a non-ARM device')

        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)

        errors = []
        for chameleon_port in finder.iterate_all_ports():
            screen_test = chameleon_screen_test.ChameleonScreenTest(
                    chameleon_port, display_facade, self.outputdir)

            logging.info('See the display on Chameleon: port %d (%s)',
                         chameleon_port.get_connector_id(),
                         chameleon_port.get_connector_type())

            logging.info('Set mirrored: %s', test_mirrored)
            display_facade.set_mirrored(test_mirrored)

            resolution = display_facade.get_external_resolution()
            logging.info('Detected resolution on CrOS: %r', resolution)

            original_cros_state = display_facade.get_content_protection()
            was_chameleon_enabled = (
                    chameleon_port.is_content_protection_enabled())
            try:
                for (enable_chameleon, request_cros, expected_cros_state,
                     expected_chameleon_state) in self.TEST_CONFIGS:
                    # Do unplug and plug to emulate switching to a different
                    # display with a different content protection state.
                    chameleon_port.unplug()
                    logging.info('Set Chameleon HDCP: %r', enable_chameleon)
                    chameleon_port.set_content_protection(enable_chameleon)
                    time.sleep(self.DURATION_UNPLUG_FOR_HDCP)
                    chameleon_port.plug()
                    chameleon_port.wait_video_input_stable()

                    logging.info('Request CrOS HDCP: %s', request_cros)
                    display_facade.set_content_protection(request_cros)

                    state = utils.wait_for_value(
                            display_facade.get_content_protection, 'Enabled',
                            timeout_sec=self.TIMEOUT_HDCP_SWITCH)
                    logging.info('Got CrOS state: %s', state)
                    if state != expected_cros_state:
                        error_message = ('Failed to enable HDCP, state: %r' %
                                         state)
                        logging.error(error_message)
                        errors.append(error_message)

                    encrypted = chameleon_port.is_video_input_encrypted()
                    logging.info('Got Chameleon state: %r', encrypted)
                    if encrypted != expected_chameleon_state:
                        error_message = ('Chameleon found HDCP in wrong state: '
                                         'expected %r but got %r' %
                                         (expected_chameleon_state, encrypted))
                        logging.error(error_message)
                        errors.append(error_message)

                    logging.info('Test screen under HDCP %s...',
                                 'enabled' if encrypted else 'disabled')
                    screen_test.test_screen_with_image(
                            resolution, test_mirrored, errors)
            finally:
                display_facade.set_content_protection(original_cros_state)
                chameleon_port.set_content_protection(was_chameleon_enabled)

        if errors:
            raise error.TestFail('; '.join(set(errors)))

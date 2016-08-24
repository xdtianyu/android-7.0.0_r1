# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is test switching the external display mode."""

import logging, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_SwitchMode(test.test):
    """External Display switch between extended and mirrored modes.

    This test switches the external display mode between extended
    and mirrored modes, and checks resolution and static test image.
    """
    version = 1
    WAIT_AFTER_SWITCH = 5

    def check_external_display(self, test_mirrored):
        """Display status check

        @param test_mirrored: is mirrored mode active

        """
        resolution = self.display_facade.get_external_resolution()
        # Check connector
        if self.screen_test.check_external_display_connected(
                self.connector_used, self.errors) is None:
            # Check test image
            self.screen_test.test_screen_with_image(
                    resolution, test_mirrored, self.errors)
        if self.errors:
            raise error.TestFail('; '.join(set(self.errors)))


    def set_mode_and_check(self, test_mirrored):
        """Sets display mode and checks status

        @param test_mirrored: is mirrored mode active

        """
        logging.info('Set mirrored: %s', test_mirrored)
        self.display_facade.set_mirrored(test_mirrored)
        time.sleep(self.WAIT_AFTER_SWITCH)
        self.check_external_display(test_mirrored)


    def run_once(self, host, repeat):
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        self.display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, self.display_facade)

        self.errors = []
        for chameleon_port in finder.iterate_all_ports():
            self.chameleon_port = chameleon_port
            self.screen_test = chameleon_screen_test.ChameleonScreenTest(
                    chameleon_port, self.display_facade, self.outputdir)

            logging.debug('See the display on Chameleon: port %d (%s)',
                         self.chameleon_port.get_connector_id(),
                         self.chameleon_port.get_connector_type())
            # Keep the original connector name, for later comparison.
            self.connector_used = (
                    self.display_facade.get_external_connector_name())

            for i in xrange(repeat):
                logging.info("Iteration %d", (i + 1))
                self.set_mode_and_check(False)
                self.set_mode_and_check(True)

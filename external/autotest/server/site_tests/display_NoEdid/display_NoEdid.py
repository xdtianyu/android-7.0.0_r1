# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side test to check no EDID on external display."""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.client.cros.chameleon import edid
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_NoEdid(test.test):
    """Server side test to check no EDID on external display.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    the case that no EDID on the external display.
    """
    version = 1

    STANDARD_MODE_RESOLUTIONS = [(1024, 768), (1024, 720), (800, 600)]

    def run_once(self, host, test_mirrored=False):
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

            with chameleon_port.use_edid(edid.NO_EDID):
                connector_name = utils.wait_for_value_changed(
                            display_facade.get_external_connector_name,
                            old_value=False)
                if not connector_name:
                    error_message = 'Failed to detect display without an EDID'
                    logging.error(error_message)
                    errors.append(error_message)
                    continue

                logging.info('Set mirrored: %s', test_mirrored)
                display_facade.set_mirrored(test_mirrored)

                resolution = display_facade.get_external_resolution()
                if resolution not in self.STANDARD_MODE_RESOLUTIONS:
                    error_message = ('Switched to a non-standard mode: %r' %
                                     resolution)
                    logging.error(error_message)
                    errors.append(error_message)
                    continue

                screen_test.test_screen_with_image(
                        resolution, test_mirrored, errors)

        if errors:
            raise error.TestFail('; '.join(set(errors)))

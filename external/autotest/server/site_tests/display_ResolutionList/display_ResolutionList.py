# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side resolution display test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.client.cros.chameleon import edid
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_ResolutionList(test.test):
    """Server side external display test.

    This test iterates the resolution list obtained from the display options
    dialog and verifies that each of them works.
    """

    version = 1
    DEFAULT_RESOLUTION_LIST = [
            ('HDMI', 1920, 1080),
            ('DP', 1920, 1080),
    ]
    RESOLUTION_CHANGE_TIME = 2

    # TODO: Allow reading testcase_spec from command line.
    def run_once(self, host, test_mirrored=False, resolution_list=None):
        if not host.get_board_type() == 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped')
        if resolution_list is None:
            resolution_list = self.DEFAULT_RESOLUTION_LIST
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
            chameleon_port_name = chameleon_port.get_connector_type()
            logging.info('Detected %s chameleon port.', chameleon_port_name)
            for interface, width, height in resolution_list:
                if not chameleon_port_name.startswith(interface):
                    continue
                test_resolution = (width, height)
                test_name = "%s_%dx%d" % ((interface,) + test_resolution)

                if not edid.is_edid_supported(host, interface, width, height):
                    logging.info('Skip unsupported EDID: %s', test_name)
                    continue
                edid_path = os.path.join(self.bindir, 'test_data', 'edids',
                                    test_name)

                logging.info('Use EDID: %s', test_name)

                with chameleon_port.use_edid_file(edid_path):
                    index = utils.wait_for_value_changed(
                            display_facade.get_first_external_display_index,
                            old_value=False)
                    if not index:
                        raise error.TestFail("No external display is found.")

                    # In mirror mode only display index is '0', as external
                    # is treated same as internal(single resolution applies)
                    if test_mirrored:
                        index = 0
                    logging.info('Set mirrored: %s', test_mirrored)
                    display_facade.set_mirrored(test_mirrored)
                    settings_resolution_list = (
                            display_facade.get_available_resolutions(index))
                    if len(settings_resolution_list) == 0:
                        raise error.TestFail("No resolution list is found.")
                    logging.info('External display %d: %d resolutions found.',
                                index, len(settings_resolution_list))

                    for r in settings_resolution_list:
                        # FIXME: send a keystroke to keep display on.
                        # This is to work around a problem where the display may be
                        # turned off if the test has run for a long time (e.g.,
                        # greater than 15 min). When the display is off,
                        # set_resolution() will fail.
                        display_facade.hide_cursor()

                        logging.info('Set resolution to %dx%d', *r)
                        display_facade.set_resolution(index, *r)
                        time.sleep(self.RESOLUTION_CHANGE_TIME)

                        chameleon_port.wait_video_input_stable()
                        screen_test.test_screen_with_image(r, test_mirrored, errors)

            if errors:
                raise error.TestFail('; '.join(set(errors)))

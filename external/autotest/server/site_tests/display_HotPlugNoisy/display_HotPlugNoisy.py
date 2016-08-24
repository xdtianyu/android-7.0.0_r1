# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This test remotely emulates noisy HPD line when connecting to an external
display in extended mode using the Chameleon board."""

import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_HotPlugNoisy(test.test):
    """Noisy display HPD test.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    DUT behavior in response to noisy HPD line.
    """
    version = 1
    PLUG_CONFIGS = [
        # (plugged_before_noise, plugged_after_noise)

        (False, False),
        (False, True),
        (True, False),
        (True, True),
    ]

    # pulse segments in msec that end with plugged state
    PULSES_PLUGGED = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
    # pulse segments in msec that end with unplugged state
    PULSES_UNPLUGGED = PULSES_PLUGGED + [2048]

    REPLUG_DELAY_SEC = 1


    def run_once(self, host, test_mirrored=False):
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)

        errors = []
        warns = []
        for chameleon_port in finder.iterate_all_ports():
            screen_test = chameleon_screen_test.ChameleonScreenTest(
                    chameleon_port, display_facade, self.outputdir)

            logging.info('See the display on Chameleon: port %d (%s)',
                         chameleon_port.get_connector_id(),
                         chameleon_port.get_connector_type())

            logging.info('Set mirrored: %s', test_mirrored)
            display_facade.set_mirrored(test_mirrored)

            # Keep the original connector name, for later comparison.
            expected_connector = display_facade.get_external_connector_name()
            resolution = display_facade.get_external_resolution()
            logging.info('See the display on DUT: %s %r',
                         expected_connector, resolution)

            for (plugged_before_noise,
                 plugged_after_noise) in self.PLUG_CONFIGS:
                logging.info('TESTING THE CASE: %s > noise > %s',
                             'plug' if plugged_before_noise else 'unplug',
                             'plug' if plugged_after_noise else 'unplug')

                chameleon_port.set_plug(plugged_before_noise)

                if screen_test.check_external_display_connected(
                        expected_connector if plugged_before_noise else False,
                        errors):
                    # Skip the following test if an unexpected display detected.
                    continue

                chameleon_port.fire_mixed_hpd_pulses(
                        self.PULSES_PLUGGED if plugged_after_noise
                                            else self.PULSES_UNPLUGGED)

                if plugged_after_noise:
                    chameleon_port.wait_video_input_stable()
                    if test_mirrored:
                        # Wait for resolution change to make sure the resolution
                        # is stable before moving on. This is to deal with the
                        # case where DUT may respond slowly after the noise.
                        # If the resolution doesn't change, then we are
                        # confident that it is stable. Otherwise, a slow
                        # response is caught.
                        r = display_facade.get_internal_resolution()
                        utils.wait_for_value_changed(
                                display_facade.get_internal_resolution,
                                old_value=r)

                    err = screen_test.check_external_display_connected(
                            expected_connector)

                    if not err:
                        err = screen_test.test_screen_with_image(
                                resolution, test_mirrored)
                    if err:
                        # When something goes wrong after the noise, a normal
                        # user would try to re-plug the cable to recover.
                        # We emulate this behavior below and report error if
                        # the problem persists.
                        logging.warn('Possibly flaky: %s', err)
                        warns.append('Possibly flaky: %s' % err)
                        logging.info('Replug and retry the screen test...')
                        chameleon_port.unplug()
                        time.sleep(self.REPLUG_DELAY_SEC)
                        chameleon_port.plug()
                        chameleon_port.wait_video_input_stable()
                        screen_test.test_screen_with_image(
                                resolution, test_mirrored, errors)
                else:
                    screen_test.check_external_display_connected(False, errors)
                    time.sleep(1)

        if errors:
            raise error.TestFail('; '.join(set(errors)))
        elif warns:
            raise error.TestWarn('; '.join(set(warns)))

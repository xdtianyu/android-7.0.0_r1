# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a display hot-plug and reboot test using the Chameleon board."""

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_HotPlugAtBoot(test.test):
    """Display hot-plug and reboot test.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    DUT behavior response to different configuration of hot-plug during boot.
    """
    version = 1
    PLUG_CONFIGS = [
        # (plugged_before_boot, plugged_after_boot)
        (False, True),
        (True, True),
        (True, False),
    ]
    # Allowed timeout for reboot.
    REBOOT_TIMEOUT = 30

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

            for plugged_before_boot, plugged_after_boot in self.PLUG_CONFIGS:
                logging.info('TESTING THE CASE: %s > reboot > %s',
                             'plug' if plugged_before_boot else 'unplug',
                             'plug' if plugged_after_boot else 'unplug')
                boot_id = host.get_boot_id()
                chameleon_port.set_plug(plugged_before_boot)

                # Don't wait DUT up. Do plug/unplug while booting.
                logging.info('Reboot...')
                host.reboot(wait=False)

                host.test_wait_for_shutdown(
                        shutdown_timeout=self.REBOOT_TIMEOUT)
                chameleon_port.set_plug(plugged_after_boot)
                host.test_wait_for_boot(boot_id)

                if screen_test.check_external_display_connected(
                        expected_connector if plugged_after_boot else False,
                        errors):
                    # Skip the following test if an unexpected display detected.
                    continue

                if plugged_after_boot:
                    if test_mirrored and (
                            not display_facade.is_mirrored_enabled()):
                        error_message = 'Error: not rebooted to mirrored mode'
                        errors.append(error_message)
                        logging.error(error_message)
                        # Sets mirrored status for next test
                        logging.info('Set mirrored: %s', True)
                        display_facade.set_mirrored(True)
                        continue

                    screen_test.test_screen_with_image(
                            resolution, test_mirrored, errors)

        if errors:
            raise error.TestFail('; '.join(set(errors)))

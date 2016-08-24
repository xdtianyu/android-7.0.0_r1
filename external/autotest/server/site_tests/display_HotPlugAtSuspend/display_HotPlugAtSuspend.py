# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a display hot-plug and suspend test using the Chameleon board."""

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_HotPlugAtSuspend(test.test):
    """Display hot-plug and suspend test.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    DUT behavior response to different configuration of hot-plug during
    suspend/resume.
    """
    version = 1
    # Duration of suspend, in second.
    SUSPEND_DURATION = 30
    # Allowed timeout for the transition of suspend.
    SUSPEND_TIMEOUT = 20
    # Allowed timeout for the transition of resume.
    RESUME_TIMEOUT = 60
    # Time margin to do plug/unplug before resume.
    TIME_MARGIN_BEFORE_RESUME = 5


    def run_once(self, host, plug_status, test_mirrored=False):
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

            for (plugged_before_suspend, plugged_after_suspend,
                 plugged_before_resume) in plug_status:
                test_case = ('TEST CASE: %s > SUSPEND > %s > %s > RESUME' %
                    ('PLUG' if plugged_before_suspend else 'UNPLUG',
                     'PLUG' if plugged_after_suspend else 'UNPLUG',
                     'PLUG' if plugged_before_resume else 'UNPLUG'))
                logging.info(test_case)
                boot_id = host.get_boot_id()
                chameleon_port.set_plug(plugged_before_suspend)

                if screen_test.check_external_display_connected(
                        expected_connector if plugged_before_suspend else False,
                        errors):
                    # Skip the following test if an unexpected display detected.
                    continue

                logging.info('GOING TO SUSPEND FOR %d SECONDS...',
                             self.SUSPEND_DURATION)
                time_before_suspend = time.time()
                display_facade.suspend_resume_bg(self.SUSPEND_DURATION)

                # Confirm DUT suspended.
                logging.info('WAITING FOR SUSPEND...')
                try:
                    host.test_wait_for_sleep(self.SUSPEND_TIMEOUT)
                except error.TestFail, ex:
                    errors.append("%s - %s" % (test_case, str(ex)))
                if plugged_after_suspend is not plugged_before_suspend:
                    chameleon_port.set_plug(plugged_after_suspend)

                current_time = time.time()
                sleep_time = (self.SUSPEND_DURATION -
                              (current_time - time_before_suspend) -
                              self.TIME_MARGIN_BEFORE_RESUME)
                if sleep_time > 0:
                    logging.info('- Sleep for %.2f seconds...', sleep_time)
                    time.sleep(sleep_time)
                if plugged_before_resume is not plugged_after_suspend:
                    chameleon_port.set_plug(plugged_before_resume)
                time.sleep(self.TIME_MARGIN_BEFORE_RESUME)

                logging.info('WAITING FOR RESUME...')
                try:
                    host.test_wait_for_resume(boot_id, self.RESUME_TIMEOUT)
                except error.TestFail, ex:
                    errors.append("%s - %s" % (test_case, str(ex)))

                logging.info('Resumed back')

                if screen_test.check_external_display_connected(
                        expected_connector if plugged_before_resume else False,
                        errors):
                    # Skip the following test if an unexpected display detected.
                    continue

                if plugged_before_resume:
                    if test_mirrored and (
                            not display_facade.is_mirrored_enabled()):
                        error_message = 'Error: not resumed to mirrored mode'
                        errors.append("%s - %s" % (test_case, error_message))
                        logging.error(error_message)
                        logging.info('Set mirrored: %s', True)
                        display_facade.set_mirrored(True)
                    else:
                        screen_test.test_screen_with_image(
                                resolution, test_mirrored, errors)

        if errors:
            raise error.TestFail('; '.join(set(errors)))

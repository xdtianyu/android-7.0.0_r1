# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a display lid close and open test using the Chameleon board."""

import logging, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory

class display_LidCloseOpen(test.test):
    """External Display Lid Close/Open test. """
    version = 1

    # Time to check if device is suspended
    TIMEOUT_SUSPEND_CHECK = 5
    # Allowed timeout for the transition of suspend.
    TIMEOUT_SUSPEND_TRANSITION = 30
    # Allowed timeout for the transition of resume.
    TIMEOUT_RESUME_TRANSITION = 60
    # Time to allow for table video input
    WAIT_TIME_STABLE_VIDEO_INPUT = 10
    # Time to allow lid transition to take effect
    WAIT_TIME_LID_TRANSITION = 5
    # Time to allow display port plug transition to take effect
    WAIT_TIME_PLUG_TRANSITION = 5


    def close_lid(self):
        """Close lid through servo"""
        logging.info('CLOSING LID...')
        self.host.servo.lid_close()
        time.sleep(self.WAIT_TIME_LID_TRANSITION)


    def open_lid(self):
        """Open lid through servo"""
        logging.info('OPENING LID...')
        self.host.servo.lid_open()
        time.sleep(self.WAIT_TIME_LID_TRANSITION)


    def check_primary_display_on_internal_screen(self):
        """Checks primary display is on onboard/internal screen"""
        if not self.display_facade.is_display_primary(internal=True):
            raise error.TestFail('Primary display is not on internal screen')


    def check_primary_display_on_external_screen(self):
        """Checks primary display is on external screen"""
        if not self.display_facade.is_display_primary(internal=False):
            raise error.TestFail('Primary display is not on external screen')


    def check_mode(self):
        """Checks the display mode is as expected"""
        if self.display_facade.is_mirrored_enabled() is not self.test_mirrored:
            raise error.TestFail('Display mode %s is not preserved!' %
                                 ('mirrored' if self.test_mirrored
                                     else 'extended'))


    def check_docked(self):
        """Checks DUT is docked"""
        # Device does not suspend
        if self.host.ping_wait_down(timeout=self.TIMEOUT_SUSPEND_TRANSITION):
            raise error.TestFail('Device suspends when docked!')
        # Verify Chameleon displays main screen
        self.check_primary_display_on_external_screen()
        logging.info('DUT IS DOCKED!')
        return self.chameleon_port.wait_video_input_stable(
            timeout=self.WAIT_TIME_STABLE_VIDEO_INPUT)


    def check_still_suspended(self):
        """Checks DUT is (still) suspended"""
        if not self.host.ping_wait_down(timeout=self.TIMEOUT_SUSPEND_CHECK):
            raise error.TestFail('Device does not stay suspended!')
        logging.info('DUT STILL SUSPENDED')


    def check_external_display(self):
        """Display status check"""
        # Check connector
        if self.screen_test.check_external_display_connected(
                self.connector_used, self.errors) is None:
            # Check mode is same as beginning of the test
            self.check_mode()
            # Check test image
            resolution = self.chameleon_port.get_resolution()
            self.screen_test.test_screen_with_image(
                    resolution, self.test_mirrored, self.errors)


    def run_once(self, host, plug_status, test_mirrored=False):

        # Check for chromebook type devices
        if not host.get_board_type() == 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped')
        self.host = host
        self.test_mirrored = test_mirrored
        self.errors = list()

        # Check the servo object
        if self.host.servo is None:
            raise error.TestError('Invalid servo object found on the host.')

        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)
        for chameleon_port in finder.iterate_all_ports():
            self.run_test_on_port(chameleon_port, display_facade, plug_status)


    def run_test_on_port(self, chameleon_port, display_facade, plug_status):
        """Run the test on the given Chameleon port.

        @param chameleon_port: a ChameleonPorts object.
        @param display_facade: a display facade object.
        @param plug_status: the plugged status before_close, after_close,
           and before_open
        """
        self.chameleon_port = chameleon_port
        self.display_facade = display_facade
        self.screen_test = chameleon_screen_test.ChameleonScreenTest(
                chameleon_port, display_facade, self.outputdir)

        # Get connector type used (HDMI,DP,...)
        self.connector_used = self.display_facade.get_external_connector_name()
        # Set main display mode for the test
        self.display_facade.set_mirrored(self.test_mirrored)

        for (plugged_before_close,
             plugged_after_close,
             plugged_before_open) in plug_status:
            logging.info('TEST CASE: %s > CLOSE_LID > %s > %s > OPEN_LID',
                'PLUG' if plugged_before_close else 'UNPLUG',
                'PLUG' if plugged_after_close else 'UNPLUG',
                'PLUG' if plugged_before_open else 'UNPLUG')

            is_suspended = False
            boot_id = self.host.get_boot_id()

            # Plug before close
            self.chameleon_port.set_plug(plugged_before_close)
            self.chameleon_port.wait_video_input_stable(
                    timeout=self.WAIT_TIME_STABLE_VIDEO_INPUT)

            # Close lid and check
            self.close_lid()
            if plugged_before_close:
                self.check_docked()
            else:
                self.host.test_wait_for_sleep(self.TIMEOUT_SUSPEND_TRANSITION)
                is_suspended = True

            # Plug after close and check
            if plugged_after_close is not plugged_before_close:
                self.chameleon_port.set_plug(plugged_after_close)
                self.chameleon_port.wait_video_input_stable(
                        timeout=self.WAIT_TIME_STABLE_VIDEO_INPUT)
                if not plugged_before_close:
                    self.check_still_suspended()
                else:
                    self.host.test_wait_for_sleep(
                        self.TIMEOUT_SUSPEND_TRANSITION)
                    is_suspended = True

            # Plug before open and check
            if plugged_before_open is not plugged_after_close:
                self.chameleon_port.set_plug(plugged_before_open)
                self.chameleon_port.wait_video_input_stable(
                        timeout=self.WAIT_TIME_STABLE_VIDEO_INPUT)
                if not plugged_before_close or not plugged_after_close:
                    self.check_still_suspended()
                else:
                    self.host.test_wait_for_sleep(
                        self.TIMEOUT_SUSPEND_TRANSITION)
                    is_suspended = True

            # Open lid and check
            self.open_lid()
            if is_suspended:
                self.host.test_wait_for_resume(boot_id,
                                               self.TIMEOUT_RESUME_TRANSITION)
                is_suspended = False

            # Check internal screen switch to primary display
            self.check_primary_display_on_internal_screen()

            # Plug monitor if not plugged, such that we can test the screen.
            if not plugged_before_open:
                self.chameleon_port.set_plug(True)
                self.chameleon_port.wait_video_input_stable(
                        timeout=self.WAIT_TIME_STABLE_VIDEO_INPUT)

            # Check status
            self.check_external_display()

            if self.errors:
                raise error.TestFail('; '.join(set(self.errors)))

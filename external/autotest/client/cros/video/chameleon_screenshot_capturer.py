# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.video import method_logger


class ChameleonScreenshotCapturer(object):
    """
    Provides an interface to capture a dut screenshot using a Chameleon Board.

    Example use:
        with ChameleonScreenshotCapturer(board, 'HDMI', dutil, '/tmp', 10) as c:
            c.capture(filename)
    """


    def __init__(self, chameleon_board, interface, display_facade, dest_dir,
                 timeout_video_input_s, box=None):
        """
        @param chameleon_board: object representing the ChameleonBoard.
        @param interface: string, display interface to use. eg.: HDMI
        @param display_facade: display facade object to interact with DUT
        @param dest_dir: path, full path to the dest dir to put the screenshot.
        @param timeout_video_input_s: int, max time to wait for chameleon video
                                      input to become stable.
        @box: int tuple, left, upper, right, lower pixel coordinates
              defining a desired image region

        """
        self.chameleon_board = chameleon_board
        self.display_facade = display_facade
        self.interface = interface.lower()
        self.dest_dir = dest_dir
        self.port = None
        self.box = box
        self.timeout_video_input_s = timeout_video_input_s
        self.was_plugged = False
        self._find_connected_port()


    @method_logger.log
    def __enter__(self):

        if not self.was_plugged:
            self.port.plug()

        self.port.wait_video_input_stable(self.timeout_video_input_s)

        self.display_facade.set_mirrored(True)
        time.sleep(self.timeout_video_input_s)

        return self


    @method_logger.log
    def _find_connected_port(self):
        """
        Gets a connected port of the pre-specified interface.

        @raises TestError if desired port was not detected.

        """
        self.chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                self.chameleon_board, self.display_facade)

        connected_port = finder.find_port(self.interface)

        if connected_port is None:
            msg = 'No %s port found.\n' % self.interface
            raise error.TestError(msg + str(finder))

        self.port = connected_port
        self.was_plugged = connected_port.plugged


    @method_logger.log
    def capture(self, filename, box=None):
        """
        Captures a screenshot using provided chameleon board.

        We save to a file because comparers like bp take files.

        @param filename: string, filename of the image to save to.
        @param box: int tuple, left, upper, right, lower pixel coordinates
                    defining a box region of what the image should be.
        @returns a fullpath to the image just captured.

        """

        fullpath = os.path.join(self.dest_dir, filename)

        if not box:
            box = self.box

        img = self.port.capture_screen()
        img.crop(box).save(fullpath)

        return fullpath


    @method_logger.log
    def __exit__(self, exc_type, exc_val, exc_tb):
        if not self.was_plugged:
            self.port.unplug()

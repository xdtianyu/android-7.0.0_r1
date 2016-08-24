# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


class ChameleonVideoCapturer(object):
    """
    Wraps around chameleon APIs to provide an easy way to capture video frames.

    """


    def __init__(self, chameleon_port, display_facade,
                 timeout_get_all_frames_s=60):

        self.chameleon_port = chameleon_port
        self.display_facade = display_facade
        self.timeout_get_all_frames_s = timeout_get_all_frames_s
        self._checksums = []

        self.was_plugged = None


    def __enter__(self):
        self.was_plugged = self.chameleon_port.plugged

        if not self.was_plugged:
            self.chameleon_port.plug()
            self.chameleon_port.wait_video_input_stable()

        return self


    def capture(self, player, max_frame_count, box=None):
        """
        Captures frames upto max_frame_count, saves the image with filename
        same as the index of the frame in the frame buffer.

        @param player: object, VimeoPlayer or NativeHTML5Player
        @param max_frame_count: int, maximum total number of frames to capture.
        @param box: int tuple, left, upper, right, lower pixel coordinates.
                    Defines the rectangular boundary within which to compare.
        @return: list of paths of captured images.

        """

        self.capture_only(player, max_frame_count, box)
        # each checksum should be saved with a filename that is its index
        ind_paths = {i : str(i) for i in self.checksums}
        return self.write_images(ind_paths)


    def capture_only(self, player, max_frame_count, box=None):
        """
        Asynchronously begins capturing video frames. Stops capturing when the
        number of frames captured is equal or more than max_frame_count. Does
        save the images, gets only the checksums.

        @param player: VimeoPlayer or NativeHTML5Player.
        @param max_frame_count: int, the maximum number of frames we want.
        @param box: int tuple, left, upper, right, lower pixel coordinates.
                    Defines the rectangular boundary within which to compare.
        @return: list of checksums

        """

        if not box:
            box = self.box

        self.chameleon_port.start_capturing_video(box)

        player.play()

        error_msg = "Expected current time to be > 1 seconds"

        utils.poll_for_condition(lambda : player.currentTime() > 1,
                                 timeout=5,
                                 sleep_interval=0.01,
                                 exception=error.TestError(error_msg))

        error_msg = "Couldn't get the right number of frames"

        utils.poll_for_condition(
                lambda: self.chameleon_port.get_captured_frame_count() >=
                        max_frame_count,
                error.TestError(error_msg),
                self.timeout_get_all_frames_s,
                sleep_interval=0.01)

        self.chameleon_port.stop_capturing_video()

        self.checksums = self.chameleon_port.get_captured_checksums()
        count = self.chameleon_port.get_captured_frame_count()

        # Due to the polling and asychronous calls we might get too many frames
        # cap at max
        del self.checksums[max_frame_count:]

        logging.debug("***# of frames received %s", count)
        logging.debug("Checksums before chopping repeated ones")
        for c in self.checksums:
            logging.debug(c)

        # Find the first frame that is different from previous ones. This
        # represents the start of 'interesting' frames
        first_index = 0
        for i in xrange(1, count):
            if self.checksums[0] != self.checksums[i]:
                first_index = i
                break

        logging.debug("*** First interesting frame at index = %s", first_index)
        self.checksums = self.checksums[first_index:]
        return self.checksums



    def write_images(self, frame_indices, dest_dir, image_format):
        """
        Saves frames of given indices to disk. The filename of the frame will be
        index in the list.
        @param frame_indices: list of frame indices to save.
        @param dest_dir: path to the desired destination dir.
        @param image_format: string, format to save the image as. e.g; PNG
        @return: list of file paths

        """
        if type(frame_indices) is not list:
            frame_indices = [frame_indices]

        test_images = []
        curr_checksum = None
        for i, frame_index in enumerate(frame_indices):
            path = os.path.join(dest_dir, str(i) + '.' + image_format)
            # previous is what was current in the previous iteration
            prev_checksum = curr_checksum
            curr_checksum = self.checksums[frame_index]
            if curr_checksum == prev_checksum:
                logging.debug("Image the same as previous image, copy it.")
            else:
                logging.debug("Read frame %d, store as %s.", i, path)
                curr_img = self.chameleon_port.read_captured_frame(frame_index)
            curr_img.save(path)
            test_images.append(path)
        return test_images


    def __exit__(self, exc_type, exc_val, exc_tb):
        if not self.was_plugged:
            self.chameleon_port.unplug()
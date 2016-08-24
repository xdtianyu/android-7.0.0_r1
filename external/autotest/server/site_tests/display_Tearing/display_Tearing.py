# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a test for screen tearing using the Chameleon board."""

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_Tearing(test.test):
    """Display tearing test by multi-color full screen animation.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    DUT behavior response to a series of multi-color full screen switch.
    """

    version = 1

    # Time to wait for Chameleon to save images into RAM.
    # Current value is decided by experiments.
    CHAMELEON_CAPTURE_WAIT_TIME_SEC = 1

    # The initial background color to set for a new tab.
    INITIAL_BACKGROUND_COLOR = 0xFFFFFF

    # Time in seconds to wait for notation bubbles, including bubbles for
    # external detection, mirror mode and fullscreen, to disappear.
    NEW_PAGE_STABILIZE_TIME = 10

    # 1. Since it is difficult to distinguish repeated frames
    #    generated from delay from real repeated frames, make
    #    sure that there are no successive repeated colors in
    #    TEST_COLOR_SEQUENCE. In fact, if so, the repeated ones
    #    will be discarded.
    # 2. Similarly make sure that the the first element of
    #    TEST_COLOR_SEQUENCE is not INITIAL_BACKGROUND_COLOR.
    # 3. Notice that the hash function in Chameleon used for
    #    checksums is weak, so it is possible to encounter
    #    hash collision. If it happens, an error will be raised
    #    during execution time of _display_and_get_checksum_table().
    TEST_COLOR_SEQUENCE = [0x010000, 0x002300, 0x000045, 0x670000,
                           0x008900, 0x0000AB, 0xCD0000, 0x00EF00] * 20

    def _open_color_sequence_tab(self, test_mirrored):
        """Sets up a new empty page for displaying color sequence.

        @param test_mirrored: True to test mirrored mode. False not to.
        """
        self._test_tab_descriptor = self._display_facade.load_url('about:blank')
        if not test_mirrored:
            self._display_facade.move_to_display(
                    self._display_facade.get_first_external_display_index())
        self._display_facade.set_fullscreen(True)
        logging.info('Waiting for the new tab to stabilize...')
        time.sleep(self.NEW_PAGE_STABILIZE_TIME)

    def _get_single_color_checksum(self, chameleon_port, color):
        """Gets the frame checksum of the full screen of the given color.

        @param chameleon_port: A general ChameleonPort object.
        @param color: the given color.
        @return The frame checksum mentioned above, which is a tuple.
        """
        try:
            chameleon_port.start_capturing_video()
            self._display_facade.load_color_sequence(self._test_tab_descriptor,
                                                     [color])
            time.sleep(self.CHAMELEON_CAPTURE_WAIT_TIME_SEC)
        finally:
            chameleon_port.stop_capturing_video()
        # Gets the checksum of the last one image.
        last = chameleon_port.get_captured_frame_count() - 1
        return tuple(chameleon_port.get_captured_checksums(last)[0])

    def _display_and_get_checksum_table(self, chameleon_port, color_sequence):
        """Makes checksum table, which maps checksums into colors.

        @param chameleon_port: A general ChameleonPort object.
        @param color_sequence: the color_sequence that will be displayed.
        @return A dictionary consists of (x: y), y is in color_sequence and
                x is the checksum of the full screen of pure color y.
        @raise an error if there is hash collision
        """
        # Resets the background color to make sure the screen looks like
        # what we expect.
        self._reset_background_color()
        checksum_table = {}
        # Makes sure that INITIAL_BACKGROUND_COLOR is in checksum_table,
        # or it may be misjudged as screen tearing.
        color_set = set(color_sequence+[self.INITIAL_BACKGROUND_COLOR])
        for color in color_set:
            checksum = self._get_single_color_checksum(chameleon_port, color)
            if checksum in checksum_table:
                raise error.TestFail('Bad color sequence: hash collision')
            checksum_table[checksum] = color
            logging.info('Color %d has checksums %r', (color, checksum))
        return checksum_table

    def _reset_background_color(self):
        """Resets the background color for displaying test color sequence."""
        self._display_facade.load_color_sequence(
                self._test_tab_descriptor,
                [self.INITIAL_BACKGROUND_COLOR])

    def _display_and_capture(self, chameleon_port, color_sequence):
        """Displays the color sequence and captures frames by Chameleon.

        @param chameleon_port: A general ChameleonPort object.
        @param color_sequence: the color sequence to display.
        @return (A list of checksums of captured frames,
                 A list of the timestamp for each switch).
        """
        # Resets the background color to make sure the screen looks like
        # what we expect.
        self._reset_background_color()
        try:
            chameleon_port.start_capturing_video()
            timestamp_list = (
                    self._display_facade.load_color_sequence(
                        self._test_tab_descriptor, color_sequence))
            time.sleep(self.CHAMELEON_CAPTURE_WAIT_TIME_SEC)
        finally:
            chameleon_port.stop_capturing_video()

        captured_checksums = chameleon_port.get_captured_checksums(0)
        captured_checksums = [tuple(x) for x in captured_checksums]
        return (captured_checksums, timestamp_list)

    def _tearing_test(self, captured_checksums, checksum_table):
        """Checks whether some captured frame is teared by checking
                their checksums.

        @param captured_checksums: A list of checksums of captured
                                   frames.
        @param checksum_table: A dictionary of reasonable checksums.
        @return True if the test passes.
        """
        for checksum in captured_checksums:
            if checksum not in checksum_table:
                return False
        return True

    def _correction_test(
            self, captured_color_sequence, expected_color_sequence):
        """Checks whether the color sequence is sent to Chameleon correctly.

        Here are the checking steps:
            1. Discard all successive repeated elements of both sequences.
            2. If the first element of the captured color sequence is
               INITIAL_BACKGROUND_COLOR, discard it.
            3. Check whether the two sequences are equal.

        @param captured_color_sequence: The sequence of colors captured by
                                        Chameleon, each element of which
                                        is an integer.
        @param expected_color_sequence: The sequence of colors expected to
                                        be displayed.
        @return True if the test passes.
        """
        def _discard_delayed_frames(sequence):
            return [sequence[i]
                    for i in xrange(len(sequence))
                    if i == 0 or sequence[i] != sequence[i-1]]

        captured_color_sequence = _discard_delayed_frames(
                captured_color_sequence)
        expected_color_sequence = _discard_delayed_frames(
                expected_color_sequence)

        if (len(captured_color_sequence) > 0 and
            captured_color_sequence[0] == self.INITIAL_BACKGROUND_COLOR):
            captured_color_sequence.pop(0)
        return captured_color_sequence == expected_color_sequence

    def _test_screen_with_color_sequence(
            self, test_mirrored, chameleon_port, error_list):
        """Tests the screen with the predefined color sequence.

        @param test_mirrored: True to test mirrored mode. False not to.
        @param chameleon_port: A general ChameleonPort object.
        @param error_list: A list to append the error message to or None.
        """
        self._open_color_sequence_tab(test_mirrored)
        checksum_table = self._display_and_get_checksum_table(
                chameleon_port, self.TEST_COLOR_SEQUENCE)
        captured_checksums, timestamp_list = self._display_and_capture(
                chameleon_port, self.TEST_COLOR_SEQUENCE)
        self._display_facade.close_tab(self._test_tab_descriptor)
        delay_time = [timestamp_list[i] - timestamp_list[i-1]
                      for i in xrange(1, len(timestamp_list))]
        logging.info('Captured %d frames\n'
                     'Checksum_table: %s\n'
                     'Captured_checksums: %s\n'
                     'Timestamp_list: %s\n'
                     'Delay informtaion:\n'
                     'max = %r, min = %r, avg = %r\n',
                     len(captured_checksums), checksum_table,
                     captured_checksums, timestamp_list,
                     max(delay_time), min(delay_time),
                     sum(delay_time)/len(delay_time))

        error = None
        if self._tearing_test(
                captured_checksums, checksum_table) is False:
            error = 'Detected screen tearing'
        else:
            captured_color_sequence = [
                    checksum_table[checksum]
                    for checksum in captured_checksums]
            if self._correction_test(
                    captured_color_sequence, self.TEST_COLOR_SEQUENCE) is False:
                error = 'Detected missing, redundant or wrong frame(s)'
        if error is not None and error_list is not None:
            error_list.append(error)

    def run_once(self, host, test_mirrored=False):
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        self._display_facade = factory.create_display_facade()
        self._test_tab_descriptor = None
        chameleon_board = host.chameleon

        chameleon_board.reset()
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, self._display_facade)

        errors = []
        for chameleon_port in finder.iterate_all_ports():

            logging.info('Set mirrored: %s', test_mirrored)
            self._display_facade.set_mirrored(test_mirrored)

            self._test_screen_with_color_sequence(
                    test_mirrored, chameleon_port, errors)

        if errors:
            raise error.TestFail('; '.join(set(errors)))

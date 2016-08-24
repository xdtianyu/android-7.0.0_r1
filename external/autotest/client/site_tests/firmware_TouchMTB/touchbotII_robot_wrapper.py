# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A wrapper for robot manipulation with Touchbot II."""

import os
import re
import shutil
import sys
import time

import common_util
import test_conf as conf

from firmware_constants import GV, MODE, OPTIONS


# Define the robot control script names.
SCRIPT_NOISE = 'generate_noise.py'
SCRIPT_LINE = 'line.py'
SCRIPT_TAP = 'tap.py'
SCRIPT_CLICK = 'click.py'
SCRIPT_ONE_STATIONARY_FINGER = 'one_stationary_finger.py'
SCRIPT_STATIONARY_WITH_TAPS = 'stationary_finger_with_taps_around_it.py'
SCRIPT_QUICKSTEP = 'quickstep.py'

# A script to "reset" the robot by having it move safely to a spot just above
# the center of the touchpad.
SCRIPT_RESET = 'reset.py'

# Define constants for coordinates.
# Normally, a gesture is performed within [START, END].
# For tests involved with RangeValidator which intends to verify
# the min/max reported coordinates, use [OFF_START, OFF_END] instead
# so that the gestures are performed off the edge.
START = 0.1
CENTER = 0.5
END = 0.9
OFF_START = -0.05
OFF_END = 1.05
ABOVE_CENTER = 0.25
BELOW_CENTER = 0.75
LEFT_TO_CENTER = 0.25
RIGHT_TO_CENTER = 0.75

OUTER_PINCH_SPACING = 70
INNER_PINCH_SPACING = 25

PHYSICAL_CLICK_SPACING = 20
PHYSICAL_CLICK_FINGER_SIZE = 2

TWO_CLOSE_FINGER_SPACING = 17
TWO_FINGER_SPACING = 25

class RobotWrapperError(Exception):
    """An exception class for the robot_wrapper module."""
    pass


class RobotWrapper:
    """A class to wrap and manipulate the robot library."""

    def __init__(self, board, options):
        self._board = board
        self._mode = options[OPTIONS.MODE]
        self._is_touchscreen = options[OPTIONS.TOUCHSCREEN]
        self._fngenerator_only = options[OPTIONS.FNGENERATOR]
        self._robot_script_dir = self._get_robot_script_dir()
        self._noise_script_dir = os.path.join(self._get_robot_script_dir(),
                                              SCRIPT_NOISE)

        # Each get_control_command method maps to a script name.
        self._robot_script_name_dict = {
            self._get_control_command_line: SCRIPT_LINE,
            self._get_control_command_rapid_taps: SCRIPT_TAP,
            self._get_control_command_single_tap: SCRIPT_TAP,
            self._get_control_command_drumroll: SCRIPT_TAP,
            self._get_control_command_click: SCRIPT_CLICK,
            self._get_control_command_one_stationary_finger:
                    SCRIPT_ONE_STATIONARY_FINGER,
            self._get_control_command_pinch: SCRIPT_LINE,
            self._get_control_command_stationary_with_taps:
                    SCRIPT_STATIONARY_WITH_TAPS,
            self._get_control_command_quickstep: SCRIPT_QUICKSTEP,
        }

        # Each gesture maps to a get_control_command method
        self._method_of_control_command_dict = {
            conf.NOISE_LINE: self._get_control_command_line,
            conf.NOISE_STATIONARY: self._get_control_command_single_tap,
            conf.NOISE_STATIONARY_EXTENDED: self._get_control_command_single_tap,
            conf.ONE_FINGER_TRACKING: self._get_control_command_line,
            conf.ONE_FINGER_TO_EDGE: self._get_control_command_line,
            conf.ONE_FINGER_SWIPE: self._get_control_command_line,
            conf.ONE_FINGER_TAP: self._get_control_command_single_tap,
            conf.ONE_FINGER_TRACKING_FROM_CENTER:
                    self._get_control_command_line,
            conf.RAPID_TAPS: self._get_control_command_rapid_taps,
            conf.TWO_FINGER_TRACKING: self._get_control_command_line,
            conf.TWO_FINGER_SWIPE: self._get_control_command_line,
            conf.TWO_FINGER_TAP: self._get_control_command_single_tap,
            conf.TWO_CLOSE_FINGERS_TRACKING: self._get_control_command_line,
            conf.RESTING_FINGER_PLUS_2ND_FINGER_MOVE:
                    self._get_control_command_one_stationary_finger,
            conf.FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS:
                    self._get_control_command_one_stationary_finger,
            conf.FINGER_CROSSING:
                    self._get_control_command_one_stationary_finger,
            conf.PINCH_TO_ZOOM: self._get_control_command_pinch,
            conf.DRUMROLL: self._get_control_command_drumroll,
            conf.TWO_FAT_FINGERS_TRACKING: self._get_control_command_line,
            conf.ONE_FINGER_PHYSICAL_CLICK: self._get_control_command_click,
            conf.TWO_FINGER_PHYSICAL_CLICK: self._get_control_command_click,
            conf.THREE_FINGER_PHYSICAL_CLICK: self._get_control_command_click,
            conf.FOUR_FINGER_PHYSICAL_CLICK: self._get_control_command_click,
            conf.STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS:
                    self._get_control_command_stationary_with_taps,
            conf.FAT_FINGER_MOVE_WITH_RESTING_FINGER:
                    self._get_control_command_one_stationary_finger,
            conf.DRAG_LATENCY:
                    self._get_control_command_quickstep,
        }

        self._line_dict = {
            GV.LR: (START, CENTER, END, CENTER),
            GV.RL: (END, CENTER, START, CENTER),
            GV.TB: (CENTER, START, CENTER, END),
            GV.BT: (CENTER, END, CENTER, START),
            GV.BLTR: (START, END, END, START),
            GV.TRBL: (END, START, START, END),
            GV.BRTL: (END, END, START, START),
            GV.TLBR: (START, START, END, END),
            GV.CUR: (CENTER, CENTER, END, START),
            GV.CUL: (CENTER, CENTER, START, START),
            GV.CLR: (CENTER, CENTER, END, END),
            GV.CLL: (CENTER, CENTER, START, END),

            # Overshoot for this one-finger gesture only: ONE_FINGER_TO_EDGE
            GV.CL: (CENTER, CENTER, OFF_START, CENTER),
            GV.CR: (CENTER, CENTER, OFF_END, CENTER),
            GV.CT: (CENTER, CENTER, CENTER, OFF_START),
            GV.CB: (CENTER, CENTER, CENTER, OFF_END),
        }

        # The angle wrt the pad that the fingers should take when doing a 2f
        # gesture along these lines, or doing 2f taps at different angles.
        self._angle_dict = {
            GV.LR: -45,
            GV.RL: -45,
            GV.TB: 45,
            GV.BT: 45,
            GV.TLBR: 90,
            GV.BLTR: 0,
            GV.TRBL: 0,
            GV.HORIZONTAL: -45,
            GV.VERTICAL: 45,
            GV.DIAGONAL: 0,
        }

        # The stationary finger locations corresponding the line specifications
        # for the finger crossing tests
        self._stationary_finger_dict = {
            GV.LR: (CENTER, ABOVE_CENTER),
            GV.RL: (CENTER, BELOW_CENTER),
            GV.TB: (RIGHT_TO_CENTER, CENTER),
            GV.BT: (LEFT_TO_CENTER, CENTER),
            GV.BLTR: (RIGHT_TO_CENTER, BELOW_CENTER),
            GV.TRBL: (LEFT_TO_CENTER, ABOVE_CENTER),
        }

        self._speed_dict = {
            GV.SLOW: 10,
            GV.NORMAL: 20,
            GV.FAST: 30,
            GV.FULL_SPEED: 100,
        }

        # The frequencies of noise in Hz.
        self._frequency_dict = {
            GV.LOW_FREQUENCY: 5000,  # 5kHz
            GV.MED_FREQUENCY: 500000,  # 500kHz
            GV.HIGH_FREQUENCY: 1000000,  # 1MHz
        }
        # Add the list of extended frequency values to the dict.
        freq_value_dict = {freq: int(freq.replace('Hz', '')) for freq in GV.EXTENDED_FREQUENCIES}
        self._frequency_dict = dict(self._frequency_dict.items() + freq_value_dict.items())

        # The waveforms of noise.
        self._waveform_dict = {
            GV.SQUARE_WAVE: 'SQUARE',
            GV.SINE_WAVE: 'SINE',
        }

        # The amplitude of noise in Vpp.
        self._amplitude_dict = {
            GV.HALF_AMPLITUDE: 10,
            GV.MAX_AMPLITUDE: 20,
        }

        self._location_dict = {
            # location parameters for one-finger taps
            GV.TL: (START, START),
            GV.TR: (END, START),
            GV.BL: (START, END),
            GV.BR: (END, END),
            GV.TS: (CENTER, START),
            GV.BS: (CENTER, END),
            GV.LS: (START, CENTER),
            GV.RS: (END, CENTER),
            GV.CENTER: (CENTER, CENTER),
        }

        self.fingertips = [None, None, None, None]

        self._build_robot_script_paths()

        self._get_device_spec()

    def _get_device_spec(self):
        if not self.is_robot_action_mode():
            return

        # First check if there is already a device spec in this directory
        if (os.path.isfile('%s.p' % self._board) and
            os.path.isfile('%s_min.p' % self._board)):
            return

        # Next, check if maybe there is a spec in the touchbotII directory
        spec_path = os.path.join(self._robot_script_dir,
                         'device_specs/%s.p' % self._board)
        spec_min_path = os.path.join(self._robot_script_dir,
                         'device_specs/%s_min.p' % self._board)
        if os.path.isfile(spec_path) and os.path.isfile(spec_min_path):
            shutil.copy(spec_path, '.')
            shutil.copy(spec_min_path, '.')
            return

        # If both of those fail, then generate a new device spec
        self._calibrate_device(self._board)

    def _get_fingertips(self, tips_to_get):
        if self.fingertips != [None, None, None, None]:
            print 'Error, there are still fingertips on'
            sys.exit(1)

        for size in conf.ALL_FINGERTIP_SIZES:
            fingers = [1 if f == size else 0 for f in tips_to_get]
            print 'size: %d\tfingers: %s' % (size, str(fingers))
            if fingers == [0, 0, 0, 0]:
                continue

            script = os.path.join(self._robot_script_dir,
                                  'manipulate_fingertips.py')
            para = (script, fingers[0], fingers[1], fingers[2], fingers[3],
                    size, str(True))
            cmd = 'python %s %d %d %d %d %d %s' % para

            if self._execute_control_command(cmd):
                print 'Error getting the figertips!'
                print 'Please make sure all the fingertips are in the correct'
                print 'positions before continuing.'
                print 'WARNING: failure to do this correctly could cause'
                print '         permanent damage to the robot!'
                print
                print 'The fingers it was attempting to get were as follows:'
                print tips_to_get
                print '(Starting with the front right finger and going counter'
                print 'clockwise from there. Finger sizes are 0-3)'
                self._wait_for_user_input('GO', 'ABORT')

        self.fingertips = tips_to_get


    def _return_fingertips(self):
        """ Return all the fingertips to the nest, one size at a time.
        This function uses the self.fingertips member variable to know which
        finger has which tip size, then returns them all to the nest
        """
        for size in conf.ALL_FINGERTIP_SIZES:
            # See which (if any) of the fingers currently have this size tip
            fingers = [1 if f == size else 0 for f in self.fingertips]
            if fingers == [0, 0, 0, 0]:
                continue

            script = os.path.join(self._robot_script_dir,
                                  'manipulate_fingertips.py')
            para = (script, fingers[0], fingers[1], fingers[2], fingers[3],
                    size, str(False))
            cmd = 'python %s %d %d %d %d %d %s' % para

            if self._execute_control_command(cmd):
                print 'Error returning the figertips!'
                print 'Please make sure all the fingertips are in their correct'
                print 'spots in the nest before continuing.'
                print 'WARNING: failure to do this correctly could cause'
                print '         permanent damage to the robot!'
                self._wait_for_user_input('GO', 'ABORT')

        self.fingertips = [None, None, None, None]

    def _wait_for_user_input(self, continue_cmd, stop_cmd):
        user_input = None
        while user_input not in [continue_cmd, stop_cmd]:
            if user_input:
                print 'Sorry, but "%s" was incorrect' % user_input
            user_input = raw_input('Type "%s" to continue or "%s" to stop: ' %
                                   (continue_cmd, stop_cmd))
        if user_input == stop_cmd:
            raise RobotWrapperError('Operator aborted after error')

    def _calibrate_device(self, board):
        """ Have the operator show the robot where the device is."""
        calib_script = os.path.join(self._robot_script_dir,
                                    'calibrate_for_new_device.py')
        calib_cmd = 'python %s %s' % (calib_script, board)
        self._execute_control_command(calib_cmd)

    def is_manual_noise_test_mode(self):
        return (self._mode in [MODE.NOISE, MODE.COMPLETE]
                and self._fngenerator_only)

    def is_robot_action_mode(self):
        """Is it in robot action mode?

        In the robot action mode, it actually invokes the robot control script.
        """
        if self.is_manual_noise_test_mode():
            return False

        return self._mode in [MODE.ROBOT, MODE.QUICKSTEP, MODE.NOISE]

    def _raise_error(self, msg):
        """Only raise an error if it is in the robot action mode."""
        if self.is_robot_action_mode():
            raise RobotWrapperError(msg)

    def _get_robot_script_dir(self):
        """Get the directory of the robot control scripts."""
        for lib_path in [conf.robot_lib_path, conf.robot_lib_path_local]:
            cmd = ('find %s -maxdepth 1 -type d -name %s' %
                        (lib_path, conf.python_package))
            path = common_util.simple_system_output(cmd)
            if path:
                robot_script_dir = os.path.join(path, conf.gestures_sub_path)
                if os.path.isdir(robot_script_dir):
                    return robot_script_dir
        return ''

    def _get_num_taps(self, gesture):
        """Determine the number of times to tap."""
        matches = re.match('[^0-9]*([0-9]*)[^0-9]*', gesture)
        return int(matches.group(1)) if matches else None

    def _reverse_coord_if_is_touchscreen(self, coordinates):
        """Reverse the coordinates if the device is a touchscreen.

        E.g., the original coordinates = (0.1, 0.9)
              After reverse, the coordinates = (1 - 0.1, 1 - 0.9) = (0.9, 0.1)

        @param coordinates: a tuple of coordinates
        """
        return (tuple(1.0 - c for c in coordinates) if self._is_touchscreen else
                coordinates)

    def _get_control_command_pinch(self, robot_script, gesture, variation):
        # Depending on which direction you're zooming, change the order of the
        # finger spacings.
        if GV.ZOOM_IN in variation:
            starting_spacing = INNER_PINCH_SPACING
            ending_spacing = OUTER_PINCH_SPACING
        else:
            starting_spacing = OUTER_PINCH_SPACING
            ending_spacing = INNER_PINCH_SPACING

        # Keep the hand centered on the pad, and make the fingers move
        # in or out with only two opposing fingers on the pad.
        para = (robot_script, self._board,
                CENTER, CENTER, 45, starting_spacing,
                CENTER, CENTER, 45, ending_spacing,
                0, 1, 0, 1, self._speed_dict[GV.SLOW], 'basic')
        return 'python %s %s_min.p %f %f %d %d %f %f %d %d %d %d %d %d %f %s' % para

    def _get_control_command_quickstep(self, robot_script, gesture, variation):
        """have the robot do the zig-zag gesture for latency testing."""
        para = (robot_script, self._board, self._speed_dict[GV.FULL_SPEED])
        return 'python %s %s_min.p %d' % para


    def _dimensions(self, device_spec):
        device_script = os.path.join(self._robot_script_dir, 'device_size.py')
        cmd = 'python %s %s' % (device_script, device_spec)
        results = common_util.simple_system_output(cmd)
        dimensions = results.split()
        return float(dimensions[0]), float(dimensions[1])

    def _adjust_for_target_distance(self, line, stationary_finger, distance):
        """ Given a point and a line in relative coordinates move the point
        so that is exactly 'distance' millimeters away from the line for the
        current board.  Sometimes the robot needs to move two fingers very
        close and this is problematic in relative coordinates as the board
        dimensions change.  This function allows the test to compensate and
        get a more accurate test.
        """
        # First convert all the values into absolute coordinates by using
        # the calibrated device spec to see the dimensions of the pad
        h, w = self._dimensions("%s_min.p" % self._board)
        abs_line = (line[0] * w, line[1] * h, line[2] * w, line[3] * h)
        x, y = (stationary_finger[0] * w, stationary_finger[1] * h)

        # Find a point near the stationary finger that is distance
        # away from the line at the closest point
        dx = abs_line[2] - abs_line[0]
        dy = abs_line[3] - abs_line[1]
        if dx == 0: # vertical line
            if x > abs_line[0]:
                x = abs_line[0] + distance
            else:
                x = abs_line[0] - distance
        elif dy == 0: # horizontal line
            if y > abs_line[1]:
                y = abs_line[1] + distance
            else:
                y = abs_line[1] - distance
        else:
            # First, find the closest point on the line to the point
            m = dy / dx
            b = abs_line[1] - m * abs_line[0]
            m_perp = -1.0 / m
            b_perp = y - m_perp * x
            x_intersect = (b_perp - b) / (m - m_perp)
            y_intersect = m * x_intersect + b

            current_distance_sq = ((x_intersect - x) ** 2 +
                                   (y_intersect - y) ** 2)
            scale = distance / (current_distance_sq ** 0.5)
            x = x_intersect - (x_intersect - x) * scale
            y = y_intersect - (y_intersect - y) * scale

        #Convert this absolute point back into relative coordinates
        x_rel = x / w
        y_rel = y / h

        return (x_rel, y_rel)

    def _get_control_command_one_stationary_finger(self, robot_script, gesture,
                                                   variation):
        line = speed = finger_gap_mm = None
        num_taps = 0
        # The stationary finger should be in the bottom left corner for resting
        # finger tests, and various locations for finger crossing tests
        stationary_finger = (START, END)

        for element in variation:
            if element in GV.GESTURE_DIRECTIONS:
                line = self._line_dict[element]
                if 'finger_crossing' in gesture:
                    stationary_finger = self._stationary_finger_dict[element]
                    finger_gap_mm = conf.FINGER_CROSSING_GAP_MM
                elif 'second_finger_taps' in gesture:
                    stationary_finger = self._location_dict[GV.BL]
                    speed = self._speed_dict[GV.SLOW]
                    num_taps = 3
                elif 'fat_finger' in gesture:
                    stationary_finger = self._stationary_finger_dict[element]
                    speed = self._speed_dict[GV.SLOW]
                    finger_gap_mm = conf.FAT_FINGER_AND_STATIONARY_FINGER_GAP_MM
            elif element in GV.GESTURE_SPEED:
                speed = self._speed_dict[element]

        if line is None or speed is None:
            msg = 'Cannot derive the line/speed parameters from %s %s.'
            self._raise_error(msg % (gesture, variation))

        # Adjust the positioning to get a precise gap between finger tips
        # if this gesture requires it
        if finger_gap_mm:
            min_space_mm = (conf.FINGERTIP_DIAMETER_MM[self.fingertips[0]] +
                            conf.FINGERTIP_DIAMETER_MM[self.fingertips[2]]) / 2
            stationary_finger = self._adjust_for_target_distance(
                                    line, stationary_finger,
                                    min_space_mm + finger_gap_mm)

        line = self._reverse_coord_if_is_touchscreen(line)
        start_x, start_y, end_x, end_y = line
        stationary_x, stationary_y = stationary_finger

        para = (robot_script, self._board, stationary_x, stationary_y,
                start_x, start_y, end_x, end_y, speed, num_taps)
        cmd = 'python %s %s_min.p %f %f %f %f %f %f %s --taps=%d' % para
        return cmd

    def _get_control_command_line(self, robot_script, gesture, variation):
        """Get robot control command for gestures using robot line script."""
        line_type = 'swipe' if bool('swipe' in gesture) else 'basic'
        line = speed = finger_angle = None
        for element in variation:
            if element in GV.GESTURE_DIRECTIONS:
                line = self._line_dict[element]
                finger_angle = self._angle_dict.get(element, None)
            elif element in GV.GESTURE_SPEED:
                speed = self._speed_dict[element]

        if not speed:
            if line_type is 'swipe':
                speed = self._speed_dict[GV.FAST]
            if 'two_close_fingers' in gesture or 'fat' in gesture:
                speed = self._speed_dict[GV.NORMAL]

        if line is None or speed is None:
            msg = 'Cannot derive the line/speed parameters from %s %s.'
            self._raise_error(msg % (gesture, variation))

        line = self._reverse_coord_if_is_touchscreen(line)
        start_x, start_y, end_x, end_y = line

        if 'two' in gesture:
            if 'close_fingers' in gesture:
                finger_spacing = TWO_CLOSE_FINGER_SPACING
                finger_angle += 45
                fingers = (1, 1, 0, 0)
            else:
                finger_spacing = TWO_FINGER_SPACING
                fingers = (0, 1, 0, 1)

            if finger_angle is None:
                msg = 'Unable to determine finger angle for %s %s.'
                self._raise_error(msg % (gesture, variation))
        else:
            finger_spacing = 17
            fingers = (0, 1, 0, 0)
            finger_angle = 0

        # Generate the CLI command
        para = (robot_script, self._board,
                start_x, start_y, finger_angle, finger_spacing,
                end_x, end_y, finger_angle, finger_spacing,
                fingers[0], fingers[1], fingers[2], fingers[3],
                speed, line_type)
        cmd = 'python %s %s.p %f %f %d %d %f %f %d %d %d %d %d %d %f %s' % para

        if self._get_noise_command(gesture, variation):
            # A one second pause to give the touchpad time to calibrate
            DELAY = 1  # 1 seconds
            cmd = '%s %d' % (cmd, DELAY)

        return cmd

    def _get_control_command_stationary_with_taps(self, robot_script, gesture,
                                                  variation):
        """ There is only one variant of this gesture, so there is only one
        command to generate.  This is the command for tapping around a
        stationary finger on the pad.
        """
        return 'python %s %s.p 0.5 0.5' % (robot_script, self._board)

    def _get_control_command_rapid_taps(self, robot_script, gesture, variation):
        num_taps = self._get_num_taps(gesture)
        return self._get_control_command_taps(robot_script, gesture,
                                              variation, num_taps)

    def _get_control_command_single_tap(self, robot_script, gesture, variation):
        # Get the single tap command
        cmd = self._get_control_command_taps(robot_script, gesture, variation, 1)

        if self._get_noise_command(gesture, variation):
            # Add the noise command and a pause to the tap
            TEST_DURATION = 3  # 3 seconds
            cmd = '%s %d' % (cmd, TEST_DURATION)

        return cmd

    def _get_control_command_drumroll(self, robot_script, gesture, variation):
        """Get robot control command for the drumroll gesture. There is only
        one so there is no need for complicated parsing here.
        """
        return ('python %s %s.p 0.5 0.5 45 20 0 1 0 1 50 drumroll' %
                (robot_script, self._board))

    def _get_control_command_taps(self, robot_script, gesture,
                                  variation, num_taps):
        """Get robot control command for tap gestures.  This includes rapid tap
        tests as well as 1 and 2 finger taps at various locations on the pad.
        """
        if num_taps is None:
            msg = 'Cannot determine the number of taps to do from %s.'
            self._raise_error(msg % gesture)

        # The tap commands have identical arguments as the click except with
        # two additional arguments at the end.  As such we generate the 'click'
        # command and add these on to make it work as a tap.
        cmd = self._get_control_command_click_tap(robot_script, gesture,
                                                  variation)
        control_cmd = '%s %d tap' % (cmd, num_taps)
        return control_cmd

    def _get_control_command_click(self, robot_script, gesture, variation):
        """Get the robot control command for a physical click gesture"""
        cmd = self._get_control_command_click_tap(robot_script, gesture,
                                                  variation)
        control_cmd = '%s %d' % (cmd, PHYSICAL_CLICK_FINGER_SIZE)
        return control_cmd

    def _get_control_command_click_tap(self, robot_script, gesture, variation):
        """Get robot control arguments that are common between pysical click
        and tapping gestures
        """
        location = None
        angle = 45
        for element in variation:
            if element in self._location_dict:
                location = self._location_dict[element]
            if 'two' in gesture and element in self._angle_dict:
                angle = self._angle_dict[element]

        # All non-one finger taps are simply perfomed in the middle of the pad
        if 'one' not in gesture and location is None:
            location = self._location_dict[GV.CENTER]

        if location is None:
            msg = 'Cannot determine the location parameters from %s %s.'
            self._raise_error(msg % (gesture, variation))

        fingers = [1, 0, 0, 0]
        if 'two' in gesture:
            fingers = [0, 1, 0, 1]
        elif 'three' in gesture:
            fingers = [0, 1, 1, 1]
        elif 'four' in gesture:
            fingers = [1, 1, 1, 1]

        location_str = ' '.join(
            map(str, self._reverse_coord_if_is_touchscreen(location)))

        para = (robot_script, self._board, location_str, angle,
                PHYSICAL_CLICK_SPACING,
                fingers[0], fingers[1], fingers[2], fingers[3])
        control_cmd = 'python %s %s.p %s %d %d %d %d %d %d' % para
        return control_cmd

    def _build_robot_script_paths(self):
        """Build the robot script paths."""
        # Check if the robot script dir could be found.
        if not self._robot_script_dir:
            script_path = os.path.join(conf.robot_lib_path, conf.python_package,
                                       conf.gestures_sub_path)
            msg = 'Cannot find robot script directory in "%s".'
            self._raise_error(msg % script_path)

        # Build the robot script path dictionary
        self._robot_script_dict = {}
        for method in self._robot_script_name_dict:
            script_name = self._robot_script_name_dict.get(method)

            # Check if the control script actually exists.
            robot_script = os.path.join(self._robot_script_dir, script_name)
            if not os.path.isfile(robot_script):
                msg = 'Cannot find the robot control script: %s'
                self._raise_error(msg % robot_script)

            self._robot_script_dict[method] = robot_script

    def _get_control_command(self, gesture, variation):
        """Get robot control command based on the gesture and variation."""
        script_method = self._method_of_control_command_dict.get(gesture)
        if not script_method:
            self._raise_error('Cannot find "%s" gesture in '
                              '_method_of_control_command_dict.' % gesture)

        robot_script = self._robot_script_dict.get(script_method)
        if not robot_script:
            msg = 'Cannot find "%s" method in _robot_script_dict.'
            self._raise_error(msg % script_method)

        return script_method(robot_script, gesture, variation)

    def _execute_control_command(self, control_cmd):
        """Execute a control command."""
        print 'Executing: "%s"' % control_cmd
        if self.is_robot_action_mode():
            # Pausing to give everything time to settle
            time.sleep(0.5)
            return common_util.simple_system(control_cmd)
        return 0

    def _reset_with_safety_clearance(self, destination):
        reset_script = os.path.join(self._robot_script_dir, SCRIPT_RESET)
        para = (reset_script, self._board, destination,
                self._speed_dict[GV.FAST])
        self._execute_control_command('python %s %s.p %s %d' % para)

    def turn_off_noise(self):
        off_cmd = 'python %s OFF' % self._noise_script_dir
        common_util.simple_system(off_cmd)

    def _execute_noise_command(self, noise_cmd):
        if self.is_robot_action_mode() or self.is_manual_noise_test_mode():
            return common_util.simple_system(noise_cmd)

    def _get_noise_command(self, gesture, variation):
        if not gesture or not variation:
            return None

        waveform = frequency = amplitude = None
        for element in variation:
            if element.endswith('Hz'):
                frequency = int(element[:-2])
            elif element in GV.NOISE_FREQUENCY:
                frequency = self._frequency_dict[element]
            elif element in GV.NOISE_WAVEFORM:
                waveform = self._waveform_dict[element]
            elif element in GV.NOISE_AMPLITUDE:
                amplitude = self._amplitude_dict[element]

        if waveform and frequency and amplitude:
            return 'python %s %s %d %d' % (self._noise_script_dir,
                                           waveform, frequency, amplitude)
        return None

    def configure_noise(self, gesture, variation):
        if not gesture or not variation:
            return None

        if not isinstance(variation, tuple):
            variation = (variation,)

        noise_cmd = self._get_noise_command(gesture.name, variation)
        if noise_cmd:
            self._execute_noise_command(noise_cmd)

    def control(self, gesture, variation):
        """Have the robot perform the gesture variation."""
        tips_needed = conf.finger_tips_required[gesture.name]
        if self.fingertips != tips_needed:
            self._reset_with_safety_clearance('nest')
            self._return_fingertips()
            self._get_fingertips(tips_needed)
            self._reset_with_safety_clearance('pad')

        if not isinstance(variation, tuple):
            variation = (variation,)
        try:
            print gesture.name, variation
            control_cmd = self._get_control_command(gesture.name, variation)
            self._execute_control_command(control_cmd)

        except RobotWrapperError as e:
            print gesture.name, variation
            print 'RobotWrapperError: %s' % str(e)
            sys.exit(1)

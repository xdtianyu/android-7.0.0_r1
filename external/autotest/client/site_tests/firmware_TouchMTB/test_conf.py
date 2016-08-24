# -*- coding: utf-8 -*-
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This configuration file defines the gestures to perform."""

from collections import defaultdict
from firmware_constants import DEV, GV, VAL
from validators import (CountPacketsValidator,
                        CountTrackingIDNormalFingerValidator,
                        CountTrackingIDFatFingerValidator,
                        DragLatencyValidator,
                        DiscardInitialSecondsValidator,
                        DrumrollValidator,
                        HysteresisValidator,
                        LinearityFatFingerValidator,
                        LinearityNormalFingerValidator,
                        MtbSanityValidator,
                        NoGapValidator,
                        NoReversedMotionValidator,
                        PhysicalClickValidator,
                        PinchValidator,
                        RangeValidator,
                        ReportRateValidator,
                        StationaryValidator,
                        StationaryFingerValidator,
                        StationaryTapValidator,
)


# Define which score aggregator is to be used. A score aggregator collects
# the scores from every tests and calculates the final score for the touch
# firmware test suite.
score_aggregator = 'fuzzy.average'


# Define some common criteria
count_packets_criteria = '>= 3, ~ -3'
drumroll_criteria = '<= 2.0'
# linearity_criteria is used for strictly straight line drawn with a ruler.
linearity_criteria = '<= 0.8, ~ +2.4'
# relaxed_linearity_criteria is used for lines drawn with thumb edge or
# fat fingers which are allowed to be curvy to some extent.
relaxed_linearity_criteria = '<= 1.5, ~ +3.0'
no_gap_criteria = '<= 1.8, ~ +1.0'
no_level_jump_criteria = '<= 10, ~ +30'
no_reversed_motion_criteria = '<= 5, ~ +30'
pinch_criteria = '>= 200, ~ -100'
range_criteria = '<= 0.01, ~ +0.07'
min_report_rate = 60
max_report_interval = 1.0 / min_report_rate * 1000
report_rate_criteria = '>= %d' % min_report_rate
stationary_finger_criteria = '<= 1.0'
stationary_tap_criteria = '<= 1.0'
hysteresis_criteria = '<= 2.0'
drag_latency_criteria = '<= 28.0'

MIN_MOVING_DISTANCE = 20


# Define filenames and paths
docroot = '/tmp'
report_basename = 'touch_firmware_report'
html_ext = '.html'
ENVIRONMENT_REPORT_HTML_NAME = 'REPORT_HTML_NAME'
log_root_dir = '/var/tmp/touch_firmware_test'
fw_prefix = 'fw_'
device_description_dir = 'tests/device'
version_filename = '.version'


# Define parameters for GUI
score_colors = ((0.9, 'blue'), (0.8, 'orange'), (0.0, 'red'))
num_chars_per_row = 28


# Define the validators that are shown only when there are failures.
validators_hidden_when_no_failures = ['PinchValidator',
                                      'CountTrackingIDNormalFingerValidator',
                                      'CountTrackingIDFatFingerValidator',
                                      'CountPacketsValidator']


# Define the parent validators from which the derived validators should be
# merged in the top-level summary table.
merged_validators = [StationaryValidator,]


# Define the path to find the robot gestures library path
robot_lib_path_local = '/usr/local/lib*'
robot_lib_path = '/usr/lib*'
python_package = 'python*\.*'
gestures_sub_path = 'site-packages/touchbotII'


# Define the gesture names
NOISE_LINE = 'noise_line'
NOISE_STATIONARY = 'noise_stationary'
NOISE_STATIONARY_EXTENDED = 'noise_stationary_extended'
ONE_FINGER_TRACKING = 'one_finger_tracking'
ONE_FINGER_TO_EDGE = 'one_finger_to_edge'
TWO_FINGER_TRACKING = 'two_finger_tracking'
FINGER_CROSSING = 'finger_crossing'
ONE_FINGER_SWIPE = 'one_finger_swipe'
TWO_FINGER_SWIPE = 'two_finger_swipe'
PINCH_TO_ZOOM = 'pinch_to_zoom'
ONE_FINGER_TAP = 'one_finger_tap'
TWO_FINGER_TAP = 'two_finger_tap'
ONE_FINGER_PHYSICAL_CLICK = 'one_finger_physical_click'
TWO_FINGER_PHYSICAL_CLICK = 'two_fingers_physical_click'
THREE_FINGER_PHYSICAL_CLICK = 'three_fingers_physical_click'
FOUR_FINGER_PHYSICAL_CLICK = 'four_fingers_physical_click'
FIVE_FINGER_PHYSICAL_CLICK = 'five_fingers_physical_click'
STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS = \
        'stationary_finger_not_affected_by_2nd_finger_taps'
FAT_FINGER_MOVE_WITH_RESTING_FINGER = 'fat_finger_move_with_resting_finger'
DRAG_EDGE_THUMB = 'drag_edge_thumb'
TWO_CLOSE_FINGERS_TRACKING = 'two_close_fingers_tracking'
RESTING_FINGER_PLUS_2ND_FINGER_MOVE = 'resting_finger_plus_2nd_finger_move'
TWO_FAT_FINGERS_TRACKING = 'two_fat_fingers_tracking'
FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS = \
        'first_finger_tracking_and_second_finger_taps'
DRUMROLL = 'drumroll'
RAPID_TAPS = 'rapid_taps_20'
ONE_FINGER_TRACKING_FROM_CENTER = 'one_finger_tracking_from_center'
DRAG_LATENCY = 'drag_latency'
# This following gesture is for pressure calibration.
PRESSURE_CALIBRATION = 'pressure_calibration'

# The gaps in MM required between fingertips for these tests
FINGER_CROSSING_GAP_MM = 1
FAT_FINGER_AND_STATIONARY_FINGER_GAP_MM = 1

# This denotes the list of the numbers of fingers for physical click tests.
# It corresponds to ONE/TWO/THREE/FOUR/FIVE_FINGER_PHYSICAL_CLICK defined above.
fingers_physical_click = [1, 2, 3, 4, 5]


# Define the complete list
gesture_names_complete = {
    DEV.TOUCHPAD: [
        NOISE_LINE,
        NOISE_STATIONARY,
        ONE_FINGER_TRACKING,
        ONE_FINGER_TO_EDGE,
        TWO_FINGER_TRACKING,
        FINGER_CROSSING,
        ONE_FINGER_SWIPE,
        ONE_FINGER_TRACKING_FROM_CENTER,
        TWO_FINGER_SWIPE,
        PINCH_TO_ZOOM,
        ONE_FINGER_TAP,
        TWO_FINGER_TAP,
        ONE_FINGER_PHYSICAL_CLICK,
        TWO_FINGER_PHYSICAL_CLICK,
        THREE_FINGER_PHYSICAL_CLICK,
        FOUR_FINGER_PHYSICAL_CLICK,
        FIVE_FINGER_PHYSICAL_CLICK,
        STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS,
        FAT_FINGER_MOVE_WITH_RESTING_FINGER,
        DRAG_EDGE_THUMB,
        TWO_CLOSE_FINGERS_TRACKING,
        RESTING_FINGER_PLUS_2ND_FINGER_MOVE,
        TWO_FAT_FINGERS_TRACKING,
        FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS,
        DRUMROLL,
        RAPID_TAPS,
        DRAG_LATENCY,
    ],
    DEV.TOUCHSCREEN: [
        NOISE_LINE,
        NOISE_STATIONARY,
        ONE_FINGER_TRACKING,
        ONE_FINGER_TO_EDGE,
        TWO_FINGER_TRACKING,
        FINGER_CROSSING,
        ONE_FINGER_SWIPE,
        ONE_FINGER_TRACKING_FROM_CENTER,
        TWO_FINGER_SWIPE,
        PINCH_TO_ZOOM,
        ONE_FINGER_TAP,
        TWO_FINGER_TAP,
        STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS,
        FAT_FINGER_MOVE_WITH_RESTING_FINGER,
        DRAG_EDGE_THUMB,
        TWO_CLOSE_FINGERS_TRACKING,
        RESTING_FINGER_PLUS_2ND_FINGER_MOVE,
        TWO_FAT_FINGERS_TRACKING,
        FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS,
        DRUMROLL,
        RAPID_TAPS,
        DRAG_LATENCY,
    ],
}


# Define what gestures the robot can perform.
# This also defines the order for the robot to perform the gestures.
# Basically, two-fingers gestures follow one-finger gestures.
robot_capability_list = [
    NOISE_LINE,
    NOISE_STATIONARY,
    ONE_FINGER_TRACKING,
    ONE_FINGER_TO_EDGE,
    ONE_FINGER_SWIPE,
    ONE_FINGER_TRACKING_FROM_CENTER,
    ONE_FINGER_TAP,
    RAPID_TAPS,
    TWO_FINGER_TRACKING,
    TWO_CLOSE_FINGERS_TRACKING,
    TWO_FINGER_SWIPE,
    TWO_FINGER_TAP,
    STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS,
    FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS,
    RESTING_FINGER_PLUS_2ND_FINGER_MOVE,
    FINGER_CROSSING,
    PINCH_TO_ZOOM,
    DRUMROLL,
    TWO_FAT_FINGERS_TRACKING,
    FAT_FINGER_MOVE_WITH_RESTING_FINGER,
    ONE_FINGER_PHYSICAL_CLICK,
    TWO_FINGER_PHYSICAL_CLICK,
    THREE_FINGER_PHYSICAL_CLICK,
    FOUR_FINGER_PHYSICAL_CLICK,
]

NO_FINGER = None
TINY_FINGER = 0
NORMAL_FINGER = 1
LARGE_FINGER = 2
FAT_FINGER = 3
ALL_FINGERTIP_SIZES = [TINY_FINGER, NORMAL_FINGER, LARGE_FINGER, FAT_FINGER]
FINGERTIP_DIAMETER_MM = {TINY_FINGER: 8, NORMAL_FINGER: 10,
                         LARGE_FINGER: 12, FAT_FINGER: 14}

custom_tips_required = {
    DRAG_LATENCY: [NO_FINGER, NO_FINGER, FAT_FINGER, NO_FINGER],
    ONE_FINGER_PHYSICAL_CLICK: [NORMAL_FINGER, NO_FINGER, NO_FINGER, NO_FINGER],
    TWO_FINGER_PHYSICAL_CLICK: [NO_FINGER, NORMAL_FINGER, NO_FINGER,
                                NORMAL_FINGER],
    THREE_FINGER_PHYSICAL_CLICK: [NO_FINGER, NORMAL_FINGER, NORMAL_FINGER,
                                  NORMAL_FINGER],
    TWO_FAT_FINGERS_TRACKING: [FAT_FINGER, FAT_FINGER, FAT_FINGER, FAT_FINGER],
    FAT_FINGER_MOVE_WITH_RESTING_FINGER: [NORMAL_FINGER, NO_FINGER,
                                          FAT_FINGER, NO_FINGER],
    FINGER_CROSSING: [NORMAL_FINGER, NO_FINGER, NORMAL_FINGER, NO_FINGER],
    NOISE_LINE: [NO_FINGER, NORMAL_FINGER, NO_FINGER, NO_FINGER],
    NOISE_STATIONARY: [NORMAL_FINGER, NO_FINGER, NO_FINGER, NO_FINGER],
    NOISE_STATIONARY_EXTENDED: [NORMAL_FINGER, NO_FINGER, NO_FINGER, NO_FINGER],
}
default_tips_required = [NORMAL_FINGER, NORMAL_FINGER,
                         NORMAL_FINGER, NORMAL_FINGER]
finger_tips_required = defaultdict(lambda:default_tips_required,
                                   custom_tips_required)

def get_gesture_names_for_robot(device):
    """Get the gesture names that a robot can do for a specified device."""
    return [gesture for gesture in robot_capability_list
                    if gesture in gesture_names_complete[device]]


# Define the list of one-finger and two-finger gestures to test using the robot.
gesture_names_robot = {
    DEV.TOUCHPAD: get_gesture_names_for_robot(DEV.TOUCHPAD),
    DEV.TOUCHSCREEN: get_gesture_names_for_robot(DEV.TOUCHSCREEN),
}

gesture_names_quickstep = [DRAG_LATENCY]

# Define the list of gestures that require a function generator to run
gesture_names_fngenerator_required = [NOISE_LINE, NOISE_STATIONARY,
                                      NOISE_STATIONARY_EXTENDED]

# Define the list of gestures to test in NOISE mode.
gesture_names_noise_extended = [NOISE_STATIONARY_EXTENDED]


# Define the gesture for pressure calibration
gesture_names_calibration = [PRESSURE_CALIBRATION,]

# Define the relative segment weights of a validator.
# For example, LinearityMiddleValidator : LinearityBothEndsValidator = 7 : 3
segment_weights = {VAL.BEGIN: 0.15,
                   VAL.MIDDLE: 0.7,
                   VAL.END: 0.15,
                   VAL.BOTH_ENDS: 0.15 + 0.15,
                   VAL.WHOLE: 0.15 + 0.7 + 0.15,
}


# Define the validator score weights
weight_rare = 1
weight_common = 2
weight_critical = 3
validator_weights = {'CountPacketsValidator': weight_common,
                     'CountTrackingIDNormalFingerValidator': weight_critical,
                     'CountTrackingIDFatFingerValidator': weight_rare,
                     'DragLatencyValidator': weight_critical,
                     'DrumrollValidator': weight_rare,
                     'LinearityNormalFingerValidator': weight_common,
                     'LinearityFatFingerValidator': weight_rare,
                     'MtbSanityValidator': weight_critical,
                     'NoGapValidator': weight_common,
                     'NoReversedMotionValidator': weight_common,
                     'PhysicalClickValidator': weight_critical,
                     'PinchValidator': weight_common,
                     'RangeValidator': weight_common,
                     'ReportRateValidator': weight_common,
                     'HysteresisValidator': weight_common,
                     'StationaryFingerValidator': weight_common,
                     'StationaryTapValidator': weight_common,
}


# Define the gesture list that the user needs to perform in the test suite.
def get_gesture_dict():
    """Define the dictionary for all gestures."""
    gesture_dict = {
        NOISE_STATIONARY:
        Gesture(
            name=NOISE_STATIONARY,
            variations=((GV.LOW_FREQUENCY, GV.MED_FREQUENCY, GV.HIGH_FREQUENCY),
                        (GV.HALF_AMPLITUDE, GV.MAX_AMPLITUDE),
                        (GV.SQUARE_WAVE,),
                        (GV.TL, GV.TR, GV.BL, GV.BR, GV.TS, GV.BS, GV.LS, GV.RS,
                         GV.CENTER),
            ),
            prompt='Hold one finger on the {3} of the touch surface with a '
                   '{0} {1} {2} in noise.',
            subprompt={
                GV.TL: ('top left corner',),
                GV.TR: ('top right corner',),
                GV.BL: ('bottom left corner',),
                GV.BR: ('bottom right corner',),
                GV.TS: ('top edge',),
                GV.BS: ('bottom side',),
                GV.LS: ('left hand side',),
                GV.RS: ('right hand side',),
                GV.CENTER: ('center',),
                GV.LOW_FREQUENCY: ('5kHz',),
                GV.MED_FREQUENCY: ('500kHz',),
                GV.HIGH_FREQUENCY: ('1MHz',),
                GV.HALF_AMPLITUDE: ('10Vpp',),
                GV.MAX_AMPLITUDE: ('20Vpp',),
                GV.SQUARE_WAVE: ('square wave',),
            },
            validators=(
                DiscardInitialSecondsValidator(
                    CountTrackingIDNormalFingerValidator('== 1')),
                DiscardInitialSecondsValidator(
                    StationaryTapValidator(stationary_tap_criteria, slot=0)),
            ),
        ),
        NOISE_LINE:
        Gesture(
            name=NOISE_LINE,
            variations=((GV.LOW_FREQUENCY, GV.MED_FREQUENCY, GV.HIGH_FREQUENCY),
                        (GV.HALF_AMPLITUDE, GV.MAX_AMPLITUDE),
                        (GV.SQUARE_WAVE,),
                        (GV.BLTR,),
                        (GV.NORMAL,),
            ),
            prompt='Draw a straight line from {3} with a {0} {1} {2} in noise.',
            subprompt={
                GV.LOW_FREQUENCY: ('5kHz',),
                GV.MED_FREQUENCY: ('500kHz',),
                GV.HIGH_FREQUENCY: ('1MHz',),
                GV.HALF_AMPLITUDE: ('10Vpp',),
                GV.MAX_AMPLITUDE: ('20Vpp',),
                GV.SQUARE_WAVE: ('square wave',),
                GV.NORMAL: ('',),
                GV.BLTR: ('bottom left to top right',),
            },
            validators=(
                DiscardInitialSecondsValidator(
                    CountTrackingIDNormalFingerValidator('== 1')),
                DiscardInitialSecondsValidator(
                    LinearityNormalFingerValidator(linearity_criteria, finger=0,
                                                   segments=VAL.MIDDLE)),
                DiscardInitialSecondsValidator(
                    NoGapValidator(no_gap_criteria, slot=0)),
                DiscardInitialSecondsValidator(
                    NoReversedMotionValidator(no_reversed_motion_criteria,
                                              slots=0, segments=VAL.MIDDLE)),
                DiscardInitialSecondsValidator(
                    NoReversedMotionValidator(no_reversed_motion_criteria,
                                              slots=0, segments=VAL.BOTH_ENDS)),
                DiscardInitialSecondsValidator(
                    ReportRateValidator(report_rate_criteria)),
            ),
        ),
        NOISE_STATIONARY_EXTENDED:
        Gesture(
            name=NOISE_STATIONARY_EXTENDED,
            variations=(tuple(GV.EXTENDED_FREQUENCIES),
                        (GV.MAX_AMPLITUDE,),
                        (GV.SQUARE_WAVE,),
                        (GV.CENTER,),
            ),
            prompt='Hold one finger on the {3} of the touch surface with a '
                   '{0} {1} {2} in noise.',
            subprompt=dict({
                GV.CENTER: ('center',),
                GV.MAX_AMPLITUDE: ('20Vpp',),
                GV.SQUARE_WAVE: ('square wave',),
            }.items() +
                {freq: (freq,) for freq in GV.EXTENDED_FREQUENCIES}.items()),
            validators=(
                DiscardInitialSecondsValidator(
                    CountTrackingIDNormalFingerValidator('== 1')),
                DiscardInitialSecondsValidator(
                    StationaryTapValidator(stationary_tap_criteria, slot=0)),
            ),
        ),
        ONE_FINGER_TRACKING:
        Gesture(
            name=ONE_FINGER_TRACKING,
            variations=((GV.LR, GV.RL, GV.TB, GV.BT, GV.BLTR, GV.TRBL),
                        (GV.SLOW, GV.NORMAL),
            ),
            prompt='Take {2} to draw a straight, {0} line {1} using a ruler.',
            subprompt={
                GV.LR: ('horizontal', 'from left to right',),
                GV.RL: ('horizontal', 'from right to left',),
                GV.TB: ('vertical', 'from top to bottom',),
                GV.BT: ('vertical', 'from bottom to top',),
                GV.BLTR: ('diagonal', 'from bottom left to top right',),
                GV.TRBL: ('diagonal', 'from top right to bottom left',),
                GV.SLOW: ('3 seconds',),
                GV.NORMAL: ('1 second',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 1'),
                LinearityNormalFingerValidator(linearity_criteria, finger=0,
                                               segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0,
                                          segments=VAL.MIDDLE),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0,
                                          segments=VAL.BOTH_ENDS),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        ONE_FINGER_TO_EDGE:
        Gesture(
            name=ONE_FINGER_TO_EDGE,
            variations=((GV.CL, GV.CR, GV.CT, GV.CB),
                        (GV.SLOW,),
            ),
            prompt='Take {2} to draw a striaght {0} line {1}.',
            subprompt={
                GV.CL: ('horizontal', 'from the center off left edge',),
                GV.CR: ('horizontal', 'from the center off right edge',),
                GV.CT: ('vertical', 'from the center  off top edge',),
                GV.CB: ('vertical', 'from the center off bottom edge',),
                GV.SLOW: ('2 seconds',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 1'),
                LinearityNormalFingerValidator(linearity_criteria, finger=0,
                                               segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                RangeValidator(range_criteria),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        TWO_FINGER_TRACKING:
        Gesture(
            name=TWO_FINGER_TRACKING,
            variations=((GV.LR, GV.RL, GV.TB, GV.BT, GV.BLTR, GV.TRBL),
                        (GV.SLOW, GV.NORMAL),
            ),
            prompt='Take {2} to draw a {0} line {1} using a ruler '
                   'with TWO fingers at the same time.',
            subprompt={
                GV.LR: ('horizontal', 'from left to right',),
                GV.RL: ('horizontal', 'from right to left',),
                GV.TB: ('vertical', 'from top to bottom',),
                GV.BT: ('vertical', 'from bottom to top',),
                GV.BLTR: ('diagonal', 'from bottom left to top right',),
                GV.TRBL: ('diagonal', 'from top right to bottom left',),
                GV.SLOW: ('3 seconds',),
                GV.NORMAL: ('1 second',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 2'),
                LinearityNormalFingerValidator(linearity_criteria, finger=0,
                                               segments=VAL.MIDDLE),
                LinearityNormalFingerValidator(linearity_criteria, finger=1,
                                               segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoGapValidator(no_gap_criteria, slot=1),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=1),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        FINGER_CROSSING:
        Gesture(
            # also covers stationary_finger_not_affected_by_2nd_moving_finger
            name=FINGER_CROSSING,
            variations=((GV.LR, GV.RL, GV.TB, GV.BT, GV.BLTR, GV.TRBL),
                        (GV.SLOW, GV.NORMAL),
            ),
            prompt='Place one stationary finger near the center of the '
                   'touch surface, then take {2} to draw a straight line '
                   '{0} {1} with a second finger',
            subprompt={
                GV.LR: ('from left to right', 'above the stationary finger'),
                GV.RL: ('from right to left', 'below the stationary finger'),
                GV.TB: ('from top to bottom',
                        'on the right to the stationary finger'),
                GV.BT: ('from bottom to top',
                        'on the left to the stationary finger'),
                GV.BLTR: ('from the bottom left to the top right',
                          'above the stationary finger',),
                GV.TRBL: ('from the top right to the bottom left',
                          'below the stationary finger'),
                GV.SLOW: ('3 seconds',),
                GV.NORMAL: ('1 second',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 2'),
                NoGapValidator(no_gap_criteria, slot=1),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=1),
                ReportRateValidator(report_rate_criteria, finger=1),
                StationaryFingerValidator(stationary_finger_criteria, slot=0),
            ),
        ),

        ONE_FINGER_SWIPE:
        Gesture(
            name=ONE_FINGER_SWIPE,
            variations=(GV.BLTR, GV.TRBL),
            prompt='Use ONE finger to quickly swipe {0}.',
            subprompt={
                GV.BLTR: ('from the bottom left to the top right',),
                GV.TRBL: ('from the top right to the bottom left',),
            },
            validators=(
                CountPacketsValidator(count_packets_criteria, slot=0),
                CountTrackingIDNormalFingerValidator('== 1'),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        TWO_FINGER_SWIPE:
        Gesture(
            name=TWO_FINGER_SWIPE,
            variations=(GV.TB, GV.BT),
            prompt='Use TWO fingers to quickly swipe {0}.',
            subprompt={
                GV.TB: ('from top to bottom',),
                GV.BT: ('from bottom to top',),
            },
            validators=(
                CountPacketsValidator(count_packets_criteria, slot=0),
                CountPacketsValidator(count_packets_criteria, slot=1),
                CountTrackingIDNormalFingerValidator('== 2'),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=1),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        PINCH_TO_ZOOM:
        Gesture(
            name=PINCH_TO_ZOOM,
            variations=(GV.ZOOM_IN, GV.ZOOM_OUT),
            prompt='Using two fingers, preform a "{0}" pinch by bringing'
                   'your fingers {1}.',
            subprompt={
                GV.ZOOM_IN: ('zoom in', 'farther apart'),
                GV.ZOOM_OUT: ('zoom out', 'closer together'),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 2'),
                PinchValidator(pinch_criteria),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        ONE_FINGER_TAP:
        Gesture(
            name=ONE_FINGER_TAP,
            variations=(GV.TL, GV.TR, GV.BL, GV.BR, GV.TS, GV.BS, GV.LS, GV.RS,
                        GV.CENTER),
            prompt='Use one finger to tap on the {0} of the touch surface.',
            subprompt={
                GV.TL: ('top left corner',),
                GV.TR: ('top right corner',),
                GV.BL: ('bottom left corner',),
                GV.BR: ('bottom right corner',),
                GV.TS: ('top edge',),
                GV.BS: ('bottom side',),
                GV.LS: ('left hand side',),
                GV.RS: ('right hand side',),
                GV.CENTER: ('center',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 1'),
                StationaryTapValidator(stationary_tap_criteria, slot=0),
            ),
        ),

        TWO_FINGER_TAP:
        Gesture(
            name=TWO_FINGER_TAP,
            variations=(GV.HORIZONTAL, GV.VERTICAL, GV.DIAGONAL),
            prompt='Use two fingers aligned {0} to tap the center of the '
                   'touch surface.',
            subprompt={
                GV.HORIZONTAL: ('horizontally',),
                GV.VERTICAL: ('vertically',),
                GV.DIAGONAL: ('diagonally',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 2'),
                StationaryTapValidator(stationary_tap_criteria, slot=0),
                StationaryTapValidator(stationary_tap_criteria, slot=1),
            ),
        ),

        ONE_FINGER_PHYSICAL_CLICK:
        Gesture(
            name=ONE_FINGER_PHYSICAL_CLICK,
            variations=(GV.CENTER, GV.BL, GV.BS, GV.BR),
            prompt='Use one finger to physically click the {0} of the '
                   'touch surface.',
            subprompt={
                GV.CENTER: ('center',),
                GV.BL: ('bottom left corner',),
                GV.BS: ('bottom side',),
                GV.BR: ('bottom right corner',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 1'),
                PhysicalClickValidator('== 1', fingers=1),
                StationaryTapValidator(stationary_tap_criteria, slot=0),
            ),
        ),

        TWO_FINGER_PHYSICAL_CLICK:
        Gesture(
            name=TWO_FINGER_PHYSICAL_CLICK,
            variations=None,
            prompt='Use two fingers physically click the center of the '
                   'touch surface.',
            subprompt=None,
            validators=(
                CountTrackingIDNormalFingerValidator('== 2'),
                PhysicalClickValidator('== 1', fingers=2),
                StationaryTapValidator(stationary_tap_criteria, slot=0),
                StationaryTapValidator(stationary_tap_criteria, slot=1),
            ),
        ),

        THREE_FINGER_PHYSICAL_CLICK:
        Gesture(
            name=THREE_FINGER_PHYSICAL_CLICK,
            variations=None,
            prompt='Use three fingers to physically click '
                   'the center of the touch surface.',
            subprompt=None,
            validators=(
                CountTrackingIDNormalFingerValidator('== 3'),
                PhysicalClickValidator('== 1', fingers=3),
            ),
        ),

        FOUR_FINGER_PHYSICAL_CLICK:
        Gesture(
            name=FOUR_FINGER_PHYSICAL_CLICK,
            variations=None,
            prompt='Use four fingers to physically click '
                   'the center of the touch surface.',
            subprompt=None,
            validators=(
                CountTrackingIDNormalFingerValidator('== 4'),
                PhysicalClickValidator('== 1', fingers=4),
            ),
        ),

        FIVE_FINGER_PHYSICAL_CLICK:
        Gesture(
            name=FIVE_FINGER_PHYSICAL_CLICK,
            variations=None,
            prompt='Use five fingers to physically click '
                   'the center of the touch surface.',
            subprompt=None,
            validators=(
                CountTrackingIDNormalFingerValidator('== 5'),
                PhysicalClickValidator('== 1', fingers=5),
            ),
        ),

        STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS:
        Gesture(
            name=STATIONARY_FINGER_NOT_AFFECTED_BY_2ND_FINGER_TAPS,
            variations=(GV.AROUND,),
            prompt='Place your one stationary finger in the middle of the '
                   'touch surface, and use a second finger to tap '
                   'all around it many times (50)',
            subprompt=None,
            validators=(
                CountTrackingIDNormalFingerValidator('>= 25'),
                StationaryFingerValidator(stationary_finger_criteria, slot=0),
            ),
        ),

        FAT_FINGER_MOVE_WITH_RESTING_FINGER:
        Gesture(
            name=FAT_FINGER_MOVE_WITH_RESTING_FINGER,
            variations=(GV.LR, GV.RL, GV.TB, GV.BT),
            prompt='With a stationary finger on the {0} of the touch surface, '
                   'draw a straight line with a FAT finger {1} {2} it.',
            subprompt={
                GV.LR: ('center', 'from left to right', 'below'),
                GV.RL: ('bottom edge', 'from right to left', 'above'),
                GV.TB: ('center', 'from top to bottom', 'on the right to'),
                GV.BT: ('center', 'from bottom to top', 'on the left to'),
            },
            validators=(
                CountTrackingIDFatFingerValidator('== 2'),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=1, segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=1),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=1),
                ReportRateValidator(report_rate_criteria, finger=1),
                StationaryFingerValidator(stationary_finger_criteria, slot=0),
            ),
        ),

        DRAG_EDGE_THUMB:
        Gesture(
            name=DRAG_EDGE_THUMB,
            variations=(GV.LR, GV.RL, GV.TB, GV.BT),
            prompt='Drag the edge of your thumb {0} in a straight line '
                   'across the touch surface',
            subprompt={
                GV.LR: ('horizontally from left to right',),
                GV.RL: ('horizontally from right to left',),
                GV.TB: ('vertically from top to bottom',),
                GV.BT: ('vertically from bottom to top',),
            },
            validators=(
                CountTrackingIDFatFingerValidator('== 1'),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=0, segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        TWO_CLOSE_FINGERS_TRACKING:
        Gesture(
            name=TWO_CLOSE_FINGERS_TRACKING,
            variations=(GV.LR, GV.TB, GV.TLBR),
            prompt='With two fingers close together (lightly touching each '
                   'other) in a two finger scrolling gesture, draw a {0} '
                   'line {1}.',
            subprompt={
                GV.LR: ('horizontal', 'from left to right',),
                GV.TB: ('vertical', 'from top to bottom',),
                GV.TLBR: ('diagonal', 'from the top left to the bottom right',),
            },
            validators=(
                CountTrackingIDFatFingerValidator('== 2'),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=0, segments=VAL.MIDDLE),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=1, segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        RESTING_FINGER_PLUS_2ND_FINGER_MOVE:
        Gesture(
            name=RESTING_FINGER_PLUS_2ND_FINGER_MOVE,
            variations=((GV.TLBR, GV.BRTL),
                        (GV.SLOW,),
            ),
            prompt='With a stationary finger in the bottom left corner, take '
                   '{1} to draw a straight line {0} with a second finger.',
            subprompt={
                GV.TLBR: ('from the top left to the bottom right',),
                GV.BRTL: ('from the bottom right to the top left',),
                GV.SLOW: ('3 seconds',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 2'),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=1, segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=1),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=1),
                ReportRateValidator(report_rate_criteria, finger=1),
                StationaryFingerValidator(stationary_finger_criteria, slot=0),
            ),
        ),

        TWO_FAT_FINGERS_TRACKING:
        Gesture(
            name=TWO_FAT_FINGERS_TRACKING,
            variations=(GV.LR, GV.RL),
            prompt='Use two FAT fingers separated by about 1cm to draw '
                   'a straight line {0}.',
            subprompt={
                GV.LR: ('from left to right',),
                GV.RL: ('from right to left',),
            },
            validators=(
                CountTrackingIDFatFingerValidator('== 2'),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=0, segments=VAL.MIDDLE),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=1, segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoGapValidator(no_gap_criteria, slot=1),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=1),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS:
        Gesture(
            name=FIRST_FINGER_TRACKING_AND_SECOND_FINGER_TAPS,
            variations=(GV.TLBR, GV.BRTL),
            prompt='While drawing a straight line {0} slowly (~3 seconds), '
                   'tap the bottom left corner with a second finger '
                   'gently 3 times.',
            subprompt={
                GV.TLBR: ('from top left to bottom right',),
                GV.BRTL: ('from bottom right to top left',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 4'),
                LinearityFatFingerValidator(relaxed_linearity_criteria,
                                            finger=0, segments=VAL.MIDDLE),
                NoGapValidator(no_gap_criteria, slot=0),
                NoReversedMotionValidator(no_reversed_motion_criteria, slots=0),
                ReportRateValidator(report_rate_criteria),
            ),
        ),

        DRUMROLL:
        Gesture(
            name=DRUMROLL,
            variations=(GV.FAST, ),
            prompt='Use the index and middle finger of one hand to make a '
                   '"drum roll" {0} by alternately tapping each finger '
                   'for 5 seconds.',
            subprompt={
                GV.FAST: ('as fast as possible',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('>= 5'),
                DrumrollValidator(drumroll_criteria),
            ),
            timeout = 2000,
        ),

        RAPID_TAPS:
        Gesture(
            name=RAPID_TAPS,
            variations=(GV.TL, GV.BR, GV.CENTER),
            prompt='Tap the {0} of the touch surface 20 times quickly',
            subprompt={
                GV.TL: ('top left corner',),
                GV.TS: ('top edge',),
                GV.TR: ('top right corner',),
                GV.LS: ('left edge',),
                GV.CENTER: ('center',),
                GV.RS: ('right edge',),
                GV.BL: ('bottom left corner',),
                GV.BS: ('bottom edge',),
                GV.BR: ('bottom right corner',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 20'),
            ),
            timeout = 2000,
        ),

        ONE_FINGER_TRACKING_FROM_CENTER:
        Gesture(
            name=ONE_FINGER_TRACKING_FROM_CENTER,
            variations=((GV.CR, GV.CT, GV.CUL, GV.CLL),
                        (GV.SLOW, GV.NORMAL),
            ),
            prompt='Place a stationary finger on the center of the touch '
                   'surface for about 1 second, and then take {2} to draw a '
                   '{0} line {1}.',
            subprompt={
                GV.CR: ('horizontal', 'to the right',),
                GV.CT: ('vertical', 'to the top',),
                GV.CUL: ('diagonal', 'to the upper left',),
                GV.CLL: ('diagonal', 'to the lower left',),
                GV.SLOW: ('2 seconds',),
                GV.NORMAL: ('1 second',),
            },
            validators=(
                HysteresisValidator(hysteresis_criteria, finger=0),
            ),
        ),

        DRAG_LATENCY:
        Gesture(
            name=DRAG_LATENCY,
            variations=None,
            prompt='Run one finger back and forth across the pad making sure '
                   'to break the laser completely on each pass.  Make at least '
                   'twenty passes.',
            subprompt=None,
            validators=(
                CountTrackingIDNormalFingerValidator('== 1'),
                DragLatencyValidator(drag_latency_criteria),
            ),
        ),

        PRESSURE_CALIBRATION:
        Gesture(
            name=PRESSURE_CALIBRATION,
            variations=(GV.SIZE0, GV.SIZE1, GV.SIZE2, GV.SIZE3, GV.SIZE4,
                        GV.SIZE5, GV.SIZE6, ),
            prompt='Draw circles continuously for 5 seconds '
                   'using the metal finger of size {0}.',
            subprompt={
                GV.SIZE0: ('0 (the smallest size)',),
                GV.SIZE1: ('1',),
                GV.SIZE2: ('2',),
                GV.SIZE3: ('3',),
                GV.SIZE4: ('4',),
                GV.SIZE5: ('5',),
                GV.SIZE6: ('6 (the largest size)',),
            },
            validators=(
                CountTrackingIDNormalFingerValidator('== 1'),
            ),
        ),
    }
    return gesture_dict


class FileName:
    """A dummy class to hold the attributes in a test file name."""
    pass
filename = FileName()
filename.sep = '-'
filename.ext = 'dat'


class Gesture:
    """A class defines the structure of Gesture."""
    # define the default timeout (in milli-seconds) when performing a gesture.
    # A gesture is considered done when finger is lifted for this time interval.
    TIMEOUT = int(1000/80*10)

    def __init__(self, name=None, variations=None, prompt=None, subprompt=None,
                 validators=None, timeout=TIMEOUT):
        self.name = name
        self.variations = variations
        self.prompt = prompt
        self.subprompt = subprompt
        self.validators = validators
        self.timeout = timeout

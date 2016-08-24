# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Define constants for firmware touch device MTB tests."""


import sys

sys.path.append('../../bin/input')
from linux_input import (KEY_D, KEY_M, KEY_X, KEY_Y, KEY_ENTER, KEY_SPACE,
                         KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT)


class _ConstantError(AttributeError):
    """A constant error exception."""
    pass


class _Constant(object):
    """This is a constant base class to ensure no rebinding of constants."""
    def __setattr__(self, name, value):
        """Check the attribute assignment. No rebinding is allowed."""
        if name in self.__dict__:
            raise _ConstantError, "Cannot rebind the constant: %s" % name
        self.__dict__[name] = value


"""Define constants classes in alphabetic order below."""


class _Axis(_Constant):
    """Constants about two axes."""
    pass
AXIS = _Axis()
AXIS.X = 'x'
AXIS.Y = 'y'
AXIS.LIST = [AXIS.X, AXIS.Y]


class _DeviceType(_Constant):
    """Constants about device types."""
DEV = _DeviceType()
DEV.TOUCHPAD = 'touchpad'
DEV.TOUCHSCREEN = 'touchscreen'
DEV.DEVICE_TYPE_LIST = list(DEV.__dict__.values())


class _Fuzzy_MF(_Constant):
    """Constants about fuzzy membership functions."""
    pass
MF = _Fuzzy_MF()
MF.PI_FUNCTION = 'Pi_Function'
MF.S_FUNCTION = 'S_Function'
MF.SINGLETON_FUNCTION = 'Singleton_Function'
MF.TRAPEZ_FUNCTION = 'Trapez_Function'
MF.TRIANGLE_FUNCTION = 'Triangle_Function'
MF.Z_FUNCTION = 'Z_Function'


class _GestureVariation(_Constant):
    """Constants about gesture variations."""
    pass
GV = _GestureVariation()
# constants about directions
GV.HORIZONTAL = 'horizontal'
GV.VERTICAL = 'vertical'
GV.DIAGONAL = 'diagonal'
GV.LR = 'left_to_right'
GV.RL = 'right_to_left'
GV.TB = 'top_to_bottom'
GV.BT = 'bottom_to_top'
GV.CL = 'center_to_left'
GV.CR = 'center_to_right'
GV.CT = 'center_to_top'
GV.CB = 'center_to_bottom'
GV.CUR = 'center_to_upper_right'
GV.CUL = 'center_to_upper_left'
GV.CLR = 'center_to_lower_right'
GV.CLL = 'center_to_lower_left'
GV.BLTR = 'bottom_left_to_top_right'
GV.BRTL = 'bottom_right_to_top_left'
GV.TRBL = 'top_right_to_bottom_left'
GV.TLBR = 'top_left_to_bottom_right'
GV.HORIZONTAL_DIRECTIONS = [GV.HORIZONTAL, GV.LR, GV.RL, GV.CL, GV.CR]
GV.VERTICAL_DIRECTIONS = [GV.VERTICAL, GV.TB, GV.BT, GV.CT, GV.CB]
GV.DIAGONAL_DIRECTIONS = [GV.DIAGONAL, GV.BLTR, GV.BRTL, GV.TRBL, GV.TLBR,
                          GV.CUR, GV.CUL, GV.CLR, GV.CLL]
GV.GESTURE_DIRECTIONS = (GV.HORIZONTAL_DIRECTIONS + GV.VERTICAL_DIRECTIONS +
                         GV.DIAGONAL_DIRECTIONS)
# constants about locations
GV.TL = 'top_left'
GV.TR = 'top_right'
GV.BL = 'bottom_left'
GV.BR = 'bottom_right'
GV.TS = 'top_side'
GV.BS = 'bottom_side'
GV.LS = 'left_side'
GV.RS = 'right_side'
GV.CENTER = 'center'
GV.AROUND = 'around'
GV.GESTURE_LOCATIONS = [GV.TL, GV.TR, GV.BL, GV.BR, GV.TS, GV.BS, GV.LS, GV.RS,
                        GV.CENTER, GV.AROUND]
# constants about pinch to zoom
GV.ZOOM_IN = 'zoom_in'
GV.ZOOM_OUT = 'zoom_out'
# constants about speed
GV.SLOW = 'slow'
GV.NORMAL = 'normal'
GV.FAST = 'fast'
GV.FULL_SPEED = 'full_speed'
GV.GESTURE_SPEED = [GV.SLOW, GV.NORMAL, GV.FAST, GV.FULL_SPEED]
# constants about noise frequency
GV.LOW_FREQUENCY = 'low_frequency'
GV.MED_FREQUENCY = 'med_frequency'
GV.HIGH_FREQUENCY = 'high_frequency'
# constants used in the extended frequency sweep.
# Sweep from 0Hz to 1MHz at an interval of 400Hz for 2500 tests.
GV.EXTENDED_FREQUENCIES = ['%dHz' % x for x in range(0, 1000000, 400)]
GV.NOISE_FREQUENCY = [GV.LOW_FREQUENCY, GV.MED_FREQUENCY, GV.HIGH_FREQUENCY] + \
                      GV.EXTENDED_FREQUENCIES
# constants about noise waveform
GV.SQUARE_WAVE = 'square_wave'
GV.SINE_WAVE = 'sine_wave'
GV.NOISE_WAVEFORM = [GV.SQUARE_WAVE, GV.SINE_WAVE]
# constants about noise amplitude
GV.MAX_AMPLITUDE = 'max_amplitude'
GV.HALF_AMPLITUDE = 'half_amplitude'
GV.NOISE_AMPLITUDE = [GV.HALF_AMPLITUDE, GV.MAX_AMPLITUDE]
# constants about metal finger sizes
GV.SIZE = 'size'
size_str = lambda i: GV.SIZE + str(i)
GV.SIZE0 = size_str(0)
GV.SIZE1 = size_str(1)
GV.SIZE2 = size_str(2)
GV.SIZE3 = size_str(3)
GV.SIZE4 = size_str(4)
GV.SIZE5 = size_str(5)
GV.SIZE6 = size_str(6)
GV.NUMBER_OF_SIZES = 7
GV.SIZE_LIST = [size_str(i) for i in range(GV.NUMBER_OF_SIZES)]


class _Mode(_Constant):
    """Constants about gesture playing mode."""
    pass
MODE = _Mode()
MODE.CALIBRATION = 'calibration'
MODE.COMPLETE = 'complete'
MODE.MANUAL = 'manual'
MODE.NOISE = 'noise'
MODE.QUICKSTEP = 'quickstep'
MODE.REPLAY = 'replay'
MODE.ROBOT = 'robot'
MODE.ROBOT_SIM = 'robot_sim'
# GESTURE_PLAY_MODE is a list of all attributes above
MODE.GESTURE_PLAY_MODE = list(MODE.__dict__.values())


class _MTB(_Constant):
    """Constants about MTB event format and MTB related constants."""
    pass
MTB = _MTB()
MTB.EV_TIME = 'EV_TIME'
MTB.EV_TYPE = 'EV_TYPE'
MTB.EV_CODE = 'EV_CODE'
MTB.EV_VALUE = 'EV_VALUE'
MTB.SYN_REPORT = 'SYN_REPORT'
MTB.SLOT = 'slot'
MTB.POINTS = 'points'


class _Options(_Constant):
    """Constants about command line options."""
    pass
OPTIONS = _Options()
OPTIONS.DEBUG = 'debug'
OPTIONS.DEVICE = 'system_device'
OPTIONS.DIR = 'directory'
OPTIONS.FNGENERATOR = 'function_generator'
OPTIONS.HELP = 'help'
OPTIONS.INDIVIDUAL = 'individual'
OPTIONS.ITERATIONS = 'iterations'
OPTIONS.METRICS = 'show_metrics'
OPTIONS.MODE = 'mode'
OPTIONS.REPLAY = 'replay'
OPTIONS.RESUME = 'resume'
OPTIONS.SCORES = 'scores'
OPTIONS.SIMPLIFIED = 'simplified'
OPTIONS.SKIP_HTML = 'skip_html'
OPTIONS.TOUCHSCREEN = 'touchscreen'
OPTIONS.UPLOAD = 'upload'


class _Platform(_Constant):
    """Constants about chromebook platforms."""
    pass
PLATFORM = _Platform()
PLATFORM.ALEX = 'alex'
PLATFORM.LUMPY = 'lumpy'
PLATFORM.LINK = 'link'
PLATFORM.LIST = [PLATFORM.ALEX, PLATFORM.LUMPY, PLATFORM.LINK]


class _RobotControl(_Constant):
    """Constants about robot control."""
    pass
RC = _RobotControl()
RC.PAUSE_TYPE = 'pause_type'
RC.PROMPT = 'finger_control_prompt'
# Finger interaction per gesture
# e.g., the TWO_FINGER_TRACKING gesture requires installing an extra finger
#       once for all variations in the same gesture.
RC.PER_GESTURE = 'per_gesture'
# Finger interaction per variation
# e.g., the FINGER_CROSSING gesture requires putting down and lifting up
# a metal finger repeatedly per variation.
RC.PER_VARIATION = 'per_variation'


class _TFK(_Constant):
    """The Test Flow Keypress (TFK) codes for test flow"""
    pass
TFK = _TFK()
TFK.DISCARD = KEY_D
TFK.EXIT = KEY_X
TFK.MORE = KEY_M
TFK.SAVE = KEY_SPACE
TFK.SAVE2 = KEY_ENTER
TFK.YES = KEY_Y
TFK.UP = KEY_UP
TFK.DOWN = KEY_DOWN
TFK.LEFT = KEY_LEFT
TFK.RIGHT = KEY_RIGHT
TFK.ARROW_KEY_LIST = [TFK.UP, TFK.DOWN, TFK.LEFT, TFK.RIGHT]


class _UNIT(_Constant):
    """Constants about units."""
    pass
UNIT = _UNIT()
UNIT.PIXEL = 'px'
UNIT.MM = 'mm'


class _Validator(_Constant):
    """Constants about validator."""
    pass
VAL = _Validator()
VAL.BEGIN = 'Begin'
VAL.MIDDLE = 'Middle'
VAL.END = 'End'
VAL.BOTH_ENDS = 'BothEnds'
VAL.WHOLE = 'Whole'
# SEGMENT_LIST is a list of all attributes above
VAL.SEGMENT_LIST = list(VAL.__dict__.values())

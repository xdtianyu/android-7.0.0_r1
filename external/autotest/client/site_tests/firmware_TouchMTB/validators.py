# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Validators to verify if events conform to specified criteria."""


'''
How to add a new validator/gesture:
(1) Implement a new validator class inheriting BaseValidator,
(2) add proper method in mtb.Mtb class,
(3) add the new validator in test_conf, and
        'from validators import the_new_validator'
    in alphabetical order, and
(4) add the validator in relevant gestures; add a new gesture if necessary.

The validator template is as follows:

class XxxValidator(BaseValidator):
    """Validator to check ...

    Example:
        To check ...
          XxxValidator('<= 0.05, ~ +0.05', fingers=2)
    """

    def __init__(self, criteria_str, mf=None, fingers=1):
        name = self.__class__.__name__
        super(X..Validator, self).__init__(criteria_str, mf, name)
        self.fingers = fingers

    def check(self, packets, variation=None):
        """Check ..."""
        self.init_check(packets)
        xxx = self.packets.xxx()
        self.print_msg(...)
        return (self.fc.mf.grade(...), self.msg_list)


Note that it is also possible to instantiate a validator as
          XxxValidator('<= 0.05, ~ +0.05', slot=0)

    Difference between fingers and slot:
      . When specifying 'fingers', e.g., fingers=2, the purpose is to pass
        the information about how many fingers there are in the gesture. In
        this case, the events in a specific slot is usually not important.
        An example is to check how many fingers there are when making a click:
            PhysicalClickValidator('== 0', fingers=2)
      . When specifying 'slot', e.g., slot=0, the purpose is pass the slot
        number to the validator to examine detailed events in that slot.
        An example of such usage:
            LinearityValidator('<= 0.03, ~ +0.07', slot=0)
'''


import copy
import numpy as np
import os
import re
import sys

import firmware_log
import fuzzy
import mtb

from collections import namedtuple, OrderedDict
from inspect import isfunction

from common_util import print_and_exit, simple_system_output
from firmware_constants import AXIS, GV, MTB, UNIT, VAL
from geometry.elements import Point

from linux_input import EV_ABS, EV_STRINGS


# Define the ratio of points taken at both ends of a line for edge tests.
END_PERCENTAGE = 0.1

# Define other constants below.
VALIDATOR = 'Validator'


def validate(packets, gesture, variation):
    """Validate a single gesture."""
    def _validate(validator, msg_list, score_list, vlogs):
        vlog = validator.check(packets, variation)
        if vlog is None:
            return False
        vlogs.append(copy.deepcopy(vlog))
        score = vlog.score
        if score is not None:
            score_list.append(score)
            # save the validator messages
            msg_validator_name = '%s' % vlog.name
            msg_criteria = '    criteria_str: %s' % vlog.criteria
            msg_score = 'score: %f' % score
            msg_list.append(os.linesep)
            msg_list.append(msg_validator_name)
            msg_list += vlog.details
            msg_list.append(msg_criteria)
            msg_list.append(msg_score)
        return score == 1.0

    if packets is None:
        return (None, None)

    msg_list = []
    score_list = []
    vlogs = []
    prerequisite_flag = True

    # If MtbSanityValidator does not pass, there exist some
    # critical problems which will be reported in its metrics.
    # No need to check the other validators.
    mtb_sanity_result = _validate(gesture.mtb_sanity_validator,
                                  msg_list, score_list, vlogs)
    if mtb_sanity_result:
        for validator in gesture.validators:
            _validate(validator, msg_list, score_list, vlogs)

    return (score_list, msg_list, vlogs)


def get_parent_validators(validator_name):
    """Get the parents of a given validator."""
    validator = getattr(sys.modules[__name__], validator_name, None)
    return validator.__bases__ if validator else []


def get_short_name(validator_name):
    """Get the short name of the validator.

    E.g, the short name of LinearityValidator is Linearity.
    """
    return validator_name.split(VALIDATOR)[0]


def get_validator_name(short_name):
    """Convert the short_name to its corresponding validator name.

    E.g, the validator_name of Linearity is LinearityValidator.
    """
    return short_name + VALIDATOR


def get_base_name_and_segment(validator_name):
    """Get the base name and segment of a validator.

    Examples:
        Ex 1: Linearity(BothEnds)Validator
            return ('Linearity', 'BothEnds')
        Ex 2: NoGapValidator
            return ('NoGap', None)
    """
    if '(' in validator_name:
        result = re.search('(.*)\((.*)\)%s' % VALIDATOR, validator_name)
        return (result.group(1), result.group(2))
    else:
        return (get_short_name(validator_name), None)


def get_derived_name(validator_name, segment):
    """Get the derived name based on segment value.

    Example:
      validator_name: LinearityValidator
      segment: Middle
      derived_name: Linearity(Middle)Validator
    """
    short_name = get_short_name(validator_name)
    derived_name = '%s(%s)%s' % (short_name, segment, VALIDATOR)
    return derived_name


def init_base_validator(device):
    """Initialize the device for all the Validators to use"""
    BaseValidator._device = device


class BaseValidator(object):
    """Base class of validators."""
    aggregator = 'fuzzy.average'
    _device = None

    def __init__(self, criteria, mf=None, device=None, name=None):
        self.criteria_str = criteria() if isfunction(criteria) else criteria
        self.fc = fuzzy.FuzzyCriteria(self.criteria_str, mf=mf)
        self.device = device if device else BaseValidator._device
        self.packets = None
        self.vlog = firmware_log.ValidatorLog()
        self.vlog.name = name
        self.vlog.criteria = self.criteria_str
        self.mnprops = firmware_log.MetricNameProps()

    def init_check(self, packets=None):
        """Initialization before check() is called."""
        self.packets = mtb.Mtb(device=self.device, packets=packets)
        self.vlog.reset()

    def _is_direction_in_variation(self, variation, directions):
        """Is any element of directions list found in variation?"""
        for direction in directions:
            if direction in variation:
                return True
        return False

    def is_horizontal(self, variation):
        """Is the direction horizontal?"""
        return self._is_direction_in_variation(variation,
                                               GV.HORIZONTAL_DIRECTIONS)

    def is_vertical(self, variation):
        """Is the direction vertical?"""
        return self._is_direction_in_variation(variation,
                                               GV.VERTICAL_DIRECTIONS)

    def is_diagonal(self, variation):
        """Is the direction diagonal?"""
        return self._is_direction_in_variation(variation,
                                               GV.DIAGONAL_DIRECTIONS)

    def get_direction(self, variation):
        """Get the direction."""
        # TODO(josephsih): raise an exception if a proper direction is not found
        if self.is_horizontal(variation):
            return GV.HORIZONTAL
        elif self.is_vertical(variation):
            return GV.VERTICAL
        elif self.is_diagonal(variation):
            return GV.DIAGONAL

    def get_direction_in_variation(self, variation):
        """Get the direction string from the variation list."""
        if isinstance(variation, tuple):
            for var in variation:
                if var in GV.GESTURE_DIRECTIONS:
                    return var
        elif variation in GV.GESTURE_DIRECTIONS:
            return variation
        return None

    def log_details(self, msg):
        """Collect the detailed messages to be printed within this module."""
        prefix_space = ' ' * 4
        formatted_msg = '%s%s' % (prefix_space, msg)
        self.vlog.insert_details(formatted_msg)

    def get_threshold(self, criteria_str, op):
        """Search the criteria_str using regular expressions and get
        the threshold value.

        @param criteria_str: the criteria string to search
        """
        # In the search pattern, '.*?' is non-greedy, which will match as
        # few characters as possible.
        #   E.g., op = '>'
        #         criteria_str = '>= 200, ~ -100'
        #         pattern below would be '>.*?\s*(\d+)'
        #         result.group(1) below would be '200'
        pattern = '{}.*?\s*(\d+)'.format(op)
        result = re.search(pattern, criteria_str)
        return int(result.group(1)) if result else None

    def _get_axes_by_finger(self, finger):
        """Get list_x, list_y, and list_t for the specified finger.

        @param finger: the finger contact
        """
        points = self.packets.get_ordered_finger_path(self.finger, 'point')
        list_x = [p.x for p in points]
        list_y = [p.y for p in points]
        list_t = self.packets.get_ordered_finger_path(self.finger, 'syn_time')
        return (list_x, list_y, list_t)


class DragLatencyValidator(BaseValidator):
    """ Validator to make check the touchpad's latency

    This is used in conjunction with a Quickstep latency measuring device. To
    compute the latencies, this validator imports the Quickstep software in the
    touchbot project and pulls the data from the Quickstep device and the
    packets collected by mtplot.  If there is no device plugged in, the
    validator will fail with an obviously erroneous value
    """
    def __init__(self, criteria_str, mf=None):
        name = self.__class__.__name__
        super(DragLatencyValidator, self).__init__(criteria_str, mf=mf,
                                                   name=name)

    def check(self, packets, variation=None):
        from quickstep import latency_measurement
        self.init_check(packets)

        # Reformat the touch events for latency measurement
        points = self.packets.get_ordered_finger_path(0, 'point')
        times = self.packets.get_ordered_finger_path(0, 'syn_time')
        finger_positions = [latency_measurement.FingerPosition(t, pt.x, pt.y)
                            for t, pt in zip(times, points)]

        # Find the sysfs entries for the Quickstep device and parse the logs
        laser_files = simple_system_output('find / -name laser')
        laser_crossings = []
        for f in laser_files.splitlines():
            laser_crossings = latency_measurement.get_laser_crossings(f)
            if laser_crossings:
                break

        # Crunch the numbers using the Quickstep latency measurement module
        latencies = latency_measurement.measure_latencies(finger_positions,
                                                          laser_crossings)
        # If there is no Quickstep plugged in, there will be no readings, so
        # to keep the test suite from crashing insert a dummy value
        if not latencies:
            latencies = [9.999]

        avg = 1000.0 * sum(latencies) / len(latencies)
        self.vlog.score = self.fc.mf.grade(avg)
        self.log_details('Average drag latency (ms): %f' % avg)
        self.log_details('Max drag latency (ms): %f' % (1000 * max(latencies)))
        self.log_details('Min drag latency (ms): %f' % (1000 * min(latencies)))
        self.vlog.metrics = [firmware_log.Metric(self.mnprops.AVG_LATENCY, avg)]
        return self.vlog


class DiscardInitialSecondsValidator(BaseValidator):
    """ Takes in another validator and validates
    all the data after the intial number of seconds specified
    """
    def __init__(self, validator, mf=None, device=None,
                 initial_seconds_to_discard=1):
        self.validator = validator
        self.initial_seconds_to_discard = initial_seconds_to_discard
        super(DiscardInitialSecondsValidator, self).__init__(
            validator.criteria_str, mf, device, validator.__class__.__name__)

    def _discard_initial_seconds(self, packets, seconds_to_discard):
        # Get the list of syn_time of all packets
        list_syn_time = self.packets.get_list_syn_time(None)

        # Get the time to cut the list at. list_syn_time is in seconds.
        cutoff_time = list_syn_time[0] + self.initial_seconds_to_discard

        # Find the index at which the list of timestamps is greater than
        # the cutoff time.
        cutoff_index = None
        for index, time in enumerate(list_syn_time):
            if time >= cutoff_time:
                cutoff_index = index
                break

        if not cutoff_index:
            return None

        # Create a packet representing the final state of the touchpad
        # at the end of the discarded seconds
        final_state_packet = mtb.create_final_state_packet(
            packets[:cutoff_index])
        if final_state_packet:
            return [final_state_packet] + packets[cutoff_index:]
        else:
            # If final_state_packet is [] which means all fingers have left
            # at this time instant, just exclude this empty packet.
            return packets[cutoff_index:]

    def check(self, packets, variation=None):
        self.init_check(packets)
        packets = self._discard_initial_seconds(packets,
                                                self.initial_seconds_to_discard)
        if packets:
            return self.validator.check(packets, variation)
        else:
            print ('ERROR: The length of the test is '
                   'less than %d second(s) long.' %
                   self.initial_seconds_to_discard)


class LinearityValidator1(BaseValidator):
    """Validator to verify linearity.

    Example:
        To check the linearity of the line drawn in finger 1:
          LinearityValidator1('<= 0.03, ~ +0.07', finger=1)
    """
    # Define the partial group size for calculating Mean Squared Error
    MSE_PARTIAL_GROUP_SIZE = 1

    def __init__(self, criteria_str, mf=None, device=None, finger=0,
                 segments=VAL.WHOLE):
        self._segments = segments
        self.finger = finger
        name = get_derived_name(self.__class__.__name__, segments)
        super(LinearityValidator1, self).__init__(criteria_str, mf, device,
                                                  name)

    def _simple_linear_regression(self, ax, ay):
        """Calculate the simple linear regression and returns the
           sum of squared residuals.

        It calculates the simple linear regression line for the points
        in the middle segment of the line. This exclude the points at
        both ends of the line which sometimes have wobbles. Then it
        calculates the fitting errors of the points at the specified segments
        against the computed simple linear regression line.
        """
        # Compute the simple linear regression line for the middle segment
        # whose purpose is to avoid wobbles on both ends of the line.
        mid_segment = self.packets.get_segments_x_and_y(ax, ay, VAL.MIDDLE,
                                                        END_PERCENTAGE)
        if not self._calc_simple_linear_regression_line(*mid_segment):
            return 0

        # Compute the fitting errors of the specified segments.
        if self._segments == VAL.BOTH_ENDS:
            bgn_segment = self.packets.get_segments_x_and_y(ax, ay, VAL.BEGIN,
                                                            END_PERCENTAGE)
            end_segment = self.packets.get_segments_x_and_y(ax, ay, VAL.END,
                                                            END_PERCENTAGE)
            bgn_error = self._calc_simple_linear_regression_error(*bgn_segment)
            end_error = self._calc_simple_linear_regression_error(*end_segment)
            return max(bgn_error, end_error)
        else:
            target_segment = self.packets.get_segments_x_and_y(ax, ay,
                    self._segments, END_PERCENTAGE)
            return self._calc_simple_linear_regression_error(*target_segment)

    def _calc_simple_linear_regression_line(self, ax, ay):
        """Calculate the simple linear regression line.

           ax: array x
           ay: array y
           This method tries to find alpha and beta in the formula
                ay = alpha + beta . ax
           such that it has the least sum of squared residuals.

           Reference:
           - Simple linear regression:
             http://en.wikipedia.org/wiki/Simple_linear_regression
           - Average absolute deviation (or mean absolute deviation) :
             http://en.wikipedia.org/wiki/Average_absolute_deviation
        """
        # Convert the int list to the float array
        self._ax = 1.0 * np.array(ax)
        self._ay = 1.0 * np.array(ay)

        # If there are less than 2 data points, it is not a line at all.
        asize = self._ax.size
        if asize <= 2:
            return False

        Sx = self._ax.sum()
        Sy = self._ay.sum()
        Sxx = np.square(self._ax).sum()
        Sxy = np.dot(self._ax, self._ay)
        Syy = np.square(self._ay).sum()
        Sx2 = Sx * Sx
        Sy2 = Sy * Sy

        # compute Mean of x and y
        Mx = self._ax.mean()
        My = self._ay.mean()

        # Compute beta and alpha of the linear regression
        self._beta = 1.0 * (asize * Sxy - Sx * Sy) / (asize * Sxx - Sx2)
        self._alpha = My - self._beta * Mx
        return True

    def _calc_simple_linear_regression_error(self, ax, ay):
        """Calculate the fitting error based on the simple linear regression
        line characterized by the equation parameters alpha and beta.
        """
        # Convert the int list to the float array
        ax = 1.0 * np.array(ax)
        ay = 1.0 * np.array(ay)

        asize = ax.size
        partial = min(asize, max(1, self.MSE_PARTIAL_GROUP_SIZE))

        # spmse: squared root of partial mean squared error
        spmse = np.square(ay - self._alpha - self._beta * ax)
        spmse.sort()
        spmse = spmse[asize - partial : asize]
        spmse = np.sqrt(np.average(spmse))
        return spmse

    def check(self, packets, variation=None):
        """Check if the packets conforms to specified criteria."""
        self.init_check(packets)
        resolution_x, resolution_y = self.device.get_resolutions()
        (list_x, list_y) = self.packets.get_x_y(self.finger)
        # Compute average distance (fitting error) in pixels, and
        # average deviation on touch device in mm.
        if self.is_vertical(variation):
            ave_distance = self._simple_linear_regression(list_y, list_x)
            deviation = ave_distance / resolution_x
        else:
            ave_distance = self._simple_linear_regression(list_x, list_y)
            deviation = ave_distance / resolution_y

        self.log_details('ave fitting error: %.2f px' % ave_distance)
        msg_device = 'deviation finger%d: %.2f mm'
        self.log_details(msg_device % (self.finger, deviation))
        self.vlog.score = self.fc.mf.grade(deviation)
        return self.vlog


class LinearityValidator(BaseValidator):
    """A validator to verify linearity based on x-t and y-t

    Example:
        To check the linearity of the line drawn in finger 1:
          LinearityValidator('<= 0.03, ~ +0.07', finger=1)
        Note: the finger number begins from 0
    """
    # Define the partial group size for calculating Mean Squared Error
    MSE_PARTIAL_GROUP_SIZE = 1

    def __init__(self, criteria_str, mf=None, device=None, finger=0,
                 segments=VAL.WHOLE):
        self._segments = segments
        self.finger = finger
        name = get_derived_name(self.__class__.__name__, segments)
        super(LinearityValidator, self).__init__(criteria_str, mf, device,
                                                  name)

    def _calc_residuals(self, line, list_t, list_y):
        """Calculate the residuals of the points in list_t, list_y against
        the line.

        @param line: the regression line of list_t and list_y
        @param list_t: a list of time instants
        @param list_y: a list of x/y coordinates

        This method returns the list of residuals, where
            residual[i] = line[t_i] - y_i
        where t_i is an element in list_t and
              y_i is a corresponding element in list_y.

        We calculate the vertical distance (y distance) here because the
        horizontal axis, list_t, always represent the time instants, and the
        vertical axis, list_y, could be either the coordinates in x or y axis.
        """
        return [float(line(t) - y) for t, y in zip(list_t, list_y)]

    def _do_simple_linear_regression(self, list_t, list_y):
        """Calculate the simple linear regression line and returns the
        sum of squared residuals.

        @param list_t: the list of time instants
        @param list_y: the list of x or y coordinates of touch contacts

        It calculates the residuals (fitting errors) of the points at the
        specified segments against the computed simple linear regression line.

        Reference:
        - Simple linear regression:
          http://en.wikipedia.org/wiki/Simple_linear_regression
        - numpy.polyfit(): used to calculate the simple linear regression line.
          http://docs.scipy.org/doc/numpy/reference/generated/numpy.polyfit.html
        """
        # At least 2 points to determine a line.
        if len(list_t) < 2 or len(list_y) < 2:
            return []

        mid_segment_t, mid_segment_y = self.packets.get_segments(
                list_t, list_y, VAL.MIDDLE, END_PERCENTAGE)

        # Check to make sure there are enough samples to continue
        if len(mid_segment_t) <= 2 or len(mid_segment_y) <= 2:
            return []

        # Calculate the simple linear regression line.
        degree = 1
        regress_line = np.poly1d(np.polyfit(mid_segment_t, mid_segment_y,
                                            degree))

        # Compute the fitting errors of the specified segments.
        if self._segments == VAL.BOTH_ENDS:
            begin_segments = self.packets.get_segments(
                    list_t, list_y, VAL.BEGIN, END_PERCENTAGE)
            end_segments = self.packets.get_segments(
                    list_t, list_y, VAL.END, END_PERCENTAGE)
            begin_error = self._calc_residuals(regress_line, *begin_segments)
            end_error = self._calc_residuals(regress_line, *end_segments)
            return begin_error + end_error
        else:
            target_segments = self.packets.get_segments(
                    list_t, list_y, self._segments, END_PERCENTAGE)
            return self._calc_residuals(regress_line, *target_segments)

    def _calc_errors_single_axis(self, list_t, list_y):
        """Calculate various errors for axis-time.

        @param list_t: the list of time instants
        @param list_y: the list of x or y coordinates of touch contacts
        """
        # It is fine if axis-time is a horizontal line.
        errors_px = self._do_simple_linear_regression(list_t, list_y)
        if not errors_px:
            return (0, 0)

        # Calculate the max errors
        max_err_px = max(map(abs, errors_px))

        # Calculate the root mean square errors
        e2 = [e * e for e in errors_px]
        rms_err_px = (float(sum(e2)) / len(e2)) ** 0.5

        return (max_err_px, rms_err_px)

    def _calc_errors_all_axes(self, list_t, list_x, list_y):
        """Calculate various errors for all axes."""
        # Calculate max error and average squared error
        (max_err_x_px, rms_err_x_px) = self._calc_errors_single_axis(
                list_t, list_x)
        (max_err_y_px, rms_err_y_px) = self._calc_errors_single_axis(
                list_t, list_y)

        # Convert the unit from pixels to mms
        self.max_err_x_mm, self.max_err_y_mm = self.device.pixel_to_mm(
                (max_err_x_px, max_err_y_px))
        self.rms_err_x_mm, self.rms_err_y_mm = self.device.pixel_to_mm(
                (rms_err_x_px, rms_err_y_px))

    def _log_details_and_metrics(self, variation):
        """Log the details and calculate the metrics.

        @param variation: the gesture variation
        """
        list_x, list_y, list_t = self._get_axes_by_finger(self.finger)
        X, Y = AXIS.LIST
        # For horizontal lines, only consider x axis
        if self.is_horizontal(variation):
            self.list_coords = {X: list_x}
        # For vertical lines, only consider y axis
        elif self.is_vertical(variation):
            self.list_coords = {Y: list_y}
        # For diagonal lines, consider both x and y axes
        elif self.is_diagonal(variation):
            self.list_coords = {X: list_x, Y: list_y}

        self.max_err_mm = {}
        self.rms_err_mm = {}
        self.vlog.metrics = []
        mnprops = self.mnprops
        pixel_to_mm = self.device.pixel_to_mm_single_axis_by_name
        for axis, list_c in self.list_coords.items():
            max_err_px, rms_err_px = self._calc_errors_single_axis(
                    list_t, list_c)
            max_err_mm = pixel_to_mm(max_err_px, axis)
            rms_err_mm = pixel_to_mm(rms_err_px, axis)
            self.log_details('max_err[%s]: %.2f mm' % (axis, max_err_mm))
            self.log_details('rms_err[%s]: %.2f mm' % (axis, rms_err_mm))
            self.vlog.metrics.extend([
                firmware_log.Metric(mnprops.MAX_ERR.format(axis), max_err_mm),
                firmware_log.Metric(mnprops.RMS_ERR.format(axis), rms_err_mm),
            ])
            self.max_err_mm[axis] = max_err_mm
            self.rms_err_mm[axis] = rms_err_mm

    def check(self, packets, variation=None):
        """Check if the packets conforms to specified criteria."""
        self.init_check(packets)
        self._log_details_and_metrics(variation)
        # Calculate the score based on the max error
        max_err = max(self.max_err_mm.values())
        self.vlog.score = self.fc.mf.grade(max_err)
        return self.vlog


class LinearityNormalFingerValidator(LinearityValidator):
    """A dummy LinearityValidator to verify linearity for gestures performed
    with normal fingers.
    """
    pass


class LinearityFatFingerValidator(LinearityValidator):
    """A dummy LinearityValidator to verify linearity for gestures performed
    with fat fingers or thumb edge.
    """
    pass


class RangeValidator(BaseValidator):
    """Validator to check the observed (x, y) positions should be within
    the range of reported min/max values.

    Example:
        To check the range of observed edge-to-edge positions:
          RangeValidator('<= 0.05, ~ +0.05')
    """

    def __init__(self, criteria_str, mf=None, device=None):
        self.name = self.__class__.__name__
        super(RangeValidator, self).__init__(criteria_str, mf, device,
                                             self.name)

    def check(self, packets, variation=None):
        """Check the left/right or top/bottom range based on the direction."""
        self.init_check(packets)
        valid_directions = [GV.CL, GV.CR, GV.CT, GV.CB]
        Range = namedtuple('Range', valid_directions)
        actual_range = Range(*self.packets.get_range())
        spec_range = Range(self.device.axis_x.min, self.device.axis_x.max,
                           self.device.axis_y.min, self.device.axis_y.max)

        direction = self.get_direction_in_variation(variation)
        if direction in valid_directions:
            actual_edge = getattr(actual_range, direction)
            spec_edge = getattr(spec_range, direction)
            short_of_range_px = abs(actual_edge - spec_edge)
        else:
            err_msg = 'Error: the gesture variation %s is not allowed in %s.'
            print_and_exit(err_msg % (variation, self.name))

        axis_spec = (self.device.axis_x if self.is_horizontal(variation)
                                        else self.device.axis_y)
        deviation_ratio = (float(short_of_range_px) /
                           (axis_spec.max - axis_spec.min))
        # Convert the direction to edge name.
        #   E.g., direction: center_to_left
        #         edge name: left
        edge_name = direction.split('_')[-1]
        metric_name = self.mnprops.RANGE.format(edge_name)
        short_of_range_mm = self.device.pixel_to_mm_single_axis(
                short_of_range_px, axis_spec)
        self.vlog.metrics = [
            firmware_log.Metric(metric_name, short_of_range_mm)
        ]
        self.log_details('actual: px %s' % str(actual_edge))
        self.log_details('spec: px %s' % str(spec_edge))
        self.log_details('short of range: %d px == %f mm' %
                         (short_of_range_px, short_of_range_mm))
        self.vlog.score = self.fc.mf.grade(deviation_ratio)
        return self.vlog


class CountTrackingIDValidator(BaseValidator):
    """Validator to check the count of tracking IDs.

    Example:
        To verify if there is exactly one finger observed:
          CountTrackingIDValidator('== 1')
    """

    def __init__(self, criteria_str, mf=None, device=None):
        name = self.__class__.__name__
        super(CountTrackingIDValidator, self).__init__(criteria_str, mf,
                                                       device, name)

    def check(self, packets, variation=None):
        """Check the number of tracking IDs observed."""
        self.init_check(packets)

        # Get the actual count of tracking id and log the details.
        actual_count_tid = self.packets.get_number_contacts()
        self.log_details('count of trackid IDs: %d' % actual_count_tid)

        # Only keep metrics with the criteria '== N'.
        # Ignore those with '>= N' which are used to assert that users have
        # performed correct gestures. As an example, we require that users
        # tap more than a certain number of times in the drumroll test.
        if '==' in self.criteria_str:
            expected_count_tid = int(self.criteria_str.split('==')[-1].strip())
            # E.g., expected_count_tid = 2
            #       actual_count_tid could be either smaller (e.g., 1) or
            #       larger (e.g., 3).
            metric_value = (actual_count_tid, expected_count_tid)
            metric_name = self.mnprops.TID
            self.vlog.metrics = [firmware_log.Metric(metric_name, metric_value)]

        self.vlog.score = self.fc.mf.grade(actual_count_tid)
        return self.vlog


class CountTrackingIDNormalFingerValidator(CountTrackingIDValidator):
    """A dummy CountTrackingIDValidator to collect data for
    normal finger gestures.
    """
    pass


class CountTrackingIDFatFingerValidator(CountTrackingIDValidator):
    """A dummy CountTrackingIDValidator to collect data for fat finger gestures.
    """
    pass


class StationaryValidator(BaseValidator):
    """Check to make sure a finger we expect to remain still doesn't move.

    This class is inherited by both StationaryFingerValidator and
    StationaryTapValidator, and is not used directly as a validator.
    """

    def __init__(self, criteria, mf=None, device=None, slot=0):
        name = self.__class__.__name__
        super(StationaryValidator, self).__init__(criteria, mf, device, name)
        self.slot = slot

    def check(self, packets, variation=None):
        """Check the moving distance of the specified slot."""
        self.init_check(packets)
        max_distance = self.packets.get_max_distance(self.slot, UNIT.MM)
        msg = 'Max distance slot%d: %.2f mm'
        self.log_details(msg % (self.slot, max_distance))
        self.vlog.metrics = [
            firmware_log.Metric(self.mnprops.MAX_DISTANCE, max_distance)
        ]
        self.vlog.score = self.fc.mf.grade(max_distance)
        return self.vlog


class StationaryFingerValidator(StationaryValidator):
    """A dummy StationaryValidator to check pulling effect by another finger.

    Example:
        To verify if the stationary finger specified by the slot is not
        pulled away more than 1.0 mm by another finger.
          StationaryFingerValidator('<= 1.0')
    """
    pass


class StationaryTapValidator(StationaryValidator):
    """A dummy StationaryValidator to check the wobble of tap/click.

    Example:
        To verify if the tapping finger specified by the slot does not
        wobble larger than 1.0 mm.
          StationaryTapValidator('<= 1.0')
    """
    pass


class NoGapValidator(BaseValidator):
    """Validator to make sure that there are no significant gaps in a line.

    Example:
        To verify if there is exactly one finger observed:
          NoGapValidator('<= 5, ~ +5', slot=1)
    """

    def __init__(self, criteria_str, mf=None, device=None, slot=0):
        name = self.__class__.__name__
        super(NoGapValidator, self).__init__(criteria_str, mf, device, name)
        self.slot = slot

    def check(self, packets, variation=None):
        """There should be no significant gaps in a line."""
        self.init_check(packets)
        # Get the largest gap ratio
        gap_ratio = self.packets.get_largest_gap_ratio(self.slot)
        msg = 'Largest gap ratio slot%d: %f'
        self.log_details(msg % (self.slot, gap_ratio))
        self.vlog.score = self.fc.mf.grade(gap_ratio)
        return self.vlog


class NoReversedMotionValidator(BaseValidator):
    """Validator to measure the reversed motions in the specified slots.

    Example:
        To measure the reversed motions in slot 0:
          NoReversedMotionValidator('== 0, ~ +20', slots=0)
    """
    def __init__(self, criteria_str, mf=None, device=None, slots=(0,),
                 segments=VAL.MIDDLE):
        self._segments = segments
        name = get_derived_name(self.__class__.__name__, segments)
        self.slots = (slots,) if isinstance(slots, int) else slots
        parent = super(NoReversedMotionValidator, self)
        parent.__init__(criteria_str, mf, device, name)

    def _get_reversed_motions(self, slot, direction):
        """Get the reversed motions opposed to the direction in the slot."""
        return self.packets.get_reversed_motions(slot,
                                                 direction,
                                                 segment_flag=self._segments,
                                                 ratio=END_PERCENTAGE)

    def check(self, packets, variation=None):
        """There should be no reversed motions in a slot."""
        self.init_check(packets)
        sum_reversed_motions = 0
        direction = self.get_direction_in_variation(variation)
        for slot in self.slots:
            # Get the reversed motions.
            reversed_motions = self._get_reversed_motions(slot, direction)
            msg = 'Reversed motions slot%d: %s px'
            self.log_details(msg % (slot, reversed_motions))
            sum_reversed_motions += sum(map(abs, reversed_motions.values()))
        self.vlog.score = self.fc.mf.grade(sum_reversed_motions)
        return self.vlog


class CountPacketsValidator(BaseValidator):
    """Validator to check the number of packets.

    Example:
        To verify if there are enough packets received about the first finger:
          CountPacketsValidator('>= 3, ~ -3', slot=0)
    """

    def __init__(self, criteria_str, mf=None, device=None, slot=0):
        self.name = self.__class__.__name__
        super(CountPacketsValidator, self).__init__(criteria_str, mf, device,
                                                    self.name)
        self.slot = slot

    def check(self, packets, variation=None):
        """Check the number of packets in the specified slot."""
        self.init_check(packets)
        # Get the number of packets in that slot
        actual_count_packets = self.packets.get_num_packets(self.slot)
        msg = 'Number of packets slot%d: %s'
        self.log_details(msg % (self.slot, actual_count_packets))

        # Add the metric for the count of packets
        expected_count_packets = self.get_threshold(self.criteria_str, '>')
        assert expected_count_packets, 'Check the criteria of %s' % self.name
        metric_value = (actual_count_packets, expected_count_packets)
        metric_name = self.mnprops.COUNT_PACKETS
        self.vlog.metrics = [firmware_log.Metric(metric_name, metric_value)]

        self.vlog.score = self.fc.mf.grade(actual_count_packets)
        return self.vlog


class PinchValidator(BaseValidator):
    """Validator to check the pinch to zoom in/out.

    Example:
        To verify that the two fingers are drawing closer:
          PinchValidator('>= 200, ~ -100')
    """

    def __init__(self, criteria_str, mf=None, device=None):
        self.name = self.__class__.__name__
        super(PinchValidator, self).__init__(criteria_str, mf, device,
                                             self.name)

    def check(self, packets, variation):
        """Check the number of packets in the specified slot."""
        self.init_check(packets)
        # Get the relative motion of the two fingers
        slots = (0, 1)
        actual_relative_motion = self.packets.get_relative_motion(slots)
        if variation == GV.ZOOM_OUT:
            actual_relative_motion = -actual_relative_motion
        msg = 'Relative motions of the two fingers: %.2f px'
        self.log_details(msg % actual_relative_motion)

        # Add the metric for relative motion distance.
        expected_relative_motion = self.get_threshold(self.criteria_str, '>')
        assert expected_relative_motion, 'Check the criteria of %s' % self.name
        metric_value = (actual_relative_motion, expected_relative_motion)
        metric_name = self.mnprops.PINCH
        self.vlog.metrics = [firmware_log.Metric(metric_name, metric_value)]

        self.vlog.score = self.fc.mf.grade(actual_relative_motion)
        return self.vlog


class PhysicalClickValidator(BaseValidator):
    """Validator to check the events generated by physical clicks

    Example:
        To verify the events generated by a one-finger physical click
          PhysicalClickValidator('== 1', fingers=1)
    """

    def __init__(self, criteria_str, fingers, mf=None, device=None):
        self.criteria_str = criteria_str
        self.name = self.__class__.__name__
        super(PhysicalClickValidator, self).__init__(criteria_str, mf, device,
                                                     self.name)
        self.fingers = fingers

    def _get_expected_number(self):
        """Get the expected number of counts from the criteria string.

        E.g., criteria_str: '== 1'
        """
        try:
            expected_count = int(self.criteria_str.split('==')[-1].strip())
        except Exception, e:
            print 'Error: %s in the criteria string of %s' % (e, self.name)
            exit(1)
        return expected_count

    def _add_metrics(self):
        """Add metrics"""
        fingers = self.fingers
        raw_click_count = self.packets.get_raw_physical_clicks()

        # This is for the metric:
        #   "of the n clicks, the % of clicks with the correct finger IDs"
        correct_click_count = self.packets.get_correct_physical_clicks(fingers)
        value_with_TIDs = (correct_click_count, raw_click_count)
        name_with_TIDs = self.mnprops.CLICK_CHECK_TIDS.format(self.fingers)

        # This is for the metric: "% of finger IDs with a click"
        expected_click_count = self._get_expected_number()
        value_clicks = (raw_click_count, expected_click_count)
        name_clicks = self.mnprops.CLICK_CHECK_CLICK.format(self.fingers)

        self.vlog.metrics = [
            firmware_log.Metric(name_with_TIDs, value_with_TIDs),
            firmware_log.Metric(name_clicks, value_clicks),
        ]

        return value_with_TIDs

    def check(self, packets, variation=None):
        """Check the number of packets in the specified slot."""
        self.init_check(packets)
        correct_click_count, raw_click_count = self._add_metrics()
        # Get the number of physical clicks made with the specified number
        # of fingers.
        msg = 'Count of %d-finger physical clicks: %s'
        self.log_details(msg % (self.fingers, correct_click_count))
        self.log_details('Count of physical clicks: %d' % raw_click_count)
        self.vlog.score = self.fc.mf.grade(correct_click_count)
        return self.vlog


class DrumrollValidator(BaseValidator):
    """Validator to check the drumroll problem.

    All points from the same finger should be within 2 circles of radius X mm
    (e.g. 2 mm)

    Example:
        To verify that the max radius of all minimal enclosing circles generated
        by alternately tapping the index and middle fingers is within 2.0 mm.
          DrumrollValidator('<= 2.0')
    """

    def __init__(self, criteria_str, mf=None, device=None):
        name = self.__class__.__name__
        super(DrumrollValidator, self).__init__(criteria_str, mf, device, name)

    def check(self, packets, variation=None):
        """The moving distance of the points in any tracking ID should be
        within the specified value.
        """
        self.init_check(packets)
        # For each tracking ID, compute the minimal enclosing circles,
        #     rocs = (radius_of_circle1, radius_of_circle2)
        # Return a list of such minimal enclosing circles of all tracking IDs.
        rocs = self.packets.get_list_of_rocs_of_all_tracking_ids()
        max_radius = max(rocs)
        self.log_details('Max radius: %.2f mm' % max_radius)
        metric_name = self.mnprops.CIRCLE_RADIUS
        self.vlog.metrics = [firmware_log.Metric(metric_name, roc)
                             for roc in rocs]
        self.vlog.score = self.fc.mf.grade(max_radius)
        return self.vlog


class NoLevelJumpValidator(BaseValidator):
    """Validator to check if there are level jumps

    When a user draws a horizontal line with thumb edge or a fat finger,
    the line could comprise a horizontal line segment followed by another
    horizontal line segment (or just dots) one level up or down, and then
    another horizontal line segment again at different horizontal level, etc.
    This validator is implemented to detect such level jumps.

    Such level jumps could also occur when drawing vertical or diagonal lines.

    Example:
        To verify the level jumps in a one-finger tracking gesture:
          NoLevelJumpValidator('<= 10, ~ +30', slots[0,])
        where slots[0,] represent the slots with numbers larger than slot 0.
        This kind of representation is required because when the thumb edge or
        a fat finger is used, due to the difficulty in handling it correctly
        in the touch device firmware, the tracking IDs and slot IDs may keep
        changing. We would like to analyze all such slots.
    """

    def __init__(self, criteria_str, mf=None, device=None, slots=0):
        name = self.__class__.__name__
        super(NoLevelJumpValidator, self).__init__(criteria_str, mf, device,
                                                   name)
        self.slots = slots

    def check(self, packets, variation=None):
        """Check if there are level jumps."""
        self.init_check(packets)
        # Get the displacements of the slots.
        slots = self.slots[0]
        displacements = self.packets.get_displacements_for_slots(slots)

        # Iterate through the collected tracking IDs
        jumps = []
        for tid in displacements:
            slot = displacements[tid][MTB.SLOT]
            for axis in AXIS.LIST:
                disp = displacements[tid][axis]
                jump = self.packets.get_largest_accumulated_level_jumps(disp)
                jumps.append(jump)
                msg = '  accu jump (%d %s): %d px'
                self.log_details(msg % (slot, axis, jump))

        # Get the largest accumulated level jump
        max_jump = max(jumps) if jumps else 0
        msg = 'Max accu jump: %d px'
        self.log_details(msg % (max_jump))
        self.vlog.score = self.fc.mf.grade(max_jump)
        return self.vlog


class ReportRateValidator(BaseValidator):
    """Validator to check the report rate.

    Example:
        To verify that the report rate is around 80 Hz. It gets 0 points
        if the report rate drops below 60 Hz.
          ReportRateValidator('== 80 ~ -20')
    """

    def __init__(self, criteria_str, finger=None, mf=None, device=None,
                 chop_off_pauses=True):
        """Initialize ReportRateValidator

        @param criteria_str: the criteria string
        @param finger: the ith contact if not None. When set to None, it means
                to examine all packets.
        @param mf: the fuzzy member function to use
        @param device: the touch device
        """
        self.name = self.__class__.__name__
        self.criteria_str = criteria_str
        self.finger = finger
        if finger is not None:
            msg = '%s: finger = %d (It is required that finger >= 0.)'
            assert finger >= 0, msg % (self.name, finger)
        self.chop_off_pauses = chop_off_pauses
        super(ReportRateValidator, self).__init__(criteria_str, mf, device,
                                                  self.name)

    def _chop_off_both_ends(self, points, distance):
        """Chop off both ends of segments such that the points in the remaining
        middle segment are distant from both ends by more than the specified
        distance.

        When performing a gesture such as finger tracking, it is possible
        that the finger will stay stationary for a while before it actually
        starts moving. Likewise, it is also possible that the finger may stay
        stationary before the finger leaves the touch surface. We would like
        to chop off the stationary segments.

        Note: if distance is 0, the effect is equivalent to keep all points.

        @param points: a list of Points
        @param distance: the distance within which the points are chopped off
        """
        def _find_index(points, distance, reversed_flag=False):
            """Find the first index of the point whose distance with the
            first point is larger than the specified distance.

            @param points: a list of Points
            @param distance: the distance
            @param reversed_flag: indicates if the points needs to be reversed
            """
            points_len = len(points)
            if reversed_flag:
                points = reversed(points)

            ref_point = None
            for i, p in enumerate(points):
                if ref_point is None:
                    ref_point = p
                if ref_point.distance(p) >= distance:
                    return (points_len - i - 1) if reversed_flag else i

            return None

        # There must be extra points in addition to the first and the last point
        if len(points) <= 2:
            return None

        begin_moving_index = _find_index(points, distance, reversed_flag=False)
        end_moving_index = _find_index(points, distance, reversed_flag=True)

        if (begin_moving_index is None or end_moving_index is None or
                begin_moving_index > end_moving_index):
            return None
        return [begin_moving_index, end_moving_index]

    def _add_report_rate_metrics2(self):
        """Calculate and add the metrics about report rate.

        Three metrics are required.
        - % of time intervals that are > (1/60) second
        - average time interval
        - max time interval

        """
        import test_conf as conf

        if self.finger:
            finger_list = [self.finger]
        else:
            ordered_finger_paths_dict = self.packets.get_ordered_finger_paths()
            finger_list = range(len(ordered_finger_paths_dict))

        # distance: the minimal moving distance within which the points
        #           at both ends will be chopped off
        distance = conf.MIN_MOVING_DISTANCE if self.chop_off_pauses else 0

        # Derive the middle moving segment in which the finger(s)
        # moves significantly.
        begin_time = float('infinity')
        end_time = float('-infinity')
        for finger in finger_list:
            list_t = self.packets.get_ordered_finger_path(finger, 'syn_time')
            points = self.packets.get_ordered_finger_path(finger, 'point')
            middle = self._chop_off_both_ends(points, distance)
            if middle:
                this_begin_index, this_end_index = middle
                this_begin_time = list_t[this_begin_index]
                this_end_time = list_t[this_end_index]
                begin_time = min(begin_time, this_begin_time)
                end_time = max(end_time, this_end_time)

        if (begin_time == float('infinity') or end_time == float('-infinity')
                or end_time <= begin_time):
            print 'Warning: %s: cannot derive a moving segment.' % self.name
            print 'begin_time: ', begin_time
            print 'end_time: ', end_time
            return

        # Get the list of SYN_REPORT time in the middle moving segment.
        list_syn_time = filter(lambda t: t >= begin_time and t <= end_time,
                               self.packets.get_list_syn_time(self.finger))

        # Each packet consists of a list of events of which The last one is
        # the sync event. The unit of sync_intervals is ms.
        sync_intervals = [1000.0 * (list_syn_time[i + 1] - list_syn_time[i])
                          for i in range(len(list_syn_time) - 1)]

        max_report_interval = conf.max_report_interval

        # Calculate the metrics and add them to vlog.
        long_intervals = [s for s in sync_intervals if s > max_report_interval]
        metric_long_intervals = (len(long_intervals), len(sync_intervals))
        ave_interval = sum(sync_intervals) / len(sync_intervals)
        max_interval = max(sync_intervals)

        name_long_intervals_pct = self.mnprops.LONG_INTERVALS.format(
            '%.2f' % max_report_interval)
        name_ave_time_interval = self.mnprops.AVE_TIME_INTERVAL
        name_max_time_interval = self.mnprops.MAX_TIME_INTERVAL

        self.vlog.metrics = [
            firmware_log.Metric(name_long_intervals_pct, metric_long_intervals),
            firmware_log.Metric(self.mnprops.AVE_TIME_INTERVAL, ave_interval),
            firmware_log.Metric(self.mnprops.MAX_TIME_INTERVAL, max_interval),
        ]

        self.log_details('%s: %f' % (self.mnprops.AVE_TIME_INTERVAL,
                         ave_interval))
        self.log_details('%s: %f' % (self.mnprops.MAX_TIME_INTERVAL,
                         max_interval))
        self.log_details('# long intervals > %s ms: %d' %
                         (self.mnprops.max_report_interval_str,
                          len(long_intervals)))
        self.log_details('# total intervals: %d' % len(sync_intervals))

    def _get_report_rate(self, list_syn_time):
        """Get the report rate in Hz from the list of syn_time.

        @param list_syn_time: a list of SYN_REPORT time instants
        """
        if len(list_syn_time) <= 1:
            return 0
        duration = list_syn_time[-1] - list_syn_time[0]
        num_packets = len(list_syn_time) - 1
        report_rate = float(num_packets) / duration
        return report_rate

    def check(self, packets, variation=None):
        """The Report rate should be within the specified range."""
        self.init_check(packets)
        # Get the list of syn_time based on the specified finger.
        list_syn_time = self.packets.get_list_syn_time(self.finger)
        # Get the report rate
        self.report_rate = self._get_report_rate(list_syn_time)
        self._add_report_rate_metrics2()
        self.vlog.score = self.fc.mf.grade(self.report_rate)
        return self.vlog


class MtbSanityValidator(BaseValidator):
    """Validator to check if the MTB format is correct.

    A ghost finger is a slot with a positive TRACKING ID without a real
    object such as a finger touching the device.

    Note that this object should be instantiated before any finger touching
    the device so that a snapshot could be derived in the very beginning.

    There are potentially many things to check in the MTB format. However,
    this validator will begin with a simple TRACKING ID examination.
    A new slot should come with a positive TRACKING ID before the slot
    can assign values to its attributes or set -1 to its TRACKING ID.
    This is sort of different from a ghost finger case. A ghost finger
    occurs when there exist slots with positive TRACKING IDs in the
    beginning by syncing with the kernel before any finger touching the
    device.

    Note that there is no need for this class to perform
        self.init_check(packets)
    """

    def __init__(self, criteria_str='== 0', mf=None, device=None,
                 device_info=None):
        name = self.__class__.__name__
        super(MtbSanityValidator, self).__init__(criteria_str, mf, device, name)
        if device_info:
            self.device_info = device_info
        else:
            sys.path.append('../../bin/input')
            import input_device
            self.device_info = input_device.InputDevice(self.device.device_node)

    def _check_ghost_fingers(self):
        """Check if there are ghost fingers by synching with the kernel."""
        self.number_fingers = self.device_info.get_num_fingers()
        self.slot_dict = self.device_info.get_slots()

        self.log_details('# fingers: %d' % self.number_fingers)
        for slot_id, slot in self.slot_dict.items():
            self.log_details('slot %d:' % slot_id)
            for prop in slot:
                prop_name = EV_STRINGS[EV_ABS].get(prop, prop)
                self.log_details(' %s=%6d' % (prop_name, slot[prop].value))

        self.vlog.metrics.append(
                firmware_log.Metric(self.mnprops.GHOST_FINGERS,
                                    (self.number_fingers, 0)),
        )
        return self.number_fingers

    def _check_mtb(self, packets):
        """Check if there are MTB format problems."""
        mtb_sanity = mtb.MtbSanity(packets)
        errors = mtb_sanity.check()
        number_errors = sum(errors.values())

        self.log_details('# MTB errors: %d' % number_errors)
        for err_string, err_count in errors.items():
            if err_count > 0:
                self.log_details('%s: %d' % (err_string, err_count))

        self.vlog.metrics.append(
                firmware_log.Metric(self.mnprops.MTB_SANITY_ERR,
                                    (number_errors, 0)),
        )
        return number_errors

    def check(self, packets, variation=None):
        """Check ghost fingers and MTB format."""
        self.vlog.metrics = []
        number_errors = self._check_ghost_fingers() + self._check_mtb(packets)
        self.vlog.score = self.fc.mf.grade(number_errors)
        return self.vlog


class HysteresisValidator(BaseValidator):
    """Validator to check if there exists a cursor jump initially

    The movement hysteresis, if existing, set in the touchpad firmware
    should not lead to an obvious cursor jump when a finger starts moving.

    Example:
        To verify if there exists a cursor jump with distance ratio larger
        than 2.0; i.e.,
        distance(point0, point1) / distance(point1, point2) should be <= 2.0
          HysteresisValidator('> 2.0')

    Raw data of tests/data/center_to_right_slow_link.dat:

    [block0]
    Event: type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 508
    Event: type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 906
    Event: type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 720
    Event: type 3 (EV_ABS), code 58 (ABS_MT_PRESSURE), value 24
    Event: -------------- SYN_REPORT ------------

    [block1]
    Event: type 3 (EV_ABS), code 58 (ABS_MT_PRESSURE), value 25
    Event: -------------- SYN_REPORT ------------

    ...  more SYN_REPORT with ABS_MT_PRESSURE only  ...

    [block2]
    Event: type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 939
    Event: type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 727
    Event: type 3 (EV_ABS), code 58 (ABS_MT_PRESSURE), value 34
    Event: -------------- SYN_REPORT ------------

    [block3]
    Event: type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 941
    Event: type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 727
    Event: type 3 (EV_ABS), code 58 (ABS_MT_PRESSURE), value 37
    Event: -------------- SYN_REPORT ------------

    ...  more data  ...

    Let point0 represents the coordinates in block0.
    Let point1 represents the coordinates in block2.
    Let point2 represents the coordinates in block3.

    Note that the data in block1 only contain a number of pressure values
    without any X/Y updates even when the finger is tracking to the right.
    This is the undesirable hysteresis effect.

    Compute ratio = distance(point0, point1) / distance(point1, point2).
    When ratio is high, it indicates the hysteresis effect.
    """

    def __init__(self, criteria_str, finger=0, mf=None, device=None):
        self.criteria_str = criteria_str
        self.finger = finger
        name = self.__class__.__name__
        super(HysteresisValidator, self).__init__(criteria_str, mf, device,
                                                  name)

    def _point_px_to_mm(self, point_px):
        """Convert a point in px to a point in mm."""
        return Point(*self.device.pixel_to_mm(point_px.value()))

    def _find_index_of_first_distinct_value(self, points):
        """Find first index, idx, such that points[idx] != points[0]."""
        for idx, point in enumerate(points):
            if points[0].distance(points[idx]) > 0:
                return idx
        return None

    def check(self, packets, variation=None):
        """There is no jump larger than a threshold at the beginning."""
        self.init_check(packets)
        points_px = self.packets.get_ordered_finger_path(self.finger, 'point')
        point1_idx = point2_idx = None
        distance1 = distance2 = None

        if len(points_px) > 0:
            point0_mm = self._point_px_to_mm(points_px[0])
            point1_idx = self._find_index_of_first_distinct_value(points_px)

        if point1_idx is not None:
            point1_mm = self._point_px_to_mm(points_px[point1_idx])
            distance1 = point0_mm.distance(point1_mm)
            if point1_idx + 1 <= len(points_px):
                point2_idx = self._find_index_of_first_distinct_value(
                        points_px[point1_idx:]) + point1_idx

        if point2_idx is not None:
            point2_mm = self._point_px_to_mm(points_px[point2_idx])
            distance2 = point1_mm.distance(point2_mm)
            ratio = (float('infinity') if distance1 == 0 else
                     distance1 / distance2)
        else:
            ratio = float('infinity')

        self.log_details('init gap ratio: %.2f' % ratio)
        self.log_details('dist(p0,p1): ' +
                         ('None' if distance1 is None else '%.2f' % distance1))
        self.log_details('dist(p1,p2): ' +
                         ('None' if distance2 is None else '%.2f' % distance2))
        self.vlog.metrics = [
                firmware_log.Metric(self.mnprops.MAX_INIT_GAP_RATIO, ratio),
                firmware_log.Metric(self.mnprops.AVE_INIT_GAP_RATIO, ratio),
        ]
        self.vlog.score = self.fc.mf.grade(ratio)
        return self.vlog

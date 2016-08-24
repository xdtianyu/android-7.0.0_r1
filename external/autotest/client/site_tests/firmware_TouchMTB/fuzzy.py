# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fuzzy comparisons and aggregations."""


import logging
import math

from firmware_constants import MF


DEFAULT_MEMBERSHIP_FUNCTION = {
    '<=': MF.Z_FUNCTION,
    '<': MF.Z_FUNCTION,
    '>=': MF.S_FUNCTION,
    '>': MF.S_FUNCTION,
    '==': MF.SINGLETON_FUNCTION,
    '~=': MF.PI_FUNCTION,
}


"""Define possible score aggregators: average() and product().

A score aggregator collects all scores from every tests, and calculate
a final score.
"""

def average(data):
    """The average of the elements in data."""
    number = len(data)
    return math.fsum(data) / number if number > 0 else None


def product(data):
    """The product of the elements in data."""
    return math.exp(math.fsum([math.log(d) for d in data]))


"""Classes of various fuzzy member functions are defined below."""

class FuzzyMemberFunctions(object):
    """The base class of membership functions."""
    def __init__(self, para):
        """Example of parameter: (0.1, 0.3)."""
        self.para_values = map(float, para)


class FuzzySingletonMemberFunction(FuzzyMemberFunctions):
    """A class provides fuzzy Singleton Membership Function.

    Singleton Membership Function:
        parameters: (left, middle, right)
        grade(x) = 0.0,        when x <= left
                   0.0 to 1.0, when left <= x <= middle
                   1.0,        when x == middle
                   1.0 to 0.0, when middle <= x <= right
                   0.0,        when x >= right
        E.g., FuzzySingletonMemberFunction((1, 1, 1))
              Usage: when we want the x == 1 in the ideal condition.
                     grade = 1.0, when x == 1
                             0.0, when x != 1

        Note: - When x is near 'middle', the grade would be pretty close to 1.
              - When x becomes near 'left' or 'right', its grade may drop
                faster and would approach 0.
              - A cosine function is used to implement this behavior.
    """
    def __init__(self, para):
        super(FuzzySingletonMemberFunction, self).__init__(para)
        self.left, self.middle, self.right = self.para_values
        self.width_right = self.right - self.middle
        self.width_left = self.middle - self.left

    def grade(self, x):
        """The grading method of the fuzzy membership function."""
        if x == self.middle:
            return 1
        elif x <= self.left or x >= self.right:
            return 0
        elif x > self.middle:
            return (0.5 + 0.5 * math.cos((x - self.middle) / self.width_right *
                    math.pi))
        elif x < self.middle:
            return (0.5 + 0.5 * math.cos((x - self.middle) / self.width_left *
                    math.pi))


class FuzzySMemberFunction(FuzzyMemberFunctions):
    """A class provides fuzzy S Membership Function.

    S Membership Function:
        parameters: (left, right)
        grade(x) = 1  for x >= right
                   0  for x <= left
        E.g., FuzzySMemberFunction((0.1, 0.3))
              Usage: when we want the x >= 0.3 in the ideal condition.
                     grade = 1.0,                 when x >= 0.3
                             between 0.0 and 1.0, when 0.1 <= x <= 0.3
                             0.0,                 when x <= 0.1

        Note: - When x is less than but near 'right' value, the grade would be
                pretty close to 1.
              - When x becomes near 'left' value, its grade may drop faster
                and would approach 0.
              - A cosine function is used to implement this behavior.
    """

    def __init__(self, para):
        super(FuzzySMemberFunction, self).__init__(para)
        self.left, self.right = self.para_values
        self.width = self.right - self.left

    def grade(self, x):
        """The grading method of the fuzzy membership function."""
        if x >= self.right:
            return 1
        elif x <= self.left:
            return 0
        else:
            return 0.5 + 0.5 * math.cos((x - self.right) / self.width * math.pi)


class FuzzyZMemberFunction(FuzzyMemberFunctions):
    """A class provides fuzzy Z Membership Function.

    Z Membership Function:
        parameters: (left, right)
        grade(x) = 1  for x <= left
                   0  for x >= right
        E.g., FuzzyZMemberFunction((0.1, 0.3))
              Usage: when we want the x <= 0.1 in the ideal condition.
                     grade = 1.0,                 when x <= 0.1
                             between 0.0 and 1.0, when 0.1 <= x <= 0.3
                             0.0,                 when x >= 0.3

        Note: - When x is greater than but near 'left' value, the grade would be
                pretty close to 1.
              - When x becomes near 'right' value, its grade may drop faster
                and would approach 0.
              - A cosine function is used to implement this behavior.
    """

    def __init__(self, para):
        super(FuzzyZMemberFunction, self).__init__(para)
        self.left, self.right = self.para_values
        self.width = self.right - self.left

    def grade(self, x):
        """The grading method of the fuzzy membership function."""
        if x <= self.left:
            return 1
        elif x >= self.right:
            return 0
        else:
            return 0.5 + 0.5 * math.cos((x - self.left) / self.width * math.pi)


# Mapping from membership functions to the fuzzy member function classes.
MF_DICT = {
    # TODO(josephsih): PI, TRAPEZ, and TRIANGLE functions are to be implemented.
    # MF.PI_FUNCTION: FuzzyPiMemberFunction,
    MF.SINGLETON_FUNCTION: FuzzySingletonMemberFunction,
    MF.S_FUNCTION: FuzzySMemberFunction,
    # MF.TRAPEZ_FUNCTION: FuzzyTrapezMemberFunction,
    # MF.TRIANGLE_FUNCTION: FuzzyTriangleMemberFunction
    MF.Z_FUNCTION: FuzzyZMemberFunction,
}


class FuzzyCriteria:
    """A class to parse criteria string and build the criteria object."""

    def __init__(self, criteria_str, mf=None):
        self.criteria_str = criteria_str
        self.mf_name = mf
        self.mf = None
        self.default_mf_name = None
        self.value_range = None
        self._parse_criteria_and_exit_on_failure()
        self._create_mf()

    def _parse_criteria(self, criteria_str):
        """Parse the criteria string.

        Example:
            Ex 1. '<= 0.05, ~ +0.07':
                  . The ideal input value should be <= 0.05. If so, it gets
                    the grade 1.0
                  . The allowable upper bound is 0.05 + 0.07 = 0.12. Anything
                    greater than or equal to 0.12 would get a grade 0.0
                  . Any input value falling between 0.05 and 0.12 would get a
                    score between 0.0 and 1.0 depending on which membership
                    function is used.
        """
        criteria_list = criteria_str.split(',')
        tolerable_delta = []
        op_value = None
        for c in criteria_list:
            op, value = c.split()
            # TODO(josephsih): should support and '~=' later.
            if op in ['<=', '<', '>=', '>', '==']:
                primary_op = op
                self.default_mf_name = DEFAULT_MEMBERSHIP_FUNCTION[op]
                op_value = float(value)
            elif op == '~':
                tolerable_delta.append(float(value))
            else:
                return False

        # Syntax error in criteria string
        if op_value is None:
            return False

        # Calculate the allowable range of values
        range_max = range_min = op_value
        for delta in tolerable_delta:
            if delta >= 0:
                range_max = op_value + delta
            else:
                range_min = op_value + delta

        if primary_op in ['<=', '<', '>=', '>']:
            self.value_range = (range_min, range_max)
        elif primary_op == '==':
            self.value_range = (range_min, op_value, range_max)
        else:
            self.value_range = None

        return True

    def _parse_criteria_and_exit_on_failure(self):
        """Call _parse_critera and exit on failure."""
        if not self._parse_criteria(self.criteria_str):
            logging.error('Parsing criteria string error.')
            exit(1)

    def _create_mf(self):
        """Parse the criteria and create its membership function object."""
        # If a membership function is specified in the test_conf, use it.
        # Otherwise, use the default one.
        mf_name = self.mf_name if self.mf_name else self.default_mf_name
        mf_class = MF_DICT[mf_name]
        self.mf = mf_class(self.value_range)

    def get_criteria_value_range(self):
        """Parse the criteria and return its op value."""
        return self.value_range

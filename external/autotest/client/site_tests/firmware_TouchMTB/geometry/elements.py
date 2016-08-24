# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a simple geometry module containing basic elements."""


# To allow roundoff error
TOLERANCE = 0.00000001


def about_eq(f1, f2):
    """Determine if two numbers are about equal within the TOLERANCE.

    @param f1: float number 1
    @param f2: float number 2
    """
    return abs(f1 - f2) < TOLERANCE


class Point:
    """A point class."""
    def __init__(self, x=None, y=None):
        """Initialize a point.

        @param x: x coordinate
        @param y: y coordinate
        """
        self.x = x if x is None else float(x)
        self.y = y if y is None else float(y)

    def __bool__(self):
        """A boolean indicating if this point is defined."""
        return self.x is not None and self.y is not None

    def __eq__(self, p):
        """Determine if this point is equal to the specified point, p.

        @param p: a point
        """
        return about_eq(self.x, p.x) and about_eq(self.y, p.y)

    def __hash__(self):
        """Redefine the hash function to meet the consistency requirement.

        In order to put an item into a set, it needs to be hashable.
        To make an object hashable, it must meet the consistency requirement:
            a == b must imply hash(a) == hash(b)
        """
        return hash((self.x, self.y))

    def __str__(self):
        """The string representation of the point value."""
        return 'Point: (%.4f, %.4f)' % (self.x, self.y)

    def distance(self, p):
        """Calculate the distance between p and this point.

        @param p: a point
        """
        dist_x = p.x - self.x
        dist_y = p.y - self.y
        return (dist_x ** 2 + dist_y ** 2 ) ** 0.5

    def value(self):
        """Return the point coordinates."""
        return (self.x, self.y)

    # __bool__ is used in Python 3.x and __nonzero__ in Python 2.x
    __nonzero__ = __bool__


class Circle:
    """A circle class."""
    def __init__(self, center, radius):
        """Initialize a circle.

        @param center: the center point of the circle
        @param radius: the radius of the circle
        """
        self.center = center
        self.radius = radius

    def __bool__(self):
        """A boolean indicating if this circle is defined."""
        return self.center is not None and self.radius is not None

    def __contains__(self, p):
        """Determine if p is enclosed in the circle.

        @param p: a point
        """
        return self.center.distance(p) <= self.radius + TOLERANCE

    def __eq__(self, c):
        """Determine if this circle is equal to the specified circle, c.

        @param c: a circle
        """
        return self.center == c.center and about_eq(self.radius, c.radius)

    def __hash__(self):
        """Redefine the hash function to meet the consistency requirement.

        In order to put an item into a sets.Set, it needs to be hashable.
        To make an object hashable, it must meet the consistency requirement:
            a == b must imply hash(a) == hash(b)
        """
        return hash((self.center, self.radius))

    def __str__(self):
        """The string representation of the circle value."""
        return ('Center: %s, %s' % (str(self.center),
                                    'Radius: %.4f' % self.radius)
                if self else 'Circle is None')

    # __bool__ is used in Python 3.x and __nonzero__ in Python 2.x
    __nonzero__ = __bool__

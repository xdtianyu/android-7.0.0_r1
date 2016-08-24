# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""minicircle: calculating the minimal enclosing circle given a set of points

   Reference:
   [1] Emo Welzl. Smallest enclosing disks (balls and ellipsoids).
         http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.46.1450
   [2] Circumscribed circle. http://en.wikipedia.org/wiki/Circumscribed_circle

 - get_two_farthest_clusters(): Classify the points into two farthest clusters

 - get_radii_of_two_minimal_enclosing_circles(): Get the radii of the
        two minimal enclosing circles

"""


import copy

from sets import Set

from elements import Point, Circle


def _mini_circle_2_points(p1, p2):
    """Derive the mini circle with p1 and p2 composing the diameter.
    This also handles the special case when p1 and p2 are located at
    the same coordinate.
    """
    center = Point((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5)
    radius = center.distance(p1)
    return Circle(center, radius)


def _mini_circle_3_points(A, B, C):
    """Derive the mini circle enclosing arbitrary three points, A, B, C.

    @param A: a point (possibly a vertex of a triangle)
    @param B: a point (possibly a vertex of a triangle)
    @param C: a point (possibly a vertex of a triangle)
    """
    # Check if this is an obtuse triangle or a right triangle,
    # including the special cases
    # (1) the 3 points are on the same line
    # (2) any 2 points are located at the same coordinate
    # (3) all 3 points are located at the same coordinate
    a = B.distance(C)
    b = C.distance(A)
    c = A.distance(B)
    if (a ** 2 >= b ** 2 + c ** 2):
        return _mini_circle_2_points(B, C)
    elif (b ** 2 >= c ** 2 + a ** 2):
        return _mini_circle_2_points(C, A)
    elif (c ** 2 >= a ** 2 + b ** 2):
        return _mini_circle_2_points(A, B)

    # It is an acute triangle. Refer to Reference [2].
    D = 2 * (A.x * (B.y - C.y) + B.x *(C.y - A.y) + C.x * (A.y - B.y))
    x = ((A.x ** 2 + A.y ** 2) * (B.y - C.y) +
         (B.x ** 2 + B.y ** 2) * (C.y - A.y) +
         (C.x ** 2 + C.y ** 2) * (A.y - B.y)) / D
    y = ((A.x ** 2 + A.y ** 2) * (C.x - B.x) +
         (B.x ** 2 + B.y ** 2) * (A.x - C.x) +
         (C.x ** 2 + C.y ** 2) * (B.x - A.x)) / D

    center = Point(x, y)
    radius = center.distance(A)
    return Circle(center, radius)


def _b_minicircle0(R):
    """build minicircle0: build the mini circle with an empty P and has R
    on the boundary.

    @param R: boundary points, a set of points which should be on the boundary
              of the circle to be built
    """
    if len(R) == 0:
        return Circle(None, None)
    if len(R) == 1:
        return Circle(R.pop(), 0)
    elif len(R) == 2:
        p1 = R.pop()
        p2 = R.pop()
        return _mini_circle_2_points(p1, p2)
    else:
        return _mini_circle_3_points(*list(R))


def _b_minicircle(P, R):
    """build minicircle: build the mini circle enclosing P and has R on the
    boundary.

    @param P: a set of points that should be enclosed in the circle to be built
    @param R: boundary points, a set of points which should be on the boundary
              of the circle to be built
    """
    P = copy.deepcopy(P)
    R = copy.deepcopy(R)
    if len(P) == 0 or len(R) == 3:
        D = _b_minicircle0(R)
    else:
        p = P.pop()
        D = _b_minicircle(P, R)
        if (not D) or (p not in D):
            R.add(p)
            D = _b_minicircle(P, R)
    return D


def _make_Set_of_Points(points):
    """Convert the points to a set of Point objects.

    @param points: could be a list/set of pairs_of_ints/Point_objects.
    """
    return Set([p if isinstance(p, Point) else Point(*p) for p in points])


def minicircle(points):
    """Get the minimal enclosing circle of the points.

    @param points: a list of points which should be enclosed in the circle to be
                   built
    """
    P = _make_Set_of_Points(points)
    return _b_minicircle(P, Set()) if P else None

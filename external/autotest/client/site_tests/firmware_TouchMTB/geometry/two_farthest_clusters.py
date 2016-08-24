# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Classify a set of points into two farthest clusters

  - get_two_farthest_clusters(): Classify the points into two farthest clusters

  - get_radii_of_two_minimal_enclosing_circles(): Get the radii of the
        two minimal enclosing circles

"""


from .minicircle import minicircle


def get_two_farthest_points(points):
    """Calculate two farthest points from the list of given points.

    Use a dumb brute force search for now since there are only a few points
    in our use cases.
    """
    if len(points) <= 1:
        return points

    max_dist = float('-infinity')
    for p1 in points:
        for p2 in points:
            dist = p1.distance(p2)
            if dist > max_dist:
                two_farthest_points = (p1, p2)
                max_dist = dist

    return two_farthest_points


def get_two_farthest_clusters(points):
    """Classify the points into two farthest clusters.

    Steps:
    (1) Calculate two points that are farthest from each other. These
        two points represent the two farthest clusters.
    (2) Classify every point to one of the two clusters based on which
        cluster the point is nearer.

    @param points: a list of points of Point type
    """
    if len(points) <= 1:
        return (points, [])

    fp1, fp2 = get_two_farthest_points(points)

    # Classify every point to the two clusters represented by the two
    # farthest points above.
    cluster1 = []
    cluster2 = []
    for p in points:
        (cluster1 if p.distance(fp1) <= p.distance(fp2) else cluster2).append(p)

    return (cluster1, cluster2)


def get_radii_of_two_minimal_enclosing_circles(points):
    """Get the radii of the two minimal enclosing circles from points.

    Return: [radius_of_circle1, radius_of_circle2]
            where circle1, circle2 are the two minimal enclosing circles
            of the two clusters derived from the two farthest points
            found in the argument points.
    """
    return [minicircle(cluster).radius
            for cluster in get_two_farthest_clusters(points) if cluster]

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import collections


def checksum_counts(checksums):
    """
    @param checksums: list of checksums, each checksum in a 4-tuple of ints
    @returns a dictionary of checksums as keys mapped to their respective
    count of occurance in the list.

    """
    counts = {}

    for checksum in checksums:
        if checksum in counts:
            counts[checksum] += 1
        else:
            counts[checksum] = 1

    return counts


def checksum_indices(checksums):
    """
    @param checksums: list of checksums.
    @returns an OrderedDict containing checksums as keys and their respective
    first-occurance indices as values

    """

    d = collections.OrderedDict()

    for i, checksum in enumerate(checksums):
        if checksum not in d:
           d[checksum] = i

    return d
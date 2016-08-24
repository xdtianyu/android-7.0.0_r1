# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Utilities for operations on sequences / lists


def lcs_length(x, y):
    """
    Computes the length of a Longest Common Subsequence (LCS) of x and y.

    Algorithm adapted from "Introduction to Algorithms" - CLRS.

    @param x: list, one sequence
    @param y: list, the other sequence

    """
    m = len(x)
    n = len(y)
    c = dict() # Use dictionary as a 2D array, keys are tuples

    for i in range(m + 1):
        c[i, 0] = 0

    for j in range(n + 1):
        c[0, j] = 0

    for i in range(1, m + 1):
        for j in range(1, n + 1):
            if x[i - 1] == y[j - 1]:
                c[i, j] = c[i - 1, j - 1] + 1
            else:
                c[i, j] = max(c[i - 1, j], c[i, j - 1])

    return c[m, n]
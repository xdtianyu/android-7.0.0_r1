#!/usr/bin/python2.7
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import numpy


def LinearRegression(x, y):
    """Perform a linear regression using numpy.

    @param x: Array of x-coordinates of the samples
    @param y: Array of y-coordinates of the samples
    @return: ((slope, intercept), r-squared)
    """
    # p(x) = p[0]*x**1 + p[1]
    p, (residual,) = numpy.polyfit(x, y, 1, full=True)[:2]
    # Calculate the coefficient of determination (R-squared) from the
    # "residual sum of squares"
    # Reference:
    # http://en.wikipedia.org/wiki/Coefficient_of_determination
    r2 = 1 - (residual / (y.size*y.var()))

    # Alternate calculation for R-squared:
    #
    # Calculate the coefficient of determination (R-squared) as the
    # square of the  sample correlation coefficient,
    # which can be calculated from the variances and covariances.
    # Reference:
    # http://en.wikipedia.org/wiki/Correlation#Pearson.27s_product-moment_coefficient
    #V = numpy.cov(x, y, ddof=0)
    #r2 = (V[0,1]*V[1,0]) / (V[0,0]*V[1,1])

    return p, r2


def FactsToNumpyArray(facts, dtype):
    """Convert "facts" (list of dicts) to a numpy array.

    @param facts: A list of dicts. Each dict must have keys matching the field
            names in dtype.
    @param dtype: A numpy.dtype used to fill the array from facts. The dtype
            must be a "structured array". ie:
            numpy.dtype([('loops', numpy.int), ('cycles', numpy.int)])
    @returns: A numpy.ndarray with dtype=dtype filled with facts.
    """
    a = numpy.zeros(len(facts), dtype=dtype)
    for i, f in enumerate(facts):
        a[i] = tuple(f[n] for n in dtype.names)
    return a

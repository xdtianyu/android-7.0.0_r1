# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import time
import functools
import logging


_registry = {}


def stat(category=None):
    """
    Decorator that registers the function as a function that, when called,
    will submit a stat to statsd.

    @param category: The set of servers to be run against.
    @param f: A function that submits stats.
    @returns: f
    """
    def curry(f):  # pylint: disable-msg=C0111
        _registry.setdefault(category, []).append(f)
        return f
    return curry


def loop_stat(category=None):
    """
    Decorator that registers the function as a function that, when called,
    will submit a stat to statsd.  This function is then registered so that
    it will be called periodically.

    You probably want to use this one.

    @param category: The set of servers to be run against.
    @param f: A function that submits stats.
    @returns: f
    """
    def curry(f):  # pylint: disable-msg=C0111
        @functools.wraps(f)
        def looped(*args, **kwargs):  # pylint: disable-msg=C0111
            while True:
                try:
                    f(*args, **kwargs)
                except Exception as e:
                    logging.exception(e)
                time.sleep(15)
        _registry.setdefault(category, []).append(looped)
        return f
    return curry


def registered_functions():
    """
    Return all functions registered as a stat.

    returns: A list of 0-arity functions.
    """
    return _registry

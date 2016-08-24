#!/usr/bin/env python
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import threading
import time
import unittest

import common
from autotest_lib.client.common_lib import decorators


class InContextTest(mox.MoxTestBase):
    """ Unit tests for the in_context decorator. """

    @decorators.in_context('lock')
    def inc_count(self):
        """ Do a slow, racy read/write. """
        temp = self.count
        time.sleep(0.0001)
        self.count = temp + 1


    def testDecorator(self):
        """ Test that the decorator works by using it with a lock. """
        self.count = 0
        self.lock = threading.RLock()
        iters = 100
        num_threads = 20
        # Note that it is important for us to go through all this bother to call
        # a method in_context N times rather than call a method in_context that
        # does something N times, because by doing the former, we acquire the
        # context N times (1 time for the latter).
        thread_body = lambda f, n: [f() for i in xrange(n)]
        threads = [threading.Thread(target=thread_body,
                                    args=(self.inc_count, iters))
                   for i in xrange(num_threads)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
        self.assertEquals(iters * num_threads, self.count)


class CachedPropertyTest(unittest.TestCase):
    def testIt(self):
        """cached_property"""
        class Example(object):
            def __init__(self, v=0):
                self.val = v

            @decorators.cached_property
            def prop(self):
                self.val = self.val + 1
                return self.val

        ex = Example()
        self.assertEquals(1, ex.prop)
        self.assertEquals(1, ex.prop)

        ex2 = Example(v=5)
        self.assertEquals(6, ex2.prop)


if __name__ == '__main__':
    unittest.main()

#! /usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for resource_method.py."""

import mox
import unittest

import common
from fake_device_server import common_util
from fake_device_server import resource_method
from fake_device_server import resource_delegate
from fake_device_server import server_errors


class ResourceMethodTest(mox.MoxTestBase):
    """Tests for the ResourceMethod class."""

    def setUp(self):
        """Sets up resource_method object and dict of resources."""
        mox.MoxTestBase.setUp(self)
        self.resources = {}
        self.resource_method = resource_method.ResourceMethod(
                resource_delegate.ResourceDelegate(self.resources))


    def testPatch(self):
        """Tests that we correctly patch a resource."""
        expected_resource = dict(id=1234, blah='hi')
        update_resource = dict(blah='hi')
        self.resources[(1234, None)] = dict(id=1234)

        self.mox.StubOutWithMock(common_util, 'parse_serialized_json')

        common_util.parse_serialized_json().AndReturn(update_resource)

        self.mox.ReplayAll()
        returned_json = self.resource_method.PATCH(1234)
        self.assertEquals(expected_resource, returned_json)
        self.mox.VerifyAll()


    def testPut(self):
        """Tests that we correctly replace a resource."""
        update_resource = dict(id=12345, blah='hi')
        self.resources[(12345, None)] = dict(id=12345)

        self.mox.StubOutWithMock(common_util, 'parse_serialized_json')

        common_util.parse_serialized_json().AndReturn(update_resource)

        self.mox.ReplayAll()
        returned_json = self.resource_method.PUT(12345)
        self.assertEquals(update_resource, returned_json)
        self.mox.VerifyAll()

        self.mox.ResetAll()

        # Ticket id doesn't match.
        update_resource = dict(id=12346, blah='hi')
        common_util.parse_serialized_json().AndReturn(update_resource)

        self.mox.ReplayAll()
        self.assertRaises(server_errors.HTTPError,
                          self.resource_method.PUT, 12345)
        self.mox.VerifyAll()


if __name__ == '__main__':
    unittest.main()

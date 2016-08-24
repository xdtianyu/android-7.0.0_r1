#! /usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for common_util methods."""

import cherrypy
import json
import tempfile
import unittest

import common
from fake_device_server import common_util
from fake_device_server import server_errors


class FakeDeviceServerTests(unittest.TestCase):
    """Contains tests for methods not included in classes."""

    def testParseSerializeJson(self):
        """Tests that we can seralize / deserialize json from cherrypy."""
        json_data = json.dumps(dict(a='b', b='c'))

        json_file = tempfile.TemporaryFile()
        json_file.write(json.dumps(json_data))
        content_length = json_file.tell()
        json_file.seek(0)
        cherrypy.request.headers['Content-Length'] = content_length

        cherrypy.request.rfile = json_file

        self.assertEquals(common_util.parse_serialized_json(), json_data)
        json_file.close()

        # Also test the edge case without an input file.
        json_file = tempfile.TemporaryFile()
        cherrypy.request.rfile = json_file

        self.assertEquals(common_util.parse_serialized_json(), None)
        json_file.close()


    def testParseCommonArgs(self):
        """Tests various flavors of the parse common args method."""
        id = 123456
        key = 'boogity'

        # Should parse all values.
        id, api_key, op = common_util.parse_common_args(
                (id, 'boogity',),
                dict(key=key), supported_operations=set(['boogity']))
        self.assertEquals(id, id)
        self.assertEquals(key, api_key)
        self.assertEquals('boogity', op)

        # Missing op.
        id, api_key, op = common_util.parse_common_args((id,), dict(key=key))
        self.assertEquals(id, id)
        self.assertEquals(key, api_key)
        self.assertIsNone(op)

        # Missing key.
        id, api_key, op = common_util.parse_common_args((id,), dict())
        self.assertEquals(id, id)
        self.assertIsNone(api_key)
        self.assertIsNone(op)

        # Missing all.
        id, api_key, op = common_util.parse_common_args(tuple(), dict())
        self.assertIsNone(id)
        self.assertIsNone(api_key)
        self.assertIsNone(op)

        # Too many args.
        self.assertRaises(server_errors.HTTPError,
                          common_util.parse_common_args,
                          (id, 'lame', 'stuff',), dict())

        # Operation when it's not expected.
        self.assertRaises(server_errors.HTTPError,
                          common_util.parse_common_args,
                          (id, 'boogity'), dict())


if __name__ == '__main__':
    unittest.main()

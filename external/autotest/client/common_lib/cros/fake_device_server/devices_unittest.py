#! /usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for devices.py."""

import mox
import unittest

import common
from fake_device_server import commands
from fake_device_server import devices
from fake_device_server import fail_control
from fake_device_server import oauth
from fake_device_server import resource_delegate
from fake_device_server import server_errors


class DevicesTest(mox.MoxTestBase):
    """Tests for the Devices class."""

    def setUp(self):
        """Sets up mox and a ticket / registration objects."""
        mox.MoxTestBase.setUp(self)
        self.devices_resource = {}
        self.fail_control = fail_control.FailControl()
        self.oauth = oauth.OAuth(self.fail_control)
        self.commands = commands.Commands(self.oauth, self.fail_control)
        self.devices = devices.Devices(
                resource_delegate.ResourceDelegate(self.devices_resource),
                self.commands,
                self.oauth,
                self.fail_control)


    def testCreateDevice(self):
        """Tests that we can create a new device."""
        good_device_config = dict(userEmail='buffet@tasty.org',
                                  name='buffet_device',
                                  channel=dict(supportedType='xmpp'))

        new_device = self.devices.create_device(None, good_device_config)
        self.assertTrue('id' in new_device)
        device_id = new_device['id']
        # New device should be registered with commands handler.
        self.assertTrue(device_id in self.commands.device_commands)

        bad_device_config = dict(name='buffet_device')
        self.assertRaises(server_errors.HTTPError,
                          self.devices.create_device, None, bad_device_config)


    def testGet(self):
        """Tests that we can retrieve a device correctly."""
        self.devices_resource[(1234, None)] = dict(id=1234)
        returned_json = self.devices.GET(1234)
        self.assertEquals(returned_json, self.devices_resource[(1234, None)])

        # Non-existing device.
        self.assertRaises(server_errors.HTTPError,
                          self.devices.GET, 1235)


    def testListing(self):
        """Tests that we can get a listing back correctly using the GET method.
        """
        self.devices_resource[(1234, None)] = dict(id=1234)
        self.devices_resource[(1235, None)] = dict(id=1235, boogity='taco')

        returned_json = self.devices.GET()
        self.assertEqual('clouddevices#devicesListResponse',
                         returned_json['kind'])
        self.assertTrue('devices' in returned_json)
        for device in self.devices_resource.values():
            self.assertIn(device, returned_json['devices'])


    def testDeleteDevice(self):
        """Tests that we correctly delete a device."""
        # Register device with commands handler first.
        self.commands.new_device(12345)
        self.devices_resource[(12345, None)] = dict(id=12345, nobody='care')
        self.devices.DELETE(12345)

        self.assertTrue(12345 not in self.devices_resource)
        # Make sure the device is deleted from the command handler.
        self.assertRaises(KeyError, self.commands.remove_device, 12345)

        # Should error out if we try to delete something that doesn't exist.
        self.assertRaises(server_errors.HTTPError,
                          self.devices.DELETE, 12500)


if __name__ == '__main__':
    unittest.main()

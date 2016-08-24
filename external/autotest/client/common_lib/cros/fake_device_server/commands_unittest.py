#! /usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for commands.py."""

import copy
import mox
import unittest

import common
from fake_device_server import commands
from fake_device_server import fake_oauth
from fake_device_server import fail_control
from fake_device_server import server_errors


class CommandsTest(mox.MoxTestBase):
    """Tests for the Commands class.

    Note unlike other unittests in this project, I set the api_key for all
    tests. This makes the logic easier to read because of the additional
    dictionary mapping of
    # commands.devices_commands[(id, api_key)] = dict of commands by command id.
    """

    def setUp(self):
        """Sets up mox and a ticket / registration objects."""
        mox.MoxTestBase.setUp(self)
        # Use a fake OAuth module to work around the hack that this
        # module bypass cherrypy by directly invoking commands.GET.
        self.oauth = fake_oauth.FakeOAuth()
        self.fail_control = fail_control.FailControl()
        self.commands = commands.Commands(self.oauth, self.fail_control)


    def testCreateCommand(self):
        """Tests that we can create a new command."""
        DEVICE_ID = '1234awesomeDevice'
        GOOD_COMMAND = {
            'deviceId': DEVICE_ID,
            'name': 'base._vendorCommand',
            'base': {
                '_vendorCommand': {
                    'name': 'specialCommand',
                    'kind': 'buffetSpecialCommand',
                }
            }
        }

        self.commands.new_device(DEVICE_ID)
        new_command = self.commands.create_command(GOOD_COMMAND)
        self.assertTrue('id' in new_command)
        command_id = new_command['id']
        self.assertEqual(new_command['state'], 'queued')
        self.assertEqual(
                self.commands.device_commands[DEVICE_ID][command_id],
                new_command)

        # Test command without necessary nesting.
        bad_command = {'base': {}}
        self.assertRaises(server_errors.HTTPError,
                          self.commands.create_command, bad_command)

        # Test adding a good command to an unknown device.
        BAD_COMMAND = copy.deepcopy(GOOD_COMMAND)
        BAD_COMMAND['deviceId'] = 'not_a_real_device'
        self.assertRaises(server_errors.HTTPError,
                          self.commands.create_command, BAD_COMMAND)


    def testGet(self):
        """Tests that we can retrieve a command correctly."""
        DEVICE_ID = 'device_id'
        COMMAND_ID = 'command_id'
        COMMAND_RESOURCE = {'faked': 'out'}
        self.commands.new_device(DEVICE_ID)
        self.commands.device_commands[DEVICE_ID][COMMAND_ID] = COMMAND_RESOURCE
        returned_json = self.commands.GET(COMMAND_ID, deviceId=DEVICE_ID)
        self.assertEquals(returned_json, COMMAND_RESOURCE)

        BAD_COMMAND_ID = 'fubar'
        # Non-existing command.
        self.assertRaises(server_errors.HTTPError,
                          self.commands.GET, BAD_COMMAND_ID)


    def testListing(self):
        """Tests that we can get a listing back correctly using the GET method.
        """
        DEVICE_ID = 'device_id'
        COMMAND = {
            'name': 'base.reboot',
            'deviceId': DEVICE_ID,
        }
        self.commands.new_device(DEVICE_ID)
        command1 = self.commands.create_command(copy.deepcopy(COMMAND))
        command2 = self.commands.create_command(copy.deepcopy(COMMAND))
        command1_id = command1['id']
        command2_id = command2['id']
        self.commands.device_commands[DEVICE_ID][command1_id]['state'] = \
                'inProgress'

        # Without state should return all commands.
        def check_has_commands(expected_ids, state=None):
            """Check that we get all the commands we expect given a state.

            @param expected_ids: list of string command ids.
            @param state: Optional state to filter on (a string like 'queued'

            """
            returned_json = self.commands.GET(deviceId=DEVICE_ID, state=state)
            self.assertEqual('clouddevices#commandsListResponse',
                             returned_json['kind'])
            self.assertTrue('commands' in returned_json)
            returned_command_ids = [command['id']
                                    for command in returned_json['commands']]
            self.assertEqual(sorted(returned_command_ids), sorted(expected_ids))

        check_has_commands([command1_id, command2_id])
        check_has_commands([command1_id], state='inProgress')


if __name__ == '__main__':
    unittest.main()

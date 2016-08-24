# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple client lib to the registration RPC."""

import json
import logging
import urllib2

import common
from fake_device_server.client_lib import common_client
from fake_device_server import commands as s_commands


class CommandsClient(common_client.CommonClient):
    """Client library for commands method."""

    def __init__(self, *args, **kwargs):
        common_client.CommonClient.__init__(
                self, s_commands.COMMANDS_PATH, *args, **kwargs)


    def get_command(self, command_id):
        """Returns info about the given command using |command_id|.

        @param command_id: valid id for a command.
        """
        request = urllib2.Request(self.get_url([command_id]),
                                  headers=self.add_auth_headers())
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def list_commands(self, device_id):
        """Returns the list of commands for the given |device_id|.

        @param command_id: valid id for a command.
        """
        request = urllib2.Request(self.get_url(params={'deviceId':device_id}),
                                  headers=self.add_auth_headers())
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def update_command(self, command_id, data, replace=False):
        """Updates the command with |data|.

        @param command_id: id of the command to update.
        @param data: data to update command with.
        @param replace: If True, replace all data with the given data using the
                PUT operation.
        """
        if not data:
            return

        headers = self.add_auth_headers({'Content-Type': 'application/json'})
        request = urllib2.Request(self.get_url([command_id]), json.dumps(data),
                                  headers=headers)
        if replace:
            request.get_method = lambda: 'PUT'
        else:
            request.get_method = lambda: 'PATCH'

        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def create_command(self, device_id, data):
        """Creates a new command.

        @device_id: ID of device to send command to.
        @param data: command.
        """
        headers = self.add_auth_headers({'Content-Type': 'application/json'})
        data['deviceId'] = device_id
        request = urllib2.Request(self.get_url(),
                                  json.dumps(data),
                                  headers=headers)
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())

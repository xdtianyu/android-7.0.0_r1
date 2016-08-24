# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple implementation of the commands RPC."""

from cherrypy import tools
import logging
import uuid

import common
from fake_device_server import common_util
from fake_device_server import constants
from fake_device_server import server_errors

COMMANDS_PATH = 'commands'


# TODO(sosa) Support upload method (and mediaPath parameter).
class Commands(object):
    """A simple implementation of the commands interface."""

    # Needed for cherrypy to expose this to requests.
    exposed = True

    # Roots of command resource representation that might contain commands.
    _COMMAND_ROOTS = set(['base', 'aggregator', 'printer', 'storage', 'test'])


    def __init__(self, oauth_handler, fail_control_handler):
        """Initializes a Commands handler."""
        # A map of device_id's to maps of command ids to command resources
        self.device_commands = dict()
        self._num_commands_created = 0
        self._oauth_handler = oauth_handler
        self._fail_control_handler = fail_control_handler


    def _generate_command_id(self):
        """@return unique command ID."""
        command_id = '%s_%03d' % (uuid.uuid4().hex[0:6],
                                  self._num_commands_created)
        self._num_commands_created += 1
        return command_id

    def new_device(self, device_id):
        """Adds knowledge of a device with the given |device_id|.

        This method should be called whenever a new device is created. It
        populates an empty command dict for each device state.

        @param device_id: Device id to add.

        """
        self.device_commands[device_id] = {}


    def remove_device(self, device_id):
        """Removes knowledge of the given device.

        @param device_id: Device id to remove.

        """
        del self.device_commands[device_id]


    def create_command(self, command_resource):
        """Creates, queues and returns a new command.

        @param api_key: Api key for the application.
        @param device_id: Device id of device to send command.
        @param command_resource: Json dict for command.
        """
        device_id = command_resource.get('deviceId', None)
        if not device_id:
            raise server_errors.HTTPError(
                    400, 'Can only create a command if you provide a deviceId.')

        if device_id not in self.device_commands:
            raise server_errors.HTTPError(
                    400, 'Unknown device with id %s' % device_id)

        if 'name' not in command_resource:
            raise server_errors.HTTPError(
                    400, 'Missing command name.')

        # Print out something useful (command base.Reboot)
        logging.info('Received command %s', command_resource['name'])

        # TODO(sosa): Check to see if command is in devices CDD.
        # Queue command, create it and insert to device->command mapping.
        command_id = self._generate_command_id()
        command_resource['id'] = command_id
        command_resource['state'] = constants.QUEUED_STATE
        self.device_commands[device_id][command_id] = command_resource
        return command_resource


    @tools.json_out()
    def GET(self, *args, **kwargs):
        """Handle GETs against the command API.

        GET .../(command_id) returns a command resource
        GET .../queue?deviceId=... returns the command queue
        GET .../?deviceId=... returns the command queue

        Supports both the GET / LIST commands for commands. List lists all
        devices a user has access to, however, this implementation just returns
        all devices.

        Raises:
            server_errors.HTTPError if the device doesn't exist.

        """
        self._fail_control_handler.ensure_not_in_failure_mode()
        args = list(args)
        requested_command_id = args.pop(0) if args else None
        device_id = kwargs.get('deviceId', None)
        if args:
            raise server_errors.HTTPError(400, 'Unsupported API')
        if not device_id or device_id not in self.device_commands:
            raise server_errors.HTTPError(
                    400, 'Can only list commands by valid deviceId.')
        if requested_command_id is None:
            requested_command_id = 'queue'

        if not self._oauth_handler.is_request_authorized():
            raise server_errors.HTTPError(401, 'Access denied.')

        if requested_command_id == 'queue':
            # Returns listing (ignores optional parameters).
            listing = {'kind': 'clouddevices#commandsListResponse'}
            requested_state = kwargs.get('state', None)
            listing['commands'] = []
            for _, command in self.device_commands[device_id].iteritems():
                # Check state for match (if None, just append all of them).
                if (requested_state is None or
                        requested_state == command['state']):
                    listing['commands'].append(command)
            logging.info('Returning queue of commands: %r', listing)
            return listing

        for command_id, resource in self.device_commands[device_id].iteritems():
            if command_id == requested_command_id:
                return self.device_commands[device_id][command_id]

        raise server_errors.HTTPError(
                400, 'No command with ID=%s found' % requested_command_id)


    @tools.json_out()
    def POST(self, *args, **kwargs):
        """Creates a new command using the incoming json data."""
        # TODO(wiley) We could check authorization here, which should be
        #             a client/owner of the device.
        self._fail_control_handler.ensure_not_in_failure_mode()
        data = common_util.parse_serialized_json()
        if not data:
            raise server_errors.HTTPError(400, 'Require JSON body')

        return self.create_command(data)

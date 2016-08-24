# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple implementation of the devices RPC."""

from cherrypy import tools
import logging
import time

import common
from fake_device_server import common_util
from fake_device_server import resource_method
from fake_device_server import server_errors


# TODO(sosa): All access to this object should technically require auth. Create
# setters/getters for the auth token for testing.

DEVICES_PATH = 'devices'


class Devices(resource_method.ResourceMethod):
    """A simple implementation of the device interface.

    A common workflow of using this API is:

    POST .../ # Creates a new device with id <id>.
    PATCH ..../<id> # Update device state.
    GET .../<id> # Get device state.
    DELETE .../<id> # Delete the device.
    """

    # Needed for cherrypy to expose this to requests.
    exposed = True


    def __init__(self, resource, commands_instance, oauth_instance,
                 fail_control_handler):
        """Initializes a registration ticket.

        @param resource: A resource delegate for storing devices.
        @param commands_instance: Instance of commands method class.
        @param oauth_instance: Instance of oauth class.
        @param fail_control_handler: Instance of FailControl.
        """
        super(Devices, self).__init__(resource)
        self.commands_instance = commands_instance
        self._oauth = oauth_instance
        self._fail_control_handler = fail_control_handler


    def _handle_state_patch(self, device_id, api_key, data):
        """Patch a device's state with the given update data.

        @param device_id: string device id to update.
        @param api_key: string api_key to support this resource delegate.
        @param data: json blob provided to patchState API.

        """
        # TODO(wiley) this.


    def _validate_device_resource(self, resource):
        # Verify required keys exist in the device draft.
        if not resource:
            raise server_errors.HTTPError(400, 'Empty device resource.')

        for key in ['name', 'channel']:
            if key not in resource:
                raise server_errors.HTTPError(400, 'Must specify %s' % key)

        # Add server fields.
        resource['kind'] = 'clouddevices#device'
        current_time_ms = str(int(round(time.time() * 1000)))
        resource['creationTimeMs'] = current_time_ms
        resource['lastUpdateTimeMs'] = current_time_ms
        resource['lastSeenTimeMs'] = current_time_ms


    def create_device(self, api_key, device_config):
        """Creates a new device given the device_config.

        @param api_key: Api key for the application.
        @param device_config: Json dict for the device.
        @raises server_errors.HTTPError: if the config is missing a required key
        """
        logging.info('Creating device with api_key=%s and device_config=%r',
                     api_key, device_config)
        self._validate_device_resource(device_config)
        new_device = self.resource.update_data_val(None, api_key,
                                                   data_in=device_config)
        self.commands_instance.new_device(new_device['id'])
        return new_device


    @tools.json_out()
    def GET(self, *args, **kwargs):
        """GET .../(device_id) gets device info or lists all devices.

        Supports both the GET / LIST commands for devices. List lists all
        devices a user has access to, however, this implementation just returns
        all devices.

        Raises:
            server_errors.HTTPError if the device doesn't exist.
        """
        self._fail_control_handler.ensure_not_in_failure_mode()
        id, api_key, _ = common_util.parse_common_args(args, kwargs)
        if not api_key:
            access_token = common_util.get_access_token()
            api_key = self._oauth.get_api_key_from_access_token(access_token)
        if id:
            return self.resource.get_data_val(id, api_key)
        else:
            # Returns listing (ignores optional parameters).
            listing = {'kind': 'clouddevices#devicesListResponse'}
            listing['devices'] = self.resource.get_data_vals()
            return listing


    @tools.json_out()
    def POST(self, *args, **kwargs):
        """Handle POSTs for a device.

        Supported APIs include:

        POST /devices/<device-id>/patchState

        """
        self._fail_control_handler.ensure_not_in_failure_mode()
        args = list(args)
        device_id = args.pop(0) if args else None
        operation = args.pop(0) if args else None
        if device_id is None or operation != 'patchState':
            raise server_errors.HTTPError(400, 'Unsupported operation.')
        data = common_util.parse_serialized_json()
        access_token = common_util.get_access_token()
        api_key = self._oauth.get_api_key_from_access_token(access_token)
        self._handle_state_patch(device_id, api_key, data)
        return {'state': self.resource.get_data_val(device_id,
                                                    api_key)['state']}


    @tools.json_out()
    def PUT(self, *args, **kwargs):
        """Update an existing device using the incoming json data.

        On startup, devices make a request like:

        PUT http://<server-host>/devices/<device-id>

        {'channel': {'supportedType': 'xmpp'},
         'commandDefs': {},
         'description': 'test_description ',
         'displayName': 'test_display_name ',
         'id': '4471f7',
         'location': 'test_location ',
         'name': 'test_device_name',
         'state': {'base': {'firmwareVersion': '6771.0.2015_02_09_1429',
                            'isProximityTokenRequired': False,
                            'localDiscoveryEnabled': False,
                            'manufacturer': '',
                            'model': '',
                            'serialNumber': '',
                            'supportUrl': '',
                            'updateUrl': ''}}}

        This PUT has no API key, but comes with an OAUTH access token.

        """
        self._fail_control_handler.ensure_not_in_failure_mode()
        device_id, _, _ = common_util.parse_common_args(args, kwargs)
        access_token = common_util.get_access_token()
        if not access_token:
            raise server_errors.HTTPError(401, 'Access denied.')
        api_key = self._oauth.get_api_key_from_access_token(access_token)
        data = common_util.parse_serialized_json()
        self._validate_device_resource(data)

        logging.info('Updating device with id=%s and device_config=%r',
                     device_id, data)
        new_device = self.resource.update_data_val(device_id, api_key,
                                                   data_in=data)
        return data


    def DELETE(self, *args, **kwargs):
        """Deletes the given device.

        Format of this call is:
        DELETE .../device_id

        Raises:
            server_errors.HTTPError if the device doesn't exist.
        """
        self._fail_control_handler.ensure_not_in_failure_mode()
        id, api_key, _ = common_util.parse_common_args(args, kwargs)
        self.resource.del_data_val(id, api_key)
        self.commands_instance.remove_device(id)

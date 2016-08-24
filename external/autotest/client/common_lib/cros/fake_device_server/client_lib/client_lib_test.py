#! /usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Small integration test for registration client

This client can work either with the fake_device_server or a live server.
To use locally (with the fake device server), start the server in the
background (e.g. ../server.py) and run the program without arguments.

Otherwise, if you want to run against a live server, you must provide an
auth code. To get an auth code, run this script with the argument URL
which will print out a link for you to visit and get your auth code.

Then re-run the test with that auth code like so:

./client_lib_test <YOUR_AUTH_CODE>.
."""

import argparse
import logging
import sys
import urllib2

import commands
import devices
import oauth_helpers
import registration


API_KEY = 'AIzaSyC55ef0RkaFTQvGvTXL_HIh6KI3pzVq4w0'
CLIENT_ID = ('522003936346-odpbgftanpuruuqhf1puk9e0' +
             'p2d5ldho.apps.googleusercontent.com')
CLIENT_SECRET = '9Om2cR2_5cKIKhSY5OFFo8uX'
SERVER_URL = 'https://www.googleapis.com/clouddevices/v1'


def parse_args(args):
    """Arg parser for this tiny program."""
    parser = argparse.ArgumentParser(usage=__doc__)
    parser.add_argument('auth_code', nargs='?',
                        help=('Either your auth code or "URL" to return the'
                              ' url to visit to get the code. If not'
                              ' specified, runs test through local fake server.'
                              ))
    return parser.parse_args(args)


def main(args):
    """Main method for integration test."""
    server_url, api_key = 'http://localhost:8080', None
    access_token = None

    parsed_args = parse_args(args)
    if parsed_args.auth_code == 'URL':
        print oauth_helpers.get_oauth2_auth_url(CLIENT_ID)
        return 0
    elif parsed_args.auth_code:
        server_url, api_key = SERVER_URL, API_KEY
        access_token = oauth_helpers.get_oauth2_user_token(
              CLIENT_ID, CLIENT_SECRET, parsed_args.auth_code)

    r_client = registration.RegistrationClient(server_url=server_url,
                                               api_key=api_key,
                                               access_token=access_token)
    # Device should support base.reboot command.
    base_reboot_command = {'reboot': {}}
    finalized_ticket = r_client.register_device(
            'test_device', 'vendor', 'xmpp', oauth_client_id=CLIENT_ID,
            base=base_reboot_command)
    new_device_id = finalized_ticket['deviceDraft']['id']
    print 'Registered new device', finalized_ticket

    # TODO(sosa): Do better. Change this to use fake auth server when it exists.
    if not parsed_args.auth_code:
        robot_token = None
    else:
        robot_token = oauth_helpers.get_oauth2_robot_token(
                CLIENT_ID, CLIENT_SECRET,
                finalized_ticket['robotAccountAuthorizationCode'])

    d_client = devices.DevicesClient(server_url=server_url,
                                     api_key=api_key, access_token=robot_token)
    if not d_client.get_device(new_device_id):
        print 'Device not found in database'
        return 1

    device_list = d_client.list_devices()['devices']
    device_ids = [device['id'] for device in device_list]
    if not new_device_id in device_ids:
        print 'Device found but not listed correctly'
        return 1


    # TODO(sosa): Figure out why I can't send commands.
    c_client = commands.CommandsClient(server_url=server_url,
                                       api_key=api_key,
                                       access_token=robot_token)
    command_dict = {'base': {'reboot': {}}}
    new_command = c_client.create_command(device['id'], command_dict)
    if not c_client.get_command(new_command['id']):
        print 'Command not found'
        return 1

    command_list = c_client.list_commands(device['id'])['commands']
    command_ids = [c['id'] for c in command_list]
    if not new_command['id'] in command_ids:
        print 'Command found but not listed correctly'
        return 1

    new_command = c_client.update_command(new_command['id'],
                                          {'state':'finished'})
    return 0


if __name__ == '__main__':
    logging_format = '%(asctime)s - %(filename)s - %(levelname)-8s: %(message)s'
    date_format = '%H:%M:%S'
    logging.basicConfig(level=logging.DEBUG, format=logging_format,
                        datefmt=date_format)
    try:
        error_code = main(sys.argv[1:])
        if error_code != 0:
            print 'Test Failed'

        sys.exit(error_code)
    except urllib2.HTTPError as e:
        print 'Received an HTTPError exception!!!'
        print e
        print e.read()
        sys.exit(1)

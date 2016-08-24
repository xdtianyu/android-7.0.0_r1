#! /usr/bin/env python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fake implementation of a Device Server.

This module can be used in testing both in autotests and locally. To use locally
you can just run this python module directly.
"""

import argparse
import logging
import logging.handlers
import cherrypy

import common
from fake_device_server import commands
from fake_device_server import devices
from fake_device_server import fail_control
from fake_device_server import meta_handler
from fake_device_server import oauth
from fake_device_server import registration_tickets
from fake_device_server import resource_delegate

PORT = 9876


def stop_server():
    """Stops the cherrypy server and blocks."""
    cherrypy.engine.stop()


def start_server(generation):
    """Starts the cherrypy server and blocks.
   
    @param generation: string unique to this instance of the fake device server.

    """
    fail_control_handler = fail_control.FailControl()
    cherrypy.tree.mount(
        fail_control_handler, '/' + fail_control.FAIL_CONTROL_PATH,
        {'/':
            {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}
        }
    )
    oauth_handler = oauth.OAuth(fail_control_handler)
    commands_handler = commands.Commands(oauth_handler, fail_control_handler)
    cherrypy.tree.mount(
        commands_handler, '/' + commands.COMMANDS_PATH,
        {'/':
            {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}
        }
    )
    devices_resource = resource_delegate.ResourceDelegate({})
    # TODO(wiley): We need to validate device commands.
    devices_handler = devices.Devices(devices_resource,
                                      commands_handler,
                                      oauth_handler,
                                      fail_control_handler)
    cherrypy.tree.mount(
        devices_handler, '/' + devices.DEVICES_PATH,
        {'/':
            {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}
        }
    )
    tickets = resource_delegate.ResourceDelegate({})
    registration_tickets_handler = registration_tickets.RegistrationTickets(
            tickets, devices_handler, fail_control_handler)
    cherrypy.tree.mount(
        registration_tickets_handler,
        '/' + registration_tickets.REGISTRATION_PATH,
        {'/':
            {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}
        }
    )
    cherrypy.tree.mount(
        oauth_handler,
        '/' + oauth.OAUTH_PATH,
        {'/':
            {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}
        }
    )
    cherrypy.tree.mount(
        meta_handler.MetaHandler(generation),
        '/' + meta_handler.META_HANDLER_PATH,
        {'/':
            {'request.dispatch': cherrypy.dispatch.MethodDispatcher()}
        }
    )
    # Don't parse POST for params.
    cherrypy.config.update({'global': {'request.process_request_body': False}})
    cherrypy.engine.start()


def main():
    """Main method for callers who start this module directly."""
    parser = argparse.ArgumentParser(
        description='Acts like a fake instance of GCD')
    parser.add_argument('generation', metavar='generation', type=str,
                        help='Unique generation id for confirming health')
    args = parser.parse_args()
    cherrypy.config.update({'server.socket_port': PORT})
    start_server(args.generation)
    cherrypy.engine.block()


if __name__ == '__main__':
    formatter = logging.Formatter(
            'fake_gcd_server: [%(levelname)s] %(message)s')
    handler = logging.handlers.SysLogHandler(address='/dev/log')
    handler.setFormatter(formatter)
    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger().addHandler(handler)
    main()

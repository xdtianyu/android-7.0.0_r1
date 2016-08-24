# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common Utility Methods"""

import cherrypy
import json
import logging

import common
from fake_device_server import server_errors


def parse_serialized_json():
    """Parses incoming cherrypy request as a json."""
    body_length = int(cherrypy.request.headers.get('Content-Length', 0))
    data = cherrypy.request.rfile.read(body_length)
    return json.loads(data) if data else None


def grab_header_field(header_name):
    """Returns the header |header_name| from an incoming request.

    @param header_name: Header name to retrieve.
    """
    return cherrypy.request.headers.get(header_name, None)


def get_access_token():
    """Returns the access token from an incoming request.

    @return string access token or None.

    """
    header = grab_header_field('Authorization')
    if header is None:
        logging.error('No authorization header found.')
        return None
    fields = header.split()
    if len(fields) != 2 or fields[0] != "Bearer":
        logging.error('No access token found.')
        return None
    logging.debug('Got authorization header "%s"', header)
    return fields[1]


def parse_common_args(args_tuple, kwargs, supported_operations=set()):
    """Common method to parse args to a CherryPy RPC for this server.

    |args_tuple| should contain all the sections of the URL after CherryPy
    removes the pieces that dispatched the URL to this handler. For instance,
    a GET method receiving '...'/<id>/<method_name> should call:
    parse_common_args(args_tuple=[<id>, <method_name>]).
    Some operations take no arguments. Other operations take
    a single argument. Still other operations take
    one of supported_operations as a second argument (in the args_tuple).

    @param args_tuple: Tuple of positional args.
    @param kwargs: Dictionary of named args passed in.
    @param supported_operations: Set of operations to support if any.

    Returns:
        A 3-tuple containing the id parsed from the args_tuple, api_key,
        and finally an optional operation if supported_operations is provided
        and args_tuple contains one of the supported ops.

    Raises:
        server_error.HTTPError if combination or args/kwargs doesn't make
        sense.
    """
    args = list(args_tuple)
    api_key = kwargs.get('key')
    id = args.pop(0) if args else None
    operation = args.pop(0) if args else None
    if operation:
        if not supported_operations:
            raise server_errors.HTTPError(
                    400, 'Received operation when operation was not '
                    'expected: %s!' % operation)
        elif not operation in supported_operations:
            raise server_errors.HTTPError(
                    400, 'Unsupported operation: %s' % operation)

    # All expected args should be popped off already.
    if args:
        raise server_errors.HTTPError(
                400, 'Could not parse all args: %s' % args)

    return id, api_key, operation

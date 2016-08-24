# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import cherrypy

import common
import logging
from fake_device_server import common_util
from fake_device_server import server_errors

OAUTH_PATH = 'oauth'

TEST_API_KEY = 'this_is_an_api_key'
TEST_DEVICE_ACCESS_TOKEN = 'a_device_access_token'
TEST_DEVICE_REFRESH_TOKEN = 'a_device_refresh_token'
TOKEN_EXPIRATION_SECONDS = 24 * 60 * 60  # 24 hours.


class OAuth(object):
    """The bare minimum to make Buffet think its talking to OAuth."""

    # Needed for cherrypy to expose this to requests.
    exposed = True

    def __init__(self, fail_control_handler):
        self._device_access_token = TEST_DEVICE_ACCESS_TOKEN
        self._device_refresh_token = TEST_DEVICE_REFRESH_TOKEN
        self._fail_control_handler = fail_control_handler


    def get_api_key_from_access_token(self, access_token):
        if access_token == self._device_access_token:
            return TEST_API_KEY
        return None


    def is_request_authorized(self):
        """Checks if the access token in an incoming request is correct."""
        access_token = common_util.get_access_token()
        if access_token == self._device_access_token:
            return True
        logging.info('Wrong access token - expected %s but device sent %s',
                     self._device_access_token, access_token)
        return False


    @cherrypy.tools.json_out()
    def POST(self, *args, **kwargs):
        """Handle a post to get a refresh/access token.

        We expect the device to provide (a subset of) the following parameters.

            code
            client_id
            client_secret
            redirect_uri
            scope
            grant_type
            refresh_token

        in the request body in query-string format (see the OAuth docs
        for details). Since we're a bare-minimum implementation we're
        going to ignore most of these.

        """
        self._fail_control_handler.ensure_not_in_failure_mode()
        path = list(args)
        if path == ['token']:
            body_length = int(cherrypy.request.headers.get('Content-Length', 0))
            body = cherrypy.request.rfile.read(body_length)
            params = cherrypy.lib.httputil.parse_query_string(body)
            refresh_token = params.get('refresh_token')
            if refresh_token and refresh_token != self._device_refresh_token:
                logging.info('Wrong refresh token - expected %s but '
                             'device sent %s',
                             self._device_refresh_token, refresh_token)
                cherrypy.response.status = 400
                response = {'error': 'invalid_grant'}
                return response
            response = {
                'access_token': self._device_access_token,
                'refresh_token': self._device_refresh_token,
                'expires_in': TOKEN_EXPIRATION_SECONDS,
            }
            return response
        elif path == ['invalidate_all_access_tokens']:
            # By concatenating '_X' to the end of existing access
            # token, this will effectively invalidate the access token
            # previously granted to a device and cause us to return
            # the concatenated one for future requests.
            self._device_access_token += '_X'
            return dict()
        elif path == ['invalidate_all_refresh_tokens']:
            # Same here, only for the refresh token.
            self._device_refresh_token += '_X'
            return dict()
        else:
            raise server_errors.HTTPError(
                    400, 'Unsupported oauth path %s' % path)

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cherrypy

import common
import logging
from fake_device_server import server_errors

FAIL_CONTROL_PATH = 'fail_control'

class FailControl(object):
    """Interface used to control failing of requests."""

    # Needed for cherrypy to expose this to requests.
    exposed = True

    def __init__(self):
        self._in_failure_mode = False

    def ensure_not_in_failure_mode(self):
        """Ensures we're not in failure mode.

        If instructed to fail, this method raises an HTTPError
        exception with code 500 (Internal Server Error). Otherwise
        does nothing.

        """
        if not self._in_failure_mode:
            return
        raise server_errors.HTTPError(500, 'Instructed to fail this request')

    @cherrypy.tools.json_out()
    def POST(self, *args, **kwargs):
        """Handle POST messages."""
        path = list(args)
        if path == ['start_failing_requests']:
            self._in_failure_mode = True
            logging.info('Requested to start failing all requests.')
            return dict()
        elif path == ['stop_failing_requests']:
            self._in_failure_mode = False
            logging.info('Requested to stop failing all requests.')
            return dict()
        else:
            raise server_errors.HTTPError(
                    400, 'Unsupported fail_control path %s' % path)

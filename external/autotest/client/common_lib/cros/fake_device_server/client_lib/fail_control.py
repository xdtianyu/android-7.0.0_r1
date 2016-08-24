# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple client lib to control failures."""

import json
import urllib2

import common
from fake_device_server.client_lib import common_client
from fake_device_server import fail_control


class FailControlClient(common_client.CommonClient):
    """Client library for control failing."""

    def __init__(self, *args, **kwargs):
        common_client.CommonClient.__init__(
                self, fail_control.FAIL_CONTROL_PATH, *args, **kwargs)


    def start_failing_requests(self):
        """Starts failing request."""
        headers = self.add_auth_headers({'Content-Type': 'application/json'})
        request = urllib2.Request(self.get_url(['start_failing_requests']),
                                  json.dumps(dict()), headers=headers)
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def stop_failing_requests(self):
        """Stops failing request."""
        headers = self.add_auth_headers({'Content-Type': 'application/json'})
        request = urllib2.Request(self.get_url(['stop_failing_requests']),
                                  json.dumps(dict()), headers=headers)
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())

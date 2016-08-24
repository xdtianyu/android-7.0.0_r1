# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple client lib to interact with OAuth."""

import json
import urllib2

import common
from fake_device_server.client_lib import common_client
from fake_device_server import oauth


class OAuthClient(common_client.CommonClient):
    """Client library for interacting with OAuth."""

    def __init__(self, *args, **kwargs):
        common_client.CommonClient.__init__(
                self, oauth.OAUTH_PATH, *args, **kwargs)


    def invalidate_all_access_tokens(self):
        """Invalidates all access tokens previously issued."""
        headers = self.add_auth_headers({'Content-Type': 'application/json'})
        request = urllib2.Request(self.get_url(['invalidate_all_access_tokens']),
                                  json.dumps(dict()), headers=headers)
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())


    def invalidate_all_refresh_tokens(self):
        """Invalidates all refresh tokens previously issued."""
        headers = self.add_auth_headers({'Content-Type': 'application/json'})
        request = urllib2.Request(
            self.get_url(['invalidate_all_refresh_tokens']),
            json.dumps(dict()), headers=headers)
        url_h = urllib2.urlopen(request)
        return json.loads(url_h.read())

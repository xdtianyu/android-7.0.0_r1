# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Contains a simple client lib to interact with server meta interfaces."""

import urllib2

import common
from fake_device_server.client_lib import common_client
from fake_device_server import meta_handler


class MetaClient(common_client.CommonClient):
    """Client library for interacting meta interfaces to the server."""

    def __init__(self, *args, **kwargs):
        common_client.CommonClient.__init__(
                self, meta_handler.META_HANDLER_PATH, *args, **kwargs)


    def get_generation(self, timeout_seconds=2):
        """Retrieve the unique generation of the server.

        @param timeout_seconds: number of seconds to wait for a response.
        @return generation string or None.

        """
        try:
            request = urllib2.urlopen(self.get_url(['generation']), None,
                                      timeout_seconds)
            return request.read()
        except urllib2.URLError:
            return None

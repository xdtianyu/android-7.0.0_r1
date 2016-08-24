# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple base class for patching or updating a resource."""

from cherrypy import tools

import common
from fake_device_server import common_util
from fake_device_server import server_errors


class ResourceMethod(object):
    """A base class for methods that expose a simple PATCH/PUT mechanism."""

    def __init__(self, resource):
        """
        @param resource: A resource delegate for storing devices.
        """
        self.resource = resource


    @tools.json_out()
    def PATCH(self, *args, **kwargs):
        """Updates the given resource with the incoming json blob.

        Format of this call is:
        PATCH .../resource_id

        Caller must define a json blob to patch the resource with.

        Raises:
            server_errors.HTTPError if the resource doesn't exist.
        """
        id, api_key, _ = common_util.parse_common_args(args, kwargs)
        if not id:
            server_errors.HTTPError(400, 'Missing id for operation')

        data = common_util.parse_serialized_json()
        return self.resource.update_data_val(id, api_key, data_in=data)


    @tools.json_out()
    def PUT(self, *args, **kwargs):
        """Replaces the given resource with the incoming json blob.

        Format of this call is:
        PUT .../resource_id

        Caller must define a json blob to patch the resource with.

        Raises:
            server_errors.HTTPError if the resource doesn't exist.
        """
        id, api_key, _ = common_util.parse_common_args(args, kwargs)
        if not id:
            server_errors.HTTPError(400, 'Missing id for operation')

        data = common_util.parse_serialized_json()
        return self.resource.update_data_val(
                id, api_key, data_in=data, update=False)

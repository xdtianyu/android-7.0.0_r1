# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cherrypy


META_HANDLER_PATH = 'meta'


class MetaHandler(object):
    """Exposes meta methods related to the server."""

    # Needed for cherrypy to expose this to requests.
    exposed = True

    def __init__(self, generation):
        """Construct an instance.

        @param generation: string unique token for this server (e.g. a UUID).

        """
        self._generation = generation

    def GET(self, *args, **kwargs):
        """Handle GET requests to this URL."""
        if ['generation'] == list(args):
            return self._generation
        cherrypy.response.status = 400
        return ''

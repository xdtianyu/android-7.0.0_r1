# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Contains errors used by the fake_device_server."""

import cherrypy


class HTTPError(cherrypy.HTTPError):
  """Exception class to log the HTTPResponse before routing it to cherrypy."""
  def __init__(self, status, message):
      """
      @param status: HTTPResponse status.
      @param message: Message associated with the response.
      """
      cherrypy.HTTPError.__init__(self, status, message)
      cherrypy.log('ServerHTTPError status: %s message: %s' % (status, message))

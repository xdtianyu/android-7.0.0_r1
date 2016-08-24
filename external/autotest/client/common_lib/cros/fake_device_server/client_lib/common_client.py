# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module contains a simple client lib to the commands RPC."""

import logging
import urllib2


class CommonClient(object):
    """Common client class."""

    _DEFAULT_SERVER_URL = 'http://localhost:9876'
    _URL = '%(server_url)s/%(method)s'


    def __init__(self, method, server_url=_DEFAULT_SERVER_URL, api_key=None,
                 access_token=None,):
        """
        @param method: REST method to call e.g. my_method/call
        @param server_url: Base url for the server e.g. http://localhost:8080
        @param api_key: API key to use with remote server.
        @param access_token: Access token to use to interact with server.
        """
        self._method = method
        self.server_url = server_url
        self.api_key = api_key
        self.access_token = access_token


    def add_auth_headers(self, additional_headers=None):
        """Returns combined auth headers with any additional headers.

        @param additional_headers: Additional headers to use.
        """
        if not self.access_token:
            return additional_headers if additional_headers else {}
        else:
            headers = {'Authorization': self.access_token}
            if additional_headers:
                headers.update(additional_headers)

            return headers


    def get_url(self, paths=[], params={}):
        """Returns url to use to talk to the server method.

        @param paths: Parts of a path to append to base url.
        @param params: Dictionary of url parameters.
        """
        if not self._method:
            raise NotImplementedError('method not defined.')

        # Create the method string.
        paths_str = ''
        if paths:
            paths_str = '/' + '/'.join([str(p) for p in paths])

        # Create the query string.
        params_str = ''
        if not params:
            params = {}

        if self.api_key:
            params.setdefault('key', self.api_key)

        params_list = []
        for kw, arg in params.iteritems():
            params_list.append('='.join([urllib2.quote(kw),
                                         urllib2.quote(arg)]))

        if params_list:
            params_str = '?' + '&'.join(params_list)

        url = self._URL % dict(
                server_url=self.server_url,
                method=self._method) + paths_str + params_str

        logging.info("Returning url: %s to use.", url)
        return url

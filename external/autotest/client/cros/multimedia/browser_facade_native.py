# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An interface to access the local browser facade."""

import logging

class BrowserFacadeNativeError(Exception):
    """Error in BrowserFacadeNative."""
    pass


class BrowserFacadeNative(object):
    """Facade to access the browser-related functionality."""
    def __init__(self, resource):
        """Initializes the USB facade.

        @param resource: A FacadeResource object.

        """
        self._resource = resource


    def new_tab(self, url):
        """Opens a new tab and loads URL.

        @param url: The URL to load.
        @return a str, the tab descriptor of the opened tab.

        """
        logging.debug('Load URL %s', url)
        return self._resource.load_url(url)


    def close_tab(self, tab_descriptor):
        """Closes a previously opened tab.

        @param tab_descriptor: Indicate which tab to be closed.

        """
        tab = self._resource.get_tab_by_descriptor(tab_descriptor)
        logging.debug('Closing URL %s', tab.url)
        self._resource.close_tab(tab_descriptor)

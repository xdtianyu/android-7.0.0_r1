# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module providing common resources for different facades."""

import exceptions

from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants

_FLAKY_CALL_RETRY_TIMEOUT_SEC = 60
_FLAKY_CHROME_CALL_RETRY_DELAY_SEC = 1

retry_chrome_call = retry.retry(
        (chrome.Error, exceptions.IndexError, exceptions.Exception),
        timeout_min=_FLAKY_CALL_RETRY_TIMEOUT_SEC / 60.0,
        delay_sec=_FLAKY_CHROME_CALL_RETRY_DELAY_SEC)

class FacadeResource(object):
    """This class provides access to telemetry chrome wrapper."""

    EXTRA_BROWSER_ARGS = ['--enable-gpu-benchmarking']

    def __init__(self, chrome_object=None, restart=False):
        """Initializes a FacadeResource.

        @param chrome_object: A chrome.Chrome object or None.
        @param restart: Preserve the previous browser state.

        """
        if chrome_object:
            self._chrome = chrome_object
        else:
            self._chrome = chrome.Chrome(
                extension_paths=[constants.MULTIMEDIA_TEST_EXTENSION],
                extra_browser_args=self.EXTRA_BROWSER_ARGS,
                clear_enterprise_policy=not restart,
                autotest_ext=True)
        self._browser = self._chrome.browser
        # The opened tabs are stored by tab descriptors.
        # Key is the tab descriptor string.
        # We use string as the key because of RPC Call. Client can use the
        # string to locate the tab object.
        # Value is the tab object.
        self._tabs = dict()


    def close(self):
        """Closes Chrome."""
        self._chrome.close()


    def __enter__(self):
        return self


    def __exit__(self, *args):
        self.close()


    @retry_chrome_call
    def get_extension(self, extension_path=None):
        """Gets the extension from the indicated path.

        @param extension_path: the path of the target extension.
                               Set to None to get autotest extension.
                               Defaults to None.
        @return an extension object.

        @raise RuntimeError if the extension is not found.
        @raise chrome.Error if the found extension has not yet been
               retrieved succesfully.

        """
        try:
            if extension_path is None:
                extension = self._chrome.autotest_ext
            else:
                extension = self._chrome.get_extension(extension_path)
        except KeyError, errmsg:
            # Trigger retry_chrome_call to retry to retrieve the
            # found extension.
            raise chrome.Error(errmsg)
        if not extension:
            if extension_path is None:
                raise RuntimeError('Autotest extension not found')
            else:
                raise RuntimeError('Extension not found in %r'
                                    % extension_path)
        return extension


    @retry_chrome_call
    def load_url(self, url):
        """Loads the given url in a new tab. The new tab will be active.

        @param url: The url to load as a string.
        @return a str, the tab descriptor of the opened tab.

        """
        tab = self._browser.tabs.New()
        tab.Navigate(url)
        tab.Activate()
        tab_descriptor = hex(id(tab))
        self._tabs[tab_descriptor] = tab
        return tab_descriptor


    def get_tabs(self):
        """Gets the tabs opened by browser.

        @returns: The tabs attribute in telemetry browser object.

        """
        return self._browser.tabs


    def get_tab_by_descriptor(self, tab_descriptor):
        """Gets the tab by the tab descriptor.

        @returns: The tab object indicated by the tab descriptor.

        """
        return self._tabs[tab_descriptor]


    @retry_chrome_call
    def close_tab(self, tab_descriptor):
        """Closes the tab.

        @param tab_descriptor: Indicate which tab to be closed.

        """
        if tab_descriptor not in self._tabs:
            raise RuntimeError('There is no tab for %s' % tab_descriptor)
        tab = self._tabs[tab_descriptor]
        del self._tabs[tab_descriptor]
        tab.Close()

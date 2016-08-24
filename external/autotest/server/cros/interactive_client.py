# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros import constants
from autotest_lib.server import autotest


class InteractiveClient(object):
    """InteractiveClient represents a remote host for interactive tests.

    An XML-RPC server is deployed to the remote host and a set of methods
    exposed that allow you to open a browser window on that device, write
    output and receive button clicks in order to develop interactive tests.
    """

    XMLRPC_BRINGUP_TIMEOUT_SECONDS = 60

    def __init__(self, client_host):
        """Construct a InteractiveClient.

        @param client_host: host object representing a remote host.

        """
        self._host = client_host
        # Make sure the client library is on the device so that the proxy code
        # is there when we try to call it.
        client_at = autotest.Autotest(self._host)
        client_at.install()
        # Start up the XML-RPC proxy on the client.
        self._proxy = self._host.rpc_server_tracker.xmlrpc_connect(
                constants.INTERACTIVE_XMLRPC_SERVER_COMMAND,
                constants.INTERACTIVE_XMLRPC_SERVER_PORT,
                command_name=
                  constants.INTERACTIVE_XMLRPC_SERVER_CLEANUP_PATTERN,
                ready_test_name=
                  constants.INTERACTIVE_XMLRPC_SERVER_READY_METHOD,
                timeout_seconds=self.XMLRPC_BRINGUP_TIMEOUT_SECONDS)


    def login(self):
        """Login to the system and open a tab.

        The tab opened is used by other methods on this server to interact
        with the user.

        @return True on success, False otherwise.

        """
        return self._proxy.login()


    def set_output(self, html):
        """Replace the contents of the tab.

        @param html: HTML document to replace tab contents with.

        @return True on success, False otherwise.

        """
        return self._proxy.set_output(html)


    def append_output(self, html):
        """Append HTML to the contents of the tab.

        @param html: HTML to append to the existing tab contents.

        @return True on success, False otherwise.

        """
        return self._proxy.append_output(html)


    def append_buttons(self, *args):
        """Append confirmation buttons to the tab.

        Each button is given an index, 0 for the first button, 1 for the second,
        and so on.

        @param title...: Title of button to append.

        @return True on success, False otherwise.

        """
        return self._proxy.append_buttons(*args)


    def wait_for_button(self, timeout):
        """Wait for a button to be clicked.

        Call append_buttons() before this to add buttons to the document.

        @param timeout: Maximum time, in seconds, to wait for a click.

        @return index of button that was clicked, or -1 on timeout.

        """
        return self._proxy.wait_for_button(timeout)


    def check_for_button(self):
        """Check whether a button has been clicked.

        Call append_buttons() before this to add buttons to the document.

        @return index of button that was clicked or -1 if no button
            has been clicked.

        """
        return self._proxy.check_for_button()


    def append_list(self, name):
        """Append a results list to the contents of the tab.

        @param name: Name to use for making modifications to the list.

        @return True.

        """
        return self._proxy.append_list(name)


    def append_list_item(self, list_name, item_name, html):
        """Append an item to a results list.

        @param list_name: Name of list provided to append_list().
        @param item_name: Name to use for making modifications to the item.
        @param html: HTML to place in the list item.

        @return True.

        """
        return self._proxy.append_list_item(list_name, item_name, html)


    def replace_list_item(self, item_name, html):
        """Replace an item in a results list.

        @param item_name: Name of item provided to append_list_item().
        @param html: HTML to place in the list item.

        @return True.

        """
        return self._proxy.replace_list_item(item_name, html)


    def close(self):
        """Tear down state associated with the client."""
        # Log out the browser.
        self._proxy.close()
        # This does not close the host because it's shared with the client.

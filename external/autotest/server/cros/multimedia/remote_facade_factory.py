# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import logging
import socket
import xmlrpclib
import pprint
import sys

from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants
from autotest_lib.server import autotest
from autotest_lib.server.cros.multimedia import audio_facade_adapter
from autotest_lib.server.cros.multimedia import browser_facade_adapter
from autotest_lib.server.cros.multimedia import display_facade_adapter
from autotest_lib.server.cros.multimedia import system_facade_adapter
from autotest_lib.server.cros.multimedia import usb_facade_adapter


class _Method:
    """Class to save the name of the RPC method instead of the real object.

    It keeps the name of the RPC method locally first such that the RPC method
    can be evalulated to a real object while it is called. Its purpose is to
    refer to the latest RPC proxy as the original previous-saved RPC proxy may
    be lost due to reboot.

    The call_method is the method which does refer to the latest RPC proxy.
    """
    def __init__(self, call_method, name):
        self.__call_method = call_method
        self.__name = name

    def __getattr__(self, name):
        # Support a nested method.
        return _Method(self.__call_method, "%s.%s" % (self.__name, name))

    def __call__(self, *args, **dargs):
        return self.__call_method(self.__name, *args, **dargs)


class RemoteFacadeProxy(object):
    """An abstraction of XML RPC proxy to the DUT multimedia server.

    The traditional XML RPC server proxy is static. It is lost when DUT
    reboots. This class reconnects the server again when it finds the
    connection is lost.

    """

    XMLRPC_CONNECT_TIMEOUT = 60
    XMLRPC_RETRY_TIMEOUT = 180
    XMLRPC_RETRY_DELAY = 10

    def __init__(self, host):
        """Construct a RemoteFacadeProxy.

        @param host: Host object representing a remote host.
        """
        self._client = host
        self._xmlrpc_proxy = None
        self.connect(reconnect=False)


    def __getattr__(self, name):
        """Return a _Method object only, not its real object."""
        return _Method(self.__call_proxy, name)


    def __call_proxy(self, name, *args, **dargs):
        """Make the call on the latest RPC proxy object.

        This method gets the internal method of the RPC proxy and calls it.

        @param name: Name of the RPC method, a nested method supported.
        @param args: The rest of arguments.
        @param dargs: The rest of dict-type arguments.
        @return: The return value of the RPC method.
        """
        try:
            # TODO(ihf): This logs all traffic from server to client. Make the spew optional.
            rpc = (
                '%s(%s, %s)' %
                (pprint.pformat(name), pprint.pformat(args),
                 pprint.pformat(dargs)))
            try:
                value = getattr(self._xmlrpc_proxy, name)(*args, **dargs)
                if type(value) is str and value.startswith('Traceback'):
                    raise Exception('RPC error: %s\n%s' % (name, value))
                logging.info('RPC %s returns %s.', rpc, pprint.pformat(value))
                return value
            except (socket.error,
                    xmlrpclib.ProtocolError,
                    httplib.BadStatusLine):
                # Reconnect the RPC server in case connection lost, e.g. reboot.
                self.connect(reconnect=True)
                # Try again.
                logging.warning('Retrying RPC %s.', rpc)
                value = getattr(self._xmlrpc_proxy, name)(*args, **dargs)
                if type(value) is str and value.startswith('Traceback'):
                    raise Exception('RPC error: %s\n%s' % (name, value))
                logging.info('RPC %s returns %s.', rpc, pprint.pformat(value))
                return value
        except:
            logging.error(
                'Failed RPC %s with status [%s].', rpc, sys.exc_info()[0])
            raise


    def connect(self, reconnect):
        """Connects the XML-RPC proxy on the client.

        @param reconnect: True for reconnection, False for the first-time.
        """
        @retry.retry((socket.error,
                      xmlrpclib.ProtocolError,
                      httplib.BadStatusLine),
                      timeout_min=self.XMLRPC_RETRY_TIMEOUT / 60.0,
                      delay_sec=self.XMLRPC_RETRY_DELAY)
        def connect_with_retries(reconnect):
            """Connects the XML-RPC proxy with retries.

            @param reconnect: True for reconnection, False for the first-time.
            """
            if reconnect:
                command = constants.MULTIMEDIA_XMLRPC_SERVER_RESTART_COMMAND
            else:
                command = constants.MULTIMEDIA_XMLRPC_SERVER_COMMAND

            self._xmlrpc_proxy = self._client.rpc_server_tracker.xmlrpc_connect(
                    command,
                    constants.MULTIMEDIA_XMLRPC_SERVER_PORT,
                    command_name=(
                        constants.MULTIMEDIA_XMLRPC_SERVER_CLEANUP_PATTERN
                    ),
                    ready_test_name=(
                        constants.MULTIMEDIA_XMLRPC_SERVER_READY_METHOD),
                    timeout_seconds=self.XMLRPC_CONNECT_TIMEOUT,
                    logfile=constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE)

        logging.info('Setup the connection to RPC server, with retries...')
        connect_with_retries(reconnect)


    def __del__(self):
        """Destructor of RemoteFacadeFactory."""
        self._client.rpc_server_tracker.disconnect(
                constants.MULTIMEDIA_XMLRPC_SERVER_PORT)


class RemoteFacadeFactory(object):
    """A factory to generate remote multimedia facades.

    The facade objects are remote-wrappers to access the DUT multimedia
    functionality, like display, video, and audio.

    """

    def __init__(self, host):
        """Construct a RemoteFacadeFactory.

        @param host: Host object representing a remote host.
        """
        self._client = host
        # Make sure the client library is on the device so that the proxy code
        # is there when we try to call it.
        client_at = autotest.Autotest(self._client)
        client_at.install()
        self._proxy = RemoteFacadeProxy(self._client)


    def ready(self):
        """Returns the proxy ready status"""
        return self._proxy.ready()


    def create_audio_facade(self):
        """Creates an audio facade object."""
        return audio_facade_adapter.AudioFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_display_facade(self):
        """Creates a display facade object."""
        return display_facade_adapter.DisplayFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_system_facade(self):
        """Creates a system facade object."""
        return system_facade_adapter.SystemFacadeRemoteAdapter(
                self._client, self._proxy)


    def create_usb_facade(self):
        """"Creates a USB facade object."""
        return usb_facade_adapter.USBFacadeRemoteAdapter(self._proxy)


    def create_browser_facade(self):
        """"Creates a browser facade object."""
        return browser_facade_adapter.BrowserFacadeRemoteAdapter(self._proxy)

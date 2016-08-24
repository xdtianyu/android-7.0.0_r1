# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import httplib
import socket
import time
import xmlrpclib

from autotest_lib.client.cros.faft.config import Config as ClientConfig
from autotest_lib.server import autotest


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


class RPCProxy(object):
    """Proxy to the FAFTClient RPC server on DUT.

    It acts as a proxy to the FAFTClient on DUT. It is smart enough to:
     - postpone the RPC connection to the first class method call;
     - reconnect to the RPC server in case connection lost, e.g. reboot;
     - always call the latest RPC proxy object.
    """
    _client_config = ClientConfig()

    def __init__(self, host):
        """Constructor.

        @param host: The host object, passed via the test control file.
        """
        self._client = host
        self._faft_client = None

    def __del__(self):
        self.disconnect()

    def __getattr__(self, name):
        """Return a _Method object only, not its real object."""
        return _Method(self.__call_faft_client, name)

    def __call_faft_client(self, name, *args, **dargs):
        """Make the call on the latest RPC proxy object.

        This method gets the internal method of the RPC proxy and calls it.

        @param name: Name of the RPC method, a nested method supported.
        @param args: The rest of arguments.
        @param dargs: The rest of dict-type arguments.
        @return: The return value of the FAFTClient RPC method.
        """
        try:
            return getattr(self._faft_client, name)(*args, **dargs)
        except (AttributeError,  # _faft_client not initialized, still None
                socket.error,
                httplib.BadStatusLine,
                xmlrpclib.ProtocolError):
            # Reconnect the RPC server in case connection lost, e.g. reboot.
            self.connect()
            # Try again.
            return getattr(self._faft_client, name)(*args, **dargs)

    def connect(self):
        """Connect the RPC server."""
        # Make sure Autotest dependency is there.
        autotest.Autotest(self._client).install()
        self._faft_client = self._client.rpc_server_tracker.xmlrpc_connect(
                self._client_config.rpc_command,
                self._client_config.rpc_port,
                command_name=self._client_config.rpc_command_short,
                ready_test_name=self._client_config.rpc_ready_call,
                timeout_seconds=self._client_config.rpc_timeout,
                logfile="%s.%s" % (self._client_config.rpc_logfile,
                                   time.time())
                )

    def disconnect(self):
        """Disconnect the RPC server."""
        self._client.rpc_server_tracker.disconnect(self._client_config.rpc_port)
        self._faft_client = None

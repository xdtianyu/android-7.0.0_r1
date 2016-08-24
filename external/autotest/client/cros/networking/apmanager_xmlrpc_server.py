#!/usr/bin/python

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import logging.handlers

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants
from autotest_lib.client.cros import xmlrpc_server
from autotest_lib.client.cros.networking import apmanager_proxy


class ApmanagerXmlRpcDelegate(xmlrpc_server.XmlRpcDelegate):
    """Exposes methods called remotely during APManager autotests.

    All instance methods of this object without a preceding '_' are exposed via
    an XMLRPC server.  This is not a stateless handler object, which means that
    if you store state inside the delegate, that state will remain around for
    future calls.

    """


    def __init__(self):
        self._apmanager_proxy = apmanager_proxy.ApmanagerProxy()


    def __enter__(self):
        super(ApmanagerXmlRpcDelegate, self).__enter__()


    def __exit__(self, exception, value, traceback):
        super(ApmanagerXmlRpcDelegate, self).__exit__(exception, value, traceback)


    @xmlrpc_server.dbus_safe(None)
    def start_service(self, config_params):
        """Create/start an AP service.

        @param config_params dictionary of configuration parameters.
        @return string object path for the AP service.

        """
        return self._apmanager_proxy.start_service(config_params)


    def terminate_service(self, service):
        """Remove/terminate an AP service.

        @param service string object path of the AP service.

        """
        self._apmanager_proxy.terminate_service(service)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    handler = logging.handlers.SysLogHandler(address='/dev/log')
    formatter = logging.Formatter(
            'apmanager_xmlrpc_server: [%(levelname)s] %(message)s')
    handler.setFormatter(formatter)
    logging.getLogger().addHandler(handler)
    logging.debug('apmanager_xmlrpc_server main...')
    server = xmlrpc_server.XmlRpcServer('localhost',
                                         constants.APMANAGER_XMLRPC_SERVER_PORT)
    if server is None:
        raise error.TestFail('Failed to setup xmlrpc server for apmanager')
    else:
        logging.debug('Server setup')
    server.register_delegate(ApmanagerXmlRpcDelegate())
    server.run()

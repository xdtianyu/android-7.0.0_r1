# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros.network import apmanager_constants
from autotest_lib.client.cros import constants
from autotest_lib.server import autotest
from autotest_lib.server.cros.network import hostap_config

XMLRPC_BRINGUP_TIMEOUT_SECONDS = 60

def get_xmlrpc_proxy(host):
    """Get an apmanager XMLRPC proxy for |host|.

    @param host: host object representing a remote device.
    @return proxy object for remote XMLRPC server.

    """
    # Make sure the client library on the device is up-to-date.
    client_at = autotest.Autotest(host)
    client_at.install()
    # Start up the XMLRPC proxy on the device.
    proxy = host.rcp_server_tracker.xmlrpc_connect(
            constants.APMANAGER_XMLRPC_SERVER_COMMAND,
            constants.APMANAGER_XMLRPC_SERVER_PORT,
            command_name=constants.APMANAGER_XMLRPC_SERVER_CLEANUP_PATTERN,
            ready_test_name=constants.APMANAGER_XMLRPC_SERVER_READY_METHOD,
            timeout_seconds=XMLRPC_BRINGUP_TIMEOUT_SECONDS)
    return proxy


class ApmanagerServiceProvider(object):
    """Provide AP service using apmanager."""

    XMLRPC_BRINGUP_TIMEOUT_SECONDS = 60
    APMANAGER_DEFAULT_CHANNEL = 6

    def __init__(self, linux_system, config_params):
        """
        @param linux_system SiteLinuxSystem machine to setup AP on.
        @param config_params dictionary of configuration parameters.
        """
        self._linux_system = linux_system
        self._config_params = config_params
        self._xmlrpc_server = None
        self._service = None


    def __enter__(self):
        # Create a managed mode interface to start the AP on. Autotest removes
        # all wifi interfaces before and after each test in SiteLinuxSystem.
        channel = apmanager_constants.DEFAULT_CHANNEL_NUMBER
        if apmanager_constants.CONFIG_CHANNEL in self._config_params:
            channel = int(
                    self._config_params[apmanager_constants.CONFIG_CHANNEL])
        self._linux_system.get_wlanif(
                hostap_config.HostapConfig.get_frequency_for_channel(
                        channel),
                'managed')
        self._xmlrpc_server = get_xmlrpc_proxy(self._linux_system.host)
        self._service = self._xmlrpc_server.start_service(self._config_params)


    def __exit__(self, exception, value, traceback):
        if self._service is not None:
            self._xmlrpc_server.terminate_service(self._service)

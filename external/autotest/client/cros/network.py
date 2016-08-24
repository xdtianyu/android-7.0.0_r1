# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import logging
import socket
import time
import urllib2

import common

# Import 'flimflam_test_path' first in order to import 'routing'.
# Disable warning about flimflam_test_path not being used since it is used
# to find routing but not explicitly used as a module.
# pylint: disable-msg=W0611
import flimflam_test_path
import routing

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cellular import mm


def _Bug24628WorkaroundEnable(modem):
    """Enables a modem.  Try again if a SerialResponseTimeout is received."""
    # http://code.google.com/p/chromium-os/issues/detail?id=24628
    tries = 5
    while tries > 0:
        try:
            modem.Enable(True)
            return
        except dbus.exceptions.DBusException, e:
            logging.error('Enable failed: %s', e)
            tries -= 1
            if tries > 0:
                logging.error('_Bug24628WorkaroundEnable:  sleeping')
                time.sleep(6)
                logging.error('_Bug24628WorkaroundEnable:  retrying')
            else:
                raise


# TODO(rochberg):  Move modem-specific functions to cellular/cell_utils
def ResetAllModems(conn_mgr):
    """
    Disables/Enables cycle all modems to ensure valid starting state.

    @param conn_mgr: Connection manager (shill)
    """
    service = conn_mgr.FindCellularService()
    if not service:
        conn_mgr.EnableTechnology('cellular')
        service = conn_mgr.FindCellularService()

    logging.info('ResetAllModems: found service %s', service)

    try:
        if service:
            service.SetProperty('AutoConnect', False),
    except dbus.exceptions.DBusException, e:
        # The service object may disappear, we can safely ignore it.
        if e._dbus_error_name != 'org.freedesktop.DBus.Error.UnknownMethod':
            raise

    for manager, path in mm.EnumerateDevices():
        modem = manager.GetModem(path)
        version = modem.GetVersion()
        # Icera modems behave weirdly if we cancel the operation while the
        # modem is connecting or disconnecting. Work around the issue by waiting
        # until the connect/disconnect operation completes.
        # TODO(benchan): Remove this workaround once the issue is addressed
        # on the modem side.
        utils.poll_for_condition(
            lambda: not modem.IsConnectingOrDisconnecting(),
            exception=utils.TimeoutError('Timed out waiting for modem to ' +
                                         'finish connecting/disconnecting'),
            sleep_interval=1,
            timeout=30)
        modem.Enable(False)
        # Although we disable at the ModemManager level, we need to wait for
        # shill to process the disable to ensure the modem is in a stable state
        # before continuing else we may end up trying to enable a modem that
        # is still in the process of disabling.
        cm_device = conn_mgr.FindElementByPropertySubstring('Device',
                                                            'DBus.Object',
                                                            path)
        utils.poll_for_condition(
            lambda: not cm_device.GetProperties()['Powered'],
            exception=utils.TimeoutError(
                'Timed out waiting for shill device disable'),
            sleep_interval=1,
            timeout=30)
        assert modem.IsDisabled()

        if 'Y3300XXKB1' in version:
            _Bug24628WorkaroundEnable(modem)
        else:
            modem.Enable(True)
            # Wait for shill to process the enable for the same reason as
            # above (during disable).
            utils.poll_for_condition(
                lambda: cm_device.GetProperties()['Powered'],
                exception=utils.TimeoutError(
                    'Timed out waiting for shill device enable'),
                sleep_interval=1,
                timeout=30)
            assert modem.IsEnabled()


class IpTablesContext(object):
    """Context manager that manages iptables rules."""
    IPTABLES = '/sbin/iptables'

    def __init__(self, initial_allowed_host=None):
        self.initial_allowed_host = initial_allowed_host
        self.rules = []

    def _IpTables(self, command):
        # Run, log, return output
        return utils.system_output('%s %s' % (self.IPTABLES, command),
                                   retain_output=True)

    def _RemoveRule(self, rule):
        self._IpTables('-D ' + rule)
        self.rules.remove(rule)

    def AllowHost(self, host):
        """
        Allows the specified host through the firewall.

        @param host: Name of host to allow
        """
        for proto in ['tcp', 'udp']:
            rule = 'INPUT -s %s/32 -p %s -m %s -j ACCEPT' % (host, proto, proto)
            output = self._IpTables('-S INPUT')
            current = [x.rstrip() for x in output.splitlines()]
            logging.error('current: %s', current)
            if '-A ' + rule in current:
                # Already have the rule
                logging.info('Not adding redundant %s', rule)
                continue
            self._IpTables('-A '+ rule)
            self.rules.append(rule)

    def _CleanupRules(self):
        for rule in self.rules:
            self._RemoveRule(rule)

    def __enter__(self):
        if self.initial_allowed_host:
            self.AllowHost(self.initial_allowed_host)
        return self

    def __exit__(self, exception, value, traceback):
        self._CleanupRules()
        return False


def NameServersForService(conn_mgr, service):
    """
    Returns the list of name servers used by a connected service.

    @param conn_mgr: Connection manager (shill)
    @param service: Name of the connected service
    @return: List of name servers used by |service|
    """
    service_properties = service.GetProperties(utf8_strings=True)
    device_path = service_properties['Device']
    device = conn_mgr.GetObjectInterface('Device', device_path)
    if device is None:
        logging.error('No device for service %s', service)
        return []

    properties = device.GetProperties(utf8_strings=True)

    hosts = []
    for path in properties['IPConfigs']:
        ipconfig = conn_mgr.GetObjectInterface('IPConfig', path)
        ipconfig_properties = ipconfig.GetProperties(utf8_strings=True)
        hosts += ipconfig_properties['NameServers']

    logging.info('Name servers: %s', ', '.join(hosts))

    return hosts


def CheckInterfaceForDestination(host, expected_interface):
    """
    Checks that routes for host go through a given interface.

    The concern here is that our network setup may have gone wrong
    and our test connections may go over some other network than
    the one we're trying to test.  So we take all the IP addresses
    for the supplied host and make sure they go through the given
    network interface.

    @param host: Destination host
    @param expected_interface: Expected interface name
    @raises: error.TestFail if the routes for the given host go through
            a different interface than the expected one.

    """
    # addrinfo records: (family, type, proto, canonname, (addr, port))
    server_addresses = [record[4][0]
                        for record in socket.getaddrinfo(host, 80)]

    routes = routing.NetworkRoutes()
    for address in server_addresses:
        interface = routes.getRouteFor(address).interface
        logging.info('interface for %s: %s', address, interface)
        if interface != expected_interface:
            raise error.TestFail('Target server %s uses interface %s'
                                 '(%s expected).' %
                                 (address, interface, expected_interface))


FETCH_URL_PATTERN_FOR_TEST = \
    'http://testing-chargen.appspot.com/download?size=%d'

def FetchUrl(url_pattern, bytes_to_fetch=10, fetch_timeout=10):
    """
    Fetches a specified number of bytes from a URL.

    @param url_pattern: URL pattern for fetching a specified number of bytes.
            %d in the pattern is to be filled in with the number of bytes to
            fetch.
    @param bytes_to_fetch: Number of bytes to fetch.
    @param fetch_timeout: Number of seconds to wait for the fetch to complete
            before it times out.
    @return: The time in seconds spent for fetching the specified number of
            bytes.
    @raises: error.TestError if one of the following happens:
            - The fetch takes no time.
            - The number of bytes fetched differs from the specified
              number.

    """
    # Limit the amount of bytes to read at a time.
    _MAX_FETCH_READ_BYTES = 1024 * 1024

    url = url_pattern % bytes_to_fetch
    logging.info('FetchUrl %s', url)
    start_time = time.time()
    result = urllib2.urlopen(url, timeout=fetch_timeout)
    bytes_fetched = 0
    while bytes_fetched < bytes_to_fetch:
        bytes_left = bytes_to_fetch - bytes_fetched
        bytes_to_read = min(bytes_left, _MAX_FETCH_READ_BYTES)
        bytes_read = len(result.read(bytes_to_read))
        bytes_fetched += bytes_read
        if bytes_read != bytes_to_read:
            raise error.TestError('FetchUrl tried to read %d bytes, but got '
                                  '%d bytes instead.' %
                                  (bytes_to_read, bytes_read))
        fetch_time = time.time() - start_time
        if fetch_time > fetch_timeout:
            raise error.TestError('FetchUrl exceeded timeout.')

    return fetch_time

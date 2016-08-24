# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Encapsulate functionality of the dhcpd Daemon. Support writing out a
configuration file as well as starting and stopping the service.
"""

import os
import signal

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

# Filenames used for execution.
DHCPV6_SERVER_EXECUTABLE = '/usr/local/sbin/dhcpd'
DHCPV6_SERVER_CONFIG_FILE = '/tmp/dhcpv6_test.conf'
DHCPV6_SERVER_PID_FILE = '/tmp/dhcpv6_test.pid'

DHCPV6_SERVER_ADDRESS = '2001:db8:0:1::1'
DHCPV6_SERVER_SUBNET_PREFIX = '2001:db8:0:1::'
DHCPV6_SERVER_SUBNET_PREFIX_LENGTH = 64
DHCPV6_ADDRESS_RANGE_LOW = 0x100
DHCPV6_ADDRESS_RANGE_HIGH = 0x1ff
DHCPV6_PREFIX_DELEGATION_INDEX_LOW = 0x1
DHCPV6_PREFIX_DELEGATION_INDEX_HIGH = 0xf
DHCPV6_PREFIX_DELEGATION_RANGE_FORMAT = '2001:db8:0:%x00::'
DHCPV6_PREFIX_DELEGATION_RANGE_LOW = (DHCPV6_PREFIX_DELEGATION_RANGE_FORMAT %
                                      (DHCPV6_PREFIX_DELEGATION_INDEX_LOW))
DHCPV6_PREFIX_DELEGATION_RANGE_HIGH = (DHCPV6_PREFIX_DELEGATION_RANGE_FORMAT %
                                       (DHCPV6_PREFIX_DELEGATION_INDEX_HIGH))
DHCPV6_PREFIX_DELEGATION_PREFIX_LENGTH = 56
DHCPV6_DEFAULT_LEASE_TIME = 600
DHCPV6_MAX_LEASE_TIME = 7200
DHCPV6_NAME_SERVERS = 'fec0:0:0:1::1'
DHCPV6_DOMAIN_SEARCH = 'domain.example'

CONFIG_DEFAULT_LEASE_TIME = 'default_lease_time'
CONFIG_MAX_LEASE_TIME = 'max_lease_time'
CONFIG_SUBNET = 'subnet'
CONFIG_RANGE = 'range'
CONFIG_NAME_SERVERS = 'name_servers'
CONFIG_DOMAIN_SEARCH = 'domain_search'
CONFIG_PREFIX_RANGE = 'prefix_range'

class Dhcpv6TestServer(object):
    """
    This is an embodiment of the DHCPv6 server (dhcpd) process.  It converts an
    config dict into parameters for the dhcpd configuration file and
    manages startup and cleanup of the process.
    """

    def __init__(self, interface = None):
        if not os.path.exists(DHCPV6_SERVER_EXECUTABLE):
            raise error.TestNAError('Could not find executable %s; '
                                    'this is likely an old version of '
                                    'ChromiumOS' %
                                    DHCPV6_SERVER_EXECUTABLE)
        self._interface = interface
        # "2001:db8:0:1::/64"
        subnet = '%s/%d' % (DHCPV6_SERVER_SUBNET_PREFIX,
                            DHCPV6_SERVER_SUBNET_PREFIX_LENGTH)
        # "2001:db8:0:1::100 2001:db8:1::1ff"
        range = '%s%x %s%x' % (DHCPV6_SERVER_SUBNET_PREFIX,
                               DHCPV6_ADDRESS_RANGE_LOW,
                               DHCPV6_SERVER_SUBNET_PREFIX,
                               DHCPV6_ADDRESS_RANGE_HIGH)
        # "2001:db8:0:100:: 2001:db8:1:f00:: /56"
        prefix_range = '%s %s /%d' % (DHCPV6_PREFIX_DELEGATION_RANGE_LOW,
                                      DHCPV6_PREFIX_DELEGATION_RANGE_HIGH,
                                      DHCPV6_PREFIX_DELEGATION_PREFIX_LENGTH)
        self._config = {
            CONFIG_DEFAULT_LEASE_TIME: DHCPV6_DEFAULT_LEASE_TIME,
            CONFIG_MAX_LEASE_TIME: DHCPV6_MAX_LEASE_TIME,
            CONFIG_SUBNET: subnet,
            CONFIG_RANGE: range,
            CONFIG_NAME_SERVERS: DHCPV6_NAME_SERVERS,
            CONFIG_DOMAIN_SEARCH: DHCPV6_DOMAIN_SEARCH,
            CONFIG_PREFIX_RANGE: prefix_range
        }


    def _write_config_file(self):
        """
        Write out a configuration file for DHCPv6 server to use.
        """
        config = '\n'.join([
                     'default-lease-time %(default_lease_time)d;',
                     'max-lease-time %(max_lease_time)d;',
                     'subnet6 %(subnet)s {',
                     '  range6 %(range)s;',
                     '  option dhcp6.name-servers %(name_servers)s;',
                     '  option dhcp6.domain-search \"%(domain_search)s\";',
                     '  prefix6 %(prefix_range)s;',
                     '}'
                     '']) % self._config
        with open(DHCPV6_SERVER_CONFIG_FILE, 'w') as f:
            f.write(config)


    def _cleanup(self):
        """
        Cleanup temporary files.  If PID file exists, also kill the
        associated process.
        """
        if os.path.exists(DHCPV6_SERVER_PID_FILE):
            pid = int(file(DHCPV6_SERVER_PID_FILE).read())
            os.remove(DHCPV6_SERVER_PID_FILE)
            try:
                os.kill(pid, signal.SIGTERM)
            except OSError:
                pass
        if os.path.exists(DHCPV6_SERVER_CONFIG_FILE):
            os.remove(DHCPV6_SERVER_CONFIG_FILE)


    def start(self):
        """
        Start the DHCPv6 server.  The server will daemonize itself and
        run in the background.
        """
        self._cleanup()
        self._write_config_file()
        utils.system('%s -6 -pf %s -cf %s %s' %
                     (DHCPV6_SERVER_EXECUTABLE,
                      DHCPV6_SERVER_PID_FILE,
                      DHCPV6_SERVER_CONFIG_FILE,
                      self._interface))


    def stop(self):
        """
        Halt the DHCPv6 server.
        """
        self._cleanup()

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Encapsulate functionality of the Linux IPv6 Router Advertisement Daemon.
Support writing out a configuration file as well as starting and stopping
the service.
"""

import os
import signal

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

# Filenames used for execution.
RADVD_EXECUTABLE = '/usr/local/sbin/radvd'
RADVD_CONFIG_FILE = '/tmp/radvd_test.conf'
RADVD_PID_FILE = '/tmp/radvd_test.pid'

# These are default configuration values.
RADVD_DEFAULT_ADV_ON_LINK = 'on'
RADVD_DEFAULT_ADV_AUTONOMOUS = 'on'
RADVD_DEFAULT_ADV_ROUTER_ADDR = 'on'
RADVD_DEFAULT_ADV_RDNSS_LIFETIME = 'infinity'
RADVD_DEFAULT_DNSSL_LIST = 'a.com b.com'
RADVD_DEFAULT_MAX_ADV_INTERVAL = 10
RADVD_DEFAULT_MIN_ADV_INTERVAL = 3
RADVD_DEFAULT_SEND_ADVERT = 'on'

# The addresses below are within the  2001:0db8/32 "documentation only" prefix
# (RFC3849), which is guaranteed never to be assigned to a real network.
RADVD_DEFAULT_SUFFIX = '/64'
RADVD_DEFAULT_PREFIX = '2001:db8:100:f101::/64'
RADVD_DEFAULT_RDNSS_SERVERS = ( '2001:db8:100:f101::1 '
                                '2001:db8:100:f101::2' )

# Option names.
OPTION_ADV_ON_LINK = 'adv_on_link'
OPTION_ADV_AUTONOMOUS = 'adv_autonomous'
OPTION_ADV_ROUTER_ADDR = 'adv_router_addr'
OPTION_ADV_RDNSS_LIFETIME = 'adv_rdnss_lifetime'
OPTION_DNSSL_LIST = 'dnssl_list'
OPTION_INTERFACE = 'interface'
OPTION_MAX_ADV_INTERVAL = 'max_adv_interval'
OPTION_MIN_ADV_INTERVAL = 'min_adv_interval'
OPTION_PREFIX = 'prefix'
OPTION_RDNSS_SERVERS = 'rdnss_servers'
OPTION_SEND_ADVERT = 'adv_send_advert'

class RadvdServer(object):
    """
    This is an embodiment of the radvd server process.  It converts an
    option dict into parameters for the radvd configuration file and
    manages startup and cleanup of the process.
    """

    def __init__(self, interface = None):
        if not os.path.exists(RADVD_EXECUTABLE):
            raise error.TestNAError('Could not find executable %s; '
                                    'this is likely an old version of '
                                    'ChromiumOS' %
                                    RADVD_EXECUTABLE)
        self._options = {
            OPTION_INTERFACE: interface,
            OPTION_ADV_ON_LINK: RADVD_DEFAULT_ADV_ON_LINK,
            OPTION_ADV_AUTONOMOUS: RADVD_DEFAULT_ADV_AUTONOMOUS,
            OPTION_ADV_ROUTER_ADDR: RADVD_DEFAULT_ADV_ROUTER_ADDR,
            OPTION_ADV_RDNSS_LIFETIME: RADVD_DEFAULT_ADV_RDNSS_LIFETIME,
            OPTION_DNSSL_LIST: RADVD_DEFAULT_DNSSL_LIST,
            OPTION_MAX_ADV_INTERVAL: RADVD_DEFAULT_MAX_ADV_INTERVAL,
            OPTION_MIN_ADV_INTERVAL: RADVD_DEFAULT_MIN_ADV_INTERVAL,
            OPTION_PREFIX: RADVD_DEFAULT_PREFIX,
            OPTION_RDNSS_SERVERS: RADVD_DEFAULT_RDNSS_SERVERS,
            OPTION_SEND_ADVERT: RADVD_DEFAULT_SEND_ADVERT
        }

    @property
    def options(self):
        """
        Property dict used to generate configuration file.
        """
        return self._options

    def _write_config_file(self):
        """
        Write out a configuration file for radvd to use.
        """
        config = '\n'.join([
                     'interface %(interface)s {',
                     '  AdvSendAdvert %(adv_send_advert)s;',
                     '  MinRtrAdvInterval %(min_adv_interval)d;',
                     '  MaxRtrAdvInterval %(max_adv_interval)d;',
                     '  prefix %(prefix)s {',
                     '    AdvOnLink %(adv_on_link)s;',
                     '    AdvAutonomous %(adv_autonomous)s;',
                     '    AdvRouterAddr %(adv_router_addr)s;',
                     '  };',
                     '  RDNSS %(rdnss_servers)s {',
                     '    AdvRDNSSLifetime %(adv_rdnss_lifetime)s;',
                     '  };',
                     '  DNSSL %(dnssl_list)s {',
                     '  };',
                     '};',
                     '']) % self.options
        with open(RADVD_CONFIG_FILE, 'w') as f:
            f.write(config)

    def _cleanup(self):
        """
        Cleanup temporary files.  If PID file exists, also kill the
        associated process.
        """
        if os.path.exists(RADVD_PID_FILE):
            pid = int(file(RADVD_PID_FILE).read())
            os.remove(RADVD_PID_FILE)
            try:
                os.kill(pid, signal.SIGTERM)
            except OSError:
                pass
        if os.path.exists(RADVD_CONFIG_FILE):
            os.remove(RADVD_CONFIG_FILE)

    def start_server(self):
        """
        Start the radvd server.  The server will daemonize itself and
        run in the background.
        """
        self._cleanup()
        self._write_config_file()
        utils.system('%s -p %s -C %s' %
                     (RADVD_EXECUTABLE, RADVD_PID_FILE, RADVD_CONFIG_FILE))

    def stop_server(self):
        """
        Halt the radvd server.
        """
        self._cleanup()

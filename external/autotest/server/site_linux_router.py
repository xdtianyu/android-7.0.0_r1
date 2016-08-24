# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import logging
import random
import string
import tempfile
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import path_utils
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.common_lib.cros.network import netblock
from autotest_lib.client.common_lib.cros.network import ping_runner
from autotest_lib.server import hosts
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros import dnsname_mangler
from autotest_lib.server.cros.network import hostap_config


StationInstance = collections.namedtuple('StationInstance',
                                         ['ssid', 'interface', 'dev_type'])
HostapdInstance = collections.namedtuple('HostapdInstance',
                                         ['ssid', 'conf_file', 'log_file',
                                          'interface', 'config_dict',
                                          'stderr_log_file',
                                          'scenario_name'])

# Send magic packets here, so they can wake up the system but are otherwise
# dropped.
UDP_DISCARD_PORT = 9

def build_router_hostname(client_hostname=None, router_hostname=None):
    """Build a router hostname from a client hostname.

    @param client_hostname: string hostname of DUT connected to a router.
    @param router_hostname: string hostname of router.
    @return string hostname of connected router or None if the hostname
            cannot be inferred from the client hostname.

    """
    if not router_hostname and not client_hostname:
        raise error.TestError('Either client_hostname or router_hostname must '
                              'be specified to build_router_hostname.')

    return dnsname_mangler.get_router_addr(client_hostname,
                                           cmdline_override=router_hostname)


def build_router_proxy(test_name='', client_hostname=None, router_addr=None,
                       enable_avahi=False):
    """Build up a LinuxRouter object.

    Verifies that the remote host responds to ping.
    Either client_hostname or router_addr must be specified.

    @param test_name: string name of this test (e.g. 'network_WiFi_TestName').
    @param client_hostname: string hostname of DUT if we're in the lab.
    @param router_addr: string DNS/IPv4 address to use for router host object.
    @param enable_avahi: boolean True iff avahi should be started on the router.

    @return LinuxRouter or raise error.TestError on failure.

    """
    router_hostname = build_router_hostname(client_hostname=client_hostname,
                                            router_hostname=router_addr)
    logging.info('Connecting to router at %s', router_hostname)
    ping_helper = ping_runner.PingRunner()
    if not ping_helper.simple_ping(router_hostname):
        raise error.TestError('Router at %s is not pingable.' %
                              router_hostname)

    return LinuxRouter(hosts.create_host(router_hostname), test_name,
                       enable_avahi=enable_avahi)


class LinuxRouter(site_linux_system.LinuxSystem):
    """Linux/mac80211-style WiFi Router support for WiFiTest class.

    This class implements test methods/steps that communicate with a
    router implemented with Linux/mac80211.  The router must
    be pre-configured to enable ssh access and have a mac80211-based
    wireless device.  We also assume hostapd 0.7.x and iw are present
    and any necessary modules are pre-loaded.

    """

    KNOWN_TEST_PREFIX = 'network_WiFi_'
    POLLING_INTERVAL_SECONDS = 0.5
    STARTUP_TIMEOUT_SECONDS = 10
    SUFFIX_LETTERS = string.ascii_lowercase + string.digits
    SUBNET_PREFIX_OCTETS = (192, 168)

    HOSTAPD_CONF_FILE_PATTERN = '/tmp/hostapd-test-%s.conf'
    HOSTAPD_LOG_FILE_PATTERN = '/tmp/hostapd-test-%s.log'
    HOSTAPD_STDERR_LOG_FILE_PATTERN = '/tmp/hostapd-stderr-test-%s.log'
    HOSTAPD_CONTROL_INTERFACE_PATTERN = '/tmp/hostapd-test-%s.ctrl'
    HOSTAPD_DRIVER_NAME = 'nl80211'

    STATION_CONF_FILE_PATTERN = '/tmp/wpa-supplicant-test-%s.conf'
    STATION_LOG_FILE_PATTERN = '/tmp/wpa-supplicant-test-%s.log'
    STATION_PID_FILE_PATTERN = '/tmp/wpa-supplicant-test-%s.pid'

    MGMT_FRAME_SENDER_LOG_FILE = '/tmp/send_management_frame-test.log'

    PROBE_RESPONSE_FOOTER_FILE = '/tmp/autotest-probe_response_footer'

    def get_capabilities(self):
        """@return iterable object of AP capabilities for this system."""
        caps = set()
        try:
            self.cmd_send_management_frame = path_utils.must_be_installed(
                    '/usr/bin/send_management_frame', host=self.host)
            caps.add(self.CAPABILITY_SEND_MANAGEMENT_FRAME)
        except error.TestFail:
            pass
        return super(LinuxRouter, self).get_capabilities().union(caps)


    @property
    def router(self):
        """Deprecated.  Use self.host instead.

        @return Host object representing the remote router.

        """
        return self.host


    @property
    def wifi_ip(self):
        """Simple accessor for the WiFi IP when there is only one AP.

        @return string IP of WiFi interface.

        """
        if len(self.local_servers) != 1:
            raise error.TestError('Could not pick a WiFi IP to return.')

        return self.get_wifi_ip(0)


    def __init__(self, host, test_name, enable_avahi=False):
        """Build a LinuxRouter.

        @param host Host object representing the remote machine.
        @param test_name string name of this test.  Used in SSID creation.
        @param enable_avahi: boolean True iff avahi should be started on the
                router.

        """
        super(LinuxRouter, self).__init__(host, 'router')
        self._ssid_prefix = test_name
        self._enable_avahi = enable_avahi
        self.__setup()


    def __setup(self):
        """Set up this system.

        Can be used either to complete initialization of a LinuxRouter
        object, or to re-establish a good state after a reboot.

        """
        self.cmd_dhcpd = '/usr/sbin/dhcpd'
        self.cmd_hostapd = path_utils.must_be_installed(
                '/usr/sbin/hostapd', host=self.host)
        self.cmd_hostapd_cli = path_utils.must_be_installed(
                '/usr/sbin/hostapd_cli', host=self.host)
        self.cmd_wpa_supplicant = path_utils.must_be_installed(
                '/usr/sbin/wpa_supplicant', host=self.host)
        self.dhcpd_conf = '/tmp/dhcpd.%s.conf'
        self.dhcpd_leases = '/tmp/dhcpd.leases'

        # Log the most recent message on the router so that we can rebuild the
        # suffix relevant to us when debugging failures.
        last_log_line = self.host.run('tail -1 /var/log/messages').stdout
        # We're trying to get the timestamp from:
        # 2014-07-23T17:29:34.961056+00:00 localhost kernel: blah blah blah
        self._log_start_timestamp = last_log_line.strip().split(None, 2)[0]
        logging.debug('Will only retrieve logs after %s.',
                      self._log_start_timestamp)

        # hostapd configuration persists throughout the test, subsequent
        # 'config' commands only modify it.
        if self._ssid_prefix.startswith(self.KNOWN_TEST_PREFIX):
            # Many of our tests start with an uninteresting prefix.
            # Remove it so we can have more unique bytes.
            self._ssid_prefix = self._ssid_prefix[len(self.KNOWN_TEST_PREFIX):]
        self._number_unique_ssids = 0

        self._total_hostapd_instances = 0
        self.local_servers = []
        self.server_address_index = []
        self.hostapd_instances = []
        self.station_instances = []
        self.dhcp_low = 1
        self.dhcp_high = 128

        # Kill hostapd and dhcp server if already running.
        self._kill_process_instance('hostapd', timeout_seconds=30)
        self.stop_dhcp_server(instance=None)

        # Place us in the US by default
        self.iw_runner.set_regulatory_domain('US')

        self.enable_all_antennas()

        # Some tests want this functionality, but otherwise, it's a distraction.
        if self._enable_avahi:
            self.host.run('start avahi', ignore_status=True)
        else:
            self.host.run('stop avahi', ignore_status=True)


    def close(self):
        """Close global resources held by this system."""
        self.deconfig()
        # dnsmasq and hostapd cause interesting events to go to system logs.
        # Retrieve only the suffix of the logs after the timestamp we stored on
        # router creation.
        self.host.run("sed -n -e '/%s/,$p' /var/log/messages >/tmp/router_log" %
                      self._log_start_timestamp, ignore_status=True)
        self.host.get_file('/tmp/router_log', 'debug/router_host_messages')
        super(LinuxRouter, self).close()


    def reboot(self, timeout):
        """Reboot this router, and restore it to a known-good state.

        @param timeout Maximum seconds to wait for router to return.

        """
        super(LinuxRouter, self).reboot(timeout)
        self.__setup()


    def has_local_server(self):
        """@return True iff this router has local servers configured."""
        return bool(self.local_servers)


    def start_hostapd(self, configuration):
        """Start a hostapd instance described by conf.

        @param configuration HostapConfig object.

        """
        # Figure out the correct interface.
        if configuration.min_streams is None:
            interface = self.get_wlanif(configuration.frequency, 'managed')
        else:
            interface = self.get_wlanif(
                configuration.frequency, 'managed', configuration.min_streams)

        conf_file = self.HOSTAPD_CONF_FILE_PATTERN % interface
        log_file = self.HOSTAPD_LOG_FILE_PATTERN % interface
        stderr_log_file = self.HOSTAPD_STDERR_LOG_FILE_PATTERN % interface
        control_interface = self.HOSTAPD_CONTROL_INTERFACE_PATTERN % interface
        hostapd_conf_dict = configuration.generate_dict(
                interface, control_interface,
                self.build_unique_ssid(suffix=configuration.ssid_suffix))
        logging.debug('hostapd parameters: %r', hostapd_conf_dict)

        # Generate hostapd.conf.
        self.router.run("cat <<EOF >%s\n%s\nEOF\n" %
            (conf_file, '\n'.join(
            "%s=%s" % kv for kv in hostapd_conf_dict.iteritems())))

        # Run hostapd.
        logging.info('Starting hostapd on %s(%s) channel=%s...',
                     interface, self.iw_runner.get_interface(interface).phy,
                     configuration.channel)
        self.router.run('rm %s' % log_file, ignore_status=True)
        self.router.run('stop wpasupplicant', ignore_status=True)
        start_command = '%s -dd -t %s > %s 2> %s & echo $!' % (
                self.cmd_hostapd, conf_file, log_file, stderr_log_file)
        pid = int(self.router.run(start_command).stdout.strip())
        self.hostapd_instances.append(HostapdInstance(
                hostapd_conf_dict['ssid'],
                conf_file,
                log_file,
                interface,
                hostapd_conf_dict.copy(),
                stderr_log_file,
                configuration.scenario_name))

        # Wait for confirmation that the router came up.
        logging.info('Waiting for hostapd to startup.')
        start_time = time.time()
        while time.time() - start_time < self.STARTUP_TIMEOUT_SECONDS:
            success = self.router.run(
                    'grep "Setup of interface done" %s' % log_file,
                    ignore_status=True).exit_status == 0
            if success:
                break

            # A common failure is an invalid router configuration.
            # Detect this and exit early if we see it.
            bad_config = self.router.run(
                    'grep "Interface initialization failed" %s' % log_file,
                    ignore_status=True).exit_status == 0
            if bad_config:
                raise error.TestFail('hostapd failed to initialize AP '
                                     'interface.')

            if pid:
                early_exit = self.router.run('kill -0 %d' % pid,
                                             ignore_status=True).exit_status
                if early_exit:
                    raise error.TestFail('hostapd process terminated.')

            time.sleep(self.POLLING_INTERVAL_SECONDS)
        else:
            raise error.TestFail('Timed out while waiting for hostapd '
                                 'to start.')


    def _kill_process_instance(self,
                               process,
                               instance=None,
                               timeout_seconds=10,
                               ignore_timeouts=False):
        """Kill a process on the router.

        Kills remote program named |process| (optionally only a specific
        |instance|).  Wait |timeout_seconds| for |process| to die
        before returning.  If |ignore_timeouts| is False, raise
        a TestError on timeouts.

        @param process: string name of process to kill.
        @param instance: string fragment of the command line unique to
                this instance of the remote process.
        @param timeout_seconds: float timeout in seconds to wait.
        @param ignore_timeouts: True iff we should ignore failures to
                kill processes.
        @return True iff the specified process has exited.

        """
        if instance is not None:
            search_arg = '-f "^%s.*%s"' % (process, instance)
        else:
            search_arg = process

        self.host.run('pkill %s' % search_arg, ignore_status=True)
        is_dead = False
        start_time = time.time()
        while not is_dead and time.time() - start_time < timeout_seconds:
            time.sleep(self.POLLING_INTERVAL_SECONDS)
            is_dead = self.host.run(
                    'pgrep -l %s' % search_arg,
                    ignore_status=True).exit_status != 0
        if is_dead or ignore_timeouts:
            return is_dead

        raise error.TestError(
                'Timed out waiting for %s%s to die' %
                (process,
                '' if instance is None else ' (instance=%s)' % instance))


    def kill_hostapd_instance(self, instance):
        """Kills a hostapd instance.

        @param instance HostapdInstance object.

        """
        is_dead = self._kill_process_instance(
                self.cmd_hostapd,
                instance=instance.conf_file,
                timeout_seconds=30,
                ignore_timeouts=True)
        if instance.scenario_name:
            log_identifier = instance.scenario_name
        else:
            log_identifier = '%d_%s' % (
                self._total_hostapd_instances, instance.interface)
        files_to_copy = [(instance.log_file,
                          'debug/hostapd_router_%s.log' % log_identifier),
                         (instance.stderr_log_file,
                          'debug/hostapd_router_%s.stderr.log' %
                          log_identifier)]
        for remote_file, local_file in files_to_copy:
            if self.host.run('ls %s >/dev/null 2>&1' % remote_file,
                             ignore_status=True).exit_status:
                logging.error('Did not collect hostapd log file because '
                              'it was missing.')
            else:
                self.router.get_file(remote_file, local_file)
        self._total_hostapd_instances += 1
        if not is_dead:
            raise error.TestError('Timed out killing hostapd.')


    def build_unique_ssid(self, suffix=''):
        """ Build our unique token by base-<len(self.SUFFIX_LETTERS)> encoding
        the number of APs we've constructed already.

        @param suffix string to append to SSID

        """
        base = len(self.SUFFIX_LETTERS)
        number = self._number_unique_ssids
        self._number_unique_ssids += 1
        unique = ''
        while number or not unique:
            unique = self.SUFFIX_LETTERS[number % base] + unique
            number = number / base
        # And salt the SSID so that tests running in adjacent cells are unlikely
        # to pick the same SSID and we're resistent to beacons leaking out of
        # cells.
        salt = ''.join([random.choice(self.SUFFIX_LETTERS) for x in range(5)])
        return '_'.join([self._ssid_prefix, unique, salt, suffix])[-32:]


    def hostap_configure(self, configuration, multi_interface=None):
        """Build up a hostapd configuration file and start hostapd.

        Also setup a local server if this router supports them.

        @param configuration HosetapConfig object.
        @param multi_interface bool True iff multiple interfaces allowed.

        """
        if multi_interface is None and (self.hostapd_instances or
                                        self.station_instances):
            self.deconfig()
        if configuration.is_11ac:
            router_caps = self.get_capabilities()
            if site_linux_system.LinuxSystem.CAPABILITY_VHT not in router_caps:
                raise error.TestNAError('Router does not have AC support')

        self.start_hostapd(configuration)
        interface = self.hostapd_instances[-1].interface
        self.iw_runner.set_tx_power(interface, 'auto')
        self.set_beacon_footer(interface, configuration.beacon_footer)
        self.start_local_server(interface)
        logging.info('AP configured.')


    def ibss_configure(self, config):
        """Configure a station based AP in IBSS mode.

        Extract relevant configuration objects from |config| despite not
        actually being a hostap managed endpoint.

        @param config HostapConfig object.

        """
        if self.station_instances or self.hostapd_instances:
            self.deconfig()
        interface = self.get_wlanif(config.frequency, 'ibss')
        ssid = (config.ssid or
                self.build_unique_ssid(suffix=config.ssid_suffix))
        # Connect the station
        self.router.run('%s link set %s up' % (self.cmd_ip, interface))
        self.iw_runner.ibss_join(interface, ssid, config.frequency)
        # Always start a local server.
        self.start_local_server(interface)
        # Remember that this interface is up.
        self.station_instances.append(
                StationInstance(ssid=ssid, interface=interface,
                                dev_type='ibss'))


    def local_server_address(self, index):
        """Get the local server address for an interface.

        When we multiple local servers, we give them static IP addresses
        like 192.168.*.254.

        @param index int describing which local server this is for.

        """
        return '%d.%d.%d.%d' % (self.SUBNET_PREFIX_OCTETS + (index, 254))


    def local_peer_ip_address(self, index):
        """Get the IP address allocated for the peer associated to the AP.

        This address is assigned to a locally associated peer device that
        is created for the DUT to perform connectivity tests with.
        When we have multiple local servers, we give them static IP addresses
        like 192.168.*.253.

        @param index int describing which local server this is for.

        """
        return '%d.%d.%d.%d' % (self.SUBNET_PREFIX_OCTETS + (index, 253))


    def local_peer_mac_address(self):
        """Get the MAC address of the peer interface.

        @return string MAC address of the peer interface.

        """
        iface = interface.Interface(self.station_instances[0].interface,
                                    self.router)
        return iface.mac_address


    def _get_unused_server_address_index(self):
        """@return an unused server address index."""
        for address_index in range(0, 256):
            if address_index not in self.server_address_index:
                return address_index
        raise error.TestFail('No available server address index')


    def change_server_address_index(self, ap_num=0, server_address_index=None):
        """Restart the local server with a different server address index.

        This will restart the local server with different gateway IP address
        and DHCP address ranges.

        @param ap_num: int hostapd instance number.
        @param server_address_index: int server address index.

        """
        interface = self.local_servers[ap_num]['interface'];
        # Get an unused server address index if one is not specified, which
        # will be different from the one that's currently in used.
        if server_address_index is None:
            server_address_index = self._get_unused_server_address_index()

        # Restart local server with the new server address index.
        self.stop_local_server(self.local_servers[ap_num])
        self.start_local_server(interface,
                                ap_num=ap_num,
                                server_address_index=server_address_index)


    def start_local_server(self,
                           interface,
                           ap_num=None,
                           server_address_index=None):
        """Start a local server on an interface.

        @param interface string (e.g. wlan0)
        @param ap_num int the ap instance to start the server for
        @param server_address_index int server address index

        """
        logging.info('Starting up local server...')

        if len(self.local_servers) >= 256:
            raise error.TestFail('Exhausted available local servers')

        # Get an unused server address index if one is not specified.
        # Validate server address index if one is specified.
        if server_address_index is None:
            server_address_index = self._get_unused_server_address_index()
        elif server_address_index in self.server_address_index:
            raise error.TestFail('Server address index %d already in used' %
                                 server_address_index)

        server_addr = netblock.from_addr(
                self.local_server_address(server_address_index),
                prefix_len=24)

        params = {}
        params['address_index'] = server_address_index
        params['netblock'] = server_addr
        params['dhcp_range'] = ' '.join(
            (server_addr.get_addr_in_block(1),
             server_addr.get_addr_in_block(128)))
        params['interface'] = interface
        params['ip_params'] = ('%s broadcast %s dev %s' %
                               (server_addr.netblock,
                                server_addr.broadcast,
                                interface))
        if ap_num is None:
            self.local_servers.append(params)
        else:
            self.local_servers.insert(ap_num, params)
        self.server_address_index.append(server_address_index)

        self.router.run('%s addr flush %s' %
                        (self.cmd_ip, interface))
        self.router.run('%s addr add %s' %
                        (self.cmd_ip, params['ip_params']))
        self.router.run('%s link set %s up' %
                        (self.cmd_ip, interface))
        self.start_dhcp_server(interface)


    def stop_local_server(self, server):
        """Stop a local server on the router

        @param server object server configuration parameters.

        """
        self.stop_dhcp_server(server['interface'])
        self.router.run("%s addr del %s" %
                        (self.cmd_ip, server['ip_params']),
                        ignore_status=True)
        self.server_address_index.remove(server['address_index'])
        self.local_servers.remove(server)


    def start_dhcp_server(self, interface):
        """Start a dhcp server on an interface.

        @param interface string (e.g. wlan0)

        """
        for server in self.local_servers:
            if server['interface'] == interface:
                params = server
                break
        else:
            raise error.TestFail('Could not find local server '
                                 'to match interface: %r' % interface)
        server_addr = params['netblock']
        dhcpd_conf_file = self.dhcpd_conf % interface
        dhcp_conf = '\n'.join([
            'port=0',  # disables DNS server
            'bind-interfaces',
            'log-dhcp',
            'dhcp-range=%s' % ','.join((server_addr.get_addr_in_block(1),
                                        server_addr.get_addr_in_block(128))),
            'interface=%s' % params['interface'],
            'dhcp-leasefile=%s' % self.dhcpd_leases])
        self.router.run('cat <<EOF >%s\n%s\nEOF\n' %
            (dhcpd_conf_file, dhcp_conf))
        self.router.run('dnsmasq --conf-file=%s' % dhcpd_conf_file)


    def stop_dhcp_server(self, instance=None):
        """Stop a dhcp server on the router.

        @param instance string instance to kill.

        """
        self._kill_process_instance('dnsmasq', instance=instance)


    def get_wifi_channel(self, ap_num):
        """Return channel of BSS corresponding to |ap_num|.

        @param ap_num int which BSS to get the channel of.
        @return int primary channel of BSS.

        """
        instance = self.hostapd_instances[ap_num]
        return instance.config_dict['channel']


    def get_wifi_ip(self, ap_num):
        """Return IP address on the WiFi subnet of a local server on the router.

        If no local servers are configured (e.g. for an RSPro), a TestFail will
        be raised.

        @param ap_num int which local server to get an address from.

        """
        if not self.local_servers:
            raise error.TestError('No IP address assigned')

        return self.local_servers[ap_num]['netblock'].addr


    def get_wifi_ip_subnet(self, ap_num):
        """Return subnet of WiFi AP instance.

        If no APs are configured a TestError will be raised.

        @param ap_num int which local server to get an address from.

        """
        if not self.local_servers:
            raise error.TestError('No APs configured.')

        return self.local_servers[ap_num]['netblock'].subnet


    def get_hostapd_interface(self, ap_num):
        """Get the name of the interface associated with a hostapd instance.

        @param ap_num: int hostapd instance number.
        @return string interface name (e.g. 'managed0').

        """
        if ap_num not in range(len(self.hostapd_instances)):
            raise error.TestFail('Invalid instance number (%d) with %d '
                                 'instances configured.' %
                                 (ap_num, len(self.hostapd_instances)))

        instance = self.hostapd_instances[ap_num]
        return instance.interface


    def get_station_interface(self, instance):
        """Get the name of the interface associated with a station.

        @param instance: int station instance number.
        @return string interface name (e.g. 'managed0').

        """
        if instance not in range(len(self.station_instances)):
            raise error.TestFail('Invalid instance number (%d) with %d '
                                 'instances configured.' %
                                 (instance, len(self.station_instances)))

        instance = self.station_instances[instance]
        return instance.interface


    def get_hostapd_mac(self, ap_num):
        """Return the MAC address of an AP in the test.

        @param ap_num int index of local server to read the MAC address from.
        @return string MAC address like 00:11:22:33:44:55.

        """
        interface_name = self.get_hostapd_interface(ap_num)
        ap_interface = interface.Interface(interface_name, self.host)
        return ap_interface.mac_address


    def get_hostapd_phy(self, ap_num):
        """Get name of phy for hostapd instance.

        @param ap_num int index of hostapd instance.
        @return string phy name of phy corresponding to hostapd's
                managed interface.

        """
        interface = self.iw_runner.get_interface(
                self.get_hostapd_interface(ap_num))
        return interface.phy


    def deconfig(self):
        """A legacy, deprecated alias for deconfig_aps."""
        self.deconfig_aps()


    def deconfig_aps(self, instance=None, silent=False):
        """De-configure an AP (will also bring wlan down).

        @param instance: int or None.  If instance is None, will bring down all
                instances of hostapd.
        @param silent: True if instances should be brought without de-authing
                the DUT.

        """
        if not self.hostapd_instances and not self.station_instances:
            return

        if self.hostapd_instances:
            local_servers = []
            if instance is not None:
                instances = [ self.hostapd_instances.pop(instance) ]
                for server in self.local_servers:
                    if server['interface'] == instances[0].interface:
                        local_servers = [server]
                        break
            else:
                instances = self.hostapd_instances
                self.hostapd_instances = []
                local_servers = copy.copy(self.local_servers)

            for instance in instances:
                if silent:
                    # Deconfigure without notifying DUT.  Remove the interface
                    # hostapd uses to send beacon and DEAUTH packets.
                    self.remove_interface(instance.interface)

                self.kill_hostapd_instance(instance)
                self.release_interface(instance.interface)
        if self.station_instances:
            local_servers = copy.copy(self.local_servers)
            instance = self.station_instances.pop()
            if instance.dev_type == 'ibss':
                self.iw_runner.ibss_leave(instance.interface)
            elif instance.dev_type == 'managed':
                self._kill_process_instance(self.cmd_wpa_supplicant,
                                            instance=instance.interface)
            else:
                self.iw_runner.disconnect_station(instance.interface)
            self.router.run('%s link set %s down' %
                            (self.cmd_ip, instance.interface))

        for server in local_servers:
            self.stop_local_server(server)


    def set_ap_interface_down(self, instance=0):
        """Bring down the hostapd interface.

        @param instance int router instance number.

        """
        self.host.run('%s link set %s down' %
                      (self.cmd_ip, self.get_hostapd_interface(instance)))


    def confirm_pmksa_cache_use(self, instance=0):
        """Verify that the PMKSA auth was cached on a hostapd instance.

        @param instance int router instance number.

        """
        log_file = self.hostapd_instances[instance].log_file
        pmksa_match = 'PMK from PMKSA cache'
        result = self.router.run('grep -q "%s" %s' % (pmksa_match, log_file),
                                 ignore_status=True)
        if result.exit_status:
            raise error.TestFail('PMKSA cache was not used in roaming.')


    def get_ssid(self, instance=None):
        """@return string ssid for the network stemming from this router."""
        if instance is None:
            instance = 0
            if len(self.hostapd_instances) > 1:
                raise error.TestFail('No instance of hostapd specified with '
                                     'multiple instances present.')

        if self.hostapd_instances:
            return self.hostapd_instances[instance].ssid

        if self.station_instances:
            return self.station_instances[0].ssid

        raise error.TestFail('Requested ssid of an unconfigured AP.')


    def deauth_client(self, client_mac):
        """Deauthenticates a client described in params.

        @param client_mac string containing the mac address of the client to be
               deauthenticated.

        """
        control_if = self.hostapd_instances[-1].config_dict['ctrl_interface']
        self.router.run('%s -p%s deauthenticate %s' %
                        (self.cmd_hostapd_cli, control_if, client_mac))


    def _prep_probe_response_footer(self, footer):
        """Write probe response footer temporarily to a local file and copy
        over to test router.

        @param footer string containing bytes for the probe response footer.
        @raises AutoservRunError: If footer file copy fails.

        """
        with tempfile.NamedTemporaryFile() as fp:
            fp.write(footer)
            fp.flush()
            try:
                self.host.send_file(fp.name, self.PROBE_RESPONSE_FOOTER_FILE)
            except error.AutoservRunError:
                logging.error('failed to copy footer file to AP')
                raise


    def send_management_frame_on_ap(self, frame_type, channel, instance=0):
        """Injects a management frame into an active hostapd session.

        @param frame_type string the type of frame to send.
        @param channel int targeted channel
        @param instance int indicating which hostapd instance to inject into.

        """
        hostap_interface = self.hostapd_instances[instance].interface
        interface = self.get_wlanif(0, 'monitor', same_phy_as=hostap_interface)
        self.router.run("%s link set %s up" % (self.cmd_ip, interface))
        self.router.run('%s -i %s -t %s -c %d' %
                        (self.cmd_send_management_frame, interface, frame_type,
                         channel))
        self.release_interface(interface)


    def setup_management_frame_interface(self, channel):
        """
        Setup interface for injecting management frames.

        @param channel int channel to inject the frames.

        @return string name of the interface.

        """
        frequency = hostap_config.HostapConfig.get_frequency_for_channel(
                channel)
        interface = self.get_wlanif(frequency, 'monitor')
        self.router.run('%s link set %s up' % (self.cmd_ip, interface))
        self.iw_runner.set_freq(interface, frequency)
        return interface


    def send_management_frame(self, interface, frame_type, channel,
                              ssid_prefix=None, num_bss=None,
                              frame_count=None, delay=None,
                              dest_addr=None, probe_resp_footer=None):
        """
        Injects management frames on specify channel |frequency|.

        This function will spawn off a new process to inject specified
        management frames |frame_type| at the specified interface |interface|.

        @param interface string interface to inject frames.
        @param frame_type string message type.
        @param channel int targeted channel.
        @param ssid_prefix string SSID prefix.
        @param num_bss int number of BSS.
        @param frame_count int number of frames to send.
        @param delay int milliseconds delay between frames.
        @param dest_addr string destination address (DA) MAC address.
        @param probe_resp_footer string footer for probe response.

        @return int PID of the newly created process.

        """
        command = '%s -i %s -t %s -c %d' % (self.cmd_send_management_frame,
                                interface, frame_type, channel)
        if ssid_prefix is not None:
            command += ' -s %s' % (ssid_prefix)
        if num_bss is not None:
            command += ' -b %d' % (num_bss)
        if frame_count is not None:
            command += ' -n %d' % (frame_count)
        if delay is not None:
            command += ' -d %d' % (delay)
        if dest_addr is not None:
            command += ' -a %s' % (dest_addr)
        if probe_resp_footer is not None:
            self._prep_probe_response_footer(footer=probe_resp_footer)
            command += ' -f %s' % (self.PROBE_RESPONSE_FOOTER_FILE)
        command += ' > %s 2>&1 & echo $!' % (self.MGMT_FRAME_SENDER_LOG_FILE)
        pid = int(self.router.run(command).stdout)
        return pid


    def detect_client_deauth(self, client_mac, instance=0):
        """Detects whether hostapd has logged a deauthentication from
        |client_mac|.

        @param client_mac string the MAC address of the client to detect.
        @param instance int indicating which hostapd instance to query.

        """
        interface = self.hostapd_instances[instance].interface
        deauth_msg = "%s: deauthentication: STA=%s" % (interface, client_mac)
        log_file = self.hostapd_instances[instance].log_file
        result = self.router.run("grep -qi '%s' %s" % (deauth_msg, log_file),
                                 ignore_status=True)
        return result.exit_status == 0


    def detect_client_coexistence_report(self, client_mac, instance=0):
        """Detects whether hostapd has logged an action frame from
        |client_mac| indicating information about 20/40MHz BSS coexistence.

        @param client_mac string the MAC address of the client to detect.
        @param instance int indicating which hostapd instance to query.

        """
        coex_msg = ('nl80211: MLME event frame - hexdump(len=.*): '
                    '.. .. .. .. .. .. .. .. .. .. %s '
                    '.. .. .. .. .. .. .. .. 04 00.*48 01 ..' %
                    ' '.join(client_mac.split(':')))
        log_file = self.hostapd_instances[instance].log_file
        result = self.router.run("grep -qi '%s' %s" % (coex_msg, log_file),
                                 ignore_status=True)
        return result.exit_status == 0


    def add_connected_peer(self, instance=0):
        """Configure a station connected to a running AP instance.

        Extract relevant configuration objects from the hostap
        configuration for |instance| and generate a wpa_supplicant
        instance that connects to it.  This allows the DUT to interact
        with a client entity that is also connected to the same AP.  A
        full wpa_supplicant instance is necessary here (instead of just
        using the "iw" command to connect) since we want to enable
        advanced features such as TDLS.

        @param instance int indicating which hostapd instance to connect to.

        """
        if not self.hostapd_instances:
            raise error.TestFail('Hostapd is not configured.')

        if self.station_instances:
            raise error.TestFail('Station is already configured.')

        ssid = self.get_ssid(instance)
        hostap_conf = self.hostapd_instances[instance].config_dict
        frequency = hostap_config.HostapConfig.get_frequency_for_channel(
                hostap_conf['channel'])
        self.configure_managed_station(
                ssid, frequency, self.local_peer_ip_address(instance))
        interface = self.station_instances[0].interface
        # Since we now have two network interfaces connected to the same
        # network, we need to disable the kernel's protection against
        # incoming packets to an "unexpected" interface.
        self.router.run('echo 2 > /proc/sys/net/ipv4/conf/%s/rp_filter' %
                        interface)

        # Similarly, we'd like to prevent the hostap interface from
        # replying to ARP requests for the peer IP address and vice
        # versa.
        self.router.run('echo 1 > /proc/sys/net/ipv4/conf/%s/arp_ignore' %
                        interface)
        self.router.run('echo 1 > /proc/sys/net/ipv4/conf/%s/arp_ignore' %
                        hostap_conf['interface'])


    def configure_managed_station(self, ssid, frequency, ip_addr):
        """Configure a router interface to connect as a client to a network.

        @param ssid: string SSID of network to join.
        @param frequency: int frequency required to join the network.
        @param ip_addr: IP address to assign to this interface
                        (e.g. '192.168.1.200').

        """
        interface = self.get_wlanif(frequency, 'managed')

        # TODO(pstew): Configure other bits like PSK, 802.11n if tests
        # require them...
        supplicant_config = (
                'network={\n'
                '  ssid="%(ssid)s"\n'
                '  key_mgmt=NONE\n'
                '}\n' % {'ssid': ssid}
        )

        conf_file = self.STATION_CONF_FILE_PATTERN % interface
        log_file = self.STATION_LOG_FILE_PATTERN % interface
        pid_file = self.STATION_PID_FILE_PATTERN % interface

        self.router.run('cat <<EOF >%s\n%s\nEOF\n' %
            (conf_file, supplicant_config))

        # Connect the station.
        self.router.run('%s link set %s up' % (self.cmd_ip, interface))
        start_command = ('%s -dd -t -i%s -P%s -c%s -D%s >%s 2>&1 &' %
                         (self.cmd_wpa_supplicant,
                         interface, pid_file, conf_file,
                         self.HOSTAPD_DRIVER_NAME, log_file))
        self.router.run(start_command)
        self.iw_runner.wait_for_link(interface)

        # Assign an IP address to this interface.
        self.router.run('%s addr add %s/24 dev %s' %
                        (self.cmd_ip, ip_addr, interface))
        self.station_instances.append(
                StationInstance(ssid=ssid, interface=interface,
                                dev_type='managed'))


    def send_magic_packet(self, dest_ip, dest_mac):
        """Sends a magic packet to the NIC with the given IP and MAC addresses.

        @param dest_ip the IP address of the device to send the packet to
        @param dest_mac the hardware MAC address of the device

        """
        # magic packet is 6 0xff bytes followed by the hardware address
        # 16 times
        mac_bytes = ''.join([chr(int(b, 16)) for b in dest_mac.split(':')])
        magic_packet = '\xff' * 6 + mac_bytes * 16

        logging.info('Sending magic packet to %s...', dest_ip)
        self.host.run('python -uc "import socket, sys;'
                      's = socket.socket(socket.AF_INET, socket.SOCK_DGRAM);'
                      's.sendto(sys.stdin.read(), (\'%s\', %d))"' %
                      (dest_ip, UDP_DISCARD_PORT),
                      stdin=magic_packet)


    def set_beacon_footer(self, interface, footer=''):
        """Sets the beacon footer (appended IE information) for this interface.

        @param interface string interface to set the footer on.
        @param footer string footer to be set on the interface.

        """
        footer_file = ('/sys/kernel/debug/ieee80211/%s/beacon_footer' %
                       self.iw_runner.get_interface(interface).phy)
        if self.router.run('test -e %s' % footer_file,
                           ignore_status=True).exit_status != 0:
            logging.info('Beacon footer file does not exist.  Ignoring.')
            return
        self.host.run('echo -ne %s > %s' % ('%r' % footer, footer_file))


    def setup_bridge_mode_dhcp_server(self):
        """Setup an DHCP server for bridge mode.

        Setup an DHCP server on the master interface of the virtual ethernet
        pair, with peer interface connected to the bridge interface. This is
        used for testing APs in bridge mode.

        """
        # Start a local server on master interface of virtual ethernet pair.
        self.start_local_server(
                self.get_virtual_ethernet_master_interface())
        # Add peer interface to the bridge.
        self.add_interface_to_bridge(
                self.get_virtual_ethernet_peer_interface())

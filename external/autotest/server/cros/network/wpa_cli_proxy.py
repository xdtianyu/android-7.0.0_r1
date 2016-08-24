# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes


# Used to represent stations we parse out of scan results.
Station = collections.namedtuple('Station',
                                 ['bssid', 'frequency', 'signal', 'ssid'])

class WpaCliProxy(object):
    """Interacts with a DUT through wpa_cli rather than shill."""

    SCANNING_INTERVAL_SECONDS = 5
    POLLING_INTERVAL_SECONDS = 0.5
    # From wpa_supplicant.c:wpa_supplicant_state_txt()
    WPA_SUPPLICANT_ASSOCIATING_STATES = (
            'AUTHENTICATING',
            'ASSOCIATING',
            'ASSOCIATED',
            '4WAY_HANDSHAKE',
            'GROUP_HANDSHAKE')
    WPA_SUPPLICANT_ASSOCIATED_STATES = (
            'COMPLETED',)
    ANDROID_CMD_FORMAT = '/system/bin/wpa_cli IFNAME={0[ifname]} {0[cmd]}'
    BRILLO_CMD_FORMAT = 'su system /system/bin/wpa_cli -i{0[ifname]} -p/data/misc/wifi/sockets {0[cmd]}'
    CROS_CMD_FORMAT = 'su wpa -s /bin/bash -c "/usr/bin/wpa_cli {0[cmd]}"'



    def __init__(self, host, wifi_if):
        self._host = host
        self._wifi_if = wifi_if
        self._created_networks = {}
        # TODO(wiley) Hardcoding this IFNAME prefix makes some big assumptions.
        #             we'll need to discover this parameter as it becomes more
        #             generally useful.
        if host.get_os_type() == 'android':
            self._wpa_cli_cmd_format = self.ANDROID_CMD_FORMAT
        elif host.get_os_type() == 'brillo':
            self._wpa_cli_cmd_format = self.BRILLO_CMD_FORMAT
        elif host.get_os_type() == 'cros':
            self._wpa_cli_cmd_format = self.CROS_CMD_FORMAT


    def _add_network(self, ssid):
        """
        Add a wpa_supplicant network for ssid.

        @param ssid string: name of network to add.
        @return int network id of added network.

        """
        add_result = self.run_wpa_cli_cmd('add_network', check_result=False)
        network_id = int(add_result.stdout.splitlines()[-1])
        self.run_wpa_cli_cmd('set_network %d ssid \\"%s\\"' %
                             (network_id, ssid))
        self._created_networks[ssid] = network_id
        logging.debug('Added network %s=%d', ssid, network_id)
        return network_id


    def run_wpa_cli_cmd(self, command, check_result=True):
        """
        Run a wpa_cli command and optionally check the result.

        @param command string: suffix of a command to be prefixed with
                an appropriate wpa_cli for this host.
        @param check_result bool: True iff we want to check that the
                command comes back with an 'OK' response.
        @return result object returned by host.run.

        """
        cmd = self._wpa_cli_cmd_format.format(
                {'ifname' : self._wifi_if, 'cmd' : command})
        result = self._host.run(cmd)
        if check_result and not result.stdout.strip().endswith('OK'):
            raise error.TestFail('wpa_cli command failed: %s' % command)

        return result


    def _get_status_dict(self):
        """
        Gets the status output for a WiFi interface.

        Get the output of wpa_cli status.  This summarizes what wpa_supplicant
        is doing with respect to the WiFi interface.

        Example output:

            Using interface 'wlan0'
            wpa_state=INACTIVE
            p2p_device_address=32:76:6f:f2:a6:c4
            address=30:76:6f:f2:a6:c4

        @return dict of key/value pairs parsed from output using = as divider.

        """
        status_result = self.run_wpa_cli_cmd('status', check_result=False)
        return dict([line.strip().split('=', 1)
                     for line in status_result.stdout.splitlines()
                     if line.find('=') > 0])


    def _is_associating_or_associated(self):
        """@return True if the DUT is assocating or associated with a BSS."""
        state = self._get_status_dict().get('wpa_state', None)
        return state in (self.WPA_SUPPLICANT_ASSOCIATING_STATES +
                         self.WPA_SUPPLICANT_ASSOCIATED_STATES)


    def _is_associated(self, ssid):
        """
        Check if the DUT is associated to a given SSID.

        @param ssid string: SSID of the network we're concerned about.
        @return True if we're associated with the specified SSID.

        """
        status_dict = self._get_status_dict()
        return (status_dict.get('ssid', None) == ssid and
                status_dict.get('wpa_state', None) in
                        self.WPA_SUPPLICANT_ASSOCIATED_STATES)


    def _is_connected(self, ssid):
        """
        Check that we're connected to |ssid| and have an IP address.

        @param ssid string: SSID of the network we're concerned about.
        @return True if we have an IP and we're associated with |ssid|.

        """
        status_dict = self._get_status_dict()
        return (status_dict.get('ssid', None) == ssid and
                status_dict.get('ip_address', None))


    def _wait_until(self, value_check, timeout_seconds):
        """
        Call a function repeatedly until we time out.

        Call value_check() every POLLING_INTERVAL_SECONDS seconds
        until |timeout_seconds| have passed.  Return whether
        value_check() returned a True value and the time we spent in this
        function.

        @param timeout_seconds numeric: number of seconds to wait.
        @return a tuple (success, duration_seconds) where success is a boolean
                and duration is a float.

        """
        start_time = time.time()
        while time.time() - start_time < timeout_seconds:
            duration = time.time() - start_time
            if value_check():
                return (True, duration)

            time.sleep(self.POLLING_INTERVAL_SECONDS)
        duration = time.time() - start_time
        return (False, duration)


    def clean_profiles(self):
        """Remove state associated with past networks we've connected to."""
        # list_networks output looks like:
        # Using interface 'wlan0'^M
        # network id / ssid / bssid / flags^M
        # 0    SimpleConnect_jstja_ch1 any     [DISABLED]^M
        # 1    SimpleConnect_gjji2_ch6 any     [DISABLED]^M
        # 2    SimpleConnect_xe9d1_ch11        any     [DISABLED]^M
        list_networks_result = self.run_wpa_cli_cmd(
                'list_networks', check_result=False)
        start_parsing = False
        for line in list_networks_result.stdout.splitlines():
            if not start_parsing:
                if line.startswith('network id'):
                    start_parsing = True
                continue

            network_id = int(line.split()[0])
            self.run_wpa_cli_cmd('remove_network %d' % network_id)
        self._created_networks = {}


    def create_profile(self, _):
        """
        This is a no op, since we don't have profiles.

        @param _ ignored.

        """
        logging.info('Skipping create_profile on %s', self.__class__.__name__)


    def pop_profile(self, _):
        """
        This is a no op, since we don't have profiles.

        @param _ ignored.

        """
        logging.info('Skipping pop_profile on %s', self.__class__.__name__)


    def push_profile(self, _):
        """
        This is a no op, since we don't have profiles.

        @param _ ignored.

        """
        logging.info('Skipping push_profile on %s', self.__class__.__name__)


    def remove_profile(self, _):
        """
        This is a no op, since we don't have profiles.

        @param _ ignored.

        """
        logging.info('Skipping remove_profile on %s', self.__class__.__name__)


    def init_test_network_state(self):
        """Create a clean slate for tests with respect to remembered networks.

        For wpa_cli hosts, this means removing all remembered networks.

        @return True iff operation succeeded, False otherwise.

        """
        self.clean_profiles()
        return True


    def connect_wifi(self, assoc_params):
        """
        Connect to the WiFi network described by AssociationParameters.

        @param assoc_params AssociationParameters object.
        @return serialized AssociationResult object.

        """
        logging.debug('connect_wifi()')
        # Ouptut should look like:
        #   Using interface 'wlan0'
        #   0
        assoc_result = xmlrpc_datatypes.AssociationResult()
        network_id = self._add_network(assoc_params.ssid)
        if assoc_params.is_hidden:
            self.run_wpa_cli_cmd('set_network %d %s %s' %
                                 (network_id, 'scan_ssid', '1'))

        sec_config = assoc_params.security_config
        for field, value in sec_config.get_wpa_cli_properties().iteritems():
            self.run_wpa_cli_cmd('set_network %d %s %s' %
                                 (network_id, field, value))
        self.run_wpa_cli_cmd('select_network %d' % network_id)

        # Wait for an appropriate BSS to appear in scan results.
        scan_results_pattern = '\t'.join(['([0-9a-f:]{17})', # BSSID
                                          '([0-9]+)',  # Frequency
                                          '(-[0-9]+)',  # Signal level
                                          '(.*)',  # Encryption types
                                          '(.*)'])  # SSID
        last_scan_time = -1.0
        start_time = time.time()
        while time.time() - start_time < assoc_params.discovery_timeout:
            assoc_result.discovery_time = time.time() - start_time
            if self._is_associating_or_associated():
                # Internally, wpa_supplicant writes its scan_results response
                # to a 4kb buffer.  When there are many BSS's, the buffer fills
                # up, and we'll never see the BSS we care about in some cases.
                break

            scan_result = self.run_wpa_cli_cmd('scan_results',
                                               check_result=False)
            found_stations = []
            for line in scan_result.stdout.strip().splitlines():
                match = re.match(scan_results_pattern, line)
                if match is None:
                    continue
                found_stations.append(
                        Station(bssid=match.group(1), frequency=match.group(2),
                                signal=match.group(3), ssid=match.group(5)))
            logging.debug('Found stations: %r',
                          [station.ssid for station in found_stations])
            if [station for station in found_stations
                    if station.ssid == assoc_params.ssid]:
                break

            if time.time() - last_scan_time > self.SCANNING_INTERVAL_SECONDS:
                # Sometimes this might fail with a FAIL-BUSY if the previous
                # scan hasn't finished.
                scan_result = self.run_wpa_cli_cmd('scan', check_result=False)
                if scan_result.stdout.strip().endswith('OK'):
                    last_scan_time = time.time()
            time.sleep(self.POLLING_INTERVAL_SECONDS)
        else:
            assoc_result.failure_reason = 'Discovery timed out'
            return assoc_result.serialize()

        # Wait on association to finish.
        success, assoc_result.association_time = self._wait_until(
                lambda: self._is_associated(assoc_params.ssid),
                assoc_params.association_timeout)
        if not success:
            assoc_result.failure_reason = 'Association timed out'
            return assoc_result.serialize()

        # Then wait for ip configuration to finish.
        success, assoc_result.configuration_time = self._wait_until(
                lambda: self._is_connected(assoc_params.ssid),
                assoc_params.configuration_timeout)
        if not success:
            assoc_result.failure_reason = 'DHCP negotiation timed out'
            return assoc_result.serialize()

        assoc_result.success = True
        logging.info('Connected to %s', assoc_params.ssid)
        return assoc_result.serialize()


    def disconnect(self, ssid):
        """
        Disconnect from a WiFi network named |ssid|.

        @param ssid string: name of network to disable in wpa_supplicant.

        """
        logging.debug('disconnect()')
        if ssid not in self._created_networks:
            return False
        self.run_wpa_cli_cmd('disable_network %d' %
                             self._created_networks[ssid])
        return True


    def delete_entries_for_ssid(self, ssid):
        """Delete a profile entry.

        @param ssid string of WiFi service for which to delete entries.
        @return True on success, False otherwise.
        """
        return self.disconnect(ssid)


    def set_device_enabled(self, wifi_interface, enabled):
        """Enable or disable the WiFi device.

        @param wifi_interface: string name of interface being modified.
        @param enabled: boolean; true if this device should be enabled,
                false if this device should be disabled.
        @return True if it worked; false, otherwise

        """
        return False


    def sync_time_to(self, epoch_seconds):
        """
        Sync time on the DUT to |epoch_seconds| from the epoch.

        @param epoch_seconds float: number of seconds since the epoch.

        """
        # This will claim to fail, but will work anyway.
        self._host.run('date -u %f' % epoch_seconds, ignore_status=True)

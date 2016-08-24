# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros.network import iw_event_logger

# These must mirror the values in 'iw list' output.
CHAN_FLAG_DISABLED = 'disabled'
CHAN_FLAG_NO_IR = 'no IR'
CHAN_FLAG_PASSIVE_SCAN = 'passive scan'
CHAN_FLAG_RADAR_DETECT = 'radar detection'
DEV_MODE_AP = 'AP'
DEV_MODE_IBSS = 'IBSS'
DEV_MODE_MONITOR = 'monitor'

HT20 = 'HT20'
HT40_ABOVE = 'HT40+'
HT40_BELOW = 'HT40-'

SECURITY_OPEN = 'open'
SECURITY_WEP = 'wep'
SECURITY_WPA = 'wpa'
SECURITY_WPA2 = 'wpa2'
# Mixed mode security is WPA2/WPA
SECURITY_MIXED = 'mixed'

# Table of lookups between the output of item 'secondary channel offset:' from
# iw <device> scan to constants.

HT_TABLE = {'no secondary': HT20,
            'above': HT40_ABOVE,
            'below': HT40_BELOW}

IwBand = collections.namedtuple(
    'Band', ['num', 'frequencies', 'frequency_flags', 'mcs_indices'])
IwBss = collections.namedtuple('IwBss', ['bss', 'frequency', 'ssid', 'security',
                                         'ht', 'signal'])
IwNetDev = collections.namedtuple('IwNetDev', ['phy', 'if_name', 'if_type'])
IwTimedScan = collections.namedtuple('IwTimedScan', ['time', 'bss_list'])

# The fields for IwPhy are as follows:
#   name: string name of the phy, such as "phy0"
#   bands: list of IwBand objects.
#   modes: List of strings containing interface modes supported, such as "AP".
#   commands: List of strings containing nl80211 commands supported, such as
#          "authenticate".
#   features: List of strings containing nl80211 features supported, such as
#          "T-DLS".
#   max_scan_ssids: Maximum number of SSIDs which can be scanned at once.
IwPhy = collections.namedtuple(
    'Phy', ['name', 'bands', 'modes', 'commands', 'features',
            'max_scan_ssids', 'avail_tx_antennas', 'avail_rx_antennas',
            'supports_setting_antenna_mask', 'support_vht'])

DEFAULT_COMMAND_IW = 'iw'

# Redirect stderr to stdout on Cros since adb commands cannot distinguish them
# on Brillo.
IW_TIME_COMMAND_FORMAT = '(time -p %s) 2>&1'
IW_TIME_COMMAND_OUTPUT_START = 'real'

IW_LINK_KEY_BEACON_INTERVAL = 'beacon int'
IW_LINK_KEY_DTIM_PERIOD = 'dtim period'
IW_LINK_KEY_FREQUENCY = 'freq'
IW_LOCAL_EVENT_LOG_FILE = './debug/iw_event_%d.log'


class IwRunner(object):
    """Defines an interface to the 'iw' command."""


    def __init__(self, remote_host=None, command_iw=DEFAULT_COMMAND_IW):
        self._run = utils.run
        self._host = remote_host
        if remote_host:
            self._run = remote_host.run
        self._command_iw = command_iw
        self._log_id = 0


    def _parse_scan_results(self, output):
        """Parse the output of the 'scan' and 'scan dump' commands.

        Here is an example of what a single network would look like for
        the input parameter.  Some fields have been removed in this example:
          BSS 00:11:22:33:44:55(on wlan0)
          freq: 2447
          beacon interval: 100 TUs
          signal: -46.00 dBm
          Information elements from Probe Response frame:
          SSID: my_open_network
          Extended supported rates: 24.0 36.0 48.0 54.0
          HT capabilities:
          Capabilities: 0x0c
          HT20
          HT operation:
          * primary channel: 8
          * secondary channel offset: no secondary
          * STA channel width: 20 MHz
          RSN: * Version: 1
          * Group cipher: CCMP
          * Pairwise ciphers: CCMP
          * Authentication suites: PSK
          * Capabilities: 1-PTKSA-RC 1-GTKSA-RC (0x0000)

        @param output: string command output.

        @returns a list of IwBss namedtuples; None if the scan fails

        """
        bss = None
        frequency = None
        ssid = None
        ht = None
        signal = None
        security = None
        supported_securities = []
        bss_list = []
        for line in output.splitlines():
            line = line.strip()
            bss_match = re.match('BSS ([0-9a-f:]+)', line)
            if bss_match:
                if bss != None:
                    security = self.determine_security(supported_securities)
                    iwbss = IwBss(bss, frequency, ssid, security, ht, signal)
                    bss_list.append(iwbss)
                    bss = frequency = ssid = security = ht = None
                    supported_securities = []
                bss = bss_match.group(1)
            if line.startswith('freq:'):
                frequency = int(line.split()[1])
            if line.startswith('signal:'):
                signal = float(line.split()[1])
            if line.startswith('SSID: '):
                _, ssid = line.split(': ', 1)
            if line.startswith('* secondary channel offset'):
                ht = HT_TABLE[line.split(':')[1].strip()]
            if line.startswith('WPA'):
               supported_securities.append(SECURITY_WPA)
            if line.startswith('RSN'):
               supported_securities.append(SECURITY_WPA2)
        security = self.determine_security(supported_securities)
        bss_list.append(IwBss(bss, frequency, ssid, security, ht, signal))
        return bss_list


    def _parse_scan_time(self, output):
        """
        Parse the scan time in seconds from the output of the 'time -p "scan"'
        command.

        'time -p' Command output format is below:
        real     0.01
        user     0.01
        sys      0.00

        @param output: string command output.

        @returns float time in seconds.

        """
        output_lines = output.splitlines()
        for line_num, line in enumerate(output_lines):
            line = line.strip()
            if (line.startswith(IW_TIME_COMMAND_OUTPUT_START) and
                output_lines[line_num + 1].startswith('user') and
                output_lines[line_num + 2].startswith('sys')):
                return float(line.split()[1])
        raise error.TestFail('Could not parse scan time.')


    def add_interface(self, phy, interface, interface_type):
        """
        Add an interface to a WiFi PHY.

        @param phy: string name of PHY to add an interface to.
        @param interface: string name of interface to add.
        @param interface_type: string type of interface to add (e.g. 'monitor').

        """
        self._run('%s phy %s interface add %s type %s' %
                  (self._command_iw, phy, interface, interface_type))


    def disconnect_station(self, interface):
        """
        Disconnect a STA from a network.

        @param interface: string name of interface to disconnect.

        """
        self._run('%s dev %s disconnect' % (self._command_iw, interface))


    def get_current_bssid(self, interface_name):
        """Get the BSSID that |interface_name| is associated with.

        @param interface_name: string name of interface (e.g. 'wlan0').
        @return string bssid of our current association, or None.

        """
        result = self._run('%s dev %s link' %
                           (self._command_iw, interface_name),
                           ignore_status=True)
        if result.exit_status:
            # See comment in get_link_value.
            return None

        # We're looking for a line like:
        #   Connected to 04:f0:21:03:7d:bb (on wlan0)
        match = re.search(
                'Connected to ([0-9a-fA-F:]{17}) \\(on %s\\)' % interface_name,
                result.stdout)
        if match is None:
            return None
        return match.group(1)


    def get_interface(self, interface_name):
        """Get full information about an interface given an interface name.

        @param interface_name: string name of interface (e.g. 'wlan0').
        @return IwNetDev tuple.

        """
        matching_interfaces = [iw_if for iw_if in self.list_interfaces()
                                     if iw_if.if_name == interface_name]
        if len(matching_interfaces) != 1:
            raise error.TestFail('Could not find interface named %s' %
                                 interface_name)

        return matching_interfaces[0]


    def get_link_value(self, interface, iw_link_key):
        """Get the value of a link property for |interface|.

        This command parses fields of iw link:

        #> iw dev wlan0 link
        Connected to 74:e5:43:10:4f:c0 (on wlan0)
              SSID: PMKSACaching_4m9p5_ch1
              freq: 5220
              RX: 5370 bytes (37 packets)
              TX: 3604 bytes (15 packets)
              signal: -59 dBm
              tx bitrate: 13.0 MBit/s MCS 1

              bss flags:      short-slot-time
              dtim period:    5
              beacon int:     100

        @param iw_link_key: string one of IW_LINK_KEY_* defined above.
        @param interface: string desired value of iw link property.

        """
        result = self._run('%s dev %s link' % (self._command_iw, interface),
                           ignore_status=True)
        if result.exit_status:
            # When roaming, there is a period of time for mac80211 based drivers
            # when the driver is 'associated' with an SSID but not a particular
            # BSS.  This causes iw to return an error code (-2) when attempting
            # to retrieve information specific to the BSS.  This does not happen
            # in mwifiex drivers.
            return None

        find_re = re.compile('\s*%s:\s*(.*\S)\s*$' % iw_link_key)
        find_results = filter(bool,
                              map(find_re.match, result.stdout.splitlines()))
        if not find_results:
            return None

        actual_value = find_results[0].group(1)
        logging.info('Found iw link key %s with value %s.',
                     iw_link_key, actual_value)
        return actual_value


    def ibss_join(self, interface, ssid, frequency):
        """
        Join a WiFi interface to an IBSS.

        @param interface: string name of interface to join to the IBSS.
        @param ssid: string SSID of IBSS to join.
        @param frequency: int frequency of IBSS in Mhz.

        """
        self._run('%s dev %s ibss join %s %d' %
                  (self._command_iw, interface, ssid, frequency))


    def ibss_leave(self, interface):
        """
        Leave an IBSS.

        @param interface: string name of interface to remove from the IBSS.

        """
        self._run('%s dev %s ibss leave' % (self._command_iw, interface))


    def list_interfaces(self, desired_if_type=None):
        """List WiFi related interfaces on this system.

        @param desired_if_type: string type of interface to filter
                our returned list of interfaces for (e.g. 'managed').

        @return list of IwNetDev tuples.

        """

        # Parse output in the following format:
        #
        #   $ adb shell iw dev
        #   phy#0
        #     Unnamed/non-netdev interface
        #       wdev 0x2
        #       addr aa:bb:cc:dd:ee:ff
        #       type P2P-device
        #     Interface wlan0
        #       ifindex 4
        #       wdev 0x1
        #       addr aa:bb:cc:dd:ee:ff
        #       ssid Whatever
        #       type managed

        output = self._run('%s dev' % self._command_iw).stdout
        interfaces = []
        phy = None
        if_name = None
        if_type = None
        for line in output.splitlines():
            m = re.match('phy#([0-9]+)', line)
            if m:
                phy = 'phy%d' % int(m.group(1))
                if_name = None
                if_type = None
                continue
            if not phy:
                continue
            m = re.match('[\s]*Interface (.*)', line)
            if m:
                if_name = m.group(1)
                continue
            if not if_name:
                continue
            # Common values for type are 'managed', 'monitor', and 'IBSS'.
            m = re.match('[\s]*type ([a-zA-Z]+)', line)
            if m:
                if_type = m.group(1)
                interfaces.append(IwNetDev(phy=phy, if_name=if_name,
                                           if_type=if_type))
                # One phy may have many interfaces, so don't reset it.
                if_name = None

        if desired_if_type:
            interfaces = [interface for interface in interfaces
                          if interface.if_type == desired_if_type]
        return interfaces


    def list_phys(self):
        """
        List WiFi PHYs on the given host.

        @return list of IwPhy tuples.

        """
        output = self._run('%s list' % self._command_iw).stdout

        pending_phy_name = None
        current_band = None
        current_section = None
        all_phys = []

        def add_pending_phy():
            """Add the pending phy into |all_phys|."""
            bands = tuple(IwBand(band.num,
                                 tuple(band.frequencies),
                                 dict(band.frequency_flags),
                                 tuple(band.mcs_indices))
                          for band in pending_phy_bands)
            new_phy = IwPhy(pending_phy_name,
                            bands,
                            tuple(pending_phy_modes),
                            tuple(pending_phy_commands),
                            tuple(pending_phy_features),
                            pending_phy_max_scan_ssids,
                            pending_phy_tx_antennas,
                            pending_phy_rx_antennas,
                            pending_phy_tx_antennas and pending_phy_rx_antennas,
                            pending_phy_support_vht)
            all_phys.append(new_phy)

        for line in output.splitlines():
            match_phy = re.search('Wiphy (.*)', line)
            if match_phy:
                if pending_phy_name:
                    add_pending_phy()
                pending_phy_name = match_phy.group(1)
                pending_phy_bands = []
                pending_phy_modes = []
                pending_phy_commands = []
                pending_phy_features = []
                pending_phy_max_scan_ssids = None
                pending_phy_tx_antennas = 0
                pending_phy_rx_antennas = 0
                pending_phy_support_vht = False
                continue

            match_section = re.match('\s*(\w.*):\s*$', line)
            if match_section:
                current_section = match_section.group(1)
                match_band = re.match('Band (\d+)', current_section)
                if match_band:
                    current_band = IwBand(num=int(match_band.group(1)),
                                          frequencies=[],
                                          frequency_flags={},
                                          mcs_indices=[])
                    pending_phy_bands.append(current_band)
                continue

            # Check for max_scan_ssids. This isn't a section, but it
            # also isn't within a section.
            match_max_scan_ssids = re.match('\s*max # scan SSIDs: (\d+)',
                                            line)
            if match_max_scan_ssids and pending_phy_name:
                pending_phy_max_scan_ssids = int(
                    match_max_scan_ssids.group(1))
                continue

            if (current_section == 'Supported interface modes' and
                pending_phy_name):
                mode_match = re.search('\* (\w+)', line)
                if mode_match:
                    pending_phy_modes.append(mode_match.group(1))
                    continue

            if current_section == 'Supported commands' and pending_phy_name:
                command_match = re.search('\* (\w+)', line)
                if command_match:
                    pending_phy_commands.append(command_match.group(1))
                    continue

            if (current_section is not None and
                current_section.startswith('VHT Capabilities') and
                pending_phy_name):
                pending_phy_support_vht = True
                continue

            match_avail_antennas = re.match('\s*Available Antennas: TX (\S+)'
                                            ' RX (\S+)', line)
            if match_avail_antennas and pending_phy_name:
                pending_phy_tx_antennas = int(
                        match_avail_antennas.group(1), 16)
                pending_phy_rx_antennas = int(
                        match_avail_antennas.group(2), 16)
                continue

            match_device_support = re.match('\s*Device supports (.*)\.', line)
            if match_device_support and pending_phy_name:
                pending_phy_features.append(match_device_support.group(1))
                continue

            if not all([current_band, pending_phy_name,
                        line.startswith('\t')]):
                continue

            # E.g.
            # * 2412 MHz [1] (20.0 dBm)
            # * 2467 MHz [12] (20.0 dBm) (passive scan)
            # * 2472 MHz [13] (disabled)
            # * 5260 MHz [52] (19.0 dBm) (no IR, radar detection)
            match_chan_info = re.search(
                r'(?P<frequency>\d+) MHz'
                r' (?P<chan_num>\[\d+\])'
                r'(?: \((?P<tx_power_limit>[0-9.]+ dBm)\))?'
                r'(?: \((?P<flags>[a-zA-Z, ]+)\))?', line)
            if match_chan_info:
                frequency = int(match_chan_info.group('frequency'))
                current_band.frequencies.append(frequency)
                flags_string = match_chan_info.group('flags')
                if flags_string:
                    current_band.frequency_flags[frequency] = frozenset(
                        flags_string.split(','))
                else:
                    # Populate the dict with an empty set, to make
                    # things uniform for client code.
                    current_band.frequency_flags[frequency] = frozenset()
                continue

            # re_mcs needs to match something like:
            # HT TX/RX MCS rate indexes supported: 0-15, 32
            if re.search('HT TX/RX MCS rate indexes supported: ', line):
                rate_string = line.split(':')[1].strip()
                for piece in rate_string.split(','):
                    if piece.find('-') > 0:
                        # Must be a range like '  0-15'
                        begin, end = piece.split('-')
                        for index in range(int(begin), int(end) + 1):
                            current_band.mcs_indices.append(index)
                    else:
                        # Must be a single rate like '32   '
                        current_band.mcs_indices.append(int(piece))
        if pending_phy_name:
            add_pending_phy()
        return all_phys


    def remove_interface(self, interface, ignore_status=False):
        """
        Remove a WiFi interface from a PHY.

        @param interface: string name of interface (e.g. mon0)
        @param ignore_status: boolean True iff we should ignore failures
                to remove the interface.

        """
        self._run('%s dev %s del' % (self._command_iw, interface),
                  ignore_status=ignore_status)


    def determine_security(self, supported_securities):
        """Determines security from the given list of supported securities.

        @param supported_securities: list of supported securities from scan

        """
        if not supported_securities:
            security = SECURITY_OPEN
        elif len(supported_securities) == 1:
            security = supported_securities[0]
        else:
            security = SECURITY_MIXED
        return security


    def scan(self, interface, frequencies=(), ssids=()):
        """Performs a scan.

        @param interface: the interface to run the iw command against
        @param frequencies: list of int frequencies in Mhz to scan.
        @param ssids: list of string SSIDs to send probe requests for.

        @returns a list of IwBss namedtuples; None if the scan fails

        """
        scan_result = self.timed_scan(interface, frequencies, ssids)
        if scan_result is None:
            return None
        return scan_result.bss_list


    def timed_scan(self, interface, frequencies=(), ssids=()):
        """Performs a timed scan.

        @param interface: the interface to run the iw command against
        @param frequencies: list of int frequencies in Mhz to scan.
        @param ssids: list of string SSIDs to send probe requests for.

        @returns a IwTimedScan namedtuple; None if the scan fails

        """
        freq_param = ''
        if frequencies:
            freq_param = ' freq %s' % ' '.join(map(str, frequencies))
        ssid_param = ''
        if ssids:
           ssid_param = ' ssid "%s"' % '" "'.join(ssids)

        iw_command = '%s dev %s scan%s%s' % (self._command_iw,
                interface, freq_param, ssid_param)
        command = IW_TIME_COMMAND_FORMAT % iw_command
        scan = self._run(command, ignore_status=True)
        if scan.exit_status != 0:
            # The device was busy
            logging.debug('scan exit_status: %d', scan.exit_status)
            return None
        if not scan.stdout:
            raise error.TestFail('Missing scan parse time')

        if scan.stdout.startswith(IW_TIME_COMMAND_OUTPUT_START):
            logging.debug('Empty scan result')
            bss_list = []
        else:
            bss_list = self._parse_scan_results(scan.stdout)
        scan_time = self._parse_scan_time(scan.stdout)
        return IwTimedScan(scan_time, bss_list)


    def scan_dump(self, interface):
        """Dump the contents of the scan cache.

        Note that this does not trigger a scan.  Instead, it returns
        the kernel's idea of what BSS's are currently visible.

        @param interface: the interface to run the iw command against

        @returns a list of IwBss namedtuples; None if the scan fails

        """
        result = self._run('%s dev %s scan dump' % (self._command_iw,
                                                    interface))
        return self._parse_scan_results(result.stdout)


    def set_tx_power(self, interface, power):
        """
        Set the transmission power for an interface.

        @param interface: string name of interface to set Tx power on.
        @param power: string power parameter. (e.g. 'auto').

        """
        self._run('%s dev %s set txpower %s' %
                  (self._command_iw, interface, power))


    def set_freq(self, interface, freq):
        """
        Set the frequency for an interface.

        @param interface: string name of interface to set frequency on.
        @param freq: int frequency

        """
        self._run('%s dev %s set freq %d' %
                  (self._command_iw, interface, freq))


    def set_regulatory_domain(self, domain_string):
        """
        Set the regulatory domain of the current machine.  Note that
        the regulatory change happens asynchronously to the exit of
        this function.

        @param domain_string: string regulatory domain name (e.g. 'US').

        """
        self._run('%s reg set %s' % (self._command_iw, domain_string))


    def get_regulatory_domain(self):
        """
        Get the regulatory domain of the current machine.

        @returns a string containing the 2-letter regulatory domain name
            (e.g. 'US').

        """
        output = self._run('%s reg get' % self._command_iw).stdout
        m = re.match('^country (..):', output)
        if not m:
            return None
        return m.group(1)


    def wait_for_scan_result(self, interface, bsses=(), ssids=(),
                             timeout_seconds=30, wait_for_all=False):
        """Returns a list of IWBSS objects for given list of bsses or ssids.

        This method will scan for a given timeout and return all of the networks
        that have a matching ssid or bss.  If wait_for_all is true and all
        networks are not found within the given timeout an empty list will
        be returned.

        @param interface: which interface to run iw against
        @param bsses: a list of BSS strings
        @param ssids: a list of ssid strings
        @param timeout_seconds: the amount of time to wait in seconds
        @param wait_for_all: True to wait for all listed bsses or ssids; False
                             to return if any of the networks were found

        @returns a list of IwBss collections that contain the given bss or ssid;
            if the scan is empty or returns an error code None is returned.

        """
        start_time = time.time()
        scan_failure_attempts = 0
        logging.info('Performing a scan with a max timeout of %d seconds.',
                     timeout_seconds)
        remaining_bsses = copy.copy(bsses)
        remaining_ssids = copy.copy(ssids)
        while time.time() - start_time < timeout_seconds:
            scan_results = self.scan(interface)
            if scan_results is None or len(scan_results) == 0:
                scan_failure_attempts += 1
                # Allow in-progress scan to complete
                time.sleep(5)
                # If the in-progress scan takes more than 30 seconds to
                # complete it will most likely never complete; abort.
                # See crbug.com/309148.
                if scan_failure_attempts > 5:
                    logging.error('Scan failed to run, see debug log for '
                                  'error code.')
                    return None
                continue
            scan_failure_attempts = 0
            matching_iwbsses = set()
            for iwbss in scan_results:
              if iwbss.bss in bsses and len(remaining_bsses) > 0:
                    remaining_bsses.remove(iwbss.bss)
                    matching_iwbsses.add(iwbss)
              if iwbss.ssid in ssids and len(remaining_ssids) > 0:
                    remaining_ssids.remove(iwbss.ssid)
                    matching_iwbsses.add(iwbss)
            if wait_for_all:
                if len(remaining_bsses) == 0 and len(remaining_ssids) == 0:
                    return list(matching_iwbsses)
            else:
                if len(matching_iwbsses) > 0:
                    return list(matching_iwbsses)


        if scan_failure_attempts > 0:
            return None
        # The SSID wasn't found, but the device is fine.
        return list()


    def wait_for_link(self, interface, timeout_seconds=10):
        """Waits until a link completes on |interface|.

        @param interface: which interface to run iw against.
        @param timeout_seconds: the amount of time to wait in seconds.

        @returns True if link was established before the timeout.

        """
        start_time = time.time()
        while time.time() - start_time < timeout_seconds:
            link_results = self._run('%s dev %s link' %
                                     (self._command_iw, interface))
            if 'Not connected' not in link_results.stdout:
                return True
            time.sleep(1)
        return False


    def set_antenna_bitmap(self, phy, tx_bitmap, rx_bitmap):
        """Set antenna chain mask on given phy (radio).

        This function will set the antennas allowed to use for TX and
        RX on the |phy| based on the |tx_bitmap| and |rx_bitmap|.
        This command is only allowed when the interfaces on the phy are down.

        @param phy: phy name
        @param tx_bitmap: bitmap of allowed antennas to use for TX
        @param rx_bitmap: bitmap of allowed antennas to use for RX

        """
        command = '%s phy %s set antenna %d %d' % (self._command_iw, phy,
                                                   tx_bitmap, rx_bitmap)
        self._run(command)


    def get_event_logger(self):
        """Create and return a IwEventLogger object.

        @returns a IwEventLogger object.

        """
        local_file = IW_LOCAL_EVENT_LOG_FILE % (self._log_id)
        self._log_id += 1
        return iw_event_logger.IwEventLogger(self._host, self._command_iw,
                                             local_file)


    def vht_supported(self):
        """Returns True if VHT is supported; False otherwise."""
        result = self._run('%s list' % self._command_iw).stdout
        if 'VHT Capabilities' in result:
            return True
        return False


    def frequency_supported(self, frequency):
        """Returns True if the given frequency is supported; False otherwise.

        @param frequency: int Wifi frequency to check if it is supported by
                          DUT.
        """
        phys = self.list_phys()
        for phy in phys:
            for band in phy.bands:
                if frequency in band.frequencies:
                    return True
        return False

#!/usr/bin/python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import pyshark
import os

class PacketCapture(object):
    """ Class to manage the packet capture file access from a chaos test. """

    LOAD_TIMEOUT = 2

    def __init__(self, file_name):
        self._file_name = file_name

    def get_output(self, display_filter=None, summaries=True, decryption=None):
        """
        Gets the packets from a trace file as Pyshark packet objects for
        further analysis.

        @param display_filer: Tshark filter to be used for extracting the
                              relevant packets.
        @param summaries: Flag to indicate whether to extract only the summaries
                          of packet or not.
        @param decryption: Decryption key to be used on the trace file.
        @returns List of pyshark packet objects.

        """
        capture = pyshark.FileCapture(self._file_name,
                                      display_filter=display_filter,
                                      only_summaries=summaries,
                                      decryption_key=decryption,
                                      encryption_type='wpa-pwd')
        capture.load_packets(timeout=self.LOAD_TIMEOUT)
        return capture

    def get_packet_number(self, index, summary):
        """
        Gets the packet that appears index |index| in the capture file.

        @param index: Extract this index from the capture file.
        @param summary: Flag to indicate whether to extract only the summary
                        of the packet or not.

        @returns pyshark packet object or None.

        """
        display_filter = "frame.number == %d" % index
        capture = pyshark.FileCapture(self._file_name,
                                      display_filter=display_filter,
                                      only_summaries=summary)
        capture.load_packets(timeout=self.LOAD_TIMEOUT)
        if not capture:
            return None
        return capture[0]

    def get_packet_after(self, packet):
        """
        Gets the packet that appears next in the capture file.

        @param packet: Reference packet -- the packet after this one will
                       be retrieved.

        @returns pyshark packet object or None.

        """
        return self.get_packet_number(int(packet.number) + 1, summary=False)

    def count_packets_with_display_filter(self, display_filter):
        """
        Counts the number of packets which match the provided display filter.

        @param display_filer: Tshark filter to be used for extracting the
                              relevant packets.
        @returns Number of packets which match the filter.

        """
        output = self.get_output(display_filter=display_filter)
        return len(output)

    def count_packets_from(self, mac_addresses):
        """
        Counts the number of packets sent from a given entity using MAC address.

        @param mac_address: Mac address of the entity.
        @returns Number of packets which matched the MAC address filter.

        """
        filter = ' or '.join(['wlan.ta==%s' % addr for addr in mac_addresses])
        return self.count_packets_with_display_filter(filter)

    def count_packets_to(self, mac_addresses):
        """
        Counts the number of packets sent to a given entity using MAC address.

        @param mac_address: Mac address of the entity.
        @returns Number of packets which matched the MAC address filter.

        """
        filter = ' or '.join(['wlan.ra==%s' % addr for addr in mac_addresses])
        return self.count_packets_with_display_filter(filter)

    def count_packets_from_or_to(self, mac_addresses):
        """
        Counts the number of packets sent to/from a given entity using MAC
        address.

        @param mac_address: Mac address of the entity.
        @returns Number of packets which matched the MAC address filter.

        """
        filter = ' or '.join(['wlan.addr==%s' % addr for addr in mac_addresses])
        return self.count_packets_with_display_filter(filter)

    def count_beacons_from(self, mac_addresses):
        """
        Counts the number of beacon packets sent from a AP using MAC address.

        @param mac_address: Mac address of the AP.
        @returns Number of packets which matched the MAC address filter.

        """
        filter = ' or '.join(['wlan.ta==%s' % addr for addr in mac_addresses])
        filter = '(%s) and wlan.fc.type_subtype == 0x0008' % (filter)
        return self.count_packets_with_display_filter(filter)

    def get_filtered_packets(self, ap, dut, summaries, decryption):
        """
        Gets the packets sent to/from the DUT from a trace file as Pyshark
        packet objects for further analysis.

        @param summaries: Flag to indicate whether to extract only the summaries
                          of packet or not.
        @param dut: Mac address of the DUT.
        @param ap: Mac address of the AP.
        @param decryption: Decryption key to be used on the trace file.
        @returns List of pyshark packet objects.

        """
        filter = 'wlan.addr==%s' % dut
        packets = self.get_output(display_filter=filter, summaries=summaries,
                                  decryption=decryption)
        return packets


class WifiStateMachineAnalyzer(object):
    """ Class to analyze the Wifi Protocol exhcange from a chaos test. """

    STATE_INIT = "INIT"
    STATE_PROBE_REQ = "PROBE_REQ"
    STATE_PROBE_RESP = "PROBE_RESP"
    STATE_AUTH_REQ = "AUTH_REQ"
    STATE_AUTH_RESP = "AUTH_RESP"
    STATE_ASSOC_REQ = "ASSOC_REQ"
    STATE_ASSOC_RESP = "ASSOC_RESP"
    STATE_KEY_MESSAGE_1 = "KEY_MESSAGE_1"
    STATE_KEY_MESSAGE_2 = "KEY_MESSAGE_2"
    STATE_KEY_MESSAGE_3 = "KEY_MESSAGE_3"
    STATE_KEY_MESSAGE_4 = "KEY_MESSAGE_4"
    STATE_DHCP_DISCOVER = "DHCP_DISCOVER"
    STATE_DHCP_OFFER = "DHCP_OFFER"
    STATE_DHCP_REQ = "DHCP_REQ"
    STATE_DHCP_REQ_ACK = "DHCP_REQ_ACK"
    STATE_END = "END"


    PACKET_MATCH_WLAN_FRAME_TYPE = "wlan.fc_type_subtype"
    PACKET_MATCH_WLAN_FRAME_RETRY_FLAG = "wlan.fc_retry"
    PACKET_MATCH_WLAN_MANAGEMENT_REASON_CODE = "wlan_mgt.fixed_reason_code"
    PACKET_MATCH_WLAN_MANAGEMENT_STATUS_CODE = "wlan_mgt.fixed_status_code"
    PACKET_MATCH_WLAN_TRANSMITTER = "wlan.ta"
    PACKET_MATCH_LLC_TYPE = "llc.type"
    PACKET_MATCH_EAP_TYPE = "eapol.type"
    PACKET_MATCH_EAP_KEY_INFO_INSTALL = "eapol.keydes_key_info_install"
    PACKET_MATCH_EAP_KEY_INFO_ACK = "eapol.keydes_key_info_key_ack"
    PACKET_MATCH_EAP_KEY_INFO_MIC = "eapol.keydes_key_info_key_mic"
    PACKET_MATCH_EAP_KEY_INFO_SECURE = "eapol.keydes_key_info_secure"
    PACKET_MATCH_IP_PROTOCOL_TYPE = "ip.proto"
    PACKET_MATCH_DHCP_MESSAGE_TYPE = "bootp.option_dhcp"
    PACKET_MATCH_RADIOTAP_DATA_RATE = "radiotap.datarate"

    WLAN_PROBE_REQ_FRAME_TYPE = '0x04'
    WLAN_PROBE_RESP_FRAME_TYPE = '0x05'
    WLAN_AUTH_REQ_FRAME_TYPE = '0x0b'
    WLAN_AUTH_RESP_FRAME_TYPE = '0x0b'
    WLAN_ASSOC_REQ_FRAME_TYPE = '0x00'
    WLAN_ASSOC_RESP_FRAME_TYPE = '0x01'
    WLAN_ACK_FRAME_TYPE = '0x1d'
    WLAN_DEAUTH_REQ_FRAME_TYPE = '0x0c'
    WLAN_DISASSOC_REQ_FRAME_TYPE = '0x0a'
    WLAN_QOS_DATA_FRAME_TYPE = '0x28'
    WLAN_MANAGEMENT_STATUS_CODE_SUCCESS = '0x0000'
    WLAN_BROADCAST_ADDRESS = 'ff:ff:ff:ff:ff:ff'
    WLAN_FRAME_CONTROL_TYPE_MANAGEMENT = '0'

    WLAN_FRAME_RETRY = '1'

    LLC_AUTH_TYPE = '0x888e'

    EAP_KEY_TYPE = '0x03'

    IP_UDP_PROTOCOL_TYPE = '17'

    DHCP_DISCOVER_MESSAGE_TYPE = '1'
    DHCP_OFFER_MESSAGE_TYPE = '2'
    DHCP_REQUEST_MESSAGE_TYPE = '3'
    DHCP_ACK_MESSAGE_TYPE = '5'

    DIR_TO_DUT = 0
    DIR_FROM_DUT = 1
    DIR_DUT_TO_AP = 2
    DIR_AP_TO_DUT = 3
    DIR_ACK = 4

    # State Info Tuples (Name, Direction, Match fields, Next State)
    StateInfo = collections.namedtuple(
            'StateInfo', ['name', 'direction', 'match_fields', 'next_state'])
    STATE_INFO_INIT = StateInfo("INIT", 0, {}, STATE_PROBE_REQ)
    STATE_INFO_PROBE_REQ = StateInfo("WLAN PROBE REQUEST",
                                     DIR_FROM_DUT,
                                     { PACKET_MATCH_WLAN_FRAME_TYPE:
                                       WLAN_PROBE_REQ_FRAME_TYPE },
                                     STATE_PROBE_RESP)
    STATE_INFO_PROBE_RESP = StateInfo("WLAN PROBE RESPONSE",
                                      DIR_AP_TO_DUT,
                                      { PACKET_MATCH_WLAN_FRAME_TYPE:
                                        WLAN_PROBE_RESP_FRAME_TYPE },
                                      STATE_AUTH_REQ)
    STATE_INFO_AUTH_REQ = StateInfo("WLAN AUTH REQUEST",
                                    DIR_DUT_TO_AP,
                                    { PACKET_MATCH_WLAN_FRAME_TYPE:
                                      WLAN_AUTH_REQ_FRAME_TYPE },
                                    STATE_AUTH_RESP)
    STATE_INFO_AUTH_RESP = StateInfo(
            "WLAN AUTH RESPONSE",
            DIR_AP_TO_DUT,
            { PACKET_MATCH_WLAN_FRAME_TYPE: WLAN_AUTH_REQ_FRAME_TYPE,
              PACKET_MATCH_WLAN_MANAGEMENT_STATUS_CODE:
              WLAN_MANAGEMENT_STATUS_CODE_SUCCESS },
            STATE_ASSOC_REQ)
    STATE_INFO_ASSOC_REQ = StateInfo("WLAN ASSOC REQUEST",
                                     DIR_DUT_TO_AP,
                                     { PACKET_MATCH_WLAN_FRAME_TYPE:
                                       WLAN_ASSOC_REQ_FRAME_TYPE },
                                     STATE_ASSOC_RESP)
    STATE_INFO_ASSOC_RESP = StateInfo(
              "WLAN ASSOC RESPONSE",
              DIR_AP_TO_DUT,
              { PACKET_MATCH_WLAN_FRAME_TYPE: WLAN_ASSOC_RESP_FRAME_TYPE,
                PACKET_MATCH_WLAN_MANAGEMENT_STATUS_CODE:
                WLAN_MANAGEMENT_STATUS_CODE_SUCCESS },
              STATE_KEY_MESSAGE_1)
    STATE_INFO_KEY_MESSAGE_1 = StateInfo("WPA KEY MESSAGE 1",
                                         DIR_AP_TO_DUT,
                                         { PACKET_MATCH_LLC_TYPE:
                                           LLC_AUTH_TYPE,
                                           PACKET_MATCH_EAP_KEY_INFO_INSTALL:
                                           '0',
                                           PACKET_MATCH_EAP_KEY_INFO_ACK:
                                           '1',
                                           PACKET_MATCH_EAP_KEY_INFO_MIC:
                                           '0',
                                           PACKET_MATCH_EAP_KEY_INFO_SECURE:
                                           '0' },
                                         STATE_KEY_MESSAGE_2)
    STATE_INFO_KEY_MESSAGE_2 = StateInfo("WPA KEY MESSAGE 2",
                                         DIR_DUT_TO_AP,
                                         { PACKET_MATCH_LLC_TYPE:
                                           LLC_AUTH_TYPE,
                                           PACKET_MATCH_EAP_KEY_INFO_INSTALL:
                                           '0',
                                           PACKET_MATCH_EAP_KEY_INFO_ACK:
                                           '0',
                                           PACKET_MATCH_EAP_KEY_INFO_MIC:
                                           '1',
                                           PACKET_MATCH_EAP_KEY_INFO_SECURE:
                                           '0' },
                                         STATE_KEY_MESSAGE_3)
    STATE_INFO_KEY_MESSAGE_3 = StateInfo("WPA KEY MESSAGE 3",
                                         DIR_AP_TO_DUT,
                                         { PACKET_MATCH_LLC_TYPE:
                                           LLC_AUTH_TYPE,
                                           PACKET_MATCH_EAP_KEY_INFO_INSTALL:
                                           '1',
                                           PACKET_MATCH_EAP_KEY_INFO_ACK:
                                           '1',
                                           PACKET_MATCH_EAP_KEY_INFO_MIC:
                                           '1',
                                           PACKET_MATCH_EAP_KEY_INFO_SECURE:
                                           '1' },
                                         STATE_KEY_MESSAGE_4)
    STATE_INFO_KEY_MESSAGE_4 = StateInfo("WPA KEY MESSAGE 4",
                                         DIR_DUT_TO_AP,
                                         { PACKET_MATCH_LLC_TYPE:
                                           LLC_AUTH_TYPE,
                                           PACKET_MATCH_EAP_KEY_INFO_INSTALL:
                                           '0',
                                           PACKET_MATCH_EAP_KEY_INFO_ACK:
                                           '0',
                                           PACKET_MATCH_EAP_KEY_INFO_MIC:
                                           '1',
                                           PACKET_MATCH_EAP_KEY_INFO_SECURE:
                                           '1' },
                                         STATE_DHCP_DISCOVER)
    STATE_INFO_DHCP_DISCOVER = StateInfo("DHCP DISCOVER",
                                         DIR_DUT_TO_AP,
                                         { PACKET_MATCH_IP_PROTOCOL_TYPE:
                                           IP_UDP_PROTOCOL_TYPE,
                                           PACKET_MATCH_DHCP_MESSAGE_TYPE:
                                           DHCP_DISCOVER_MESSAGE_TYPE },
                                         STATE_DHCP_OFFER)
    STATE_INFO_DHCP_OFFER = StateInfo("DHCP OFFER",
                                      DIR_AP_TO_DUT,
                                      { PACKET_MATCH_IP_PROTOCOL_TYPE:
                                        IP_UDP_PROTOCOL_TYPE,
                                        PACKET_MATCH_DHCP_MESSAGE_TYPE:
                                        DHCP_OFFER_MESSAGE_TYPE },
                                      STATE_DHCP_REQ)
    STATE_INFO_DHCP_REQ = StateInfo("DHCP REQUEST",
                                    DIR_DUT_TO_AP,
                                    { PACKET_MATCH_IP_PROTOCOL_TYPE:
                                      IP_UDP_PROTOCOL_TYPE,
                                      PACKET_MATCH_DHCP_MESSAGE_TYPE:
                                      DHCP_REQUEST_MESSAGE_TYPE },
                                    STATE_DHCP_REQ_ACK)
    STATE_INFO_DHCP_REQ_ACK = StateInfo("DHCP ACK",
                                        DIR_AP_TO_DUT,
                                        { PACKET_MATCH_IP_PROTOCOL_TYPE:
                                          IP_UDP_PROTOCOL_TYPE,
                                          PACKET_MATCH_DHCP_MESSAGE_TYPE:
                                          DHCP_ACK_MESSAGE_TYPE },
                                        STATE_END)
    STATE_INFO_END = StateInfo("END", 0, {}, STATE_END)
    # Master State Table Map of State Infos
    STATE_INFO_MAP = {STATE_INIT:         STATE_INFO_INIT,
                      STATE_PROBE_REQ:    STATE_INFO_PROBE_REQ,
                      STATE_PROBE_RESP:   STATE_INFO_PROBE_RESP,
                      STATE_AUTH_REQ:     STATE_INFO_AUTH_REQ,
                      STATE_AUTH_RESP:    STATE_INFO_AUTH_RESP,
                      STATE_ASSOC_REQ:    STATE_INFO_ASSOC_REQ,
                      STATE_ASSOC_RESP:   STATE_INFO_ASSOC_RESP,
                      STATE_KEY_MESSAGE_1:STATE_INFO_KEY_MESSAGE_1,
                      STATE_KEY_MESSAGE_2:STATE_INFO_KEY_MESSAGE_2,
                      STATE_KEY_MESSAGE_3:STATE_INFO_KEY_MESSAGE_3,
                      STATE_KEY_MESSAGE_4:STATE_INFO_KEY_MESSAGE_4,
                      STATE_DHCP_DISCOVER:STATE_INFO_DHCP_DISCOVER,
                      STATE_DHCP_OFFER:   STATE_INFO_DHCP_OFFER,
                      STATE_DHCP_REQ:     STATE_INFO_DHCP_REQ,
                      STATE_DHCP_REQ_ACK: STATE_INFO_DHCP_REQ_ACK,
                      STATE_END:          STATE_INFO_END}

    # Packet Details Tuples (User friendly name, Field name)
    PacketDetail = collections.namedtuple(
            "PacketDetail", ["friendly_name", "field_name"])
    PACKET_DETAIL_REASON_CODE = PacketDetail(
            "Reason Code",
            PACKET_MATCH_WLAN_MANAGEMENT_REASON_CODE)
    PACKET_DETAIL_STATUS_CODE = PacketDetail(
            "Status Code",
            PACKET_MATCH_WLAN_MANAGEMENT_STATUS_CODE)
    PACKET_DETAIL_SENDER = PacketDetail(
            "Sender", PACKET_MATCH_WLAN_TRANSMITTER)

    # Error State Info Tuples (Name, Match fields)
    ErrorStateInfo = collections.namedtuple(
            'ErrorStateInfo', ['name', 'match_fields', 'details'])
    ERROR_STATE_INFO_DEAUTH = ErrorStateInfo("WLAN DEAUTH REQUEST",
                                             { PACKET_MATCH_WLAN_FRAME_TYPE:
                                               WLAN_DEAUTH_REQ_FRAME_TYPE },
                                             [ PACKET_DETAIL_SENDER,
                                               PACKET_DETAIL_REASON_CODE ])
    ERROR_STATE_INFO_DEASSOC = ErrorStateInfo("WLAN DISASSOC REQUEST",
                                            { PACKET_MATCH_WLAN_FRAME_TYPE:
                                              WLAN_DISASSOC_REQ_FRAME_TYPE },
                                            [ PACKET_DETAIL_SENDER,
                                              PACKET_DETAIL_REASON_CODE ])
    # Master State Table Tuple of Error State Infos
    ERROR_STATE_INFO_TUPLE = (ERROR_STATE_INFO_DEAUTH, ERROR_STATE_INFO_DEASSOC)

    # These warnings actually match successful states, but since the we
    # check forwards and backwards through the state machine for the successful
    # version of these packets, they can only match a failure.
    WARNING_INFO_AUTH_REJ = ErrorStateInfo(
            "WLAN AUTH REJECTED",
            { PACKET_MATCH_WLAN_FRAME_TYPE: WLAN_AUTH_REQ_FRAME_TYPE },
            [ PACKET_DETAIL_STATUS_CODE ])
    WARNING_INFO_ASSOC_REJ = ErrorStateInfo(
            "WLAN ASSOC REJECTED",
            { PACKET_MATCH_WLAN_FRAME_TYPE: WLAN_ASSOC_RESP_FRAME_TYPE },
            [ PACKET_DETAIL_STATUS_CODE ])

    # Master Table Tuple of warning information.
    WARNING_INFO_TUPLE = (WARNING_INFO_AUTH_REJ, WARNING_INFO_ASSOC_REJ)


    def __init__(self, ap_macs, dut_mac, filtered_packets, capture, logger):
        self._current_state = self._get_state(self.STATE_INIT)
        self._reached_states = []
        self._skipped_states = []
        self._packets = filtered_packets
        self._capture = capture
        self._dut_mac = dut_mac
        self._ap_macs = ap_macs
        self._log = logger
        self._acks = []

    @property
    def acks(self):
        return self._acks

    def _get_state(self, state):
        return self.STATE_INFO_MAP[state]

    def _get_next_state(self, state):
        return self._get_state(state.next_state)

    def _get_curr_next_state(self):
        return self._get_next_state(self._current_state)

    def _fetch_packet_field_value(self, packet, field):
        layer_object = packet
        for layer in field.split('.'):
            try:
                layer_object = getattr(layer_object, layer)
            except AttributeError:
                return None
        return layer_object

    def _match_packet_fields(self, packet, fields):
        for field, exp_value in fields.items():
            value = self._fetch_packet_field_value(packet, field)
            if exp_value != value:
                return False
        return True

    def _fetch_packet_data_rate(self, packet):
        return self._fetch_packet_field_value(packet,
                self.PACKET_MATCH_RADIOTAP_DATA_RATE)

    def _does_packet_match_state(self, state, packet):
        fields = state.match_fields
        if self._match_packet_fields(packet, fields):
            if state.direction == self.DIR_TO_DUT:
                # This should have receiver addr of DUT
                if packet.wlan.ra == self._dut_mac:
                    return True
            elif state.direction == self.DIR_FROM_DUT:
                # This should have transmitter addr of DUT
                if packet.wlan.ta == self._dut_mac:
                    return True
            elif state.direction == self.DIR_AP_TO_DUT:
                # This should have receiver addr of DUT &
                # transmitter addr of AP's
                if ((packet.wlan.ra == self._dut_mac) and
                    (packet.wlan.ta in self._ap_macs)):
                    return True
            elif state.direction == self.DIR_DUT_TO_AP:
                # This should have transmitter addr of DUT &
                # receiver addr of AP's
                if ((packet.wlan.ta == self._dut_mac) and
                    (packet.wlan.ra in self._ap_macs)):
                    return True
        return False

    def _does_packet_match_error_state(self, state, packet):
        fields = state.match_fields
        return self._match_packet_fields(packet, fields)

    def _get_packet_detail(self, details, packet):
        attributes = []
        attributes.append("Packet number: %s" % packet.number)
        for detail in details:
            value = self._fetch_packet_field_value(packet, detail.field_name)
            attributes.append("%s: %s" % (detail.friendly_name, value))
        return attributes

    def _does_packet_match_ack_state(self, packet):
        fields = { self.PACKET_MATCH_WLAN_FRAME_TYPE: self.WLAN_ACK_FRAME_TYPE }
        return self._match_packet_fields(packet, fields)

    def _does_packet_contain_retry_flag(self, packet):
        fields = { self.PACKET_MATCH_WLAN_FRAME_RETRY_FLAG:
                   self.WLAN_FRAME_RETRY }
        return self._match_packet_fields(packet, fields)

    def _check_for_ack(self, state, packet):
        if (packet.wlan.da == self.WLAN_BROADCAST_ADDRESS and
            packet.wlan.fc_type == self.WLAN_FRAME_CONTROL_TYPE_MANAGEMENT):
            # Broadcast management frames are not ACKed.
            return True
        next_packet = self._capture.get_packet_after(packet)
        if not next_packet or not (
                (self._does_packet_match_ack_state(next_packet)) and
                (next_packet.wlan.addr == packet.wlan.ta)):
            msg = "WARNING! Missing ACK for state: " + \
                  state.name + "."
            self._log.log_to_output_file(msg)
            return False
        self._acks.append(int(next_packet.number))
        return True

    def _check_for_error(self, packet):
        for error_state in self.ERROR_STATE_INFO_TUPLE:
            if self._does_packet_match_error_state(error_state, packet):
                error_attributes = self._get_packet_detail(error_state.details,
                                                           packet)
                msg = "ERROR! State Machine encountered error due to " + \
                      error_state.name + ", " + \
                      ", ".join(error_attributes) + "."
                self._log.log_to_output_file(msg)
                return True
        return False

    def _check_for_warning(self, packet):
        for warning in self.WARNING_INFO_TUPLE:
            if self._does_packet_match_error_state(warning, packet):
                error_attributes = self._get_packet_detail(warning.details,
                                                           packet)
                msg = "WARNING! " + warning.name + " found, " + \
                      ", ".join(error_attributes) + "."
                self._log.log_to_output_file(msg)
                return True
        return False

    def _check_for_repeated_state(self, packet):
        for state in self._reached_states:
            if self._does_packet_match_state(state, packet):
                msg = "WARNING! Repeated State: " + \
                      state.name + ", Packet number: " + \
                      str(packet.number)
                if self._does_packet_contain_retry_flag(packet):
                    msg += " due to retransmission."
                else:
                    msg +=  "."
                self._log.log_to_output_file(msg)

    def _is_from_previous_state(self, packet):
        for state in self._reached_states + self._skipped_states:
            if self._does_packet_match_state(state, packet):
                return True
        return False

    def _step(self, reached_state, packet):
        # We missed a few packets in between
        if self._current_state != reached_state:
            msg = "WARNING! Missed states: "
            skipped_state = self._current_state
            while skipped_state != reached_state:
                msg += skipped_state.name + ", "
                self._skipped_states.append(skipped_state)
                skipped_state = self._get_next_state(skipped_state)
            msg = msg[:-2]
            msg += "."
            self._log.log_to_output_file(msg)
        msg = "Found state: " + reached_state.name
        if packet:
            msg += ", Packet number: " + str(packet.number) + \
                   ", Data rate: " + str(self._fetch_packet_data_rate(packet))+\
                   "Mbps."
        else:
            msg += "."
        self._log.log_to_output_file(msg)
        # Ignore the Init state in the reached states
        if packet:
            self._reached_states.append(reached_state)
        self._current_state = self._get_next_state(reached_state)

    def _step_init(self):
        #self.log_to_output_file("Starting Analysis")
        self._current_state = self._get_curr_next_state()

    def analyze(self):
        """ Starts the analysis of the Wifi Protocol Exchange. """

        # Start the state machine iteration
        self._step_init()
        packet_iterator = iter(self._packets)
        for packet in packet_iterator:
            self._check_for_repeated_state(packet)
            # Try to look ahead in the state machine to account for occasional
            # packet capture misses.
            next_state = self._current_state
            while next_state != self.STATE_INFO_END:
                if self._does_packet_match_state(next_state, packet):
                    self._step(next_state, packet)
                    self._check_for_ack(next_state, packet)
                    break
                next_state = self._get_next_state(next_state)
            if self._current_state == self.STATE_INFO_END:
                self._log.log_to_output_file("State Machine completed!")
                return True
            if self._check_for_error(packet):
                return False
            if not self._is_from_previous_state(packet):
                self._check_for_warning(packet)
        msg = "ERROR! State Machine halted at " + self._current_state.name + \
              " state."
        self._log.log_to_output_file(msg)
        return False


class ChaosCaptureAnalyzer(object):
    """ Class to analyze the packet capture from a chaos test . """

    def __init__(self, ap_bssids, ap_ssid, dut_mac, logger):
        self._ap_bssids = ap_bssids
        self._ap_ssid = ap_ssid
        self._dut_mac = dut_mac
        self._log = logger

    def _validate_ap_presence(self, capture, bssids, ssid):
        beacon_count = capture.count_beacons_from(bssids)
        if not beacon_count:
            packet_count = capture.count_packets_from(bssids)
            if not packet_count:
                self._log.log_to_output_file(
                        "No packets at all from AP BSSIDs %r!" % bssids)
            else:
                self._log.log_to_output_file(
                        "No beacons from AP BSSIDs %r but %d packets!" %
                        (bssids, packet_count))
            return False
        self._log.log_to_output_file("AP BSSIDs: %s, SSID: %s." %
                                     (bssids, ssid))
        self._log.log_to_output_file("AP beacon count: %d." % beacon_count)
        return True

    def _validate_dut_presence(self, capture, dut_mac):
        tx_count = capture.count_packets_from([dut_mac])
        if not tx_count:
            self._log.log_to_output_file(
                    "No packets Tx at all from DUT MAC %r!" % dut_mac)
            return False
        rx_count = capture.count_packets_to([dut_mac])
        self._log.log_to_output_file("DUT MAC: %s." % dut_mac)
        self._log.log_to_output_file(
                "DUT packet count Tx: %d, Rx: %d." % (tx_count, rx_count))
        return True

    def _ack_interleave(self, packets, capture, acks):
        """Generator that interleaves packets with their associated ACKs."""
        for packet in packets:
            packet_number = int(packet.no)
            while acks and acks[0] < packet_number:
                # ACK packet does not appear in the filtered capture.
                yield capture.get_packet_number(acks.pop(0), summary=True)
            if acks and acks[0] == packet_number:
                # ACK packet also appears in the capture.
                acks.pop(0)
            yield packet

    def analyze(self, trace):
        """
        Starts the analysis of the Chaos capture.

        @param trace: Packet capture file path to analyze.

        """
        basename = os.path.basename(trace)
        self._log.log_start_section("Packet Capture File: %s" % basename)
        capture = PacketCapture(trace)
        bssids = self._ap_bssids
        ssid =  self._ap_ssid
        if not self._validate_ap_presence(capture, bssids, ssid):
            return
        dut_mac = self._dut_mac
        if not self._validate_dut_presence(capture, dut_mac):
            return
        decryption = 'chromeos:%s' % ssid
        self._log.log_start_section("WLAN Protocol Verification")
        filtered_packets = capture.get_filtered_packets(
               bssids, dut_mac, False, decryption)
        wifi_state_machine = WifiStateMachineAnalyzer(
               bssids, dut_mac, filtered_packets, capture, self._log)
        wifi_state_machine.analyze()
        self._log.log_start_section("Filtered Packet Capture Summary")
        filtered_packets = capture.get_filtered_packets(
               bssids, dut_mac, True, decryption)
        for packet in self._ack_interleave(
               filtered_packets, capture, wifi_state_machine.acks):
            self._log.log_to_output_file("%s" % (packet))

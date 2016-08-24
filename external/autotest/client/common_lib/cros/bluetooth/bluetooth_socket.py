# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import array
import btsocket
import fcntl
import logging
import socket
import struct


# Constants from lib/mgmt.h in BlueZ source
MGMT_INDEX_NONE = 0xFFFF

MGMT_HDR_SIZE = 6

MGMT_STATUS_SUCCESS            = 0x00
MGMT_STATUS_UNKNOWN_COMMAND    = 0x01
MGMT_STATUS_NOT_CONNECTED      = 0x02
MGMT_STATUS_FAILED             = 0x03
MGMT_STATUS_CONNECT_FAILED     = 0x04
MGMT_STATUS_AUTH_FAILED        = 0x05
MGMT_STATUS_NOT_PAIRED         = 0x06
MGMT_STATUS_NO_RESOURCES       = 0x07
MGMT_STATUS_TIMEOUT            = 0x08
MGMT_STATUS_ALREADY_CONNECTED  = 0x09
MGMT_STATUS_BUSY               = 0x0a
MGMT_STATUS_REJECTED           = 0x0b
MGMT_STATUS_NOT_SUPPORTED      = 0x0c
MGMT_STATUS_INVALID_PARAMS     = 0x0d
MGMT_STATUS_DISCONNECTED       = 0x0e
MGMT_STATUS_NOT_POWERED        = 0x0f
MGMT_STATUS_CANCELLED          = 0x10
MGMT_STATUS_INVALID_INDEX      = 0x11
MGMT_STATUS_RFKILLED           = 0x12

MGMT_OP_READ_VERSION           = 0x0001
MGMT_OP_READ_COMMANDS          = 0x0002
MGMT_OP_READ_INDEX_LIST        = 0x0003
MGMT_OP_READ_INFO              = 0x0004
MGMT_OP_SET_POWERED            = 0x0005
MGMT_OP_SET_DISCOVERABLE       = 0x0006
MGMT_OP_SET_CONNECTABLE        = 0x0007
MGMT_OP_SET_FAST_CONNECTABLE   = 0x0008
MGMT_OP_SET_PAIRABLE           = 0x0009
MGMT_OP_SET_LINK_SECURITY      = 0x000A
MGMT_OP_SET_SSP                = 0x000B
MGMT_OP_SET_HS                 = 0x000C
MGMT_OP_SET_LE                 = 0x000D
MGMT_OP_SET_DEV_CLASS          = 0x000E
MGMT_OP_SET_LOCAL_NAME         = 0x000F
MGMT_OP_ADD_UUID               = 0x0010
MGMT_OP_REMOVE_UUID            = 0x0011
MGMT_OP_LOAD_LINK_KEYS         = 0x0012
MGMT_OP_LOAD_LONG_TERM_KEYS    = 0x0013
MGMT_OP_DISCONNECT             = 0x0014
MGMT_OP_GET_CONNECTIONS        = 0x0015
MGMT_OP_PIN_CODE_REPLY         = 0x0016
MGMT_OP_PIN_CODE_NEG_REPLY     = 0x0017
MGMT_OP_SET_IO_CAPABILITY      = 0x0018
MGMT_OP_PAIR_DEVICE            = 0x0019
MGMT_OP_CANCEL_PAIR_DEVICE     = 0x001A
MGMT_OP_UNPAIR_DEVICE          = 0x001B
MGMT_OP_USER_CONFIRM_REPLY     = 0x001C
MGMT_OP_USER_CONFIRM_NEG_REPLY = 0x001D
MGMT_OP_USER_PASSKEY_REPLY     = 0x001E
MGMT_OP_USER_PASSKEY_NEG_REPLY = 0x001F
MGMT_OP_READ_LOCAL_OOB_DATA    = 0x0020
MGMT_OP_ADD_REMOTE_OOB_DATA    = 0x0021
MGMT_OP_REMOVE_REMOTE_OOB_DATA = 0x0022
MGMT_OP_START_DISCOVERY        = 0x0023
MGMT_OP_STOP_DISCOVERY         = 0x0024
MGMT_OP_CONFIRM_NAME           = 0x0025
MGMT_OP_BLOCK_DEVICE           = 0x0026
MGMT_OP_UNBLOCK_DEVICE         = 0x0027
MGMT_OP_SET_DEVICE_ID          = 0x0028
MGMT_OP_SET_ADVERTISING        = 0x0029
MGMT_OP_SET_BREDR              = 0x002A
MGMT_OP_SET_STATIC_ADDRESS     = 0x002B
MGMT_OP_SET_SCAN_PARAMS        = 0x002C
MGMT_OP_SET_SECURE_CONN        = 0x002D
MGMT_OP_SET_DEBUG_KEYS         = 0x002E
MGMT_OP_SET_PRIVACY            = 0x002F
MGMT_OP_LOAD_IRKS              = 0x0030
MGMT_OP_GET_CONN_INFO          = 0x0031
MGMT_OP_GET_CLOCK_INFO         = 0x0032
MGMT_OP_ADD_DEVICE             = 0x0033
MGMT_OP_REMOVE_DEVICE          = 0x0034
MGMT_OP_LOAD_CONN_PARAM        = 0x0035
MGMT_OP_READ_UNCONF_INDEX_LIST = 0x0036
MGMT_OP_READ_CONFIG_INFO       = 0x0037
MGMT_OP_SET_EXTERNAL_CONFIG    = 0x0038
MGMT_OP_SET_PUBLIC_ADDRESS     = 0x0039

MGMT_EV_CMD_COMPLETE           = 0x0001
MGMT_EV_CMD_STATUS             = 0x0002
MGMT_EV_CONTROLLER_ERROR       = 0x0003
MGMT_EV_INDEX_ADDED            = 0x0004
MGMT_EV_INDEX_REMOVED          = 0x0005
MGMT_EV_NEW_SETTINGS           = 0x0006
MGMT_EV_CLASS_OF_DEV_CHANGED   = 0x0007
MGMT_EV_LOCAL_NAME_CHANGED     = 0x0008
MGMT_EV_NEW_LINK_KEY           = 0x0009
MGMT_EV_NEW_LONG_TERM_KEY      = 0x000A
MGMT_EV_DEVICE_CONNECTED       = 0x000B
MGMT_EV_DEVICE_DISCONNECTED    = 0x000C
MGMT_EV_CONNECT_FAILED         = 0x000D
MGMT_EV_PIN_CODE_REQUEST       = 0x000E
MGMT_EV_USER_CONFIRM_REQUEST   = 0x000F
MGMT_EV_USER_PASSKEY_REQUEST   = 0x0010
MGMT_EV_AUTH_FAILED            = 0x0011
MGMT_EV_DEVICE_FOUND           = 0x0012
MGMT_EV_DISCOVERING            = 0x0013
MGMT_EV_DEVICE_BLOCKED         = 0x0014
MGMT_EV_DEVICE_UNBLOCKED       = 0x0015
MGMT_EV_DEVICE_UNPAIRED        = 0x0016
MGMT_EV_PASSKEY_NOTIFY         = 0x0017
MGMT_EV_NEW_IRK                = 0x0018
MGMT_EV_NEW_CSRK               = 0x0019
MGMT_EV_DEVICE_ADDED           = 0x001a
MGMT_EV_DEVICE_REMOVED         = 0x001b
MGMT_EV_NEW_CONN_PARAM         = 0x001c
MGMT_EV_UNCONF_INDEX_ADDED     = 0x001d
MGMT_EV_UNCONF_INDEX_REMOVED   = 0x001e
MGMT_EV_NEW_CONFIG_OPTIONS     = 0x001f

# Settings returned by MGMT_OP_READ_INFO
MGMT_SETTING_POWERED            = 0x00000001
MGMT_SETTING_CONNECTABLE        = 0x00000002
MGMT_SETTING_FAST_CONNECTABLE   = 0x00000004
MGMT_SETTING_DISCOVERABLE       = 0x00000008
MGMT_SETTING_PAIRABLE           = 0x00000010
MGMT_SETTING_LINK_SECURITY      = 0x00000020
MGMT_SETTING_SSP                = 0x00000040
MGMT_SETTING_BREDR              = 0x00000080
MGMT_SETTING_HS                 = 0x00000100
MGMT_SETTING_LE                 = 0x00000200
MGMT_SETTING_ADVERTISING        = 0x00000400
MGMT_SETTING_SECURE_CONNECTIONS = 0x00000800
MGMT_SETTING_DEBUG_KEYS         = 0x00001000
MGMT_SETTING_PRIVACY            = 0x00002000
MGMT_SETTING_CONTROLLER_CONFIG  = 0x00004000

# Options returned by MGMT_OP_READ_CONFIG_INFO
MGMT_OPTION_EXTERNAL_CONFIG    = 0x00000001
MGMT_OPTION_PUBLIC_ADDRESS     = 0x00000002

# Disconnect reason returned in MGMT_EV_DEVICE_DISCONNECTED
MGMT_DEV_DISCONN_UNKNOWN       = 0x00
MGMT_DEV_DISCONN_TIMEOUT       = 0x01
MGMT_DEV_DISCONN_LOCAL_HOST    = 0x02
MGMT_DEV_DISCONN_REMOTE        = 0x03

# Flags returned in MGMT_EV_DEVICE_FOUND
MGMT_DEV_FOUND_CONFIRM_NAME    = 0x01
MGMT_DEV_FOUND_LEGACY_PAIRING  = 0x02


# EIR Data field types
EIR_FLAGS                      = 0x01
EIR_UUID16_SOME                = 0x02
EIR_UUID16_ALL                 = 0x03
EIR_UUID32_SOME                = 0x04
EIR_UUID32_ALL                 = 0x05
EIR_UUID128_SOME               = 0x06
EIR_UUID128_ALL                = 0x07
EIR_NAME_SHORT                 = 0x08
EIR_NAME_COMPLETE              = 0x09
EIR_TX_POWER                   = 0x0A
EIR_CLASS_OF_DEV               = 0x0D
EIR_SSP_HASH                   = 0x0E
EIR_SSP_RANDOMIZER             = 0x0F
EIR_DEVICE_ID                  = 0x10
EIR_GAP_APPEARANCE             = 0x19


# Derived from lib/hci.h
HCIGETDEVLIST                  = 0x800448d2
HCIGETDEVINFO                  = 0x800448d3

HCI_UP                         = 1 << 0
HCI_INIT                       = 1 << 1
HCI_RUNNING                    = 1 << 2
HCI_PSCAN                      = 1 << 3
HCI_ISCAN                      = 1 << 4
HCI_AUTH                       = 1 << 5
HCI_ENCRYPT                    = 1 << 6
HCI_INQUIRY                    = 1 << 7
HCI_RAW                        = 1 << 8


def parse_eir(eirdata):
    """Parse Bluetooth Extended Inquiry Result (EIR) data structuree.

    @param eirdata: Encoded eir data structure.

    @return Dictionary equivalent to the expanded structure keyed by EIR_*
            fields, with any data members parsed to useful formats.

    """
    fields = {}
    pos = 0
    while pos < len(eirdata):
        # Byte at the current position is the field length, which should be
        # zero at the end of the structure.
        (field_len,) = struct.unpack('B', buffer(eirdata, pos, 1))
        if field_len == 0:
            break
        # Next byte is the field type, and the rest of the field is the data.
        # Note that the length field doesn't include itself so that's why the
        # offsets and lengths look a little odd.
        (field_type,) = struct.unpack('B', buffer(eirdata, pos + 1, 1))
        data = eirdata[pos+2:pos+field_len+1]
        pos += field_len + 1
        # Parse the individual fields to make the data meaningful.
        if field_type == EIR_NAME_SHORT or field_type == EIR_NAME_COMPLETE:
            data = data.rstrip('\0')
        # Place in the dictionary keyed by type.
        fields[field_type] = data

    return fields



class BluetoothSocketError(Exception):
    """Error raised for general issues with BluetoothSocket."""
    pass

class BluetoothInvalidPacketError(Exception):
    """Error raised when an invalid packet is received from the socket."""
    pass

class BluetoothControllerError(Exception):
    """Error raised when the Controller Error event is received."""
    pass


class BluetoothSocket(btsocket.socket):
    """Bluetooth Socket.

    BluetoothSocket wraps the btsocket.socket() class, and thus the system
    socket.socket() class, to implement the necessary send and receive methods
    for the HCI Control and Monitor protocols (aka mgmt_ops) of the
    Linux Kernel.

    Instantiate either BluetoothControlSocket or BluetoothRawSocket rather
    than this class directly.

    See bluez/doc/mgmt_api.txt for details.

    """

    def __init__(self):
        super(BluetoothSocket, self).__init__(family=btsocket.AF_BLUETOOTH,
                                              type=socket.SOCK_RAW,
                                              proto=btsocket.BTPROTO_HCI)
        self.events = []


    def send_command(self, code, index, data=''):
        """Send a command to the socket.

        To send a command, wait for the reply event, and parse it use
        send_command_and_wait() instead.

        @param code: Command Code.
        @param index: Controller index, may be MGMT_INDEX_NONE.
        @param data: Parameters as bytearray or str (optional).

        """
        # Send the command to the kernel
        msg = struct.pack('<HHH', code, index, len(data)) + data

        length = self.send(msg)
        if length != len(msg):
            raise BluetoothSocketError('Short write on socket')


    def recv_event(self):
        """Receive a single event from the socket.

        The event data is not parsed; in the case of command complete events
        this means it includes both the data for the event and the response
        for the command.

        Use settimeout() to set whether this method will block if there is no
        data, return immediately or wait for a specific length of time before
        timing out and raising TimeoutError.

        @return tuple of (event, index, data)

        """
        # Read the message from the socket
        hdr = bytearray(MGMT_HDR_SIZE)
        data = bytearray(512)
        try:
            (nbytes, ancdata, msg_flags, address) = self.recvmsg_into(
                    (hdr, data))
        except btsocket.timeout as e:
            raise BluetoothSocketError('Error receiving event: %s' % e)
        if nbytes < MGMT_HDR_SIZE:
            raise BluetoothInvalidPacketError('Packet shorter than header')

        # Parse the header
        (event, index, length) = struct.unpack_from('<HHH', buffer(hdr))
        if nbytes < MGMT_HDR_SIZE + length:
            raise BluetoothInvalidPacketError('Packet shorter than length')

        return (event, index, data[:length])


    def send_command_and_wait(self, cmd_code, cmd_index, cmd_data='',
                              expected_length=None):
        """Send a command to the socket and wait for the reply.

        Additional events are appended to the events list of the socket object.

        @param cmd_code: Command Code.
        @param cmd_index: Controller index, may be btsocket.HCI_DEV_NONE.
        @param cmd_data: Parameters as bytearray or str.
        @param expected_length: May be set to verify the length of the data.

        Use settimeout() to set whether this method will block if there is no
        reply, return immediately or wait for a specific length of time before
        timing out and raising TimeoutError.

        @return tuple of (status, data)

        """
        self.send_command(cmd_code, cmd_index, cmd_data)

        while True:
            (event, index, data) = self.recv_event()

            if event == MGMT_EV_CMD_COMPLETE:
                if index != cmd_index:
                    raise BluetoothInvalidPacketError(
                            ('Response for wrong controller index received: ' +
                             '0x%04d (expected 0x%04d)' % (index, cmd_index)))
                if len(data) < 3:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect command complete event data length: ' +
                             '%d (expected at least 3)' % len(data)))

                (code, status) = struct.unpack_from('<HB', buffer(data, 0, 3))
                logging.debug('[0x%04x] command 0x%04x complete: 0x%02x',
                              index, code, status)

                if code != cmd_code:
                    raise BluetoothInvalidPacketError(
                            ('Response for wrong command code received: ' +
                             '0x%04d (expected 0x%04d)' % (code, cmd_code)))

                response_length = len(data) - 3
                if (expected_length is not None and
                    response_length != expected_length):
                    raise BluetoothInvalidPacketError(
                            ('Incorrect length of data for response: ' +
                             '%d (expected %d)' % (response_length,
                                                   expected_length)))

                return (status, data[3:])

            elif event == MGMT_EV_CMD_STATUS:
                if index != cmd_index:
                    raise BluetoothInvalidPacketError(
                            ('Response for wrong controller index received: ' +
                             '0x%04d (expected 0x%04d)' % (index, cmd_index)))
                if len(data) != 3:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect command status event data length: ' +
                             '%d (expected 3)' % len(data)))

                (code, status) = struct.unpack_from('<HB', buffer(data, 0, 3))
                logging.debug('[0x%04x] command 0x%02x status: 0x%02x',
                              index, code, status)

                if code != cmd_code:
                    raise BluetoothInvalidPacketError(
                            ('Response for wrong command code received: ' +
                             '0x%04d (expected 0x%04d)' % (code, cmd_code)))

                return (status, None)

            elif event == MGMT_EV_CONTROLLER_ERROR:
                if len(data) != 1:
                    raise BluetoothInvalidPacketError(
                        ('Incorrect controller error event data length: ' +
                         '%d (expected 1)' % len(data)))

                (error_code) = struct.unpack_from('<B', buffer(data, 0, 1))

                raise BluetoothControllerError('Controller error: %d' %
                                               error_code)

            else:
                logging.debug('[0x%04x] event 0x%02x length: %d',
                              index, event, len(data))
                self.events.append((event, index, data))


    def wait_for_events(self, index, events):
        """Wait for and return the first of a set of events specified.

        @param index: Controller index of event, may be btsocket.HCI_DEV_NONE.
        @param events: List of event codes to wait for.

        Use settimeout() to set whether this method will block if there is no
        event received, return immediately or wait for a specific length of
        time before timing out and raising TimeoutError.

        @return Tuple of (event, data)

        """
        while True:
            for idx, (event, event_index, data) in enumerate(self.events):
                if event_index == index and event in events:
                    self.events.pop(idx)
                    return (event, data)

            (event, event_index, data) = self.recv_event()
            if event_index == index and event in events:
                return (event, data)
            elif event == MGMT_EV_CMD_COMPLETE:
                if len(data) < 3:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect command complete event data length: ' +
                             '%d (expected at least 3)' % len(data)))

                (code, status) = struct.unpack_from('<HB', buffer(data, 0, 3))
                logging.debug('[0x%04x] command 0x%04x complete: 0x%02x '
                              '(Ignored)', index, code, status)

            elif event == MGMT_EV_CMD_STATUS:
                if len(data) != 3:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect command status event data length: ' +
                             '%d (expected 3)' % len(data)))

                (code, status) = struct.unpack_from('<HB', buffer(data, 0, 3))
                logging.debug('[0x%04x] command 0x%02x status: 0x%02x '
                              '(Ignored)', index, code, status)

            elif event == MGMT_EV_CONTROLLER_ERROR:
                if len(data) != 1:
                    raise BluetoothInvalidPacketError(
                        ('Incorrect controller error event data length: ' +
                         '%d (expected 1)' % len(data)))

                (error_code) = struct.unpack_from('<B', buffer(data, 0, 1))
                logging.debug('[0x%04x] controller error: %d (Ignored)',
                              index, error_code)

            else:
                self.events.append((event, index, data))


class BluetoothControlSocket(BluetoothSocket):
    """Bluetooth Control Socket.

    BluetoothControlSocket provides convenient methods mapping to each mgmt_ops
    command that send an appropriately formatted command and parse the response.

    """

    DEFAULT_TIMEOUT = 15

    def __init__(self):
        super(BluetoothControlSocket, self).__init__()
        self.bind((btsocket.HCI_DEV_NONE, btsocket.HCI_CHANNEL_CONTROL))
        self.settimeout(self.DEFAULT_TIMEOUT)

        # Certain features will depend on the management version and revision,
        # so check those now.
        (version, revision) = self.read_version()
        logging.debug('MGMT API %d.%d', version, revision)
        self._kernel_confirms_name = (
                (version > 1) or ((version == 1) and (revision >= 5)))

    def read_version(self):
        """Read the version of the management interface.

        @return tuple (version, revision) on success, None on failure.

        """
        (status, data) = self.send_command_and_wait(
                MGMT_OP_READ_VERSION,
                MGMT_INDEX_NONE,
                expected_length=3)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (version, revision) = struct.unpack_from('<BH', buffer(data))
        return (version, revision)


    def read_supported_commands(self):
        """Read the supported management commands and events.

        @return tuple (commands, events) on success, None on failure.

        """
        (status, data) = self.send_command_and_wait(
                MGMT_OP_READ_COMMANDS,
                MGMT_INDEX_NONE)
        if status != MGMT_STATUS_SUCCESS:
            return None
        if len(data) < 4:
            raise BluetoothInvalidPacketError(
                    ('Incorrect length of data for response: ' +
                     '%d (expected at least 4)' % len(data)))

        (ncommands, nevents) = struct.unpack_from('<HH', buffer(data, 0, 4))
        offset = 4
        expected_length = offset + (ncommands * 2) + (nevents * 2)
        if len(data) != expected_length:
            raise BluetoothInvalidPacketError(
                    ('Incorrect length of data for response: ' +
                     '%d (expected %d)' % (len(data), expected_length)))

        commands = []
        while len(commands) < ncommands:
            commands.extend(struct.unpack_from('<H', buffer(data, offset, 2)))
            offset += 2

        events = []
        while len(events) < nevents:
            events.extend(struct.unpack_from('<H', buffer(data, offset, 2)))
            offset += 2

        return (commands, events)


    def read_index_list(self):
        """Read the list of currently known controllers.

        @return array of controller indexes on success, None on failure.

        """
        (status, data) = self.send_command_and_wait(
                MGMT_OP_READ_INDEX_LIST,
                MGMT_INDEX_NONE)
        if status != MGMT_STATUS_SUCCESS:
            return None
        if len(data) < 2:
            raise BluetoothInvalidPacketError(
                    ('Incorrect length of data for response: ' +
                     '%d (expected at least 2)' % len(data)))

        (nindexes,) = struct.unpack_from('<H', buffer(data, 0, 2))
        offset = 2
        expected_length = offset + (nindexes * 2)
        if len(data) != expected_length:
            raise BluetoothInvalidPacketError(
                    ('Incorrect length of data for response: ' +
                     '%d (expected %d)' % (len(data), expected_length)))

        indexes = []
        while len(indexes) < nindexes:
            indexes.extend(struct.unpack_from('<H', buffer(data, offset, 2)))
            offset += 2

        return indexes


    def read_info(self, index):
        """Read the state and basic information of a controller.

        Address is returned as a string in upper-case hex to match the
        BlueZ property.

        @param index: Controller index.

        @return tuple (address, bluetooth_version, manufacturer,
                       supported_settings, current_settings,
                       class_of_device, name, short_name) on success,
                None on failure.

        """
        (status, data) = self.send_command_and_wait(
                MGMT_OP_READ_INFO,
                index,
                expected_length=280)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (address, bluetooth_version, manufacturer,
         supported_settings, current_settings,
         class_of_device_lo, class_of_device_mid, class_of_device_hi,
         name, short_name) = struct.unpack_from(
                '<6sBHLL3B249s11s',
                buffer(data))

        return (
                ':'.join('%02X' % x
                         for x in reversed(struct.unpack('6B', address))),
                bluetooth_version,
                manufacturer,
                supported_settings,
                current_settings,
                (class_of_device_lo |(class_of_device_mid << 8) |
                        (class_of_device_hi << 16)),
                name.rstrip('\0'),
                short_name.rstrip('\0'))


    def set_powered(self, index, powered):
        """Set the powered state of a controller.

        @param index: Controller index.
        @param powered: Whether controller radio should be powered.

        @return New controller settings on success, None on failure.

        """
        msg_data = struct.pack('<B', bool(powered))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_POWERED,
                index,
                msg_data,
                expected_length=4)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_discoverable(self, index, discoverable, timeout=0):
        """Set the discoverable state of a controller.

        @param index: Controller index.
        @param discoverable: Whether controller should be discoverable.
        @param timeout: Timeout in seconds before disabling discovery again,
                ignored when discoverable is False, must not be zero when
                discoverable is True.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<BH', bool(discoverable), timeout)
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_DISCOVERABLE,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not discoverable:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_connectable(self, index, connectable):
        """Set the connectable state of a controller.

        @param index: Controller index.
        @param connectable: Whether controller should be connectable.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(connectable))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_CONNECTABLE,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not connectable:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_fast_connectable(self, index, connectable):
        """Set the fast connectable state of a controller.

        Fast Connectable is a state where page scan parameters are set to favor
        faster connect times at the expense of higher power consumption.

        Unlike most other set_* commands, this may only be used when the
        controller is powered.

        @param index: Controller index.
        @param connectable: Whether controller should be fast connectable.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False or the controller is
                powered down, None otherwise.

        """
        msg_data = struct.pack('<B', bool(connectable))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_FAST_CONNECTABLE,
                index,
                msg_data)
        if status == MGMT_STATUS_NOT_SUPPORTED and not connectable:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None
        # This is documented as returning current settings, but doesn't in
        # our kernel version (probably a bug), so if no data is returned,
        # pretend that was success.
        if len(data) == 0:
            return 0
        elif len(data) != 4:
            raise BluetoothInvalidPacketError(
                    ('Incorrect length of data for response: ' +
                     '%d (expected 4)' % len(data)))

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_pairable(self, index, pairable):
        """Set the pairable state of a controller.

        @param index: Controller index.
        @param pairable: Whether controller should be pairable.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(pairable))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_PAIRABLE,
                index,
                msg_data,
                expected_length=4)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_link_security(self, index, link_security):
        """Set the link security state of a controller.

        Toggles the use of link level security (aka Security Mode 3) for a
        controller.

        @param index: Controller index.
        @param link_security: Whether controller should be link_security.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(link_security))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_LINK_SECURITY,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not link_security:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_ssp(self, index, ssp):
        """Set the whether a controller supports Simple Secure Pairing.

        @param index: Controller index.
        @param ssp: Whether controller should support SSP.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(ssp))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_SSP,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not ssp:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_hs(self, index, hs):
        """Set the whether a controller supports Bluetooth High Speed.

        @param index: Controller index.
        @param hs: Whether controller should support High Speed.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(hs))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_HS,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not hs:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_le(self, index, le):
        """Set the whether a controller supports Bluetooth Low Energy.

        @param index: Controller index.
        @param le: Whether controller should support Low Energy.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(le))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_LE,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not le:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_device_class(self, index, major, minor):
        """Set the device class of the controller.

        Consult the Bluetooth Baseband Assigned Numbers specification for valid
        values, in general both values are bit fields defined by that
        specification.

        If the device class is set while the controller is powered off, 0 will
        be returned, but the new class will be set by the host subsystem after
        the controller is powered on.

        @param index: Controller index.
        @param major: Major device class.
        @param minor: Minor device class.

        @return New three-octet device class on success, None on failure.

        """
        msg_data = struct.pack('<BB', major, minor)
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_DEV_CLASS,
                index,
                msg_data,
                expected_length=3)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (class_of_device_lo, class_of_device_mid,
         class_of_device_hi) = struct.unpack_from('<3B', buffer(data))
        return (class_of_device_lo |(class_of_device_mid << 8) |
                (class_of_device_hi << 16))


    def set_local_name(self, index, name, short_name):
        """Set the local name of the controller.

        @param index: Controller index.
        @param name: Full length name, up to 248 characters.
        @param short_name: Short name, up to 10 characters.

        @return Tuple of (name, short_name) on success, None on failure.

        """
        # Truncate the provided parameters and then zero-pad using struct
        # so we pass a fixed-length null-terminated string to the kernel.
        msg_data = struct.pack('<249s11s', name[:248], short_name[:10])
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_LOCAL_NAME,
                index,
                msg_data,
                expected_length=260)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (name, short_name) = struct.unpack_from('<249s11s', buffer(data))
        return (name.rstrip('\0'), short_name.rstrip('\0'))


    def start_discovery(self, index, address_type):
        """Start discovering remote devices.

        Call get_discovered_devices() to retrieve the list of devices found.

        @param index: Controller index.
        @param address_type: Address types to discover.

        @return Address types discovery was started for on success,
                None on failure.

        """
        msg_data = struct.pack('<B', address_type)
        (status, data) = self.send_command_and_wait(
                MGMT_OP_START_DISCOVERY,
                index,
                msg_data,
                expected_length=1)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (address_type,) = struct.unpack_from('<B', buffer(data))
        return address_type


    def stop_discovery(self, index, address_type):
        """Stop discovering remote devices.

        There is usually no need to call this method explicitly as discovery
        is automatically stopped once it has iterated through the necessary
        channels.

        @param index: Controller index.
        @param address_type: Address types to stop discovering.

        @return Address types discovery was stopped for on success,
                None on failure.

        """
        msg_data = struct.pack('<B', address_type)
        (status, data) = self.send_command_and_wait(
                MGMT_OP_STOP_DISCOVERY,
                index,
                msg_data,
                expected_length=1)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (address_type,) = struct.unpack_from('<B', buffer(data))
        return address_type


    def get_discovered_devices(self, index):
        """Return list of discovered remote devices.

        This method may be called any time after start_discovery() and will
        wait until the full list of devices has been returned, there is usually
        no need to call stop_discovery() explicitly.

        Use settimeout() to set whether this method will block if there are no
        events, return immediately or wait for a specific length of time before
        timing out and raising TimeoutError.

        @param index: Controller index.

        @return List of devices found as tuples with the format
                (address, address_type, rssi, flags, eirdata)

        """
        devices = []
        discovering = True
        while discovering:
            (event, data) = self.wait_for_events(
                    index,
                    ( MGMT_EV_DISCOVERING, MGMT_EV_DEVICE_FOUND ))

            if event == MGMT_EV_DISCOVERING:
                if len(data) != 2:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect discovering event data length: ' +
                             '%d (expected 2)' % len(data)))

                (address_type,
                 discovering) = struct.unpack_from('<BB', buffer(data))

            elif event == MGMT_EV_DEVICE_FOUND:
                if len(data) < 14:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect device found event data length: ' +
                             '%d (expected at least 14)' % len(data)))

                (address, address_type, rssi,
                 flags, eir_len) = struct.unpack_from('<6sBbLH',
                                                      buffer(data, 0, 14))

                if len(data) != 14 + eir_len:
                    raise BluetoothInvalidPacketError(
                            ('Incorrect device found event data length: ' +
                             '%d (expected %d)' % (len(data), 14 + eir_len)))

                devices.append((
                        ':'.join('%02X' % x
                                 for x in reversed(
                                        struct.unpack('6B', address))),
                        address_type,
                        rssi,
                        flags,
                        bytes(data[14:])
                ))

                # The kernel might want us to confirm whether or not we
                # know the name of the device. We don't really care whether
                # or not this works, we just have to do it to get the EIR
                # Request.
                if flags & MGMT_DEV_FOUND_CONFIRM_NAME:
                    msg_data = struct.pack('<6sBB',
                                           address, address_type, False)
                    if self._kernel_confirms_name:
                        self.send_command_and_wait(
                                MGMT_OP_CONFIRM_NAME,
                                index,
                                msg_data)
                    else:
                        self.send_command(
                                MGMT_OP_CONFIRM_NAME,
                                index,
                                msg_data)


        return devices


    def set_advertising(self, index, advertising):
        """Set the whether a controller is advertising via LE.

        @param index: Controller index.
        @param advertising: Whether controller should advertise via LE.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(advertising))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_ADVERTISING,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not advertising:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def set_bredr(self, index, bredr):
        """Set the whether a controller supports Bluetooth BR/EDR (classic).

        @param index: Controller index.
        @param bredr: Whether controller should support BR/EDR.

        @return New controller settings on success, 0 if the feature is not
                supported and the parameter was False, None otherwise.

        """
        msg_data = struct.pack('<B', bool(bredr))
        (status, data) = self.send_command_and_wait(
                MGMT_OP_SET_BREDR,
                index,
                msg_data,
                expected_length=4)
        if status == MGMT_STATUS_NOT_SUPPORTED and not bredr:
            return 0
        elif status != MGMT_STATUS_SUCCESS:
            return None

        (current_settings, ) = struct.unpack_from('<L', buffer(data))
        return current_settings


    def add_device(self, index, address, address_type, action):
        """Add a device to the action list.

        @param index: Controller index.
        @param address: Address of the device to add.
        @param address_type: Type of device in @address.
        @param action: Action to take.

        @return Tuple of ( address, address_type ) on success,
                None on failure.

        """
        msg_data = struct.pack('<6sBB', address, address_type, action)
        (status, data) = self.send_command_and_wait(
                MGMT_OP_ADD_DEVICE,
                index,
                msg_data,
                expected_length=7)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (address, address_type,) = struct.unpack_from('<6sB', buffer(data))
        return (address, address_type)


    def remove_device(self, index, address, address_type):
        """Remove a device from the action list.

        @param index: Controller index.
        @param address: Address of the device to remove.
        @param address_type: Type of device in @address.

        @return Tuple of ( address, address_type ) on success,
                None on failure.

        """
        msg_data = struct.pack('<6sB', address, address_type)
        (status, data) = self.send_command_and_wait(
                MGMT_OP_REMOVE_DEVICE,
                index,
                msg_data,
                expected_length=7)
        if status != MGMT_STATUS_SUCCESS:
            return None

        (address, address_type,) = struct.unpack_from('<6sB', buffer(data))
        return (address, address_type)


class BluetoothRawSocket(BluetoothSocket):
    """Bluetooth Raw HCI Socket.

    BluetoothRawSocket is a subclass of BluetoothSocket representing raw access
    to the HCI controller and providing commands corresponding to ioctls that
    can be made on that kind of socket.

    """

    def get_dev_info(self, index):
        """Read HCI device information.

        This method uses the same underlying ioctl as the hciconfig tool.

        Address is returned as a string in upper-case hex to match the
        BlueZ property.

        @param index: Device index.

        @return tuple (index, name, address, flags, device_type, bus_type,
                       features, pkt_type, link_policy, link_mode,
                       acl_mtu, acl_pkts, sco_mtu, sco_pkts,
                       err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx,
                       sco_tx, sco_rx, byte_rx, byte_tx) on success,
                None on failure.

        """
        buf = array.array('B', [0] * 96)
        fcntl.ioctl(self.fileno(), HCIGETDEVINFO, buf, 1)

        ( dev_id, name, address, flags, dev_type, features, pkt_type,
          link_policy, link_mode, acl_mtu, acl_pkts, sco_mtu, sco_pkts,
          err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx, sco_tx, sco_rx,
          byte_rx, byte_tx ) = struct.unpack_from(
                '@H8s6sIBQIIIHHHHIIIIIIIIII', buf)

        return (
                dev_id,
                name.rstrip('\0'),
                ':'.join('%02X' % x
                         for x in reversed(struct.unpack('6B', address))),
                flags,
                (dev_type & 0x30) >> 4,
                dev_type & 0x0f,
                features,
                pkt_type,
                link_policy,
                link_mode,
                acl_mtu,
                acl_pkts,
                sco_mtu,
                sco_pkts,
                err_rx,
                err_tx,
                cmd_tx,
                evt_rx,
                acl_tx,
                acl_rx,
                sco_tx,
                sco_rx,
                byte_rx,
                byte_tx)

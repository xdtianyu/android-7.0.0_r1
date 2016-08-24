# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Tools for serializing and deserializing DHCP packets.

DhcpPacket is a class that represents a single DHCP packet and contains some
logic to create and parse binary strings containing on the wire DHCP packets.

While you could call the constructor explicitly, most users should use the
static factories to construct packets with reasonable default values in most of
the fields, even if those values are zeros.

For example:

packet = dhcp_packet.create_offer_packet(transaction_id,
                                         hwmac_addr,
                                         offer_ip,
                                         server_ip)
socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Sending to the broadcast address needs special permissions.
socket.sendto(response_packet.to_binary_string(),
              ("255.255.255.255", 68))

Note that if you make changes, make sure that the tests in the bottom of this
file still pass.
"""

import collections
import logging
import random
import socket
import struct


def CreatePacketPieceClass(super_class, field_format):
    class PacketPiece(super_class):
        @staticmethod
        def pack(value):
            return struct.pack(field_format, value)

        @staticmethod
        def unpack(byte_string):
            return struct.unpack(field_format, byte_string)[0]
    return PacketPiece

"""
Represents an option in a DHCP packet.  Options may or may not be present in any
given packet, depending on the configurations of the client and the server.
Using namedtuples as super classes gets us the comparison operators we want to
use these Options in dictionaries as keys.  Below, we'll subclass Option to
reflect that different kinds of options serialize to on the wire formats in
different ways.

|name|
A human readable name for this option.

|number|
Every DHCP option has a number that goes into the packet to indicate
which particular option is being encoded in the next few bytes.  This
property returns that number for each option.
"""
Option = collections.namedtuple("Option", ["name", "number"])

ByteOption = CreatePacketPieceClass(Option, "!B")

ShortOption = CreatePacketPieceClass(Option, "!H")

IntOption = CreatePacketPieceClass(Option, "!I")

class IpAddressOption(Option):
    @staticmethod
    def pack(value):
        return socket.inet_aton(value)

    @staticmethod
    def unpack(byte_string):
        return socket.inet_ntoa(byte_string)


class IpListOption(Option):
    @staticmethod
    def pack(value):
        return "".join([socket.inet_aton(addr) for addr in value])

    @staticmethod
    def unpack(byte_string):
        return [socket.inet_ntoa(byte_string[idx:idx+4])
                for idx in range(0, len(byte_string), 4)]


class RawOption(Option):
    @staticmethod
    def pack(value):
        return value

    @staticmethod
    def unpack(byte_string):
        return byte_string


class ByteListOption(Option):
    @staticmethod
    def pack(value):
        return "".join(chr(v) for v in value)

    @staticmethod
    def unpack(byte_string):
        return [ord(c) for c in byte_string]


class ClasslessStaticRoutesOption(Option):
    """
    This is a RFC 3442 compliant classless static route option parser and
    serializer.  The symbolic "value" packed and unpacked from this class
    is a list (prefix_size, destination, router) tuples.
    """

    @staticmethod
    def pack(value):
        route_list = value
        byte_string = ""
        for prefix_size, destination, router in route_list:
            byte_string += chr(prefix_size)
            # Encode only the significant octets of the destination
            # that fall within the prefix.
            destination_address_count = (prefix_size + 7) / 8
            destination_address = socket.inet_aton(destination)
            byte_string += destination_address[:destination_address_count]
            byte_string += socket.inet_aton(router)

        return byte_string

    @staticmethod
    def unpack(byte_string):
        route_list = []
        offset = 0
        while offset < len(byte_string):
            prefix_size = ord(byte_string[offset])
            destination_address_count = (prefix_size + 7) / 8
            entry_end = offset + 1 + destination_address_count + 4
            if entry_end > len(byte_string):
                raise Exception("Classless domain list is corrupted.")
            offset += 1
            destination_address_end = offset + destination_address_count
            destination_address = byte_string[offset:destination_address_end]
            # Pad the destination address bytes with zero byte octets to
            # fill out an IPv4 address.
            destination_address += '\x00' * (4 - destination_address_count)
            router_address = byte_string[destination_address_end:entry_end]
            route_list.append((prefix_size,
                               socket.inet_ntoa(destination_address),
                               socket.inet_ntoa(router_address)))
            offset = entry_end

        return route_list


class DomainListOption(Option):
    """
    This is a RFC 1035 compliant domain list option parser and serializer.
    There are some clever compression optimizations that it does not implement
    for serialization, but correctly parses.  This should be sufficient for
    testing.
    """
    # Various RFC's let you finish a domain name by pointing to an existing
    # domain name rather than repeating the same suffix.  All such pointers are
    # two bytes long, specify the offset in the byte string, and begin with
    # |POINTER_PREFIX| to distinguish them from normal characters.
    POINTER_PREFIX = ord("\xC0")

    @staticmethod
    def pack(value):
        domain_list = value
        byte_string = ""
        for domain in domain_list:
            for part in domain.split("."):
                byte_string += chr(len(part))
                byte_string += part
            byte_string += "\x00"
        return byte_string

    @staticmethod
    def unpack(byte_string):
        domain_list = []
        offset = 0
        try:
            while offset < len(byte_string):
                (new_offset, domain_parts) = DomainListOption._read_domain_name(
                        byte_string,
                        offset)
                domain_name = ".".join(domain_parts)
                domain_list.append(domain_name)
                if new_offset <= offset:
                    raise Exception("Parsing logic error is letting domain "
                                    "list parsing go on forever.")
                offset = new_offset
        except ValueError:
            # Badly formatted packets are not necessarily test errors.
            logging.warning("Found badly formatted DHCP domain search list")
            return None
        return domain_list

    @staticmethod
    def _read_domain_name(byte_string, offset):
        """
        Recursively parse a domain name from a domain name list.
        """
        parts = []
        while True:
            if offset >= len(byte_string):
                raise ValueError("Domain list ended without a NULL byte.")
            maybe_part_len = ord(byte_string[offset])
            offset += 1
            if maybe_part_len == 0:
                # Domains are terminated with either a 0 or a pointer to a
                # domain suffix within |byte_string|.
                return (offset, parts)
            elif ((maybe_part_len & DomainListOption.POINTER_PREFIX) ==
                  DomainListOption.POINTER_PREFIX):
                if offset >= len(byte_string):
                    raise ValueError("Missing second byte of domain suffix "
                                     "pointer.")
                maybe_part_len &= ~DomainListOption.POINTER_PREFIX
                pointer_offset = ((maybe_part_len << 8) +
                                  ord(byte_string[offset]))
                offset += 1
                (_, more_parts) = DomainListOption._read_domain_name(
                        byte_string,
                        pointer_offset)
                parts.extend(more_parts)
                return (offset, parts)
            else:
                # That byte was actually the length of the next part, not a
                # pointer back into the data.
                part_len = maybe_part_len
                if offset + part_len >= len(byte_string):
                    raise ValueError("Part of a domain goes beyond data "
                                     "length.")
                parts.append(byte_string[offset : offset + part_len])
                offset += part_len


"""
Represents a required field in a DHCP packet.  Similar to Option, we'll
subclass Field to reflect that different fields serialize to on the wire formats
in different ways.

|name|
A human readable name for this field.

|offset|
The |offset| for a field defines the starting byte of the field in the
binary packet string.  |offset| is used during parsing, along with
|size| to extract the byte string of a field.

|size|
Fields in DHCP packets have a fixed size that must be respected.  This
size property is used in parsing to indicate that |self._size| number of
bytes make up this field.
"""
Field = collections.namedtuple("Field", ["name", "offset", "size"])

ByteField = CreatePacketPieceClass(Field, "!B")

ShortField = CreatePacketPieceClass(Field, "!H")

IntField = CreatePacketPieceClass(Field, "!I")

HwAddrField = CreatePacketPieceClass(Field, "!16s")

ServerNameField = CreatePacketPieceClass(Field, "!64s")

BootFileField = CreatePacketPieceClass(Field, "!128s")

class IpAddressField(Field):
    @staticmethod
    def pack(value):
        return socket.inet_aton(value)

    @staticmethod
    def unpack(byte_string):
        return socket.inet_ntoa(byte_string)


# This is per RFC 2131.  The wording doesn't seem to say that the packets must
# be this big, but that has been the historic assumption in implementations.
DHCP_MIN_PACKET_SIZE = 300

IPV4_NULL_ADDRESS = "0.0.0.0"

# These are required in every DHCP packet.  Without these fields, the
# packet will not even pass DhcpPacket.is_valid
FIELD_OP = ByteField("op", 0, 1)
FIELD_HWTYPE = ByteField("htype", 1, 1)
FIELD_HWADDR_LEN = ByteField("hlen", 2, 1)
FIELD_RELAY_HOPS = ByteField("hops", 3, 1)
FIELD_TRANSACTION_ID = IntField("xid", 4, 4)
FIELD_TIME_SINCE_START = ShortField("secs", 8, 2)
FIELD_FLAGS = ShortField("flags", 10, 2)
FIELD_CLIENT_IP = IpAddressField("ciaddr", 12, 4)
FIELD_YOUR_IP = IpAddressField("yiaddr", 16, 4)
FIELD_SERVER_IP = IpAddressField("siaddr", 20, 4)
FIELD_GATEWAY_IP = IpAddressField("giaddr", 24, 4)
FIELD_CLIENT_HWADDR = HwAddrField("chaddr", 28, 16)
# The following two fields are considered "legacy BOOTP" fields but may
# sometimes be used by DHCP clients.
FIELD_LEGACY_SERVER_NAME = ServerNameField("servername", 44, 64);
FIELD_LEGACY_BOOT_FILE = BootFileField("bootfile", 108, 128);
FIELD_MAGIC_COOKIE = IntField("magic_cookie", 236, 4)

OPTION_TIME_OFFSET = IntOption("time_offset", 2)
OPTION_ROUTERS = IpListOption("routers", 3)
OPTION_SUBNET_MASK = IpAddressOption("subnet_mask", 1)
OPTION_TIME_SERVERS = IpListOption("time_servers", 4)
OPTION_NAME_SERVERS = IpListOption("name_servers", 5)
OPTION_DNS_SERVERS = IpListOption("dns_servers", 6)
OPTION_LOG_SERVERS = IpListOption("log_servers", 7)
OPTION_COOKIE_SERVERS = IpListOption("cookie_servers", 8)
OPTION_LPR_SERVERS = IpListOption("lpr_servers", 9)
OPTION_IMPRESS_SERVERS = IpListOption("impress_servers", 10)
OPTION_RESOURCE_LOC_SERVERS = IpListOption("resource_loc_servers", 11)
OPTION_HOST_NAME = RawOption("host_name", 12)
OPTION_BOOT_FILE_SIZE = ShortOption("boot_file_size", 13)
OPTION_MERIT_DUMP_FILE = RawOption("merit_dump_file", 14)
OPTION_DOMAIN_NAME = RawOption("domain_name", 15)
OPTION_SWAP_SERVER = IpAddressOption("swap_server", 16)
OPTION_ROOT_PATH = RawOption("root_path", 17)
OPTION_EXTENSIONS = RawOption("extensions", 18)
OPTION_INTERFACE_MTU = ShortOption("interface_mtu", 26)
OPTION_VENDOR_ENCAPSULATED_OPTIONS = RawOption(
        "vendor_encapsulated_options", 43)
OPTION_REQUESTED_IP = IpAddressOption("requested_ip", 50)
OPTION_IP_LEASE_TIME = IntOption("ip_lease_time", 51)
OPTION_OPTION_OVERLOAD = ByteOption("option_overload", 52)
OPTION_DHCP_MESSAGE_TYPE = ByteOption("dhcp_message_type", 53)
OPTION_SERVER_ID = IpAddressOption("server_id", 54)
OPTION_PARAMETER_REQUEST_LIST = ByteListOption("parameter_request_list", 55)
OPTION_MESSAGE = RawOption("message", 56)
OPTION_MAX_DHCP_MESSAGE_SIZE = ShortOption("max_dhcp_message_size", 57)
OPTION_RENEWAL_T1_TIME_VALUE = IntOption("renewal_t1_time_value", 58)
OPTION_REBINDING_T2_TIME_VALUE = IntOption("rebinding_t2_time_value", 59)
OPTION_VENDOR_ID = RawOption("vendor_id", 60)
OPTION_CLIENT_ID = RawOption("client_id", 61)
OPTION_TFTP_SERVER_NAME = RawOption("tftp_server_name", 66)
OPTION_BOOTFILE_NAME = RawOption("bootfile_name", 67)
OPTION_FULLY_QUALIFIED_DOMAIN_NAME = RawOption("fqdn", 81)
OPTION_DNS_DOMAIN_SEARCH_LIST = DomainListOption("domain_search_list", 119)
OPTION_CLASSLESS_STATIC_ROUTES = ClasslessStaticRoutesOption(
        "classless_static_routes", 121)
OPTION_WEB_PROXY_AUTO_DISCOVERY = RawOption("wpad", 252)

# Unlike every other option, which are tuples like:
# <number, length in bytes, data>, the pad and end options are just
# single bytes "\x00" and "\xff" (without length or data fields).
OPTION_PAD = 0
OPTION_END = 255

DHCP_COMMON_FIELDS = [
        FIELD_OP,
        FIELD_HWTYPE,
        FIELD_HWADDR_LEN,
        FIELD_RELAY_HOPS,
        FIELD_TRANSACTION_ID,
        FIELD_TIME_SINCE_START,
        FIELD_FLAGS,
        FIELD_CLIENT_IP,
        FIELD_YOUR_IP,
        FIELD_SERVER_IP,
        FIELD_GATEWAY_IP,
        FIELD_CLIENT_HWADDR,
        ]

DHCP_REQUIRED_FIELDS = DHCP_COMMON_FIELDS + [
        FIELD_MAGIC_COOKIE,
        ]

DHCP_ALL_FIELDS = DHCP_COMMON_FIELDS + [
        FIELD_LEGACY_SERVER_NAME,
        FIELD_LEGACY_BOOT_FILE,
        FIELD_MAGIC_COOKIE,
        ]

# The op field in an ipv4 packet is either 1 or 2 depending on
# whether the packet is from a server or from a client.
FIELD_VALUE_OP_CLIENT_REQUEST = 1
FIELD_VALUE_OP_SERVER_RESPONSE = 2
# 1 == 10mb ethernet hardware address type (aka MAC).
FIELD_VALUE_HWTYPE_10MB_ETH = 1
# MAC addresses are still 6 bytes long.
FIELD_VALUE_HWADDR_LEN_10MB_ETH = 6
FIELD_VALUE_MAGIC_COOKIE = 0x63825363

OPTIONS_START_OFFSET = 240

MessageType = collections.namedtuple('MessageType', 'name option_value')
# From RFC2132, the valid DHCP message types are:
MESSAGE_TYPE_UNKNOWN = MessageType('UNKNOWN', 0)
MESSAGE_TYPE_DISCOVERY = MessageType('DISCOVERY', 1)
MESSAGE_TYPE_OFFER = MessageType('OFFER', 2)
MESSAGE_TYPE_REQUEST = MessageType('REQUEST', 3)
MESSAGE_TYPE_DECLINE = MessageType('DECLINE', 4)
MESSAGE_TYPE_ACK = MessageType('ACK', 5)
MESSAGE_TYPE_NAK = MessageType('NAK', 6)
MESSAGE_TYPE_RELEASE = MessageType('RELEASE', 7)
MESSAGE_TYPE_INFORM = MessageType('INFORM', 8)
MESSAGE_TYPE_BY_NUM = [
    None,
    MESSAGE_TYPE_DISCOVERY,
    MESSAGE_TYPE_OFFER,
    MESSAGE_TYPE_REQUEST,
    MESSAGE_TYPE_DECLINE,
    MESSAGE_TYPE_ACK,
    MESSAGE_TYPE_NAK,
    MESSAGE_TYPE_RELEASE,
    MESSAGE_TYPE_INFORM
]

OPTION_VALUE_PARAMETER_REQUEST_LIST_DEFAULT = [
        OPTION_REQUESTED_IP.number,
        OPTION_IP_LEASE_TIME.number,
        OPTION_SERVER_ID.number,
        OPTION_SUBNET_MASK.number,
        OPTION_ROUTERS.number,
        OPTION_DNS_SERVERS.number,
        OPTION_HOST_NAME.number,
        ]

# These are possible options that may not be in every packet.
# Frequently, the client can include a bunch of options that indicate
# that it would like to receive information about time servers, routers,
# lpr servers, and much more, but the DHCP server can usually ignore
# those requests.
#
# Eventually, each option is encoded as:
#     <option.number, option.size, [array of option.size bytes]>
# Unlike fields, which make up a fixed packet format, options can be in
# any order, except where they cannot.  For instance, option 1 must
# follow option 3 if both are supplied.  For this reason, potential
# options are in this list, and added to the packet in this order every
# time.
#
# size < 0 indicates that this is variable length field of at least
# abs(length) bytes in size.
DHCP_PACKET_OPTIONS = [
        OPTION_TIME_OFFSET,
        OPTION_ROUTERS,
        OPTION_SUBNET_MASK,
        OPTION_TIME_SERVERS,
        OPTION_NAME_SERVERS,
        OPTION_DNS_SERVERS,
        OPTION_LOG_SERVERS,
        OPTION_COOKIE_SERVERS,
        OPTION_LPR_SERVERS,
        OPTION_IMPRESS_SERVERS,
        OPTION_RESOURCE_LOC_SERVERS,
        OPTION_HOST_NAME,
        OPTION_BOOT_FILE_SIZE,
        OPTION_MERIT_DUMP_FILE,
        OPTION_SWAP_SERVER,
        OPTION_DOMAIN_NAME,
        OPTION_ROOT_PATH,
        OPTION_EXTENSIONS,
        OPTION_INTERFACE_MTU,
        OPTION_VENDOR_ENCAPSULATED_OPTIONS,
        OPTION_REQUESTED_IP,
        OPTION_IP_LEASE_TIME,
        OPTION_OPTION_OVERLOAD,
        OPTION_DHCP_MESSAGE_TYPE,
        OPTION_SERVER_ID,
        OPTION_PARAMETER_REQUEST_LIST,
        OPTION_MESSAGE,
        OPTION_MAX_DHCP_MESSAGE_SIZE,
        OPTION_RENEWAL_T1_TIME_VALUE,
        OPTION_REBINDING_T2_TIME_VALUE,
        OPTION_VENDOR_ID,
        OPTION_CLIENT_ID,
        OPTION_TFTP_SERVER_NAME,
        OPTION_BOOTFILE_NAME,
        OPTION_FULLY_QUALIFIED_DOMAIN_NAME,
        OPTION_DNS_DOMAIN_SEARCH_LIST,
        OPTION_CLASSLESS_STATIC_ROUTES,
        OPTION_WEB_PROXY_AUTO_DISCOVERY,
        ]

def get_dhcp_option_by_number(number):
    for option in DHCP_PACKET_OPTIONS:
        if option.number == number:
            return option
    return None

class DhcpPacket(object):
    @staticmethod
    def create_discovery_packet(hwmac_addr):
        """
        Create a discovery packet.

        Fill in fields of a DHCP packet as if it were being sent from
        |hwmac_addr|.  Requests subnet masks, broadcast addresses, router
        addresses, dns addresses, domain search lists, client host name, and NTP
        server addresses.  Note that the offer packet received in response to
        this packet will probably not contain all of that information.
        """
        # MAC addresses are actually only 6 bytes long, however, for whatever
        # reason, DHCP allocated 12 bytes to this field.  Ease the burden on
        # developers and hide this detail.
        while len(hwmac_addr) < 12:
            hwmac_addr += chr(OPTION_PAD)

        packet = DhcpPacket()
        packet.set_field(FIELD_OP, FIELD_VALUE_OP_CLIENT_REQUEST)
        packet.set_field(FIELD_HWTYPE, FIELD_VALUE_HWTYPE_10MB_ETH)
        packet.set_field(FIELD_HWADDR_LEN, FIELD_VALUE_HWADDR_LEN_10MB_ETH)
        packet.set_field(FIELD_RELAY_HOPS, 0)
        packet.set_field(FIELD_TRANSACTION_ID, random.getrandbits(32))
        packet.set_field(FIELD_TIME_SINCE_START, 0)
        packet.set_field(FIELD_FLAGS, 0)
        packet.set_field(FIELD_CLIENT_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_YOUR_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_SERVER_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_GATEWAY_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_CLIENT_HWADDR, hwmac_addr)
        packet.set_field(FIELD_MAGIC_COOKIE, FIELD_VALUE_MAGIC_COOKIE)
        packet.set_option(OPTION_DHCP_MESSAGE_TYPE,
                          MESSAGE_TYPE_DISCOVERY.option_value)
        return packet

    @staticmethod
    def create_offer_packet(transaction_id,
                            hwmac_addr,
                            offer_ip,
                            server_ip):
        """
        Create an offer packet, given some fields that tie the packet to a
        particular offer.
        """
        packet = DhcpPacket()
        packet.set_field(FIELD_OP, FIELD_VALUE_OP_SERVER_RESPONSE)
        packet.set_field(FIELD_HWTYPE, FIELD_VALUE_HWTYPE_10MB_ETH)
        packet.set_field(FIELD_HWADDR_LEN, FIELD_VALUE_HWADDR_LEN_10MB_ETH)
        # This has something to do with relay agents
        packet.set_field(FIELD_RELAY_HOPS, 0)
        packet.set_field(FIELD_TRANSACTION_ID, transaction_id)
        packet.set_field(FIELD_TIME_SINCE_START, 0)
        packet.set_field(FIELD_FLAGS, 0)
        packet.set_field(FIELD_CLIENT_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_YOUR_IP, offer_ip)
        packet.set_field(FIELD_SERVER_IP, server_ip)
        packet.set_field(FIELD_GATEWAY_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_CLIENT_HWADDR, hwmac_addr)
        packet.set_field(FIELD_MAGIC_COOKIE, FIELD_VALUE_MAGIC_COOKIE)
        packet.set_option(OPTION_DHCP_MESSAGE_TYPE,
                          MESSAGE_TYPE_OFFER.option_value)
        return packet

    @staticmethod
    def create_request_packet(transaction_id,
                              hwmac_addr):
        packet = DhcpPacket()
        packet.set_field(FIELD_OP, FIELD_VALUE_OP_CLIENT_REQUEST)
        packet.set_field(FIELD_HWTYPE, FIELD_VALUE_HWTYPE_10MB_ETH)
        packet.set_field(FIELD_HWADDR_LEN, FIELD_VALUE_HWADDR_LEN_10MB_ETH)
        # This has something to do with relay agents
        packet.set_field(FIELD_RELAY_HOPS, 0)
        packet.set_field(FIELD_TRANSACTION_ID, transaction_id)
        packet.set_field(FIELD_TIME_SINCE_START, 0)
        packet.set_field(FIELD_FLAGS, 0)
        packet.set_field(FIELD_CLIENT_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_YOUR_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_SERVER_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_GATEWAY_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_CLIENT_HWADDR, hwmac_addr)
        packet.set_field(FIELD_MAGIC_COOKIE, FIELD_VALUE_MAGIC_COOKIE)
        packet.set_option(OPTION_DHCP_MESSAGE_TYPE,
                          MESSAGE_TYPE_REQUEST.option_value)
        return packet

    @staticmethod
    def create_acknowledgement_packet(transaction_id,
                                      hwmac_addr,
                                      granted_ip,
                                      server_ip):
        packet = DhcpPacket()
        packet.set_field(FIELD_OP, FIELD_VALUE_OP_SERVER_RESPONSE)
        packet.set_field(FIELD_HWTYPE, FIELD_VALUE_HWTYPE_10MB_ETH)
        packet.set_field(FIELD_HWADDR_LEN, FIELD_VALUE_HWADDR_LEN_10MB_ETH)
        # This has something to do with relay agents
        packet.set_field(FIELD_RELAY_HOPS, 0)
        packet.set_field(FIELD_TRANSACTION_ID, transaction_id)
        packet.set_field(FIELD_TIME_SINCE_START, 0)
        packet.set_field(FIELD_FLAGS, 0)
        packet.set_field(FIELD_CLIENT_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_YOUR_IP, granted_ip)
        packet.set_field(FIELD_SERVER_IP, server_ip)
        packet.set_field(FIELD_GATEWAY_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_CLIENT_HWADDR, hwmac_addr)
        packet.set_field(FIELD_MAGIC_COOKIE, FIELD_VALUE_MAGIC_COOKIE)
        packet.set_option(OPTION_DHCP_MESSAGE_TYPE,
                          MESSAGE_TYPE_ACK.option_value)
        return packet

    @staticmethod
    def create_nak_packet(transaction_id, hwmac_addr):
        """
        Create a negative acknowledge packet.

        @param transaction_id: The DHCP transaction ID.
        @param hwmac_addr: The client's MAC address.
        """
        packet = DhcpPacket()
        packet.set_field(FIELD_OP, FIELD_VALUE_OP_SERVER_RESPONSE)
        packet.set_field(FIELD_HWTYPE, FIELD_VALUE_HWTYPE_10MB_ETH)
        packet.set_field(FIELD_HWADDR_LEN, FIELD_VALUE_HWADDR_LEN_10MB_ETH)
        # This has something to do with relay agents
        packet.set_field(FIELD_RELAY_HOPS, 0)
        packet.set_field(FIELD_TRANSACTION_ID, transaction_id)
        packet.set_field(FIELD_TIME_SINCE_START, 0)
        packet.set_field(FIELD_FLAGS, 0)
        packet.set_field(FIELD_CLIENT_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_YOUR_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_SERVER_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_GATEWAY_IP, IPV4_NULL_ADDRESS)
        packet.set_field(FIELD_CLIENT_HWADDR, hwmac_addr)
        packet.set_field(FIELD_MAGIC_COOKIE, FIELD_VALUE_MAGIC_COOKIE)
        packet.set_option(OPTION_DHCP_MESSAGE_TYPE,
                          MESSAGE_TYPE_NAK.option_value)
        return packet

    def __init__(self, byte_str=None):
        """
        Create a DhcpPacket, filling in fields from a byte string if given.

        Assumes that the packet starts at offset 0 in the binary string.  This
        includes the fields and options.  Fields are different from options in
        that we bother to decode these into more usable data types like
        integers rather than keeping them as raw byte strings.  Fields are also
        required to exist, unlike options which may not.

        Each option is encoded as a tuple <option number, length, data> where
        option number is a byte indicating the type of option, length indicates
        the number of bytes in the data for option, and data is a length array
        of bytes.  The only exceptions to this rule are the 0 and 255 options,
        which have 0 data length, and no length byte.  These tuples are then
        simply appended to each other.  This encoding is the same as the BOOTP
        vendor extention field encoding.
        """
        super(DhcpPacket, self).__init__()
        self._options = {}
        self._fields = {}
        if byte_str is None:
            return
        if len(byte_str) < OPTIONS_START_OFFSET + 1:
            logging.error("Invalid byte string for packet.")
            return
        for field in DHCP_ALL_FIELDS:
            self._fields[field] = field.unpack(byte_str[field.offset :
                                                        field.offset +
                                                        field.size])
        offset = OPTIONS_START_OFFSET
        domain_search_list_byte_string = ""
        while offset < len(byte_str) and ord(byte_str[offset]) != OPTION_END:
            data_type = ord(byte_str[offset])
            offset += 1
            if data_type == OPTION_PAD:
                continue
            data_length = ord(byte_str[offset])
            offset += 1
            data = byte_str[offset: offset + data_length]
            offset += data_length
            option = get_dhcp_option_by_number(data_type)
            if option is None:
                logging.warning("Unsupported DHCP option found.  "
                                "Option number: %d", data_type)
                continue
            if option == OPTION_DNS_DOMAIN_SEARCH_LIST:
                # In a cruel twist of fate, the server is allowed to give
                # multiple options with this number.  The client is expected to
                # concatenate the byte strings together and use it as a single
                # value.
                domain_search_list_byte_string += data
                continue
            option_value = option.unpack(data)
            if option == OPTION_PARAMETER_REQUEST_LIST:
                logging.info("Requested options: %s", str(option_value))
            self._options[option] = option_value
        if domain_search_list_byte_string:
            self._options[OPTION_DNS_DOMAIN_SEARCH_LIST] = option_value


    @property
    def client_hw_address(self):
        return self._fields.get(FIELD_CLIENT_HWADDR)

    @property
    def is_valid(self):
        """
        Checks that we have (at a minimum) values for all the required fields,
        and that the magic cookie is set correctly.
        """
        for field in DHCP_REQUIRED_FIELDS:
            if self._fields.get(field) is None:
                logging.warning("Missing field %s in packet.", field)
                return False
        if self._fields[FIELD_MAGIC_COOKIE] != FIELD_VALUE_MAGIC_COOKIE:
            return False
        return True

    @property
    def message_type(self):
        """
        Gets the value of the DHCP Message Type option in this packet.

        If the option is not present, or the value of the option is not
        recognized, returns MESSAGE_TYPE_UNKNOWN.

        @returns The MessageType for this packet, or MESSAGE_TYPE_UNKNOWN.
        """
        if (self._options.has_key(OPTION_DHCP_MESSAGE_TYPE) and
            self._options[OPTION_DHCP_MESSAGE_TYPE] > 0 and
            self._options[OPTION_DHCP_MESSAGE_TYPE] < len(MESSAGE_TYPE_BY_NUM)):
            return MESSAGE_TYPE_BY_NUM[self._options[OPTION_DHCP_MESSAGE_TYPE]]
        else:
            return MESSAGE_TYPE_UNKNOWN

    @property
    def transaction_id(self):
        return self._fields.get(FIELD_TRANSACTION_ID)

    def get_field(self, field):
        return self._fields.get(field)

    def get_option(self, option):
        return self._options.get(option)

    def set_field(self, field, field_value):
        self._fields[field] = field_value

    def set_option(self, option, option_value):
        self._options[option] = option_value

    def to_binary_string(self):
        if not self.is_valid:
            return None
        # A list of byte strings to be joined into a single string at the end.
        data = []
        offset = 0
        for field in DHCP_ALL_FIELDS:
            if field not in self._fields:
                continue
            field_data = field.pack(self._fields[field])
            while offset < field.offset:
                # This should only happen when we're padding the fields because
                # we're not filling in legacy BOOTP stuff.
                data.append("\x00")
                offset += 1
            data.append(field_data)
            offset += field.size
        # Last field processed is the magic cookie, so we're ready for options.
        # Have to process options
        for option in DHCP_PACKET_OPTIONS:
            option_value = self._options.get(option)
            if option_value is None:
                continue
            serialized_value = option.pack(option_value)
            data.append(struct.pack("BB",
                                    option.number,
                                    len(serialized_value)))
            offset += 2
            data.append(serialized_value)
            offset += len(serialized_value)
        data.append(chr(OPTION_END))
        offset += 1
        while offset < DHCP_MIN_PACKET_SIZE:
            data.append(chr(OPTION_PAD))
            offset += 1
        return "".join(data)

    def __str__(self):
        options = [k.name + "=" + str(v) for k, v in self._options.items()]
        fields = [k.name + "=" + str(v) for k, v in self._fields.items()]
        return "<DhcpPacket fields=%s, options=%s>" % (fields, options)

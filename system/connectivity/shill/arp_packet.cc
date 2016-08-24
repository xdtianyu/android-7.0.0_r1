//
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "shill/arp_packet.h"

#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <string.h>

#include <limits>

#include "shill/logging.h"

namespace shill {

const size_t ArpPacket::kMinPayloadSize = ETH_ZLEN - ETH_HLEN;

ArpPacket::ArpPacket()
    : operation_(0),
      local_ip_address_(IPAddress::kFamilyUnknown),
      remote_ip_address_(IPAddress::kFamilyUnknown) {}

ArpPacket::ArpPacket(
    const IPAddress& local_ip, const IPAddress& remote_ip,
    const ByteString& local_mac, const ByteString& remote_mac)
    : local_ip_address_(local_ip),
      remote_ip_address_(remote_ip),
      local_mac_address_(local_mac),
      remote_mac_address_(remote_mac) {}

ArpPacket::~ArpPacket() {}

// Format of an ARP packet (all multi-byte values are big-endian):
//
//       Byte 0            Byte 1           Byte 2             Byte 3
// +-----------------+-----------------+-----------------+-----------------+
// | Format of hardware address (ether)| Format of Protocol Address (IP)   |
// +-----------------+-----------------+-----------------------------------+
// | Hardware Length | Protocol Length |       ARP Protocol OpCode         |
// +-----------------+-----------------+-----------------------------------+
//
// plus a variable length section...
//
// +-----------------------------------------------------------------------+
// | Sender Hardware Address (of length "Hardware Length")...              |
// +-----------------------------------------------------------------------+
// | Sender IP Address (of length "Protocol Length")...                    |
// +-----------------------------------------------------------------------+
// | Target Hardware Address (of length "Hardware Length")...              |
// +-----------------------------------------------------------------------+
// | Target IP Address (of length "Protocol Length")...                    |
// +-----------------------------------------------------------------------+
bool ArpPacket::Parse(const ByteString& packet) {
  arphdr header;
  if (packet.GetLength() < sizeof(header)) {
    LOG(ERROR) << "Packet size " << packet.GetLength()
               << " is too short to contain ARP header.";
    return false;
  }

  memcpy(&header, packet.GetConstData(), sizeof(header));

  const uint16_t hardware_type = ntohs(header.ar_hrd);
  if (hardware_type != ARPHRD_ETHER) {
    NOTIMPLEMENTED() << "Packet is of unknown ARPHRD type "
                     << hardware_type;
    return false;
  }
  const uint16_t protocol = ntohs(header.ar_pro);
  IPAddress::Family family = IPAddress::kFamilyUnknown;
  if (protocol == ETHERTYPE_IP) {
    family = IPAddress::kFamilyIPv4;
  } else if (protocol == ETHERTYPE_IPV6) {
    family = IPAddress::kFamilyIPv6;
  } else {
    NOTIMPLEMENTED() << "Packet has unknown protocol "
                     << protocol;
    return false;
  }
  if (header.ar_hln != ETH_ALEN) {
    LOG(ERROR) << "Packet has unexpected hardware address length "
               << static_cast<int>(header.ar_hln) << "; expected " << ETH_ALEN;
    return false;
  }
  size_t ip_address_length = IPAddress::GetAddressLength(family);
  if (header.ar_pln != ip_address_length) {
    LOG(ERROR) << "Packet has unexpected protocol address length "
               << static_cast<int>(header.ar_hln) << "; expected "
               << ip_address_length;
    return false;
  }
  const uint16_t operation = ntohs(header.ar_op);
  if (operation != ARPOP_REPLY && operation != ARPOP_REQUEST) {
    NOTIMPLEMENTED() << "Packet is not an ARP reply or request but of type "
                     << operation;
    return false;
  }
  size_t min_packet_size =
      sizeof(header) + 2 * ip_address_length + 2 * ETH_ALEN;
  if (packet.GetLength() < min_packet_size) {
    NOTIMPLEMENTED() << "Packet of size "
                     << packet.GetLength()
                     << " is too small to contain entire ARP payload; "
                     << "expected at least "
                     << min_packet_size;
    return false;
  }
  operation_ = operation;
  local_mac_address_ = packet.GetSubstring(sizeof(header), ETH_ALEN);
  local_ip_address_ = IPAddress(family, packet.GetSubstring(
      sizeof(header) + ETH_ALEN, ip_address_length));
  remote_mac_address_ = packet.GetSubstring(
      sizeof(header) + ETH_ALEN + ip_address_length, ETH_ALEN);
  remote_ip_address_ = IPAddress(family, packet.GetSubstring(
      sizeof(header) + ETH_ALEN * 2 + ip_address_length, ip_address_length));
  return true;
}

// Output a payload from local parameters.
bool ArpPacket::FormatRequest(ByteString* packet) const {
  if (!local_ip_address_.IsValid() || !remote_ip_address_.IsValid()) {
    LOG(ERROR) << "Local or remote IP address is not valid.";
    return false;
  }
  if (local_ip_address_.family() != remote_ip_address_.family()) {
    LOG(ERROR) << "Local and remote IP address families do not match!";
    return false;
  }
  uint16_t protocol;
  IPAddress::Family family = local_ip_address_.family();
  if (family == IPAddress::kFamilyIPv4) {
    protocol = ETHERTYPE_IP;
  } else if (family == IPAddress::kFamilyIPv6) {
    protocol = ETHERTYPE_IPV6;
  } else {
    NOTIMPLEMENTED() << "Address family "
                     << IPAddress::GetAddressFamilyName(family)
                     << " is not supported.";
    return false;
  }
  size_t ip_address_length = IPAddress::GetAddressLength(family);
  CHECK(ip_address_length < std::numeric_limits<uint8_t>::max());
  if (local_mac_address_.GetLength() != ETH_ALEN ||
      remote_mac_address_.GetLength() != ETH_ALEN) {
    LOG(ERROR) << "Local or remote MAC address length is incorrect.";
    return false;
  }

  arphdr header;
  header.ar_hrd = htons(ARPHRD_ETHER);
  header.ar_pro = htons(protocol);
  header.ar_hln = ETH_ALEN;
  header.ar_pln = ip_address_length;
  header.ar_op = htons(ARPOP_REQUEST);

  *packet = ByteString(reinterpret_cast<const unsigned char*>(&header),
                       sizeof(header));

  packet->Append(local_mac_address_);
  packet->Append(local_ip_address_.address());
  packet->Append(remote_mac_address_);
  packet->Append(remote_ip_address_.address());

  if (packet->GetLength() < kMinPayloadSize) {
    packet->Append(ByteString(kMinPayloadSize - packet->GetLength()));
  }

  return true;
}

bool ArpPacket::IsReply() const {
  return operation_ == ARPOP_REPLY;
}

}  // namespace shill

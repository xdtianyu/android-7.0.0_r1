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

#include "shill/arp_client.h"

#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <string.h>

#include "shill/arp_packet.h"
#include "shill/logging.h"
#include "shill/net/byte_string.h"
#include "shill/net/sockets.h"

namespace shill {

// ARP opcode is the last uint16_t in the ARP header.
const size_t ArpClient::kArpOpOffset = sizeof(arphdr) - sizeof(uint16_t);

// The largest packet we expect is one with IPv6 addresses in it.
const size_t ArpClient::kMaxArpPacketLength =
    sizeof(arphdr) + sizeof(in6_addr) * 2 + ETH_ALEN * 2;

ArpClient::ArpClient(int interface_index)
    : interface_index_(interface_index),
      sockets_(new Sockets()),
      socket_(-1) {}

ArpClient::~ArpClient() {}

bool ArpClient::StartReplyListener() {
  return Start(ARPOP_REPLY);
}

bool ArpClient::StartRequestListener() {
  return Start(ARPOP_REQUEST);
}

bool ArpClient::Start(uint16_t arp_opcode) {
  if (!CreateSocket(arp_opcode)) {
    LOG(ERROR) << "Could not open ARP socket.";
    Stop();
    return false;
  }
  return true;
}

void ArpClient::Stop() {
  socket_closer_.reset();
}


bool ArpClient::CreateSocket(uint16_t arp_opcode) {
  int socket = sockets_->Socket(PF_PACKET, SOCK_DGRAM, htons(ETHERTYPE_ARP));
  if (socket == -1) {
    PLOG(ERROR) << "Could not create ARP socket";
    return false;
  }
  socket_ = socket;
  socket_closer_.reset(new ScopedSocketCloser(sockets_.get(), socket_));

  // Create a packet filter incoming ARP packets.
  const sock_filter arp_filter[] = {
    // If a packet contains the ARP opcode we are looking for...
    BPF_STMT(BPF_LD | BPF_H | BPF_ABS, kArpOpOffset),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, arp_opcode, 0, 1),
    // Return the the packet (up to largest expected packet size).
    BPF_STMT(BPF_RET | BPF_K, kMaxArpPacketLength),
    // Otherwise, drop it.
    BPF_STMT(BPF_RET | BPF_K, 0),
  };

  sock_fprog pf;
  pf.filter = const_cast<sock_filter*>(arp_filter);
  pf.len = arraysize(arp_filter);
  if (sockets_->AttachFilter(socket_, &pf) != 0) {
    PLOG(ERROR) << "Could not attach packet filter";
    return false;
  }

  if (sockets_->SetNonBlocking(socket_) != 0) {
    PLOG(ERROR) << "Could not set socket to be non-blocking";
    return false;
  }

  sockaddr_ll socket_address;
  memset(&socket_address, 0, sizeof(socket_address));
  socket_address.sll_family = AF_PACKET;
  socket_address.sll_protocol = htons(ETHERTYPE_ARP);
  socket_address.sll_ifindex = interface_index_;

  if (sockets_->Bind(socket_,
                     reinterpret_cast<struct sockaddr*>(&socket_address),
                     sizeof(socket_address)) != 0) {
    PLOG(ERROR) << "Could not bind socket to interface";
    return false;
  }

  return true;
}

bool ArpClient::ReceivePacket(ArpPacket* packet, ByteString* sender) const {
  ByteString payload(kMaxArpPacketLength);
  sockaddr_ll socket_address;
  memset(&socket_address, 0, sizeof(socket_address));
  socklen_t socklen = sizeof(socket_address);
  int result = sockets_->RecvFrom(
      socket_,
      payload.GetData(),
      payload.GetLength(),
      0,
      reinterpret_cast<struct sockaddr*>(&socket_address),
      &socklen);
  if (result < 0) {
    PLOG(ERROR) << "Socket recvfrom failed";
    return false;
  }

  payload.Resize(result);
  if (!packet->Parse(payload)) {
    LOG(ERROR) << "Failed to parse ARP packet.";
    return false;
  }

  // The socket address returned may only be big enough to contain
  // the hardware address of the sender.
  CHECK(socklen >=
        sizeof(socket_address) - sizeof(socket_address.sll_addr) + ETH_ALEN);
  CHECK(socket_address.sll_halen == ETH_ALEN);
  *sender = ByteString(
      reinterpret_cast<const unsigned char*>(&socket_address.sll_addr),
      socket_address.sll_halen);
  return true;
}

bool ArpClient::TransmitRequest(const ArpPacket& packet) const {
  ByteString payload;
  if (!packet.FormatRequest(&payload)) {
    return false;
  }

  sockaddr_ll socket_address;
  memset(&socket_address, 0, sizeof(socket_address));
  socket_address.sll_family = AF_PACKET;
  socket_address.sll_protocol = htons(ETHERTYPE_ARP);
  socket_address.sll_hatype = ARPHRD_ETHER;
  socket_address.sll_halen = ETH_ALEN;
  socket_address.sll_ifindex = interface_index_;

  ByteString remote_address = packet.remote_mac_address();
  CHECK(sizeof(socket_address.sll_addr) >= remote_address.GetLength());
  if (remote_address.IsZero()) {
    // If the destination MAC address is unspecified, send the packet
    // to the broadcast (all-ones) address.
    remote_address.BitwiseInvert();
  }
  memcpy(&socket_address.sll_addr, remote_address.GetConstData(),
         remote_address.GetLength());

  int result = sockets_->SendTo(
      socket_,
      payload.GetConstData(),
      payload.GetLength(),
      0,
      reinterpret_cast<struct sockaddr*>(&socket_address),
      sizeof(socket_address));
  const int expected_result  = static_cast<int>(payload.GetLength());
  if (result != expected_result) {
    if (result < 0) {
      PLOG(ERROR) << "Socket sendto failed";
    } else if (result < static_cast<int>(payload.GetLength())) {
      LOG(ERROR) << "Socket sendto returned "
                 << result
                 << " which is different from expected result "
                 << expected_result;
    }
    return false;
  }

  return true;
}

}  // namespace shill

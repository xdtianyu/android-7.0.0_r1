//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/icmp.h"

#include <netinet/ip_icmp.h>

#include "shill/logging.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"

namespace shill {

const int Icmp::kIcmpEchoCode = 0;  // value specified in RFC 792.

Icmp::Icmp()
    : sockets_(new Sockets()),
      socket_(-1) {}

Icmp::~Icmp() {}

bool Icmp::Start() {
  int socket = sockets_->Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (socket == -1) {
    PLOG(ERROR) << "Could not create ICMP socket";
    Stop();
    return false;
  }
  socket_ = socket;
  socket_closer_.reset(new ScopedSocketCloser(sockets_.get(), socket_));

  if (sockets_->SetNonBlocking(socket_) != 0) {
    PLOG(ERROR) << "Could not set socket to be non-blocking";
    Stop();
    return false;
  }

  return true;
}

void Icmp::Stop() {
  socket_closer_.reset();
  socket_ = -1;
}

bool Icmp::IsStarted() const {
  return socket_closer_.get();
}

bool Icmp::TransmitEchoRequest(const IPAddress& destination, uint16_t id,
                               uint16_t seq_num) {
  if (!IsStarted() && !Start()) {
    return false;
  }

  if (!destination.IsValid()) {
    LOG(ERROR) << "Destination address is not valid.";
    return false;
  }

  if (destination.family() != IPAddress::kFamilyIPv4) {
    NOTIMPLEMENTED() << "Only IPv4 destination addresses are implemented.";
    return false;
  }

  struct icmphdr icmp_header;
  memset(&icmp_header, 0, sizeof(icmp_header));
  icmp_header.type = ICMP_ECHO;
  icmp_header.code = kIcmpEchoCode;
  icmp_header.un.echo.id = id;
  icmp_header.un.echo.sequence = seq_num;
  icmp_header.checksum = ComputeIcmpChecksum(icmp_header, sizeof(icmp_header));

  struct sockaddr_in destination_address;
  destination_address.sin_family = AF_INET;
  CHECK_EQ(sizeof(destination_address.sin_addr.s_addr),
           destination.GetLength());
  memcpy(&destination_address.sin_addr.s_addr,
         destination.address().GetConstData(),
         sizeof(destination_address.sin_addr.s_addr));

  int result = sockets_->SendTo(
      socket_,
      &icmp_header,
      sizeof(icmp_header),
      0,
      reinterpret_cast<struct sockaddr*>(&destination_address),
      sizeof(destination_address));
  int expected_result = sizeof(icmp_header);
  if (result != expected_result) {
    if (result < 0) {
      PLOG(ERROR) << "Socket sendto failed";
    } else if (result < expected_result) {
      LOG(ERROR) << "Socket sendto returned "
                 << result
                 << " which is less than the expected result "
                 << expected_result;
    }
    return false;
  }

  return true;
}

// static
uint16_t Icmp::ComputeIcmpChecksum(const struct icmphdr& hdr, size_t len) {
  // Compute Internet Checksum for "len" bytes beginning at location "hdr".
  // Adapted directly from the canonical implementation in RFC 1071 Section 4.1.
  uint32_t sum = 0;
  const uint16_t* addr = reinterpret_cast<const uint16_t*>(&hdr);

  while (len > 1) {
    sum += *addr;
    ++addr;
    len -= sizeof(*addr);
  }

  // Add left-over byte, if any.
  if (len > 0) {
    sum += *reinterpret_cast<const uint8_t*>(addr);
  }

  // Fold 32-bit sum to 16 bits.
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  return static_cast<uint16_t>(~sum);
}

}  // namespace shill

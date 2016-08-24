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

#ifndef SHILL_ARP_PACKET_H_
#define SHILL_ARP_PACKET_H_

#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"

namespace shill {

// ArpPacket encapsulates the task of creating and parsing
// Address Resolution Protocol (ARP) packets for IP and
// IPv6 protocols on Ethernet (or Ethernet-like) networks.
class ArpPacket {
 public:
  ArpPacket();
  ArpPacket(const IPAddress& local_ip, const IPAddress& remote_ip,
            const ByteString& local_mac, const ByteString& remote_mac);
  virtual ~ArpPacket();

  // Parse a payload and save to local parameters.
  bool Parse(const ByteString& packet);

  // Output a payload from local parameters.
  bool FormatRequest(ByteString* packet) const;

  // Returns true if this packet is an ARP response.
  bool IsReply() const;

  // Getters and seters.
  const IPAddress& local_ip_address() const { return local_ip_address_; }
  void set_local_ip_address(const IPAddress& address) {
    local_ip_address_ = address;
  }

  const IPAddress& remote_ip_address() const { return remote_ip_address_; }
  void set_remote_ip_address(const IPAddress& address) {
    remote_ip_address_ = address;
  }

  const ByteString& local_mac_address() const { return local_mac_address_; }
  void set_local_mac_address(const ByteString& address) {
    local_mac_address_ = address;
  }

  const ByteString& remote_mac_address() const { return remote_mac_address_; }
  void set_remote_mac_address(const ByteString& address) {
    remote_mac_address_ = address;
  }

  uint16_t operation() const { return operation_; }
  void set_operation(uint16_t operation) {
    operation_ = operation;
  }

 private:
  friend class ArpPacketTest;

  // The minimum number of bytes of ARP payload which will produce the
  // smallest valid Ethernet frame.
  static const size_t kMinPayloadSize;

  uint16_t operation_;
  IPAddress local_ip_address_;
  IPAddress remote_ip_address_;
  ByteString local_mac_address_;
  ByteString remote_mac_address_;

  DISALLOW_COPY_AND_ASSIGN(ArpPacket);
};

}  // namespace shill

#endif  // SHILL_ARP_PACKET_H_

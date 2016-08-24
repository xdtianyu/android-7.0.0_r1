//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef DHCP_CLIENT_DHCPV4_H_
#define DHCP_CLIENT_DHCPV4_H_

#include <random>
#include <string>

#include <base/macros.h>
#include <base/strings/stringprintf.h>
#include <shill/net/byte_string.h>
#include <shill/net/io_handler_factory_container.h>
#include <shill/net/sockets.h>

#include "dhcp_client/dhcp.h"
#include "dhcp_client/dhcp_message.h"
#include "dhcp_client/event_dispatcher_interface.h"

namespace dhcp_client {

class DHCPV4 : public DHCP {
 public:
  DHCPV4(const std::string& interface_name,
         const shill::ByteString& hardware_address,
         unsigned int interface_index,
         const std::string& network_id,
         bool request_hostname,
         bool arp_gateway,
         bool unicast_arp,
         EventDispatcherInterface* event_dispatcher);

  virtual ~DHCPV4();

  bool Start();
  void Stop();

 private:
  bool CreateRawSocket();
  bool MakeRawPacket(const DHCPMessage& message, shill::ByteString* buffer);
  void OnReadError(const std::string& error_msg);
  void ParseRawPacket(shill::InputData* data);
  bool SendRawPacket(const shill::ByteString& buffer);
  // Validate the IP and UDP header and return the total headers length.
  // Return -1 if any header is invalid.
  int ValidatePacketHeader(const unsigned char* buffer, size_t len);

  void HandleOffer(const DHCPMessage& msg);
  void HandleAck(const DHCPMessage& msg);
  void HandleNak(const DHCPMessage& msg);

  // Interface parameters.
  std::string interface_name_;
  shill::ByteString hardware_address_;
  unsigned int interface_index_;

  // Unique network/connection identifier,
  // lease will persist to storage if this identifier is specified.
  std::string network_id_;

  // DHCP IPv4 configurations:
  // Request hostname from server.
  bool request_hostname_;
  // ARP for default gateway.
  bool arp_gateway_;
  // Enable unicast ARP on renew.
  bool unicast_arp_;

  EventDispatcherInterface* event_dispatcher_;
  shill::IOHandlerFactory *io_handler_factory_;
  std::unique_ptr<shill::IOHandler> input_handler_;

  // DHCP state variables.
  State state_;
  uint32_t server_identifier_;
  uint32_t transaction_id_;
  uint32_t from_;
  uint32_t to_;

  // Socket used for sending and receiving DHCP messages.
  int socket_;
  // Helper class with wrapped socket relavent functions.
  std::unique_ptr<shill::Sockets> sockets_;

  std::default_random_engine random_engine_;

  DISALLOW_COPY_AND_ASSIGN(DHCPV4);
};

}  // namespace dhcp_client

#endif  // DHCP_CLIENT_DHCPV4_H_

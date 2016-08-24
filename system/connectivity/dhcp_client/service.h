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

#ifndef DHCP_CLIENT_SERVICE_H_
#define DHCP_CLIENT_SERVICE_H_

#include <string>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/variant_dictionary.h>

#include "dhcp_client/dhcp.h"
#include "dhcp_client/dhcpv4.h"
#include "dhcp_client/event_dispatcher_interface.h"
#include "shill/net/byte_string.h"

namespace dhcp_client {

class Manager;

class Service : public base::RefCounted<Service> {
 public:
  Service(Manager* manager,
          int service_identifier,
          EventDispatcherInterface* event_dispatcher,
          const brillo::VariantDictionary& configs);

  virtual ~Service();
  bool Start();
  void Stop();

 private:
  Manager* manager_;
  // Indentifier number of this service.
  int identifier_;
  EventDispatcherInterface* event_dispatcher_;
  // Interface parameters.
  std::string interface_name_;
  shill::ByteString hardware_address_;
  unsigned int interface_index_;

  // Unique network/connection identifier,
  // lease will persist to storage if this identifier is specified.
  std::string network_id_;

  // Type of the DHCP service.
  // It can be IPv4 only or IPv6 only or both.
  DHCP::ServiceType type_;

  // DHCP IPv4 configurations:
  // Request hostname from server.
  bool request_hostname_;
  // ARP for default gateway.
  bool arp_gateway_;
  // Enable unicast ARP on renew.
  bool unicast_arp_;

  // DHCP IPv6 configurations:
  // Request non-temporary address.
  bool request_na_;
  // Request prefix delegation.
  bool request_pd_;

  std::unique_ptr<DHCPV4> state_machine_ipv4_;
  // Parse DHCP configurations from the VariantDictionary.
  void ParseConfigs(const brillo::VariantDictionary& configs);

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace dhcp_client

#endif  // DHCP_CLIENT_SERVICE_H_

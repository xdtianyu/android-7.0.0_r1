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

#ifndef SHILL_CONNECTION_INFO_H_
#define SHILL_CONNECTION_INFO_H_

#include <base/macros.h>

#include "shill/net/ip_address.h"

namespace shill {

class ConnectionInfo {
 public:
  ConnectionInfo();
  ConnectionInfo(int protocol,
                 int64_t time_to_expire_seconds,
                 bool is_unreplied,
                 IPAddress original_source_ip_address,
                 uint16_t original_source_port,
                 IPAddress original_destination_ip_address,
                 uint16_t original_destination_port,
                 IPAddress reply_source_ip_address,
                 uint16_t reply_source_port,
                 IPAddress reply_destination_ip_address,
                 uint16_t reply_destination_port);
  ConnectionInfo(const ConnectionInfo& info);
  ~ConnectionInfo();

  ConnectionInfo& operator=(const ConnectionInfo& info);

  int protocol() const { return protocol_; }
  void set_protocol(int protocol) { protocol_ = protocol; }

  int64_t time_to_expire_seconds() const { return time_to_expire_seconds_; }
  void set_time_to_expire_seconds(int64_t time_to_expire_seconds) {
    time_to_expire_seconds_ = time_to_expire_seconds;
  }

  bool is_unreplied() const { return is_unreplied_; }
  void set_is_unreplied(bool is_unreplied) { is_unreplied_ = is_unreplied; }

  const IPAddress& original_source_ip_address() const {
    return original_source_ip_address_;
  }
  void set_original_source_ip_address(
      const IPAddress& original_source_ip_address) {
    original_source_ip_address_ = original_source_ip_address;
  }

  uint16_t original_source_port() const { return original_source_port_; }
  void set_original_source_port(uint16_t original_source_port) {
    original_source_port_ = original_source_port;
  }

  const IPAddress& original_destination_ip_address() const {
    return original_destination_ip_address_;
  }
  void set_original_destination_ip_address(
      const IPAddress& original_destination_ip_address) {
    original_destination_ip_address_ = original_destination_ip_address;
  }

  uint16_t original_destination_port() const {
    return original_destination_port_;
  }
  void set_original_destination_port(uint16_t original_destination_port) {
    original_destination_port_ = original_destination_port;
  }

  const IPAddress& reply_source_ip_address() const {
    return reply_source_ip_address_;
  }
  void set_reply_source_ip_address(
      const IPAddress& reply_source_ip_address) {
    reply_source_ip_address_ = reply_source_ip_address;
  }

  uint16_t reply_source_port() const { return reply_source_port_; }
  void set_reply_source_port(uint16_t reply_source_port) {
    reply_source_port_ = reply_source_port;
  }

  const IPAddress& reply_destination_ip_address() const {
    return reply_destination_ip_address_;
  }
  void set_reply_destination_ip_address(
      const IPAddress& reply_destination_ip_address) {
    reply_destination_ip_address_ = reply_destination_ip_address;
  }

  uint16_t reply_destination_port() const { return reply_destination_port_; }
  void set_reply_destination_port(uint16_t reply_destination_port) {
    reply_destination_port_ = reply_destination_port;
  }

 private:
  int protocol_;
  int64_t time_to_expire_seconds_;
  bool is_unreplied_;

  IPAddress original_source_ip_address_;
  uint16_t original_source_port_;
  IPAddress original_destination_ip_address_;
  uint16_t original_destination_port_;

  IPAddress reply_source_ip_address_;
  uint16_t reply_source_port_;
  IPAddress reply_destination_ip_address_;
  uint16_t reply_destination_port_;

  // No DISALLOW_COPY_AND_ASSIGN(ConnectionInfo) as ConnectionInfo needs to be
  // kept in STL containers.
};

}  // namespace shill

#endif  // SHILL_CONNECTION_INFO_H_

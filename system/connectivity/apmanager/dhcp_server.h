//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef APMANAGER_DHCP_SERVER_H_
#define APMANAGER_DHCP_SERVER_H_

#include <string>

#include <base/macros.h>
#include <shill/net/ip_address.h>
#include <shill/net/rtnl_handler.h>

#include "apmanager/file_writer.h"
#include "apmanager/process_factory.h"

namespace apmanager {

class DHCPServer {
 public:
  DHCPServer(uint16_t server_address_index,
             const std::string& interface_name);
  virtual ~DHCPServer();

  // Start the DHCP server
  virtual bool Start();

 private:
  friend class DHCPServerTest;

  std::string GenerateConfigFile();

  static const char kDnsmasqPath[];
  static const char kDnsmasqConfigFilePathFormat[];
  static const char kDHCPLeasesFilePathFormat[];
  static const char kServerAddressFormat[];
  static const char kAddressRangeLowFormat[];
  static const char kAddressRangeHighFormat[];
  static const int kServerAddressPrefix;
  static const int kTerminationTimeoutSeconds;
#if defined(__ANDROID__)
  static const char kDnsmasqPidFilePath[];
#endif  // __ANDROID__

  uint16_t server_address_index_;
  std::string interface_name_;
  shill::IPAddress server_address_;
  std::unique_ptr<brillo::Process> dnsmasq_process_;
  shill::RTNLHandler* rtnl_handler_;
  FileWriter* file_writer_;
  ProcessFactory* process_factory_;

  DISALLOW_COPY_AND_ASSIGN(DHCPServer);
};

}  // namespace apmanager

#endif  // APMANAGER_DHCP_SERVER_H_

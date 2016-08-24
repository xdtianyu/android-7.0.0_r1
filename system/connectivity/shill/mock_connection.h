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

#ifndef SHILL_MOCK_CONNECTION_H_
#define SHILL_MOCK_CONNECTION_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/connection.h"

namespace shill {

class MockConnection : public Connection {
 public:
  explicit MockConnection(const DeviceInfo* device_info);
  ~MockConnection() override;

  MOCK_METHOD1(UpdateFromIPConfig, void(const IPConfigRefPtr& config));
  MOCK_CONST_METHOD0(GetLowerConnection, ConnectionRefPtr());
  MOCK_CONST_METHOD0(is_default, bool());
  MOCK_METHOD1(SetIsDefault, void(bool is_default));
  MOCK_CONST_METHOD0(ipconfig_rpc_identifier, const std::string&());
  MOCK_METHOD0(RequestRouting, void());
  MOCK_METHOD0(ReleaseRouting, void());
  MOCK_CONST_METHOD0(interface_name, const std::string&());
  MOCK_CONST_METHOD0(dns_servers, const std::vector<std::string>&());
  MOCK_METHOD1(RequestHostRoute, bool(const IPAddress& destination));
  MOCK_METHOD2(PinPendingRoutes,
               bool(int interface_index, RoutingTableEntry entry));
  MOCK_CONST_METHOD0(local, const IPAddress&());
  MOCK_CONST_METHOD0(gateway, const IPAddress&());
  MOCK_CONST_METHOD0(technology, Technology::Identifier());
  MOCK_METHOD0(CreateGatewayRoute, bool());
  MOCK_METHOD0(GetCarrierConnection, ConnectionRefPtr());
  MOCK_CONST_METHOD0(tethering, std::string&());
  MOCK_METHOD1(UpdateDNSServers,
               void(const std::vector<std::string>& dns_servers));
  MOCK_METHOD0(IsIPv6, bool());
  MOCK_CONST_METHOD0(GetSubnetName, std::string());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockConnection);
};

}  // namespace shill

#endif  // SHILL_MOCK_CONNECTION_H_

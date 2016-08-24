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

#ifndef SHILL_DHCP_MOCK_DHCP_PROVIDER_H_
#define SHILL_DHCP_MOCK_DHCP_PROVIDER_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/dhcp/dhcp_config.h"
#include "shill/dhcp/dhcp_provider.h"
#include "shill/dhcp_properties.h"
#include "shill/refptr_types.h"

namespace shill {

class MockDHCPProvider : public DHCPProvider {
 public:
  MockDHCPProvider();
  ~MockDHCPProvider() override;

  MOCK_METHOD3(Init,
               void(ControlInterface*, EventDispatcher*, Metrics*));
  MOCK_METHOD4(CreateIPv4Config,
               DHCPConfigRefPtr(const std::string& device_name,
                                const std::string& storage_identifier,
                                bool arp_gateway,
                                const DhcpProperties& dhcp_props));
  MOCK_METHOD2(CreateIPv6Config,
               DHCPConfigRefPtr(const std::string& device_name,
                                const std::string& storage_identifier));
  MOCK_METHOD2(BindPID, void(int pid, const DHCPConfigRefPtr& config));
  MOCK_METHOD1(UnbindPID, void(int pid));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDHCPProvider);
};

}  // namespace shill

#endif  // SHILL_DHCP_MOCK_DHCP_PROVIDER_H_

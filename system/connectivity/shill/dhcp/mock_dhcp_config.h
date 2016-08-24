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

#ifndef SHILL_DHCP_MOCK_DHCP_CONFIG_H_
#define SHILL_DHCP_MOCK_DHCP_CONFIG_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/dhcp/dhcp_config.h"

namespace shill {

class MockDHCPConfig : public DHCPConfig {
 public:
  MockDHCPConfig(ControlInterface* control_interface,
                 const std::string& device_name);
  ~MockDHCPConfig() override;

  void ProcessEventSignal(const std::string& reason,
                          const KeyValueStore& configuration) override;
  void ProcessStatusChangeSignal(const std::string& status) override;

  MOCK_METHOD0(RequestIP, bool());
  MOCK_METHOD1(ReleaseIP, bool(ReleaseReason));
  MOCK_METHOD0(RenewIP, bool());
  MOCK_METHOD1(set_minimum_mtu, void(int));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDHCPConfig);
};

}  // namespace shill

#endif  // SHILL_DHCP_MOCK_DHCP_CONFIG_H_

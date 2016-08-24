//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_MOCK_IPCONFIG_H_
#define SHILL_MOCK_IPCONFIG_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/ipconfig.h"

namespace shill {

class MockIPConfig : public IPConfig {
 public:
  MockIPConfig(ControlInterface* control_interface,
               const std::string& device_name);
  ~MockIPConfig() override;

  MOCK_CONST_METHOD0(properties, const Properties& (void));
  MOCK_METHOD0(RequestIP, bool(void));
  MOCK_METHOD0(RenewIP, bool(void));
  MOCK_METHOD1(ReleaseIP, bool(ReleaseReason reason));
  MOCK_METHOD0(ResetProperties, void(void));
  MOCK_METHOD0(EmitChanges, void(void));
  MOCK_METHOD1(UpdateDNSServers,
               void(const std::vector<std::string>& dns_servers));
  MOCK_METHOD1(UpdateLeaseExpirationTime, void(uint32_t new_lease_duration));
  MOCK_METHOD0(ResetLeaseExpirationTime, void(void));

 private:
  const Properties& real_properties() {
    return IPConfig::properties();
  }

  DISALLOW_COPY_AND_ASSIGN(MockIPConfig);
};

}  // namespace shill

#endif  // SHILL_MOCK_IPCONFIG_H_

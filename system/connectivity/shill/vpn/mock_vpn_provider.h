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

#ifndef SHILL_VPN_MOCK_VPN_PROVIDER_H_
#define SHILL_VPN_MOCK_VPN_PROVIDER_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/vpn/vpn_provider.h"

namespace shill {

class MockVPNProvider : public VPNProvider {
 public:
  MockVPNProvider();
  ~MockVPNProvider() override;

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD2(OnDeviceInfoAvailable, bool(const std::string& link_name,
                                           int interface_index));
  MOCK_CONST_METHOD0(HasActiveService, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockVPNProvider);
};

}  // namespace shill

#endif  // SHILL_VPN_MOCK_VPN_PROVIDER_H_

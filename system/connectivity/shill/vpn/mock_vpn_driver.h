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

#ifndef SHILL_VPN_MOCK_VPN_DRIVER_H_
#define SHILL_VPN_MOCK_VPN_DRIVER_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/vpn/vpn_driver.h"

namespace shill {

class MockVPNDriver : public VPNDriver {
 public:
  MockVPNDriver();
  ~MockVPNDriver() override;

  MOCK_METHOD2(ClaimInterface, bool(const std::string& link_name,
                                    int interface_index));
  MOCK_METHOD2(Connect, void(const VPNServiceRefPtr& service, Error* error));
  MOCK_METHOD0(Disconnect, void());
  MOCK_METHOD0(OnConnectionDisconnected, void());
  MOCK_METHOD2(Load, bool(StoreInterface* storage,
                          const std::string& storage_id));
  MOCK_METHOD3(Save, bool(StoreInterface* storage,
                          const std::string& storage_id,
                          bool save_credentials));
  MOCK_METHOD0(UnloadCredentials, void());
  MOCK_METHOD1(InitPropertyStore, void(PropertyStore* store));
  MOCK_CONST_METHOD0(GetProviderType, std::string());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockVPNDriver);
};

}  // namespace shill

#endif  // SHILL_VPN_MOCK_VPN_DRIVER_H_

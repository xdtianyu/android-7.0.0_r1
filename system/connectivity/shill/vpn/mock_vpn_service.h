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

#ifndef SHILL_VPN_MOCK_VPN_SERVICE_H_
#define SHILL_VPN_MOCK_VPN_SERVICE_H_

#include <gmock/gmock.h>

#include "shill/vpn/vpn_service.h"

namespace shill {

class MockVPNService : public VPNService {
 public:
  MockVPNService(ControlInterface* control,
                 EventDispatcher* dispatcher,
                 Metrics* metrics,
                 Manager* manager,
                 VPNDriver* driver);
  ~MockVPNService() override;

  MOCK_METHOD1(SetState, void(ConnectState state));
  MOCK_METHOD1(SetFailure, void(ConnectFailure failure));
  MOCK_METHOD0(InitDriverPropertyStore, void());
  MOCK_CONST_METHOD0(unloaded, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockVPNService);
};

}  // namespace shill

#endif  // SHILL_VPN_MOCK_VPN_SERVICE_H_

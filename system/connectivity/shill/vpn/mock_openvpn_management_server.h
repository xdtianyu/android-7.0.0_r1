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

#ifndef SHILL_VPN_MOCK_OPENVPN_MANAGEMENT_SERVER_H_
#define SHILL_VPN_MOCK_OPENVPN_MANAGEMENT_SERVER_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/vpn/openvpn_management_server.h"

namespace shill {

class MockOpenVPNManagementServer : public OpenVPNManagementServer {
 public:
  MockOpenVPNManagementServer();
  ~MockOpenVPNManagementServer() override;

  MOCK_METHOD3(Start, bool(EventDispatcher* dispatcher,
                           Sockets* sockets,
                           std::vector<std::vector<std::string>>* options));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(ReleaseHold, void());
  MOCK_METHOD0(Hold, void());
  MOCK_METHOD0(Restart, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockOpenVPNManagementServer);
};

}  // namespace shill

#endif  // SHILL_VPN_MOCK_OPENVPN_MANAGEMENT_SERVER_H_

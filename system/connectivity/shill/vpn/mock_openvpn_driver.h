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

#ifndef SHILL_VPN_MOCK_OPENVPN_DRIVER_H_
#define SHILL_VPN_MOCK_OPENVPN_DRIVER_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/vpn/openvpn_driver.h"

namespace shill {

class MockOpenVPNDriver : public OpenVPNDriver {
 public:
  MockOpenVPNDriver();
  ~MockOpenVPNDriver() override;

  MOCK_METHOD1(OnReconnecting, void(ReconnectReason reason));
  MOCK_METHOD0(IdleService, void());
  MOCK_METHOD2(FailService, void(Service::ConnectFailure failure,
                                 const std::string& error_details));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockOpenVPNDriver);
};

}  // namespace shill

#endif  // SHILL_VPN_MOCK_OPENVPN_DRIVER_H_

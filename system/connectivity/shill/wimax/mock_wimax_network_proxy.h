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

#ifndef SHILL_WIMAX_MOCK_WIMAX_NETWORK_PROXY_H_
#define SHILL_WIMAX_MOCK_WIMAX_NETWORK_PROXY_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/wimax/wimax_network_proxy_interface.h"

namespace shill {

class MockWiMaxNetworkProxy : public WiMaxNetworkProxyInterface {
 public:
  MockWiMaxNetworkProxy();
  ~MockWiMaxNetworkProxy() override;

  MOCK_CONST_METHOD0(path, RpcIdentifier());
  MOCK_METHOD1(set_signal_strength_changed_callback,
               void(const SignalStrengthChangedCallback& callback));
  MOCK_METHOD1(Identifier, uint32_t(Error* error));
  MOCK_METHOD1(Name, std::string(Error* error));
  MOCK_METHOD1(Type, int(Error* error));
  MOCK_METHOD1(CINR, int(Error* error));
  MOCK_METHOD1(RSSI, int(Error* error));
  MOCK_METHOD1(SignalStrength, int(Error* error));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiMaxNetworkProxy);
};

}  // namespace shill

#endif  // SHILL_WIMAX_MOCK_WIMAX_NETWORK_PROXY_H_

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

#ifndef SHILL_SUPPLICANT_SUPPLICANT_NETWORK_PROXY_INTERFACE_H_
#define SHILL_SUPPLICANT_SUPPLICANT_NETWORK_PROXY_INTERFACE_H_

#include <map>

namespace shill {

// SupplicantNetworkProxyInterface declares only the subset of
// fi::w1::wpa_supplicant1::Network_proxy that is actually used by WiFi.
class SupplicantNetworkProxyInterface {
 public:
  virtual ~SupplicantNetworkProxyInterface() {}

  virtual bool SetEnabled(bool enabled) = 0;
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_SUPPLICANT_NETWORK_PROXY_INTERFACE_H_

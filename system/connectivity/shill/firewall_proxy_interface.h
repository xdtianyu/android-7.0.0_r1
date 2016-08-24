//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_FIREWALL_PROXY_INTERFACE_H_
#define SHILL_FIREWALL_PROXY_INTERFACE_H_

#include <string>
#include <vector>

namespace shill {

class FirewallProxyInterface {
 public:
  virtual ~FirewallProxyInterface() {}
  virtual bool RequestVpnSetup(const std::vector<std::string>& user_names,
                               const std::string& interface) = 0;
  virtual bool RemoveVpnSetup() = 0;
};

}  // namespace shill

#endif  // SHILL_FIREWALL_PROXY_INTERFACE_H_

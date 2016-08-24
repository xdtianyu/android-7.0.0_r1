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

#ifndef APMANAGER_FIREWALL_PROXY_INTERFACE_H_
#define APMANAGER_FIREWALL_PROXY_INTERFACE_H_

#include <string>

namespace apmanager {

class FirewallProxyInterface {
 public:
  virtual ~FirewallProxyInterface() {}

  virtual bool RequestUdpPortAccess(const std::string& interface,
                                    uint16_t port) = 0;
  virtual bool ReleaseUdpPortAccess(const std::string& interface,
                                    uint16_t port) = 0;
};

}  // namespace apmanager

#endif  // APMANAGER_FIREWALL_PROXY_INTERFACE_H_

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

#ifndef SHILL_DBUS_CHROMEOS_FIREWALLD_PROXY_H_
#define SHILL_DBUS_CHROMEOS_FIREWALLD_PROXY_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <firewalld/dbus-proxies.h>

#include "shill/firewall_proxy_interface.h"

namespace shill {

class ChromeosFirewalldProxy : public FirewallProxyInterface {
 public:
  explicit ChromeosFirewalldProxy(const scoped_refptr<dbus::Bus>& bus);
  ~ChromeosFirewalldProxy() override;

  bool RequestVpnSetup(const std::vector<std::string>& user_names,
                       const std::string& interface) override;

  bool RemoveVpnSetup() override;

 private:
  std::unique_ptr<org::chromium::FirewalldProxy> proxy_;
  std::vector<std::string> user_names_;
  std::string interface_name_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosFirewalldProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_FIREWALLD_PROXY_H_

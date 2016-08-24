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

#ifndef SHILL_DBUS_CHROMEOS_DHCPCD_PROXY_H_
#define SHILL_DBUS_CHROMEOS_DHCPCD_PROXY_H_

#include <string>

#include <base/macros.h>

#include "dhcpcd/dbus-proxies.h"
#include "shill/dhcp/dhcp_proxy_interface.h"

namespace shill {

// There's a single DHCPCD proxy per DHCP client identified by its process id.
class ChromeosDHCPCDProxy : public DHCPProxyInterface {
 public:
  ChromeosDHCPCDProxy(const scoped_refptr<dbus::Bus>& bus,
                      const std::string& service_name);
  ~ChromeosDHCPCDProxy() override;

  // Inherited from DHCPProxyInterface.
  void Rebind(const std::string& interface) override;
  void Release(const std::string& interface) override;

 private:
  void LogDBusError(const brillo::ErrorPtr& error,
                    const std::string& method,
                    const std::string& interface);

  std::unique_ptr<org::chromium::dhcpcdProxy> dhcpcd_proxy_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosDHCPCDProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_DHCPCD_PROXY_H_

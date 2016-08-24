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

#ifndef SHILL_DBUS_CHROMEOS_THIRD_PARTY_VPN_DBUS_ADAPTOR_H_
#define SHILL_DBUS_CHROMEOS_THIRD_PARTY_VPN_DBUS_ADAPTOR_H_

#include <map>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/memory/scoped_ptr.h>

#include "dbus_bindings/org.chromium.flimflam.ThirdPartyVpn.h"
#include "shill/adaptor_interfaces.h"
#include "shill/dbus/chromeos_dbus_adaptor.h"

namespace shill {

class ThirdPartyVpnDriver;

class ChromeosThirdPartyVpnDBusAdaptor
    : public org::chromium::flimflam::ThirdPartyVpnAdaptor,
      public org::chromium::flimflam::ThirdPartyVpnInterface,
      public ChromeosDBusAdaptor,
      public ThirdPartyVpnAdaptorInterface {
 public:
  enum ExternalConnectState {
    kStateConnected = 1,
    kStateFailure,
  };

  ChromeosThirdPartyVpnDBusAdaptor(const scoped_refptr<dbus::Bus>& bus,
                                   ThirdPartyVpnDriver* client);
  ~ChromeosThirdPartyVpnDBusAdaptor() override;

  // Implementation of ThirdPartyVpnAdaptorInterface
  void EmitPacketReceived(const std::vector<uint8_t>& packet) override;
  void EmitPlatformMessage(uint32_t message) override;

  // Implementation of org::chromium::flimflam::ThirdPartyVpnAdaptor
  bool SetParameters(
      brillo::ErrorPtr* error,
      const std::map<std::string, std::string>& parameters,
      std::string* warning_message) override;
  bool UpdateConnectionState(brillo::ErrorPtr* error,
                             uint32_t connection_state) override;
  bool SendPacket(brillo::ErrorPtr* error,
                  const std::vector<uint8_t>& ip_packet) override;

 private:
  ThirdPartyVpnDriver* client_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosThirdPartyVpnDBusAdaptor);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_THIRD_PARTY_VPN_DBUS_ADAPTOR_H_

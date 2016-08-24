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

#ifndef SHILL_SUPPLICANT_SUPPLICANT_INTERFACE_PROXY_INTERFACE_H_
#define SHILL_SUPPLICANT_SUPPLICANT_INTERFACE_PROXY_INTERFACE_H_

#include <map>
#include <string>

#include "shill/key_value_store.h"

namespace shill {

// SupplicantInterfaceProxyInterface declares only the subset of
// fi::w1::wpa_supplicant1::Interface_proxy that is actually used by WiFi.
class SupplicantInterfaceProxyInterface {
 public:
  virtual ~SupplicantInterfaceProxyInterface() {}

  virtual bool AddNetwork(const KeyValueStore& args,
                          std::string* network) = 0;
  virtual bool EnableHighBitrates() = 0;
  virtual bool EAPLogoff() = 0;
  virtual bool EAPLogon() = 0;
  virtual bool Disconnect() = 0;
  virtual bool FlushBSS(const uint32_t& age) = 0;
  virtual bool NetworkReply(const std::string& network,
                            const std::string& field,
                            const std::string& value) = 0;
  virtual bool Reassociate() = 0;
  virtual bool Reattach() = 0;
  virtual bool RemoveAllNetworks() = 0;
  virtual bool RemoveNetwork(const std::string& network) = 0;
  virtual bool Roam(const std::string& addr) = 0;
  virtual bool Scan(const KeyValueStore& args) = 0;
  virtual bool SelectNetwork(const std::string& network) = 0;
  virtual bool SetFastReauth(bool enabled) = 0;
  virtual bool SetRoamThreshold(uint16_t seconds) = 0;
  virtual bool SetScanInterval(int seconds) = 0;
  virtual bool SetDisableHighBitrates(bool disable_high_bitrates) = 0;
  virtual bool SetSchedScan(bool enable) = 0;
  virtual bool SetScan(bool enable) = 0;
  virtual bool TDLSDiscover(const std::string& peer) = 0;
  virtual bool TDLSSetup(const std::string& peer) = 0;
  virtual bool TDLSStatus(const std::string& peer,
                                      std::string* status) = 0;
  virtual bool TDLSTeardown(const std::string& peer) = 0;
  virtual bool SetHT40Enable(const std::string& network, bool enable) = 0;
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_SUPPLICANT_INTERFACE_PROXY_INTERFACE_H_

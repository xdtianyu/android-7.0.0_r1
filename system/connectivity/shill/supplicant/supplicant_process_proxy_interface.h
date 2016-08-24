//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_SUPPLICANT_SUPPLICANT_PROCESS_PROXY_INTERFACE_H_
#define SHILL_SUPPLICANT_SUPPLICANT_PROCESS_PROXY_INTERFACE_H_

#include <string>

#include "shill/key_value_store.h"

namespace shill {

// SupplicantProcessProxyInterface declares only the subset of
// fi::w1::wpa_supplicant1_proxy that is actually used by WiFi.
class SupplicantProcessProxyInterface {
 public:
  virtual ~SupplicantProcessProxyInterface() {}
  virtual bool CreateInterface(const KeyValueStore& args,
                               std::string* rpc_identifier) = 0;
  virtual bool GetInterface(const std::string& ifname,
                            std::string* rpc_identifier) = 0;
  virtual bool RemoveInterface(const std::string& rpc_identifier) = 0;
  virtual bool SetDebugLevel(const std::string& level) = 0;
  virtual bool GetDebugLevel(std::string* level) = 0;
  virtual bool ExpectDisconnect() = 0;
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_SUPPLICANT_PROCESS_PROXY_INTERFACE_H_

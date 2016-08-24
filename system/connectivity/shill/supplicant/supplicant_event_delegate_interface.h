//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_SUPPLICANT_SUPPLICANT_EVENT_DELEGATE_INTERFACE_H_
#define SHILL_SUPPLICANT_SUPPLICANT_EVENT_DELEGATE_INTERFACE_H_

#include <string>

namespace shill {

// SupplicantEventDelegateInterface declares the set of methods that
// a SupplicantInterfaceProxy calls on an interested party when
// wpa_supplicant events occur on the network interface interface.
class SupplicantEventDelegateInterface {
 public:
  virtual ~SupplicantEventDelegateInterface() {}

  // Supplicant has added a BSS to its table of visible endpoints.
  virtual void BSSAdded(const std::string& BSS,
                        const KeyValueStore& properties) = 0;

  // Supplicant has removed a BSS from its table of visible endpoints.
  virtual void BSSRemoved(const std::string& BSS) = 0;

  // Supplicant has received a certficate from the remote server during
  // the process of authentication.
  virtual void Certification(const KeyValueStore& properties) = 0;

  // Supplicant state machine has output an EAP event notification.
  virtual void EAPEvent(const std::string& status,
                        const std::string& parameter) = 0;

  // The interface element in the supplicant has changed one or more
  // properties.
  virtual void PropertiesChanged(const KeyValueStore& properties) = 0;

  // A scan has completed on this interface.
  virtual void ScanDone(const bool& success) = 0;

  // A TDLS discovery response received on this interface.
  virtual void TDLSDiscoverResponse(const std::string& peer_address) = 0;
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_SUPPLICANT_EVENT_DELEGATE_INTERFACE_H_

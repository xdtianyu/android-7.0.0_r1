//
// Copyright 2015 The Android Open Source Project
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

#ifndef APMANAGER_SHILL_PROXY_INTERFACE_H_
#define APMANAGER_SHILL_PROXY_INTERFACE_H_

#include <string>

namespace apmanager {

class ShillProxyInterface {
 public:
  virtual ~ShillProxyInterface() {}

  // Claim the given interface |interface_name| from shill.
  virtual bool ClaimInterface(const std::string& interface_name) = 0;
  // Release the given interface |interface_name| to shill.
  virtual bool ReleaseInterface(const std::string& interface_name) = 0;
#if defined(__BRILLO__)
  // Setup an AP mode interface.
  virtual bool SetupApModeInterface(std::string* interface_name) = 0;
  // Setup a station mode interface.
  virtual bool SetupStationModeInterface(std::string* interface_name) = 0;
#endif  // __BRILLO__
};

}  // namespace apmanager

#endif  // APMANAGER_SHILL_PROXY_INTERFACE_H_

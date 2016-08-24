//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef APMANAGER_SHILL_MANAGER_H_
#define APMANAGER_SHILL_MANAGER_H_

#include <set>
#include <string>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include "apmanager/shill_proxy_interface.h"

namespace apmanager {

class ControlInterface;

class ShillManager {
 public:
  ShillManager();
  virtual ~ShillManager();

  void Init(ControlInterface* control_interface);

  // Claim the given interface |interface_name| from shill.
  virtual void ClaimInterface(const std::string& interface_name);
  // Release the given interface |interface_name| to shill.
  virtual void ReleaseInterface(const std::string& interface_name);
#if defined(__BRILLO__)
  // Setup an AP mode interface.
  virtual bool SetupApModeInterface(std::string* interface_name);
  // Setup a station mode interface.
  virtual bool SetupStationModeInterface(std::string* interface_name);
#endif  // __BRILLO__

 private:
  void OnShillServiceAppeared();
  void OnShillServiceVanished();

  std::unique_ptr<ShillProxyInterface> shill_proxy_;
  // List of interfaces apmanager have claimed.
  std::set<std::string> claimed_interfaces_;

  base::WeakPtrFactory<ShillManager> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ShillManager);
};

}  // namespace apmanager

#endif  // APMANAGER_SHILL_MANAGER_H_

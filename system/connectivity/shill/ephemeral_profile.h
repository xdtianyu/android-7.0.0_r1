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

#ifndef SHILL_EPHEMERAL_PROFILE_H_
#define SHILL_EPHEMERAL_PROFILE_H_

#include <string>
#include <vector>

#include "shill/event_dispatcher.h"
#include "shill/profile.h"
#include "shill/property_store.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;
class Manager;
class StoreInterface;

// An in-memory profile that is not persisted to disk, but allows the
// promotion of entries contained herein to the currently active profile.
class EphemeralProfile : public Profile {
 public:
  EphemeralProfile(ControlInterface* control_interface,
                   Metrics* metrics,
                   Manager* manager);
  ~EphemeralProfile() override;

  std::string GetFriendlyName() override;
  bool AdoptService(const ServiceRefPtr& service) override;
  bool AbandonService(const ServiceRefPtr& service) override;

  // Should not be called.
  bool Save() override;

 private:
  static const char kFriendlyName[];

  DISALLOW_COPY_AND_ASSIGN(EphemeralProfile);
};

}  // namespace shill

#endif  // SHILL_EPHEMERAL_PROFILE_H_

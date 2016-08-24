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

#include "shill/ephemeral_profile.h"

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/adaptor_interfaces.h"
#include "shill/control_interface.h"
#include "shill/logging.h"
#include "shill/manager.h"

namespace shill {

using std::string;

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kProfile;
static string ObjectID(EphemeralProfile* e) { return e->GetRpcIdentifier(); }
}

// static
const char EphemeralProfile::kFriendlyName[] = "(ephemeral)";

EphemeralProfile::EphemeralProfile(ControlInterface* control_interface,
                                   Metrics* metrics,
                                   Manager* manager)
    : Profile(control_interface, metrics, manager, Identifier(),
              base::FilePath(), false) {
}

EphemeralProfile::~EphemeralProfile() {}

string EphemeralProfile::GetFriendlyName() {
  return kFriendlyName;
}

bool EphemeralProfile::AdoptService(const ServiceRefPtr& service) {
  SLOG(this, 2) << "Adding service " << service->unique_name()
                << " to ephemeral profile.";
  service->SetProfile(this);
  return true;
}

bool EphemeralProfile::AbandonService(const ServiceRefPtr& service) {
  if (service->profile() == this)
    service->SetProfile(nullptr);
  SLOG(this, 2) << "Removing service " << service->unique_name()
                << " from ephemeral profile.";
  return true;
}

bool EphemeralProfile::Save() {
  NOTREACHED();
  return false;
}

}  // namespace shill

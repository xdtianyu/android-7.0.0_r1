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

#include "shill/power_manager_proxy_stub.h"

namespace shill {

PowerManagerProxyStub::PowerManagerProxyStub() {}

bool PowerManagerProxyStub::RegisterSuspendDelay(
    base::TimeDelta /*timeout*/,
    const std::string& /*description*/,
    int* /*delay_id_out*/) {
  // STUB IMPLEMENTATION.
  return false;
}

bool PowerManagerProxyStub::UnregisterSuspendDelay(int /*delay_id*/) {
  // STUB IMPLEMENTATION.
  return false;
}

bool PowerManagerProxyStub::ReportSuspendReadiness(int /*delay_id*/,
                                                   int /*suspend_id*/) {
  // STUB IMPLEMENTATION.
  return false;
}

bool PowerManagerProxyStub::RegisterDarkSuspendDelay(
    base::TimeDelta /*timeout*/,
    const std::string& /*description*/,
    int* /*delay_id_out*/) {
  // STUB IMPLEMENTATION.
  return false;
}

bool PowerManagerProxyStub::UnregisterDarkSuspendDelay(int /*delay_id*/) {
  // STUB IMPLEMENTATION.
  return false;
}

bool PowerManagerProxyStub::ReportDarkSuspendReadiness(int /*delay_id*/,
                                                       int /*suspend_id*/) {
  // STUB IMPLEMENTATION.
  return false;
}

bool PowerManagerProxyStub::RecordDarkResumeWakeReason(
    const std::string& /*wake_reason*/) {
  // STUB IMPLEMENTATION.
  return false;
}

}  // namespace shill

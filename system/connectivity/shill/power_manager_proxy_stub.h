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

#ifndef SHILL_POWER_MANAGER_PROXY_STUB_H_
#define SHILL_POWER_MANAGER_PROXY_STUB_H_

#include <string>

#include "shill/power_manager_proxy_interface.h"

namespace shill {

// Stub implementation for PowerManagerProxyInterface.
class PowerManagerProxyStub : public PowerManagerProxyInterface {
 public:
  PowerManagerProxyStub();
  ~PowerManagerProxyStub() override = default;

  // Inherited from PowerManagerProxyInterface.
  bool RegisterSuspendDelay(base::TimeDelta timeout,
                            const std::string& description,
                            int* delay_id_out) override;

  bool UnregisterSuspendDelay(int delay_id) override;

  bool ReportSuspendReadiness(int delay_id, int suspend_id) override;

  bool RegisterDarkSuspendDelay(base::TimeDelta timeout,
                                const std::string& description,
                                int* delay_id_out) override;

  bool UnregisterDarkSuspendDelay(int delay_id) override;

  bool ReportDarkSuspendReadiness(int delay_id, int suspend_id) override;

  bool RecordDarkResumeWakeReason(const std::string& wake_reason) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerManagerProxyStub);
};

}  // namespace shill

#endif  // SHILL_POWER_MANAGER_PROXY_STUB_H_

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

#ifndef SHILL_MOCK_POWER_MANAGER_PROXY_H_
#define SHILL_MOCK_POWER_MANAGER_PROXY_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/power_manager_proxy_interface.h"

namespace shill {

class MockPowerManagerProxy : public PowerManagerProxyInterface {
 public:
  MockPowerManagerProxy();
  ~MockPowerManagerProxy() override;

  MOCK_METHOD3(RegisterSuspendDelay,
               bool(base::TimeDelta timeout,
                    const std::string& description,
                    int* delay_id_out));
  MOCK_METHOD1(UnregisterSuspendDelay, bool(int delay_id));
  MOCK_METHOD2(ReportSuspendReadiness, bool(int delay_id, int suspend_id));
  MOCK_METHOD3(RegisterDarkSuspendDelay,
               bool(base::TimeDelta timeout,
                    const std::string& description,
                    int* delay_id_out));
  MOCK_METHOD1(UnregisterDarkSuspendDelay, bool(int delay_id));
  MOCK_METHOD2(ReportDarkSuspendReadiness, bool(int delay_id, int suspend_id));
  MOCK_METHOD1(RecordDarkResumeWakeReason,
               bool(const std::string& wake_reason));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPowerManagerProxy);
};

}  // namespace shill

#endif  // SHILL_MOCK_POWER_MANAGER_PROXY_H_

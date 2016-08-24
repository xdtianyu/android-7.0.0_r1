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

#ifndef SHILL_MOCK_POWER_MANAGER_H_
#define SHILL_MOCK_POWER_MANAGER_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/power_manager.h"

namespace shill {

class ControlInterface;

class MockPowerManager : public PowerManager {
 public:
  MockPowerManager(EventDispatcher* dispatcher,
                   ControlInterface* control_interface);
  ~MockPowerManager() override;

  MOCK_METHOD0(ReportSuspendReadiness, bool());
  MOCK_METHOD0(ReportDarkSuspendReadiness, bool());
  MOCK_METHOD4(
      Start,
      void(base::TimeDelta suspend_delay,
           const PowerManager::SuspendImminentCallback& imminent_callback,
           const PowerManager::SuspendDoneCallback& done_callback,
           const PowerManager::DarkSuspendImminentCallback& dark_imminent));
  MOCK_METHOD0(Stop, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPowerManager);
};

}  // namespace shill

#endif  // SHILL_MOCK_POWER_MANAGER_H_

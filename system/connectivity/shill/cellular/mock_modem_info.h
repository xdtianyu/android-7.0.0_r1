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

#ifndef SHILL_CELLULAR_MOCK_MODEM_INFO_H_
#define SHILL_CELLULAR_MOCK_MODEM_INFO_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/modem_info.h"
#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_pending_activation_store.h"

namespace shill {

class MockModemInfo : public ModemInfo {
 public:
  MockModemInfo();

  // All nullptr parameters are replaced by mock objects.
  MockModemInfo(ControlInterface* control,
                EventDispatcher* dispatcher,
                Metrics* metrics,
                Manager* manager);

  ~MockModemInfo() override;

  // Replaces data members in ModemInfo by mock objects.
  // The following are relaced by mocks if they are nullptr: control_interface,
  // dispatcher, metrics, manager.
  // The following are always replaced by mocks: pending_activation_store.
  void SetMockMembers();

  // Accessors for mock objects
  MockPendingActivationStore* mock_pending_activation_store() const {
    return mock_pending_activation_store_;
  }
  MockControl* mock_control_interface() const {
    return mock_control_.get();
  }
  MockEventDispatcher* mock_dispatcher() const {
    return mock_dispatcher_.get();
  }
  MockMetrics* mock_metrics() const {
    return mock_metrics_.get();
  }
  MockManager* mock_manager() const {
    return mock_manager_.get();
  }

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD1(OnDeviceInfoAvailable, void(const std::string& link_name));

 private:
  std::unique_ptr<MockControl> mock_control_;
  std::unique_ptr<MockEventDispatcher> mock_dispatcher_;
  std::unique_ptr<MockMetrics> mock_metrics_;
  std::unique_ptr<MockManager> mock_manager_;

  // owned by ModemInfo
  MockPendingActivationStore* mock_pending_activation_store_;

  DISALLOW_COPY_AND_ASSIGN(MockModemInfo);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MODEM_INFO_H_

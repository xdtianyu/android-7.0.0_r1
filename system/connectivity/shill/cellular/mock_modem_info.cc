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

#include "shill/cellular/mock_modem_info.h"

namespace shill {

MockModemInfo::MockModemInfo() :
    ModemInfo(nullptr, nullptr, nullptr, nullptr),
    mock_pending_activation_store_(nullptr) {}

MockModemInfo::MockModemInfo(ControlInterface* control,
                             EventDispatcher* dispatcher,
                             Metrics* metrics,
                             Manager* manager)
    : ModemInfo(control, dispatcher, metrics, manager),
      mock_pending_activation_store_(nullptr) {
  SetMockMembers();
}

MockModemInfo::~MockModemInfo() {}

void MockModemInfo::SetMockMembers() {
  // These are always replaced by mocks.
  // Assumes ownership.
  set_pending_activation_store(new MockPendingActivationStore());
  mock_pending_activation_store_ =
      static_cast<MockPendingActivationStore*>(pending_activation_store());
  // These are replaced by mocks only if current unset in ModemInfo.
  if (control_interface() == nullptr) {
    mock_control_.reset(new MockControl());
    set_control_interface(mock_control_.get());
  }
  if (dispatcher() == nullptr) {
    mock_dispatcher_.reset(new MockEventDispatcher());
    set_event_dispatcher(mock_dispatcher_.get());
  }
  if (metrics() == nullptr) {
    mock_metrics_.reset(new MockMetrics(dispatcher()));
    set_metrics(mock_metrics_.get());
  }
  if (manager() == nullptr) {
    mock_manager_.reset(new MockManager(control_interface(), dispatcher(),
                                        metrics()));
    set_manager(mock_manager_.get());
  }
}

}  // namespace shill

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

#include "shill/mock_manager.h"

#include <vector>

#include <gmock/gmock.h>

using std::string;
using std::vector;
using testing::_;
using testing::Invoke;
using testing::Return;

namespace shill {

MockManager::MockManager(ControlInterface* control_interface,
                         EventDispatcher* dispatcher,
                         Metrics* metrics)
    : Manager(control_interface, dispatcher, metrics, "", "", ""),
      mock_device_info_(nullptr) {
  EXPECT_CALL(*this, device_info())
      .WillRepeatedly(Invoke(this, &MockManager::mock_device_info));
  ON_CALL(*this, FilterPrependDNSServersByFamily(_))
      .WillByDefault(Return(vector<string>()));
}

MockManager::~MockManager() {}

}  // namespace shill

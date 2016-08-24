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

#include "shill/mock_service.h"

#include <string>

#include <base/memory/ref_counted.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>

#include "shill/refptr_types.h"
#include "shill/store_interface.h"
#include "shill/technology.h"

using std::string;
using testing::_;
using testing::Return;
using testing::ReturnRef;

namespace shill {

class ControlInterface;
class EventDispatcher;
class Manager;

MockService::MockService(ControlInterface* control_interface,
                         EventDispatcher* dispatcher,
                         Metrics* metrics,
                         Manager* manager)
    : Service(control_interface, dispatcher, metrics, manager,
              Technology::kUnknown) {
  const string& id = unique_name();
  EXPECT_CALL(*this, GetRpcIdentifier()).WillRepeatedly(Return(id));
  EXPECT_CALL(*this, GetStorageIdentifier()).WillRepeatedly(Return(id));
  ON_CALL(*this, IsVisible()).WillByDefault(Return(true));
  ON_CALL(*this, state()).WillByDefault(Return(kStateUnknown));
  ON_CALL(*this, failure()).WillByDefault(Return(kFailureUnknown));
  ON_CALL(*this, technology()).WillByDefault(Return(Technology::kUnknown));
  ON_CALL(*this, connection()).WillByDefault(ReturnRef(mock_connection_));
}

MockService::~MockService() {}

bool MockService::FauxSave(StoreInterface* store) {
  return store->SetString(GetStorageIdentifier(), "dummy", "dummy");
}

}  // namespace shill

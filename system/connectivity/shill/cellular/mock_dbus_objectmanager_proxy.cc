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

#include "shill/cellular/mock_dbus_objectmanager_proxy.h"

#include "shill/testing.h"

using ::testing::_;
using ::testing::AnyNumber;

namespace shill {
MockDBusObjectManagerProxy::MockDBusObjectManagerProxy() {
  ON_CALL(*this, GetManagedObjects(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
}

MockDBusObjectManagerProxy::~MockDBusObjectManagerProxy() {}

void MockDBusObjectManagerProxy::IgnoreSetCallbacks() {
  EXPECT_CALL(*this, set_interfaces_added_callback(_)).Times(AnyNumber());
  EXPECT_CALL(*this, set_interfaces_removed_callback(_)).Times(AnyNumber());
}
}  // namespace shill

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

#include "shill/cellular/mock_mm1_modem_time_proxy.h"

#include "shill/testing.h"

using testing::_;

namespace shill {
namespace mm1 {

MockModemTimeProxy::MockModemTimeProxy() {
  ON_CALL(*this, GetNetworkTime(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
}

MockModemTimeProxy::~MockModemTimeProxy() {}

}  // namespace mm1
}  // namespace shill

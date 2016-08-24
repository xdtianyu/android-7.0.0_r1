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

#include "shill/cellular/mock_mm1_modem_proxy.h"

#include "shill/testing.h"

using testing::_;

namespace shill {
namespace mm1 {

MockModemProxy::MockModemProxy() {
  ON_CALL(*this, Enable(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, CreateBearer(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, DeleteBearer(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, Reset(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
  ON_CALL(*this, FactoryReset(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, SetCurrentCapabilities(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, SetCurrentModes(_, _, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<2>());
  ON_CALL(*this, Command(_, _, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<2>());
  ON_CALL(*this, SetPowerState(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
}

MockModemProxy::~MockModemProxy() {}

}  // namespace mm1
}  // namespace shill

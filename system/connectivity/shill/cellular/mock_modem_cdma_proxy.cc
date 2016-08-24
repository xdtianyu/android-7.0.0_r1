//
// Copyright (C) 2011 The Android Open Source Project
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

#include "shill/cellular/mock_modem_cdma_proxy.h"

#include "shill/testing.h"

using testing::_;

namespace shill {

MockModemCDMAProxy::MockModemCDMAProxy() {
  ON_CALL(*this, Activate(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, GetRegistrationState(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
  ON_CALL(*this, GetSignalQuality(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
}

MockModemCDMAProxy::~MockModemCDMAProxy() {}

}  // namespace shill

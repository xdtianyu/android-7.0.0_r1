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

#include "shill/cellular/mock_modem_gsm_card_proxy.h"

#include "shill/testing.h"

using testing::_;

namespace shill {

MockModemGSMCardProxy::MockModemGSMCardProxy() {
  ON_CALL(*this, GetIMEI(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
  ON_CALL(*this, GetIMSI(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
  ON_CALL(*this, GetSPN(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
  ON_CALL(*this, GetMSISDN(_, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<0>());
  ON_CALL(*this, EnablePIN(_, _, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<2>());
  ON_CALL(*this, SendPIN(_, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<1>());
  ON_CALL(*this, SendPUK(_, _, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<2>());
  ON_CALL(*this, ChangePIN(_, _, _, _, _))
      .WillByDefault(SetOperationFailedInArgumentAndWarn<2>());
}

MockModemGSMCardProxy::~MockModemGSMCardProxy() {}

}  // namespace shill

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

#ifndef SHILL_CELLULAR_MOCK_MODEM_PROXY_H_
#define SHILL_CELLULAR_MOCK_MODEM_PROXY_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/modem_proxy_interface.h"

namespace shill {

class MockModemProxy : public ModemProxyInterface {
 public:
  MockModemProxy();
  ~MockModemProxy() override;

  MOCK_METHOD4(Enable, void(bool enable, Error* error,
                            const ResultCallback& callback, int timeout));
  MOCK_METHOD3(Disconnect, void(Error* error, const ResultCallback& callback,
                                int timeout));
  MOCK_METHOD3(GetModemInfo, void(Error* error,
                                  const ModemInfoCallback& callback,
                                  int timeout));
  MOCK_METHOD1(set_state_changed_callback,
               void(const ModemStateChangedSignalCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockModemProxy);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MODEM_PROXY_H_

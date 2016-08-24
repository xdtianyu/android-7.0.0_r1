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

#ifndef SHILL_CELLULAR_MOCK_MM1_MODEM_SIMPLE_PROXY_H_
#define SHILL_CELLULAR_MOCK_MM1_MODEM_SIMPLE_PROXY_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/mm1_modem_simple_proxy_interface.h"

namespace shill {
namespace mm1 {

class MockModemSimpleProxy : public ModemSimpleProxyInterface {
 public:
  MockModemSimpleProxy();
  ~MockModemSimpleProxy() override;

  MOCK_METHOD4(Connect, void(const KeyValueStore& properties,
                             Error* error,
                             const RpcIdentifierCallback& callback,
                             int timeout));
  MOCK_METHOD4(Disconnect, void(const std::string& bearer,
                                Error* error,
                                const ResultCallback& callback,
                                int timeout));
  MOCK_METHOD3(GetStatus, void(Error* error,
                               const KeyValueStoreCallback& callback,
                               int timeout));
 private:
  DISALLOW_COPY_AND_ASSIGN(MockModemSimpleProxy);
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MM1_MODEM_SIMPLE_PROXY_H_

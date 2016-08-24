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

#ifndef SHILL_CELLULAR_MOCK_MM1_MODEM_MODEM3GPP_PROXY_H_
#define SHILL_CELLULAR_MOCK_MM1_MODEM_MODEM3GPP_PROXY_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/mm1_modem_modem3gpp_proxy_interface.h"

namespace shill {
namespace mm1 {

class MockModemModem3gppProxy : public ModemModem3gppProxyInterface {
 public:
  MockModemModem3gppProxy();
  ~MockModemModem3gppProxy() override;

  MOCK_METHOD4(Register, void(const std::string& operator_id,
                              Error* error,
                              const ResultCallback& callback,
                              int timeout));
  MOCK_METHOD3(Scan, void(Error* error,
                          const KeyValueStoresCallback& callback,
                          int timeout));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockModemModem3gppProxy);
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MM1_MODEM_MODEM3GPP_PROXY_H_

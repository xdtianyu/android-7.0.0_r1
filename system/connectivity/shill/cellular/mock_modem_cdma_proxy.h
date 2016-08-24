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

#ifndef SHILL_CELLULAR_MOCK_MODEM_CDMA_PROXY_H_
#define SHILL_CELLULAR_MOCK_MODEM_CDMA_PROXY_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/modem_cdma_proxy_interface.h"

namespace shill {

class MockModemCDMAProxy : public ModemCDMAProxyInterface {
 public:
  MockModemCDMAProxy();
  ~MockModemCDMAProxy() override;

  MOCK_METHOD4(Activate, void(const std::string& carrier, Error* error,
                              const ActivationResultCallback& callback,
                              int timeout));
  MOCK_METHOD3(GetRegistrationState,
               void(Error* error, const RegistrationStateCallback& callback,
                    int timeout));
  MOCK_METHOD3(GetSignalQuality, void(Error* error,
                                      const SignalQualityCallback& callback,
                                      int timeout));
  MOCK_METHOD0(MEID, const std::string());
  MOCK_METHOD1(set_activation_state_callback,
      void(const ActivationStateSignalCallback& callback));
  MOCK_METHOD1(set_signal_quality_callback,
      void(const SignalQualitySignalCallback& callback));
  MOCK_METHOD1(set_registration_state_callback,
      void(const RegistrationStateSignalCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockModemCDMAProxy);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MODEM_CDMA_PROXY_H_

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

#ifndef SHILL_CELLULAR_MOCK_MM1_SIM_PROXY_H_
#define SHILL_CELLULAR_MOCK_MM1_SIM_PROXY_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/mm1_sim_proxy_interface.h"

namespace shill {
namespace mm1 {

class MockSimProxy : public SimProxyInterface {
 public:
  MockSimProxy();
  ~MockSimProxy() override;

  MOCK_METHOD4(SendPin, void(const std::string& pin,
                             Error* error,
                             const ResultCallback& callback,
                             int timeout));
  MOCK_METHOD5(SendPuk, void(const std::string& puk,
                             const std::string& pin,
                             Error* error,
                             const ResultCallback& callback,
                             int timeout));
  MOCK_METHOD5(EnablePin, void(const std::string& pin,
                               const bool enabled,
                               Error* error,
                               const ResultCallback& callback,
                               int timeout));
  MOCK_METHOD5(ChangePin, void(const std::string& old_pin,
                               const std::string& new_pin,
                               Error* error,
                               const ResultCallback& callback,
                               int timeout));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSimProxy);
};

}  // namespace mm1
}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MM1_SIM_PROXY_H_

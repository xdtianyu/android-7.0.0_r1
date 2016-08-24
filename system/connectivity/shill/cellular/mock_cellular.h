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

#ifndef SHILL_CELLULAR_MOCK_CELLULAR_H_
#define SHILL_CELLULAR_MOCK_CELLULAR_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/cellular/cellular.h"

namespace shill {

class MockCellular : public Cellular {
 public:
  MockCellular(ModemInfo* modem_info,
               const std::string& link_name,
               const std::string& address,
               int interface_index,
               Type type,
               const std::string& service,
               const std::string& path);
  ~MockCellular() override;

  MOCK_METHOD1(Connect, void(Error* error));
  MOCK_METHOD2(Disconnect, void(Error* error, const char* reason));
  MOCK_METHOD3(OnPropertiesChanged, void(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties));
  MOCK_METHOD1(set_modem_state, void(ModemState state));
  MOCK_METHOD0(DestroyService, void());
  MOCK_METHOD1(StartPPP, void(const std::string& serial_device));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockCellular);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_CELLULAR_H_

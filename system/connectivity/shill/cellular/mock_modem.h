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

#ifndef SHILL_CELLULAR_MOCK_MODEM_H_
#define SHILL_CELLULAR_MOCK_MODEM_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/cellular/modem.h"

namespace shill {

class MockModem : public Modem {
 public:
  MockModem(const std::string& service,
            const std::string& path,
            ModemInfo* modem_info,
            ControlInterface* control_interface);
  ~MockModem() override;

  // This class only mocks the pure virtual methods; if you need a
  // more thorough mock, know that modem_unittest.cc depends on the
  // incompleteness of this mock.
  MOCK_METHOD1(SetModemStateFromProperties,
               void(const KeyValueStore& properties));
  MOCK_CONST_METHOD2(GetLinkName,
                     bool(const KeyValueStore& modem_properties,
                          std::string* name));
  MOCK_CONST_METHOD0(GetModemInterface,
                     std::string(void));
  MOCK_METHOD3(ConstructCellular, Cellular*(
      const std::string& link_name,
      const std::string& device_name,
      int ifindex));
};
typedef ::testing::StrictMock<MockModem> StrictModem;

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_MODEM_H_

//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_MOCK_DHCP_PROPERTIES_H_
#define SHILL_MOCK_DHCP_PROPERTIES_H_

#include "shill/dhcp_properties.h"

#include <gmock/gmock.h>

namespace shill {

class MockDhcpProperties : public DhcpProperties {
 public:
  MockDhcpProperties();
  ~MockDhcpProperties() override;

  MOCK_CONST_METHOD2(Save, void(StoreInterface* store, const std::string& id));
  MOCK_METHOD2(Load, void(StoreInterface* store, const std::string& id));
  MOCK_METHOD2(GetValueForProperty,
               bool(std::string& name, std::string* value));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDhcpProperties);
};

}  // namespace shill

#endif  // SHILL_MOCK_DHCP_PROPERTIES_H_


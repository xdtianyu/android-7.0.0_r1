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

#ifndef SHILL_MOCK_DBUS_PROPERTIES_PROXY_H_
#define SHILL_MOCK_DBUS_PROPERTIES_PROXY_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/dbus_properties_proxy_interface.h"

namespace shill {

class MockDBusPropertiesProxy : public DBusPropertiesProxyInterface {
 public:
  MockDBusPropertiesProxy();
  ~MockDBusPropertiesProxy() override;

  MOCK_METHOD1(GetAll, KeyValueStore(const std::string& interface_name));
  MOCK_METHOD2(Get, brillo::Any(const std::string& interface_name,
                                const std::string& property));
  MOCK_METHOD1(set_properties_changed_callback,
               void(const PropertiesChangedCallback& callback));
  MOCK_METHOD1(set_modem_manager_properties_changed_callback,
               void(const ModemManagerPropertiesChangedCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDBusPropertiesProxy);
};

}  // namespace shill

#endif  // SHILL_MOCK_DBUS_PROPERTIES_PROXY_H_

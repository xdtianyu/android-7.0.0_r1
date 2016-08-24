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

#ifndef SHILL_DBUS_PROPERTIES_PROXY_INTERFACE_H_
#define SHILL_DBUS_PROPERTIES_PROXY_INTERFACE_H_

#include <string>
#include <vector>

#include <base/callback.h>

#include "shill/key_value_store.h"

namespace shill {

// This is a cellular-specific DBus Properties interface, as it supports
// cellular-specific signal (ModemManagerPropertiesChanged).
// These are the methods that a DBusProperties proxy must support. The interface
// is provided so that it can be mocked in tests.
class DBusPropertiesProxyInterface {
 public:
  // Callback invoked when an object sends a DBus property change signal.
  typedef base::Callback<void(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties)>
    PropertiesChangedCallback;

  // Callback invoked when the classic modem manager sends a DBus
  // property change signal.
  typedef base::Callback<void(
      const std::string& interface,
      const KeyValueStore& properties)>
    ModemManagerPropertiesChangedCallback;

  virtual ~DBusPropertiesProxyInterface() {}

  virtual KeyValueStore GetAll(const std::string& interface_name) = 0;
  virtual brillo::Any Get(const std::string& interface_name,
                          const std::string& property) = 0;

  virtual void set_properties_changed_callback(
      const PropertiesChangedCallback& callback) = 0;
  virtual void set_modem_manager_properties_changed_callback(
      const ModemManagerPropertiesChangedCallback& callback) = 0;
};

}  // namespace shill

#endif  // SHILL_DBUS_PROPERTIES_PROXY_INTERFACE_H_

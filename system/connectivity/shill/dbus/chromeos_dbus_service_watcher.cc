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

#include "shill/dbus/chromeos_dbus_service_watcher.h"

namespace shill {

ChromeosDBusServiceWatcher::ChromeosDBusServiceWatcher(
      scoped_refptr<dbus::Bus> bus,
      const std::string& connection_name,
      const base::Closure& on_connection_vanished)
    : watcher_(
        new brillo::dbus_utils::DBusServiceWatcher(
            bus, connection_name, on_connection_vanished)) {}

ChromeosDBusServiceWatcher::~ChromeosDBusServiceWatcher() {}

}  // namespace shill

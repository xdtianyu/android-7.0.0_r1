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

#ifndef SHILL_DBUS_CHROMEOS_DBUS_SERVICE_WATCHER_H_
#define SHILL_DBUS_CHROMEOS_DBUS_SERVICE_WATCHER_H_

#include <string>

#include <brillo/dbus/dbus_service_watcher.h>

namespace shill {

// Wrapper for brillo::dbus::DBusServiceWatcher for monitoring remote
// DBus service.
class ChromeosDBusServiceWatcher {
 public:
  ChromeosDBusServiceWatcher(
      scoped_refptr<dbus::Bus> bus,
      const std::string& connection_name,
      const base::Closure& on_connection_vanished);
  ~ChromeosDBusServiceWatcher();

 protected:
  ChromeosDBusServiceWatcher() {}  // for mocking.

 private:
  std::unique_ptr<brillo::dbus_utils::DBusServiceWatcher> watcher_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosDBusServiceWatcher);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_DBUS_SERVICE_WATCHER_H_

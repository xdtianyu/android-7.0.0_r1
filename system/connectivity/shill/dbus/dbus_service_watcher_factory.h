//
// Copyright (C) 2016 The Android Open Source Project
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

#ifndef SHILL_DBUS_DBUS_SERVICE_WATCHER_FACTORY_H_
#define SHILL_DBUS_DBUS_SERVICE_WATCHER_FACTORY_H_

#include <memory>
#include <string>

#include <base/callback.h>
#include <base/lazy_instance.h>
#include <dbus/bus.h>

namespace shill {

class ChromeosDBusServiceWatcher;

class DBusServiceWatcherFactory {
 public:
  virtual ~DBusServiceWatcherFactory();

  // This is a singleton. Use DBusServiceWatcherFactory::GetInstance()->Foo().
  static DBusServiceWatcherFactory* GetInstance();

  virtual ChromeosDBusServiceWatcher* CreateDBusServiceWatcher(
      scoped_refptr<dbus::Bus> bus, const std::string& connection_name,
      const base::Closure& on_connection_vanish);

 protected:
  DBusServiceWatcherFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<DBusServiceWatcherFactory>;

  DISALLOW_COPY_AND_ASSIGN(DBusServiceWatcherFactory);
};

}  // namespace shill

#endif  // SHILL_DBUS_DBUS_SERVICE_WATCHER_FACTORY_H_

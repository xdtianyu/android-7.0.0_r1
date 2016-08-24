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

#ifndef SHILL_DBUS_MOCK_DBUS_SERVICE_WATCHER_FACTORY_H_
#define SHILL_DBUS_MOCK_DBUS_SERVICE_WATCHER_FACTORY_H_

#include <gmock/gmock.h>

#include "shill/dbus/dbus_service_watcher_factory.h"

namespace shill {

class MockDBusServiceWatcherFactory : public DBusServiceWatcherFactory {
 public:
  MockDBusServiceWatcherFactory() {}
  virtual ~MockDBusServiceWatcherFactory() {}

  MOCK_METHOD3(
      CreateDBusServiceWatcher,
      ChromeosDBusServiceWatcher*(scoped_refptr<dbus::Bus> bus,
                                  const std::string& connection_name,
                                  const base::Closure& on_connection_vanish));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDBusServiceWatcherFactory);
};

}  // namespace shill

#endif  // SHILL_DBUS_MOCK_DBUS_SERVICE_WATCHER_FACTORY_H_

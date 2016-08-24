//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_MOCK_DBUS_MANAGER_H_
#define SHILL_MOCK_DBUS_MANAGER_H_

#include "shill/dbus_manager.h"

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

namespace shill {

class MockDBusManager : public DBusManager {
 public:
  MockDBusManager();
  ~MockDBusManager() override;

  MOCK_METHOD3(CreateNameWatcher,
      DBusNameWatcher* (
          const std::string& name,
          const DBusNameWatcher::NameAppearedCallback& name_appeared_callback,
          const DBusNameWatcher::NameVanishedCallback& name_vanished_callback));
  MOCK_METHOD1(RemoveNameWatcher, void(DBusNameWatcher* name_watcher));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDBusManager);
};

}  // namespace shill

#endif  // SHILL_MOCK_DBUS_MANAGER_H_

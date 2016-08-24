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

#ifndef SHILL_CELLULAR_MOCK_DBUS_OBJECTMANAGER_PROXY_H_
#define SHILL_CELLULAR_MOCK_DBUS_OBJECTMANAGER_PROXY_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/cellular/dbus_objectmanager_proxy_interface.h"

namespace shill {

class MockDBusObjectManagerProxy : public DBusObjectManagerProxyInterface {
 public:
  MockDBusObjectManagerProxy();
  ~MockDBusObjectManagerProxy() override;

  MOCK_METHOD3(GetManagedObjects, void(Error* error,
                                       const ManagedObjectsCallback& callback,
                                       int timeout));
  MOCK_METHOD1(set_interfaces_added_callback,
      void(const InterfacesAddedSignalCallback& callback));
  MOCK_METHOD1(set_interfaces_removed_callback,
      void(const InterfacesRemovedSignalCallback& callback));
  void IgnoreSetCallbacks();

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDBusObjectManagerProxy);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOCK_DBUS_OBJECTMANAGER_PROXY_H_

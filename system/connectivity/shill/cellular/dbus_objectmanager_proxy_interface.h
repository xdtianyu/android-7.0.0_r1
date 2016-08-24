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

#ifndef SHILL_CELLULAR_DBUS_OBJECTMANAGER_PROXY_INTERFACE_H_
#define SHILL_CELLULAR_DBUS_OBJECTMANAGER_PROXY_INTERFACE_H_

#include <map>
#include <string>
#include <vector>

#include <base/callback.h>

#include "shill/key_value_store.h"

namespace shill {

class Error;

typedef std::map<std::string, KeyValueStore> InterfaceToProperties;
typedef std::map<std::string, InterfaceToProperties>
    ObjectsWithProperties;
typedef base::Callback<void(const ObjectsWithProperties&, const Error&)>
    ManagedObjectsCallback;
typedef base::Callback<void(const InterfaceToProperties&, const Error&)>
    InterfaceAndPropertiesCallback;
typedef base::Callback<void(const std::string&,
                            const InterfaceToProperties&)>
    InterfacesAddedSignalCallback;
typedef base::Callback<void(const std::string&,
                            const std::vector<std::string>&)>
    InterfacesRemovedSignalCallback;

// These are the methods that a org.freedesktop.DBus.ObjectManager
// proxy must support.  The interface is provided so that it can be
// mocked in tests.  All calls are made asynchronously. Call completion
// is signalled via the callbacks passed to the methods.
class DBusObjectManagerProxyInterface {
 public:
  virtual ~DBusObjectManagerProxyInterface() {}
  virtual void GetManagedObjects(Error* error,
                                 const ManagedObjectsCallback& callback,
                                 int timeout) = 0;
  virtual void set_interfaces_added_callback(
      const InterfacesAddedSignalCallback& callback) = 0;
  virtual void set_interfaces_removed_callback(
      const InterfacesRemovedSignalCallback& callback) = 0;
};

}  // namespace shill

#endif  // SHILL_CELLULAR_DBUS_OBJECTMANAGER_PROXY_INTERFACE_H_

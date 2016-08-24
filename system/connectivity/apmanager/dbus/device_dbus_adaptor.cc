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

#include "apmanager/dbus/device_dbus_adaptor.h"

#include <base/strings/stringprintf.h>
#include <dbus_bindings/org.chromium.apmanager.Manager.h>

#include "apmanager/device.h"

using brillo::dbus_utils::ExportedObjectManager;
using brillo::dbus_utils::DBusObject;
using org::chromium::apmanager::ManagerAdaptor;
using std::string;

namespace apmanager {

DeviceDBusAdaptor::DeviceDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    ExportedObjectManager* object_manager,
    Device* device)
    : adaptor_(this),
      object_path_(
          base::StringPrintf("%s/devices/%d",
                             ManagerAdaptor::GetObjectPath().value().c_str(),
                             device->identifier())),
      dbus_object_(object_manager, bus, object_path_) {
  // Register D-Bus object.
  adaptor_.RegisterWithDBusObject(&dbus_object_);
  dbus_object_.RegisterAndBlock();
}

DeviceDBusAdaptor::~DeviceDBusAdaptor() {}

void DeviceDBusAdaptor::SetDeviceName(const string& device_name) {
  adaptor_.SetDeviceName(device_name);
}

string DeviceDBusAdaptor::GetDeviceName() {
  return adaptor_.GetDeviceName();
}

void DeviceDBusAdaptor::SetPreferredApInterface(const string& interface_name) {
  adaptor_.SetPreferredApInterface(interface_name);
}

string DeviceDBusAdaptor::GetPreferredApInterface() {
  return adaptor_.GetPreferredApInterface();
}

void DeviceDBusAdaptor::SetInUse(bool in_use) {
  adaptor_.SetInUse(in_use);
}

bool DeviceDBusAdaptor::GetInUse() {
  return adaptor_.GetInUse();
}

}  // namespace apmanager

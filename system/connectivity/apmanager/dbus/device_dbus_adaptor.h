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

#ifndef APMANAGER_DEVICE_DBUS_ADAPTOR_H_
#define APMANAGER_DEVICE_DBUS_ADAPTOR_H_

#include <base/macros.h>

#include <dbus_bindings/org.chromium.apmanager.Device.h>

#include "apmanager/device_adaptor_interface.h"

namespace apmanager {

class Device;

class DeviceDBusAdaptor : public org::chromium::apmanager::DeviceInterface,
                          public DeviceAdaptorInterface {
 public:
  DeviceDBusAdaptor(const scoped_refptr<dbus::Bus>& bus,
                    brillo::dbus_utils::ExportedObjectManager* object_manager,
                    Device* device);
  ~DeviceDBusAdaptor() override;

  // Implementation of DeviceAdaptorInterface.
  void SetDeviceName(const std::string& device_name) override;
  std::string GetDeviceName() override;
  void SetInUse(bool in_use) override;
  bool GetInUse() override;
  void SetPreferredApInterface(const std::string& interface_name) override;
  std::string GetPreferredApInterface() override;

 private:
  org::chromium::apmanager::DeviceAdaptor adaptor_;
  dbus::ObjectPath object_path_;
  brillo::dbus_utils::DBusObject dbus_object_;

  DISALLOW_COPY_AND_ASSIGN(DeviceDBusAdaptor);
};

}  // namespace apmanager

#endif  // APMANAGER_DEVICE_DBUS_ADAPTOR_H_

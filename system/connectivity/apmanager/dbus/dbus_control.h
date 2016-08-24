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

#ifndef APMANAGER_DBUS_DBUS_CONTROL_H_
#define APMANAGER_DBUS_DBUS_CONTROL_H_

#include <base/macros.h>
#include <brillo/dbus/exported_object_manager.h>
#include <dbus/bus.h>

#include "apmanager/control_interface.h"
#include "apmanager/manager.h"

namespace apmanager {

// D-Bus control interface for IPC through D-Bus.
class DBusControl : public ControlInterface {
 public:
  DBusControl();
  ~DBusControl() override;

  // Inheritted from ControlInterface.
  void Init() override;
  void Shutdown() override;
  std::unique_ptr<ConfigAdaptorInterface> CreateConfigAdaptor(
      Config* config, int service_identifier) override;
  std::unique_ptr<DeviceAdaptorInterface> CreateDeviceAdaptor(
      Device* device) override;
  std::unique_ptr<ManagerAdaptorInterface> CreateManagerAdaptor(
      Manager* manager) override;
  std::unique_ptr<ServiceAdaptorInterface> CreateServiceAdaptor(
      Service* device) override;
  std::unique_ptr<FirewallProxyInterface> CreateFirewallProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) override;
  std::unique_ptr<ShillProxyInterface> CreateShillProxy(
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) override;

 private:
  // Invoked when D-Bus objects for both ObjectManager and Manager
  // are registered to the bus.
  void OnObjectRegistrationCompleted(bool registration_success);

  // NOTE: No dedicated bus is needed for the proxies, since the proxies
  // being created here doesn't listen for any broadcast signals.
  // Use a dedicated bus for the proxies if this condition is not true
  // anymore.
  scoped_refptr<dbus::Bus> bus_;
  std::unique_ptr<brillo::dbus_utils::ExportedObjectManager> object_manager_;
  std::unique_ptr<Manager> manager_;

  DISALLOW_COPY_AND_ASSIGN(DBusControl);
};

}  // namespace apmanager

#endif  // APMANAGER_DBUS_DBUS_CONTROL_H_

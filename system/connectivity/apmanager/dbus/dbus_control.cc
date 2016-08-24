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

#include "apmanager/dbus/dbus_control.h"

#include "apmanager/dbus/config_dbus_adaptor.h"
#include "apmanager/dbus/device_dbus_adaptor.h"
#include "apmanager/dbus/manager_dbus_adaptor.h"
#include "apmanager/dbus/service_dbus_adaptor.h"
#include "apmanager/dbus/shill_dbus_proxy.h"
#include "apmanager/manager.h"

#if !defined(__ANDROID__)
#include "apmanager/dbus/permission_broker_dbus_proxy.h"
#else
#include "apmanager/dbus/firewalld_dbus_proxy.h"
#endif  //__ANDROID__

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;

namespace apmanager {

namespace {
const char kServiceName[] = "org.chromium.apmanager";
const char kServicePath[] = "/org/chromium/apmanager";
}  // namespace

DBusControl::DBusControl() {}

DBusControl::~DBusControl() {}

void DBusControl::Init() {
  // Setup bus connection.
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  bus_ = new dbus::Bus(options);
  CHECK(bus_->Connect());

  // Create and register ObjectManager.
  scoped_refptr<AsyncEventSequencer> sequencer(new AsyncEventSequencer());
  object_manager_.reset(
      new ExportedObjectManager(bus_, dbus::ObjectPath(kServicePath)));
  object_manager_->RegisterAsync(
      sequencer->GetHandler("ObjectManager.RegisterAsync() failed.", true));

  // Create and register Manager.
  manager_.reset(new Manager(this));
  manager_->RegisterAsync(
      sequencer->GetHandler("Manager.RegisterAsync() failed.", true));

  // Take over the service ownership once the objects registration is completed.
  sequencer->OnAllTasksCompletedCall({
    base::Bind(&DBusControl::OnObjectRegistrationCompleted,
               base::Unretained(this))
  });
}

void DBusControl::Shutdown() {
  manager_.reset();
  object_manager_.reset();
  if (bus_) {
    bus_->ShutdownAndBlock();
  }
}

void DBusControl::OnObjectRegistrationCompleted(bool registration_success) {
  // Success should always be true since we've said that failures are fatal.
  CHECK(registration_success) << "Init of one or more objects has failed.";
  CHECK(bus_->RequestOwnershipAndBlock(kServiceName,
                                       dbus::Bus::REQUIRE_PRIMARY))
      << "Unable to take ownership of " << kServiceName;

  // D-Bus service is ready, now we can start the Manager.
  manager_->Start();
}

std::unique_ptr<ConfigAdaptorInterface> DBusControl::CreateConfigAdaptor(
    Config* config, int service_identifier) {
  return std::unique_ptr<ConfigAdaptorInterface>(
      new ConfigDBusAdaptor(
          bus_, object_manager_.get(), config, service_identifier));
}

std::unique_ptr<DeviceAdaptorInterface> DBusControl::CreateDeviceAdaptor(
    Device* device) {
  return std::unique_ptr<DeviceAdaptorInterface>(
      new DeviceDBusAdaptor(bus_, object_manager_.get(), device));
}

std::unique_ptr<ManagerAdaptorInterface> DBusControl::CreateManagerAdaptor(
    Manager* manager) {
  return std::unique_ptr<ManagerAdaptorInterface>(
      new ManagerDBusAdaptor(bus_, object_manager_.get(), manager));
}

std::unique_ptr<ServiceAdaptorInterface> DBusControl::CreateServiceAdaptor(
    Service* service) {
  return std::unique_ptr<ServiceAdaptorInterface>(
      new ServiceDBusAdaptor(bus_, object_manager_.get(), service));
}

std::unique_ptr<FirewallProxyInterface> DBusControl::CreateFirewallProxy(
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback) {
#if !defined(__ANDROID__)
  return std::unique_ptr<FirewallProxyInterface>(
      new PermissionBrokerDBusProxy(
          bus_, service_appeared_callback, service_vanished_callback));
#else
  return std::unique_ptr<FirewallProxyInterface>(
      new FirewalldDBusProxy(
          bus_, service_appeared_callback, service_vanished_callback));
#endif  // __ANDROID__
}

std::unique_ptr<ShillProxyInterface> DBusControl::CreateShillProxy(
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback) {
  return std::unique_ptr<ShillProxyInterface>(
      new ShillDBusProxy(
          bus_, service_appeared_callback, service_vanished_callback));
}

}  // namespace apmanager

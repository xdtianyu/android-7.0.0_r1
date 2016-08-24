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

#include "shill/dbus/chromeos_dbus_objectmanager_proxy.h"

#include <memory>

#include "shill/cellular/cellular_error.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}

ChromeosDBusObjectManagerProxy::ChromeosDBusObjectManagerProxy(
    EventDispatcher* dispatcher,
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& path,
    const std::string& service,
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback)
    : proxy_(
        new org::freedesktop::DBus::ObjectManagerProxy(
            bus, service, dbus::ObjectPath(path))),
      dispatcher_(dispatcher),
      service_appeared_callback_(service_appeared_callback),
      service_vanished_callback_(service_vanished_callback),
      service_available_(false) {
  // Register signal handlers.
  proxy_->RegisterInterfacesAddedSignalHandler(
      base::Bind(&ChromeosDBusObjectManagerProxy::InterfacesAdded,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosDBusObjectManagerProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));
  proxy_->RegisterInterfacesRemovedSignalHandler(
      base::Bind(&ChromeosDBusObjectManagerProxy::InterfacesRemoved,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosDBusObjectManagerProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));

  // Monitor service owner changes. This callback lives for the lifetime of
  // the ObjectProxy.
  proxy_->GetObjectProxy()->SetNameOwnerChangedCallback(
      base::Bind(&ChromeosDBusObjectManagerProxy::OnServiceOwnerChanged,
                 weak_factory_.GetWeakPtr()));

  // One time callback when service becomes available.
  proxy_->GetObjectProxy()->WaitForServiceToBeAvailable(
      base::Bind(&ChromeosDBusObjectManagerProxy::OnServiceAvailable,
                 weak_factory_.GetWeakPtr()));
}

ChromeosDBusObjectManagerProxy::~ChromeosDBusObjectManagerProxy() {}

void ChromeosDBusObjectManagerProxy::GetManagedObjects(
    Error* error,
    const ManagedObjectsCallback& callback,
    int timeout) {
  if (!service_available_) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInternalError,
                          "Service not available");
    return;
  }
  proxy_->GetManagedObjectsAsync(
      base::Bind(&ChromeosDBusObjectManagerProxy::OnGetManagedObjectsSuccess,
                 weak_factory_.GetWeakPtr(),
                 callback),
      base::Bind(&ChromeosDBusObjectManagerProxy::OnGetManagedObjectsFailure,
                 weak_factory_.GetWeakPtr(),
                 callback));
}

void ChromeosDBusObjectManagerProxy::OnServiceAvailable(bool available) {
  LOG(INFO) << __func__ << ": " << available;

  // The callback might invoke calls to the ObjectProxy, so defer the callback
  // to event loop.
  if (available && !service_appeared_callback_.is_null()) {
    dispatcher_->PostTask(service_appeared_callback_);
  } else if (!available && !service_vanished_callback_.is_null()) {
    dispatcher_->PostTask(service_vanished_callback_);
  }
  service_available_ = available;
}

void ChromeosDBusObjectManagerProxy::OnServiceOwnerChanged(
    const string& old_owner, const string& new_owner) {
  LOG(INFO) << __func__ << " old: " << old_owner << " new: " << new_owner;
  if (new_owner.empty()) {
    OnServiceAvailable(false);
  } else {
    OnServiceAvailable(true);
  }
}

void ChromeosDBusObjectManagerProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name
             << " signal: " << signal_name << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

void ChromeosDBusObjectManagerProxy::InterfacesAdded(
    const dbus::ObjectPath& object_path,
    const DBusInterfaceToProperties& dbus_interface_to_properties) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "("
      << object_path.value() << ")";
  InterfaceToProperties interface_to_properties;
  ConvertDBusInterfaceProperties(dbus_interface_to_properties,
                                 &interface_to_properties);
  interfaces_added_callback_.Run(object_path.value(), interface_to_properties);
}

void ChromeosDBusObjectManagerProxy::InterfacesRemoved(
    const dbus::ObjectPath& object_path,
    const std::vector<std::string>& interfaces) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__ << "("
      << object_path.value() << ")";
  interfaces_removed_callback_.Run(object_path.value(), interfaces);
}

void ChromeosDBusObjectManagerProxy::OnGetManagedObjectsSuccess(
    const ManagedObjectsCallback& callback,
    const DBusObjectsWithProperties& dbus_objects_with_properties) {
  SLOG(&proxy_->GetObjectPath(), 2) << __func__;
  ObjectsWithProperties objects_with_properties;
  for (const auto& object : dbus_objects_with_properties) {
    InterfaceToProperties interface_to_properties;
    ConvertDBusInterfaceProperties(object.second, &interface_to_properties);
    objects_with_properties.emplace(object.first.value(),
                                    interface_to_properties);
  }
  callback.Run(objects_with_properties, Error());
}

void ChromeosDBusObjectManagerProxy::OnGetManagedObjectsFailure(
    const ManagedObjectsCallback& callback,
    brillo::Error* dbus_error) {
  Error error;
  CellularError::FromChromeosDBusError(dbus_error, &error);
  callback.Run(ObjectsWithProperties(), error);
}

void ChromeosDBusObjectManagerProxy::ConvertDBusInterfaceProperties(
    const DBusInterfaceToProperties& dbus_interface_to_properties,
    InterfaceToProperties* interface_to_properties) {
  for (const auto& interface : dbus_interface_to_properties) {
    KeyValueStore properties;
    KeyValueStore::ConvertFromVariantDictionary(interface.second, &properties);
    interface_to_properties->emplace(interface.first, properties);
  }
}

}  // namespace shill

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

#include "apmanager/dbus/manager_dbus_adaptor.h"

#if !defined(__ANDROID__)
#include <chromeos/dbus/service_constants.h>
#else
#include <dbus/apmanager/dbus-constants.h>
#endif  // __ANDROID__

#include "apmanager/manager.h"

using brillo::dbus_utils::ExportedObjectManager;
using org::chromium::apmanager::ManagerAdaptor;
using std::string;

namespace apmanager {

ManagerDBusAdaptor::ManagerDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    ExportedObjectManager* object_manager,
    Manager* manager)
    : adaptor_(this),
      dbus_object_(object_manager, bus, ManagerAdaptor::GetObjectPath()),
      bus_(bus),
      manager_(manager) {}

ManagerDBusAdaptor::~ManagerDBusAdaptor() {}

void ManagerDBusAdaptor::RegisterAsync(
    const base::Callback<void(bool)>& completion_callback) {
  adaptor_.RegisterWithDBusObject(&dbus_object_);
  dbus_object_.RegisterAsync(completion_callback);
}

bool ManagerDBusAdaptor::CreateService(brillo::ErrorPtr* dbus_error,
                                       dbus::Message* message,
                                       dbus::ObjectPath* out_service) {
  auto service = manager_->CreateService();
  if (!service) {
    brillo::Error::AddTo(dbus_error,
                         FROM_HERE,
                         brillo::errors::dbus::kDomain,
                         kErrorInternalError,
                         "Failed to create new service");
    return false;
  }

  *out_service = service->adaptor()->GetRpcObjectIdentifier();

  // Setup monitoring for the service's remote owner.
  service_owner_watchers_[*out_service] =
      ServiceOwnerWatcherContext(
          service,
          std::unique_ptr<DBusServiceWatcher>(
              new DBusServiceWatcher(
                  bus_,
                  message->GetSender(),
                  base::Bind(&ManagerDBusAdaptor::OnServiceOwnerVanished,
                             base::Unretained(this),
                             *out_service))));
  return true;
}

bool ManagerDBusAdaptor::RemoveService(brillo::ErrorPtr* dbus_error,
                                       dbus::Message* message,
                                       const dbus::ObjectPath& in_service) {
  auto watcher_context = service_owner_watchers_.find(in_service);
  if (watcher_context == service_owner_watchers_.end()) {
    brillo::Error::AddToPrintf(
        dbus_error,
        FROM_HERE,
        brillo::errors::dbus::kDomain,
        kErrorInvalidArguments,
        "Service %s not found",
        in_service.value().c_str());
    return false;
  }

  Error error;
  manager_->RemoveService(watcher_context->second.service, &error);
  service_owner_watchers_.erase(watcher_context);
  return !error.ToDBusError(dbus_error);
}

void ManagerDBusAdaptor::OnServiceOwnerVanished(
    const dbus::ObjectPath& service_path) {
  LOG(INFO) << "Owner for service " << service_path.value() << " vanished";
  // Remove service watcher.
  auto watcher_context = service_owner_watchers_.find(service_path);
  CHECK(watcher_context != service_owner_watchers_.end())
      << "Owner vanished without watcher setup.";

  // Tell Manager to remove this service.
  manager_->RemoveService(watcher_context->second.service, nullptr);
  service_owner_watchers_.erase(watcher_context);
}

}  // namespace apmanager

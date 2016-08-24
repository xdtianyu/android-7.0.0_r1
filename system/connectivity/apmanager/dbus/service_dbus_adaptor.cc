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

#include "apmanager/dbus/service_dbus_adaptor.h"

#include <base/bind.h>
#include <base/strings/stringprintf.h>
#include <dbus_bindings/org.chromium.apmanager.Manager.h>

#include "apmanager/service.h"

using brillo::dbus_utils::ExportedObjectManager;
using brillo::dbus_utils::DBusMethodResponse;
using brillo::dbus_utils::DBusObject;
using org::chromium::apmanager::ManagerAdaptor;
using std::string;

namespace apmanager {

ServiceDBusAdaptor::ServiceDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    ExportedObjectManager* object_manager,
    Service* service)
    : adaptor_(this),
      object_path_(
          base::StringPrintf("%s/services/%d",
                             ManagerAdaptor::GetObjectPath().value().c_str(),
                             service->identifier())),
      dbus_object_(object_manager, bus, object_path_),
      service_(service) {
  // Need to initialize Config property with a valid path before registering
  // to the bus.
  SetConfig(service_->config());
  // Register D-Bus object.
  adaptor_.RegisterWithDBusObject(&dbus_object_);
  dbus_object_.RegisterAndBlock();
}

ServiceDBusAdaptor::~ServiceDBusAdaptor() {}

void ServiceDBusAdaptor::Start(
    std::unique_ptr<DBusMethodResponse<>> response) {
  service_->Start(
      base::Bind(&ServiceDBusAdaptor::OnStartCompleted,
                 base::Unretained(this),
                 base::Passed(&response)));
}

bool ServiceDBusAdaptor::Stop(brillo::ErrorPtr* dbus_error) {
  Error error;
  service_->Stop(&error);
  return !error.ToDBusError(dbus_error);
}

RPCObjectIdentifier ServiceDBusAdaptor::GetRpcObjectIdentifier() {
  return object_path_;
}

void ServiceDBusAdaptor::SetConfig(Config* config) {
  adaptor_.SetConfig(config->adaptor()->GetRpcObjectIdentifier());
}

void ServiceDBusAdaptor::SetState(const string& state) {
  adaptor_.SetState(state);
}

void ServiceDBusAdaptor::OnStartCompleted(
    std::unique_ptr<DBusMethodResponse<>> response, const Error& error) {
  brillo::ErrorPtr dbus_error;
  if (error.ToDBusError(&dbus_error)) {
    response->ReplyWithError(dbus_error.get());
  } else {
    response->Return();
  }
}

}  // namespace apmanager

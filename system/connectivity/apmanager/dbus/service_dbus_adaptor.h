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

#ifndef APMANAGER_DBUS_SERVICE_DBUS_ADAPTOR_H_
#define APMANAGER_DBUS_SERVICE_DBUS_ADAPTOR_H_

#include <base/macros.h>

#include <dbus_bindings/org.chromium.apmanager.Service.h>

#include "apmanager/error.h"
#include "apmanager/service_adaptor_interface.h"

namespace apmanager {

class Service;

class ServiceDBusAdaptor : public org::chromium::apmanager::ServiceInterface,
                           public ServiceAdaptorInterface {
 public:
  ServiceDBusAdaptor(const scoped_refptr<dbus::Bus>& bus,
                     brillo::dbus_utils::ExportedObjectManager* object_manager,
                     Service* service);
  ~ServiceDBusAdaptor() override;

  // Implementation of org::chromium::apmanager::ServiceInterface
  void Start(
      std::unique_ptr<brillo::dbus_utils::DBusMethodResponse<>>
          response) override;
  bool Stop(brillo::ErrorPtr* dbus_error) override;

  // Implementation of ServiceAdaptorInterface.
  RPCObjectIdentifier GetRpcObjectIdentifier() override;
  void SetConfig(Config* config) override;
  void SetState(const std::string& state) override;

 private:
  void OnStartCompleted(
      std::unique_ptr<brillo::dbus_utils::DBusMethodResponse<>> response,
      const Error& error);

  org::chromium::apmanager::ServiceAdaptor adaptor_;
  dbus::ObjectPath object_path_;
  brillo::dbus_utils::DBusObject dbus_object_;
  Service* service_;

  DISALLOW_COPY_AND_ASSIGN(ServiceDBusAdaptor);
};

}  // namespace apmanager

#endif  // APMANAGER_DBUS_SERVICE_DBUS_ADAPTOR_H_

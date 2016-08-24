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

#ifndef SHILL_DBUS_CHROMEOS_SERVICE_DBUS_ADAPTOR_H_
#define SHILL_DBUS_CHROMEOS_SERVICE_DBUS_ADAPTOR_H_

#include <map>
#include <string>
#include <vector>

#include <base/macros.h>

#include "dbus_bindings/org.chromium.flimflam.Service.h"
#include "shill/adaptor_interfaces.h"
#include "shill/dbus/chromeos_dbus_adaptor.h"

namespace shill {

class Service;

// Subclass of DBusAdaptor for Service objects
// There is a 1:1 mapping between Service and ChromeosServiceDBusAdaptor
// instances.  Furthermore, the Service owns the ChromeosServiceDBusAdaptor
// and manages its lifetime, so we're OK with ChromeosServiceDBusAdaptor
// having a bare pointer to its owner service.
class ChromeosServiceDBusAdaptor
    : public org::chromium::flimflam::ServiceAdaptor,
      public org::chromium::flimflam::ServiceInterface,
      public ChromeosDBusAdaptor,
      public ServiceAdaptorInterface {
 public:
  static const char kPath[];

  ChromeosServiceDBusAdaptor(const scoped_refptr<dbus::Bus>& bus,
                             Service* service);
  ~ChromeosServiceDBusAdaptor() override;

  // Implementation of ServiceAdaptorInterface.
  const std::string& GetRpcIdentifier() override { return dbus_path().value(); }
  void EmitBoolChanged(const std::string& name, bool value) override;
  void EmitUint8Changed(const std::string& name, uint8_t value) override;
  void EmitUint16Changed(const std::string& name, uint16_t value) override;
  void EmitUint16sChanged(const std::string& name,
                          const Uint16s& value) override;
  void EmitUintChanged(const std::string& name, uint32_t value) override;
  void EmitIntChanged(const std::string& name, int value) override;
  void EmitRpcIdentifierChanged(
      const std::string& name, const std::string& value) override;
  void EmitStringChanged(
      const std::string& name, const std::string& value) override;
  void EmitStringmapChanged(const std::string& name,
                            const Stringmap& value) override;

  // Implementation of ServiceAdaptor
  bool GetProperties(brillo::ErrorPtr* error,
                     brillo::VariantDictionary* properties) override;
  bool SetProperty(brillo::ErrorPtr* error,
                   const std::string& name,
                   const brillo::Any& value) override;
  bool SetProperties(brillo::ErrorPtr* error,
                     const brillo::VariantDictionary& properties) override;
  bool ClearProperty(brillo::ErrorPtr* error,
                     const std::string& name) override;
  bool ClearProperties(brillo::ErrorPtr* error,
                       const std::vector<std::string>& names,
                       std::vector<bool>* results) override;
  bool Connect(brillo::ErrorPtr* error) override;
  bool Disconnect(brillo::ErrorPtr* error) override;
  bool Remove(brillo::ErrorPtr* error) override;
  void ActivateCellularModem(DBusMethodResponsePtr<> response,
                             const std::string& carrier) override;
  bool CompleteCellularActivation(brillo::ErrorPtr* error) override;
  bool GetLoadableProfileEntries(
      brillo::ErrorPtr* error,
      std::map<dbus::ObjectPath, std::string>* entries) override;

  Service* service() const { return service_; }

 private:
  Service* service_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosServiceDBusAdaptor);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_SERVICE_DBUS_ADAPTOR_H_

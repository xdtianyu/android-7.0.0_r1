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

#ifndef SHILL_DBUS_CHROMEOS_IPCONFIG_DBUS_ADAPTOR_H_
#define SHILL_DBUS_CHROMEOS_IPCONFIG_DBUS_ADAPTOR_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "dbus_bindings/org.chromium.flimflam.IPConfig.h"
#include "shill/adaptor_interfaces.h"
#include "shill/dbus/chromeos_dbus_adaptor.h"

namespace shill {

class IPConfig;

// Subclass of DBusAdaptor for IPConfig objects
// There is a 1:1 mapping between IPConfig and ChromeosIPConfigDBusAdaptor
// instances.  Furthermore, the IPConfig owns the ChromeosIPConfigDBusAdaptor
// and manages its lifetime, so we're OK with ChromeosIPConfigDBusAdaptor
// having a bare pointer to its owner ipconfig.
class ChromeosIPConfigDBusAdaptor
    : public org::chromium::flimflam::IPConfigAdaptor,
      public org::chromium::flimflam::IPConfigInterface,
      public ChromeosDBusAdaptor,
      public IPConfigAdaptorInterface {
 public:
  static const char kInterfaceName[];
  static const char kPath[];

  ChromeosIPConfigDBusAdaptor(const scoped_refptr<dbus::Bus>& bus,
                              IPConfig* ipconfig);
  ~ChromeosIPConfigDBusAdaptor() override;

  // Implementation of IPConfigAdaptorInterface.
  const std::string& GetRpcIdentifier() override { return dbus_path().value(); }
  void EmitBoolChanged(const std::string& name, bool value) override;
  void EmitUintChanged(const std::string& name, uint32_t value) override;
  void EmitIntChanged(const std::string& name, int value) override;
  void EmitStringChanged(const std::string& name,
                         const std::string& value) override;
  void EmitStringsChanged(const std::string& name,
                          const std::vector<std::string>& value) override;

  // Implementation of IPConfigAdaptor
  bool GetProperties(brillo::ErrorPtr* error,
                     brillo::VariantDictionary* properties) override;
  bool SetProperty(brillo::ErrorPtr* error,
                   const std::string& name,
                   const brillo::Any& value) override;
  bool ClearProperty(brillo::ErrorPtr* error,
                     const std::string& name) override;
  bool Remove(brillo::ErrorPtr* error) override;
  bool Refresh(brillo::ErrorPtr* error) override;

 private:
  IPConfig* ipconfig_;
  DISALLOW_COPY_AND_ASSIGN(ChromeosIPConfigDBusAdaptor);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_IPCONFIG_DBUS_ADAPTOR_H_

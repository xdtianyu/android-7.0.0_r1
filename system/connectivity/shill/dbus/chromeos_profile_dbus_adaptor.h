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

#ifndef SHILL_DBUS_CHROMEOS_PROFILE_DBUS_ADAPTOR_H_
#define SHILL_DBUS_CHROMEOS_PROFILE_DBUS_ADAPTOR_H_

#include <string>

#include <base/macros.h>

#include "dbus_bindings/org.chromium.flimflam.Profile.h"
#include "shill/adaptor_interfaces.h"
#include "shill/dbus/chromeos_dbus_adaptor.h"

namespace shill {

class Profile;

// Subclass of DBusAdaptor for Profile objects
// There is a 1:1 mapping between Profile and ChromeosProfileDBusAdaptor
// instances.  Furthermore, the Profile owns the ChromeosProfileDBusAdaptor
// and manages its lifetime, so we're OK with ChromeosProfileDBusAdaptor
// having a bare pointer to its owner profile.
//
// A Profile is a collection of Entry structures (which we will define later).
class ChromeosProfileDBusAdaptor
    : public org::chromium::flimflam::ProfileAdaptor,
      public org::chromium::flimflam::ProfileInterface,
      public ChromeosDBusAdaptor,
      public ProfileAdaptorInterface {
 public:
  static const char kPath[];

  ChromeosProfileDBusAdaptor(
      const scoped_refptr<dbus::Bus>& bus,
      Profile* profile);
  ~ChromeosProfileDBusAdaptor() override;

  // Implementation of ProfileAdaptorInterface.
  const std::string& GetRpcIdentifier() override { return dbus_path().value(); }
  void EmitBoolChanged(const std::string& name, bool value) override;
  void EmitUintChanged(const std::string& name, uint32_t value) override;
  void EmitIntChanged(const std::string& name, int value) override;
  void EmitStringChanged(const std::string& name,
                         const std::string& value) override;

  // Implementation of ProfileAdaptor
  bool GetProperties(brillo::ErrorPtr* error,
                     brillo::VariantDictionary* properties) override;
  bool SetProperty(brillo::ErrorPtr* error,
                   const std::string& name,
                   const brillo::Any& value) override;

  // Gets an "Entry", which is apparently a different set of properties than
  // those returned by GetProperties.
  bool GetEntry(brillo::ErrorPtr* error,
                const std::string& name,
                brillo::VariantDictionary* entry_properties) override;

  // Deletes an Entry.
  bool DeleteEntry(brillo::ErrorPtr* error, const std::string& name) override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosProfileDBusAdaptor);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_PROFILE_DBUS_ADAPTOR_H_

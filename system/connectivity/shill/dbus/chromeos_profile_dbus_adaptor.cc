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

#include "shill/dbus/chromeos_profile_dbus_adaptor.h"

#include <string>
#include <vector>

#include "shill/error.h"
#include "shill/logging.h"
#include "shill/profile.h"
#include "shill/service.h"

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(ChromeosProfileDBusAdaptor* p) {
  return p->GetRpcIdentifier();
}
}


// static
const char ChromeosProfileDBusAdaptor::kPath[] = "/profile/";

ChromeosProfileDBusAdaptor::ChromeosProfileDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    Profile* profile)
    : org::chromium::flimflam::ProfileAdaptor(this),
      ChromeosDBusAdaptor(bus, kPath + profile->GetFriendlyName()),
      profile_(profile) {
  // Register DBus object.
  RegisterWithDBusObject(dbus_object());
  dbus_object()->RegisterAndBlock();
}

ChromeosProfileDBusAdaptor::~ChromeosProfileDBusAdaptor() {
  dbus_object()->UnregisterAsync();
  profile_ = nullptr;
}

void ChromeosProfileDBusAdaptor::EmitBoolChanged(const string& name,
                                                 bool value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosProfileDBusAdaptor::EmitUintChanged(const string& name,
                                                 uint32_t value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosProfileDBusAdaptor::EmitIntChanged(const string& name, int value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosProfileDBusAdaptor::EmitStringChanged(const string& name,
                                                   const string& value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

bool ChromeosProfileDBusAdaptor::GetProperties(
    brillo::ErrorPtr* error, brillo::VariantDictionary* properties) {
  SLOG(this, 2) << __func__;
  return ChromeosDBusAdaptor::GetProperties(profile_->store(),
                                            properties,
                                            error);
}

bool ChromeosProfileDBusAdaptor::SetProperty(
    brillo::ErrorPtr* error, const string& name, const brillo::Any& value) {
  SLOG(this, 2) << __func__ << ": " << name;
  return ChromeosDBusAdaptor::SetProperty(profile_->mutable_store(),
                                          name,
                                          value,
                                          error);
}

bool ChromeosProfileDBusAdaptor::GetEntry(
    brillo::ErrorPtr* error,
    const std::string& name,
    brillo::VariantDictionary* entry_properties) {
  SLOG(this, 2) << __func__ << ": " << name;
  Error e;
  ServiceRefPtr service = profile_->GetServiceFromEntry(name, &e);
  if (!e.IsSuccess()) {
    return !e.ToChromeosError(error);
  }
  return ChromeosDBusAdaptor::GetProperties(service->store(),
                                            entry_properties,
                                            error);
}

bool ChromeosProfileDBusAdaptor::DeleteEntry(brillo::ErrorPtr* error,
                                             const std::string& name) {
  SLOG(this, 2) << __func__ << ": " << name;
  Error e;
  profile_->DeleteEntry(name, &e);
  return !e.ToChromeosError(error);
}

}  // namespace shill

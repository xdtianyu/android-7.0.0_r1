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

#include "shill/dbus/chromeos_ipconfig_dbus_adaptor.h"

#include <string>
#include <vector>

#include <base/strings/stringprintf.h>

#include "shill/error.h"
#include "shill/ipconfig.h"
#include "shill/logging.h"

using base::StringPrintf;
using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(ChromeosIPConfigDBusAdaptor* i) {
  return i->GetRpcIdentifier();
}
}

// static
const char ChromeosIPConfigDBusAdaptor::kPath[] = "/ipconfig/";

ChromeosIPConfigDBusAdaptor::ChromeosIPConfigDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    IPConfig* config)
    : org::chromium::flimflam::IPConfigAdaptor(this),
      ChromeosDBusAdaptor(bus,
                          StringPrintf("%s%s_%u_%s",
                                       kPath,
                                       SanitizePathElement(
                                           config->device_name()).c_str(),
                                       config->serial(),
                                       config->type().c_str())),
      ipconfig_(config) {
  // Register DBus object.
  RegisterWithDBusObject(dbus_object());
  dbus_object()->RegisterAndBlock();
}

ChromeosIPConfigDBusAdaptor::~ChromeosIPConfigDBusAdaptor() {
  dbus_object()->UnregisterAsync();
  ipconfig_ = nullptr;
}

void ChromeosIPConfigDBusAdaptor::EmitBoolChanged(const string& name,
                                                  bool value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosIPConfigDBusAdaptor::EmitUintChanged(const string& name,
                                                  uint32_t value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosIPConfigDBusAdaptor::EmitIntChanged(const string& name,
                                                 int value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosIPConfigDBusAdaptor::EmitStringChanged(const string& name,
                                                    const string& value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

void ChromeosIPConfigDBusAdaptor::EmitStringsChanged(
    const string& name, const vector<string>& value) {
  SLOG(this, 2) << __func__ << ": " << name;
  SendPropertyChangedSignal(name, brillo::Any(value));
}

bool ChromeosIPConfigDBusAdaptor::GetProperties(
    brillo::ErrorPtr* error, brillo::VariantDictionary* properties) {
  SLOG(this, 2) << __func__;
  return ChromeosDBusAdaptor::GetProperties(ipconfig_->store(),
                                            properties,
                                            error);
}

bool ChromeosIPConfigDBusAdaptor::SetProperty(
    brillo::ErrorPtr* error, const string& name, const brillo::Any& value) {
  SLOG(this, 2) << __func__ << ": " << name;
  return ChromeosDBusAdaptor::SetProperty(ipconfig_->mutable_store(),
                                          name,
                                          value,
                                          error);
}

bool ChromeosIPConfigDBusAdaptor::ClearProperty(
    brillo::ErrorPtr* error, const string& name) {
  SLOG(this, 2) << __func__ << ": " << name;
  return ChromeosDBusAdaptor::ClearProperty(ipconfig_->mutable_store(),
                                            name,
                                            error);
}

bool ChromeosIPConfigDBusAdaptor::Remove(brillo::ErrorPtr* error) {
  SLOG(this, 2) << __func__;
  return !Error(Error::kNotSupported).ToChromeosError(error);
}

bool ChromeosIPConfigDBusAdaptor::Refresh(brillo::ErrorPtr* error) {
  SLOG(this, 2) << __func__;
  Error e;
  ipconfig_->Refresh(&e);
  return !e.ToChromeosError(error);
}

}  // namespace shill

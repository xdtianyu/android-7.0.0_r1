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

#include "shill/dbus/chromeos_rpc_task_dbus_adaptor.h"

#include <base/strings/stringprintf.h>

#include "shill/error.h"
#include "shill/logging.h"
#include "shill/rpc_task.h"

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;
using std::map;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(ChromeosRPCTaskDBusAdaptor* r) {
  return r->GetRpcIdentifier();
}
}

// static
const char ChromeosRPCTaskDBusAdaptor::kPath[] = "/task/";

ChromeosRPCTaskDBusAdaptor::ChromeosRPCTaskDBusAdaptor(
    const scoped_refptr<dbus::Bus>& bus,
    RPCTask* task)
    : org::chromium::flimflam::TaskAdaptor(this),
      ChromeosDBusAdaptor(bus, kPath + task->UniqueName()),
      task_(task),
      connection_name_(bus->GetConnectionName()) {
  // Register DBus object.
  RegisterWithDBusObject(dbus_object());
  dbus_object()->RegisterAndBlock();
}

ChromeosRPCTaskDBusAdaptor::~ChromeosRPCTaskDBusAdaptor() {
  dbus_object()->UnregisterAsync();
  task_ = nullptr;
}

const string& ChromeosRPCTaskDBusAdaptor::GetRpcIdentifier() {
  return dbus_path().value();
}

const string& ChromeosRPCTaskDBusAdaptor::GetRpcConnectionIdentifier() {
  return connection_name_;
}

bool ChromeosRPCTaskDBusAdaptor::getsec(
    brillo::ErrorPtr* /*error*/, string* user, string* password) {
  SLOG(this, 2) << __func__ << ": " << user;
  task_->GetLogin(user, password);
  return true;
}

bool ChromeosRPCTaskDBusAdaptor::notify(brillo::ErrorPtr* /*error*/,
                                        const string& reason,
                                        const map<string, string>& dict) {
  SLOG(this, 2) << __func__ << ": " << reason;
  task_->Notify(reason, dict);
  return true;
}

}  // namespace shill

//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/cellular/modem_manager.h"

#include <base/bind.h>
#include <base/stl_util.h>
#include <ModemManager/ModemManager.h>

#include "shill/cellular/modem.h"
#include "shill/control_interface.h"
#include "shill/error.h"
#include "shill/logging.h"

using base::Bind;
using std::string;
using std::shared_ptr;
using std::vector;

namespace shill {

ModemManager1::ModemManager1(ControlInterface* control_interface,
                             const string& service,
                             const string& path,
                             ModemInfo* modem_info)
    : ModemManager(control_interface,
                   service,
                   path,
                   modem_info),
      weak_ptr_factory_(this) {}

ModemManager1::~ModemManager1() {
  Stop();
}

void ModemManager1::Start() {
  LOG(INFO) << "Start watching modem manager service: " << service();
  CHECK(!proxy_);
  proxy_.reset(
      control_interface()->CreateDBusObjectManagerProxy(
          path(),
          service(),
          base::Bind(&ModemManager1::OnAppeared, base::Unretained(this)),
          base::Bind(&ModemManager1::OnVanished, base::Unretained(this))));
  proxy_->set_interfaces_added_callback(
      Bind(&ModemManager1::OnInterfacesAddedSignal,
           weak_ptr_factory_.GetWeakPtr()));
  proxy_->set_interfaces_removed_callback(
      Bind(&ModemManager1::OnInterfacesRemovedSignal,
           weak_ptr_factory_.GetWeakPtr()));
}

void ModemManager1::Stop() {
  LOG(INFO) << "Stop watching modem manager service: " << service();
  proxy_.reset();
  Disconnect();
}

void ModemManager1::Connect() {
  ModemManager::Connect();
  // TODO(rochberg):  Make global kDBusDefaultTimeout and use it here
  Error error;
  proxy_->GetManagedObjects(&error,
                            Bind(&ModemManager1::OnGetManagedObjectsReply,
                                 weak_ptr_factory_.GetWeakPtr()),
                            5000);
}

void ModemManager1::Disconnect() {
  ModemManager::Disconnect();
}

void ModemManager1::AddModem1(const string& path,
                              const InterfaceToProperties& properties) {
  if (ModemExists(path)) {
    return;
  }
  shared_ptr<Modem1> modem1(new Modem1(service(),
                                       path,
                                       modem_info(),
                                       control_interface()));
  RecordAddedModem(modem1);
  InitModem1(modem1, properties);
}

void ModemManager1::InitModem1(shared_ptr<Modem1> modem,
                               const InterfaceToProperties& properties) {
  if (modem == nullptr) {
    return;
  }
  modem->CreateDeviceMM1(properties);
}

// signal methods
// Also called by OnGetManagedObjectsReply
void ModemManager1::OnInterfacesAddedSignal(
    const string& object_path,
    const InterfaceToProperties& properties) {
  if (ContainsKey(properties, MM_DBUS_INTERFACE_MODEM)) {
    AddModem1(object_path, properties);
  } else {
    LOG(ERROR) << "Interfaces added, but not modem interface.";
  }
}

void ModemManager1::OnInterfacesRemovedSignal(
    const string& object_path,
    const vector<string>& interfaces) {
  LOG(INFO) << "MM1:  Removing interfaces from " << object_path;
  if (find(interfaces.begin(),
           interfaces.end(),
           MM_DBUS_INTERFACE_MODEM) != interfaces.end()) {
    RemoveModem(object_path);
  } else {
    // In theory, a modem could drop, say, 3GPP, but not CDMA.  In
    // practice, we don't expect this
    LOG(ERROR) << "Interfaces removed, but not modem interface";
  }
}

// DBusObjectManagerProxy async method call
void ModemManager1::OnGetManagedObjectsReply(
    const ObjectsWithProperties& objects,
    const Error& error) {
  if (error.IsSuccess()) {
    ObjectsWithProperties::const_iterator m;
    for (m = objects.begin(); m != objects.end(); ++m) {
      OnInterfacesAddedSignal(m->first, m->second);
    }
  }
}

}  // namespace shill

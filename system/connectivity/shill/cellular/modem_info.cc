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

#include "shill/cellular/modem_info.h"

#include <base/files/file_path.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/cellular/modem_manager.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/pending_activation_store.h"

using base::FilePath;
using std::string;

namespace shill {

ModemInfo::ModemInfo(ControlInterface* control_interface,
                     EventDispatcher* dispatcher,
                     Metrics* metrics,
                     Manager* manager)
    : control_interface_(control_interface),
      dispatcher_(dispatcher),
      metrics_(metrics),
      manager_(manager) {}

ModemInfo::~ModemInfo() {
  Stop();
}

void ModemInfo::Start() {
  pending_activation_store_.reset(new PendingActivationStore());
  pending_activation_store_->InitStorage(manager_->storage_path());

  RegisterModemManager(new ModemManagerClassic(control_interface_,
                                               cromo::kCromoServiceName,
                                               cromo::kCromoServicePath,
                                               this));
  RegisterModemManager(
      new ModemManager1(control_interface_,
                        modemmanager::kModemManager1ServiceName,
                        modemmanager::kModemManager1ServicePath,
                        this));
}

void ModemInfo::Stop() {
  pending_activation_store_.reset();
  modem_managers_.clear();
}

void ModemInfo::OnDeviceInfoAvailable(const string& link_name) {
  for (const auto& manager : modem_managers_) {
    manager->OnDeviceInfoAvailable(link_name);
  }
}

void ModemInfo::set_pending_activation_store(
    PendingActivationStore* pending_activation_store) {
  pending_activation_store_.reset(pending_activation_store);
}

void ModemInfo::RegisterModemManager(ModemManager* manager) {
  modem_managers_.push_back(manager);  // Passes ownership.
  manager->Start();
}

}  // namespace shill

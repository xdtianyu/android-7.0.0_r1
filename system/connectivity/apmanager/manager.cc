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

#include "apmanager/manager.h"

#include <base/bind.h>

#include "apmanager/control_interface.h"
#include "apmanager/manager.h"

using std::string;

namespace apmanager {

Manager::Manager(ControlInterface* control_interface)
    : control_interface_(control_interface),
      service_identifier_(0),
      device_info_(this),
      adaptor_(control_interface->CreateManagerAdaptor(this)) {}

Manager::~Manager() {}

void Manager::RegisterAsync(
    const base::Callback<void(bool)>& completion_callback) {
  adaptor_->RegisterAsync(completion_callback);
}

void Manager::Start() {
  shill_manager_.Init(control_interface_);
  firewall_manager_.Init(control_interface_);
  device_info_.Start();
}

void Manager::Stop() {
  device_info_.Stop();
}

scoped_refptr<Service> Manager::CreateService() {
  LOG(INFO) << "Manager::CreateService";
  scoped_refptr<Service> service = new Service(this, service_identifier_++);
  services_.push_back(service);
  return service;
}

bool Manager::RemoveService(const scoped_refptr<Service>& service,
                            Error* error) {
  for (auto it = services_.begin(); it != services_.end(); ++it) {
    if (*it == service) {
      services_.erase(it);
      return true;
    }
  }

  Error::PopulateAndLog(error,
                        Error::kInvalidArguments,
                        "Service does not exist",
                        FROM_HERE);
  return false;
}

scoped_refptr<Device> Manager::GetAvailableDevice() {
  for (const auto& device : devices_) {
    // Look for an unused device with AP interface mode support.
    if (!device->GetInUse() && !device->GetPreferredApInterface().empty()) {
      return device;
    }
  }
  return nullptr;
}

scoped_refptr<Device> Manager::GetDeviceFromInterfaceName(
    const string& interface_name) {
  for (const auto& device : devices_) {
    if (device->InterfaceExists(interface_name)) {
      return device;
    }
  }
  return nullptr;
}

void Manager::RegisterDevice(const scoped_refptr<Device>& device) {
  LOG(INFO) << "Manager::RegisterDevice: registering device "
            << device->GetDeviceName();
  devices_.push_back(device);
  // TODO(zqiu): Property update for available devices.
}

void Manager::ClaimInterface(const string& interface_name) {
  shill_manager_.ClaimInterface(interface_name);
}

void Manager::ReleaseInterface(const string& interface_name) {
  shill_manager_.ReleaseInterface(interface_name);
}

#if defined(__BRILLO__)
bool Manager::SetupApModeInterface(string* interface_name) {
  return shill_manager_.SetupApModeInterface(interface_name);
}

bool Manager::SetupStationModeInterface(string* interface_name) {
  return shill_manager_.SetupStationModeInterface(interface_name);
}
#endif  // __BRILLO__

void Manager::RequestDHCPPortAccess(const string& interface) {
  firewall_manager_.RequestDHCPPortAccess(interface);
}

void Manager::ReleaseDHCPPortAccess(const string& interface) {
  firewall_manager_.ReleaseDHCPPortAccess(interface);
}

}  // namespace apmanager

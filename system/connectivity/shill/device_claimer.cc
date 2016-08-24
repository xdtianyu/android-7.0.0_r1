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

#include "shill/device_claimer.h"

#include "shill/control_interface.h"
#include "shill/device_info.h"

using std::string;

namespace shill {

DeviceClaimer::DeviceClaimer(
    const std::string& service_name,
    DeviceInfo* device_info,
    bool default_claimer)
    : service_name_(service_name),
      device_info_(device_info),
      default_claimer_(default_claimer) {}

DeviceClaimer::~DeviceClaimer() {
  // Release claimed devices if there is any.
  if (DevicesClaimed()) {
    for (const auto& device : claimed_device_names_) {
      device_info_->RemoveDeviceFromBlackList(device);
    }
    // Clear claimed device list.
    claimed_device_names_.clear();
  }
}

bool DeviceClaimer::Claim(const string& device_name, Error* error) {
  // Check if device is claimed already.
  if (claimed_device_names_.find(device_name) != claimed_device_names_.end()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Device " + device_name +
                          " had already been claimed");
    return false;
  }

  // Add device to the black list.
  device_info_->AddDeviceToBlackList(device_name);

  claimed_device_names_.insert(device_name);
  released_device_names_.erase(device_name);
  return true;
}

bool DeviceClaimer::Release(const std::string& device_name,
                            Error* error) {
  // Make sure this is a device that have been claimed.
  if (claimed_device_names_.find(device_name) == claimed_device_names_.end()) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                          "Device " + device_name +
                          " have not been claimed");
    return false;
  }

  // Remove the device from the black list.
  device_info_->RemoveDeviceFromBlackList(device_name);

  claimed_device_names_.erase(device_name);
  released_device_names_.insert(device_name);
  return true;
}

bool DeviceClaimer::DevicesClaimed() {
  return !claimed_device_names_.empty();
}

bool DeviceClaimer::IsDeviceReleased(const string& device_name) {
  return released_device_names_.find(device_name) !=
      released_device_names_.end();
}

}  // namespace shill

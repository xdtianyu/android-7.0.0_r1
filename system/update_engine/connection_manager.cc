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

#include "update_engine/connection_manager.h"

#include <set>
#include <string>

#include <base/stl_util.h>
#include <base/strings/string_util.h>
#include <policy/device_policy.h>
#include <shill/dbus-constants.h>
#include <shill/dbus-proxies.h>

#include "update_engine/common/prefs.h"
#include "update_engine/common/utils.h"
#include "update_engine/system_state.h"

using org::chromium::flimflam::ManagerProxyInterface;
using org::chromium::flimflam::ServiceProxyInterface;
using std::set;
using std::string;

namespace chromeos_update_engine {

namespace {

NetworkConnectionType ParseConnectionType(const string& type_str) {
  if (type_str == shill::kTypeEthernet) {
    return NetworkConnectionType::kEthernet;
  } else if (type_str == shill::kTypeWifi) {
    return NetworkConnectionType::kWifi;
  } else if (type_str == shill::kTypeWimax) {
    return NetworkConnectionType::kWimax;
  } else if (type_str == shill::kTypeBluetooth) {
    return NetworkConnectionType::kBluetooth;
  } else if (type_str == shill::kTypeCellular) {
    return NetworkConnectionType::kCellular;
  }
  return NetworkConnectionType::kUnknown;
}

NetworkTethering ParseTethering(const string& tethering_str) {
  if (tethering_str == shill::kTetheringNotDetectedState) {
    return NetworkTethering::kNotDetected;
  } else if (tethering_str == shill::kTetheringSuspectedState) {
    return NetworkTethering::kSuspected;
  } else if (tethering_str == shill::kTetheringConfirmedState) {
    return NetworkTethering::kConfirmed;
  }
  LOG(WARNING) << "Unknown Tethering value: " << tethering_str;
  return NetworkTethering::kUnknown;
}

}  // namespace

ConnectionManager::ConnectionManager(ShillProxyInterface* shill_proxy,
                                     SystemState* system_state)
    : shill_proxy_(shill_proxy), system_state_(system_state) {}

bool ConnectionManager::IsUpdateAllowedOver(NetworkConnectionType type,
                                            NetworkTethering tethering) const {
  switch (type) {
    case NetworkConnectionType::kBluetooth:
      return false;

    case NetworkConnectionType::kCellular: {
      set<string> allowed_types;
      const policy::DevicePolicy* device_policy =
          system_state_->device_policy();

      // A device_policy is loaded in a lazy way right before an update check,
      // so the device_policy should be already loaded at this point. If it's
      // not, return a safe value for this setting.
      if (!device_policy) {
        LOG(INFO) << "Disabling updates over cellular networks as there's no "
                     "device policy loaded yet.";
        return false;
      }

      if (device_policy->GetAllowedConnectionTypesForUpdate(&allowed_types)) {
        // The update setting is enforced by the device policy.

        if (!ContainsKey(allowed_types, shill::kTypeCellular)) {
          LOG(INFO) << "Disabling updates over cellular connection as it's not "
                       "allowed in the device policy.";
          return false;
        }

        LOG(INFO) << "Allowing updates over cellular per device policy.";
        return true;
      } else {
        // There's no update setting in the device policy, using the local user
        // setting.
        PrefsInterface* prefs = system_state_->prefs();

        if (!prefs || !prefs->Exists(kPrefsUpdateOverCellularPermission)) {
          LOG(INFO) << "Disabling updates over cellular connection as there's "
                       "no device policy setting nor user preference present.";
          return false;
        }

        bool stored_value;
        if (!prefs->GetBoolean(kPrefsUpdateOverCellularPermission,
                               &stored_value)) {
          return false;
        }

        if (!stored_value) {
          LOG(INFO) << "Disabling updates over cellular connection per user "
                       "setting.";
          return false;
        }
        LOG(INFO) << "Allowing updates over cellular per user setting.";
        return true;
      }
    }

    default:
      if (tethering == NetworkTethering::kConfirmed) {
        // Treat this connection as if it is a cellular connection.
        LOG(INFO) << "Current connection is confirmed tethered, using Cellular "
                     "setting.";
        return IsUpdateAllowedOver(NetworkConnectionType::kCellular,
                                   NetworkTethering::kUnknown);
      }
      return true;
  }
}

// static
const char* ConnectionManager::StringForConnectionType(
    NetworkConnectionType type) {
  switch (type) {
    case NetworkConnectionType::kEthernet:
      return shill::kTypeEthernet;
    case NetworkConnectionType::kWifi:
      return shill::kTypeWifi;
    case NetworkConnectionType::kWimax:
      return shill::kTypeWimax;
    case NetworkConnectionType::kBluetooth:
      return shill::kTypeBluetooth;
    case NetworkConnectionType::kCellular:
      return shill::kTypeCellular;
    case NetworkConnectionType::kUnknown:
      return "Unknown";
  }
  return "Unknown";
}

bool ConnectionManager::GetConnectionProperties(
    NetworkConnectionType* out_type,
    NetworkTethering* out_tethering) {
  dbus::ObjectPath default_service_path;
  TEST_AND_RETURN_FALSE(GetDefaultServicePath(&default_service_path));
  if (!default_service_path.IsValid())
    return false;
  // Shill uses the "/" service path to indicate that it is not connected.
  if (default_service_path.value() == "/")
    return false;
  TEST_AND_RETURN_FALSE(
      GetServicePathProperties(default_service_path, out_type, out_tethering));
  return true;
}

bool ConnectionManager::GetDefaultServicePath(dbus::ObjectPath* out_path) {
  brillo::VariantDictionary properties;
  brillo::ErrorPtr error;
  ManagerProxyInterface* manager_proxy = shill_proxy_->GetManagerProxy();
  if (!manager_proxy)
    return false;
  TEST_AND_RETURN_FALSE(manager_proxy->GetProperties(&properties, &error));

  const auto& prop_default_service =
      properties.find(shill::kDefaultServiceProperty);
  if (prop_default_service == properties.end())
    return false;

  *out_path = prop_default_service->second.TryGet<dbus::ObjectPath>();
  return out_path->IsValid();
}

bool ConnectionManager::GetServicePathProperties(
    const dbus::ObjectPath& path,
    NetworkConnectionType* out_type,
    NetworkTethering* out_tethering) {
  // We create and dispose the ServiceProxyInterface on every request.
  std::unique_ptr<ServiceProxyInterface> service =
      shill_proxy_->GetServiceForPath(path);

  brillo::VariantDictionary properties;
  brillo::ErrorPtr error;
  TEST_AND_RETURN_FALSE(service->GetProperties(&properties, &error));

  // Populate the out_tethering.
  const auto& prop_tethering = properties.find(shill::kTetheringProperty);
  if (prop_tethering == properties.end()) {
    // Set to Unknown if not present.
    *out_tethering = NetworkTethering::kUnknown;
  } else {
    // If the property doesn't contain a string value, the empty string will
    // become kUnknown.
    *out_tethering = ParseTethering(prop_tethering->second.TryGet<string>());
  }

  // Populate the out_type property.
  const auto& prop_type = properties.find(shill::kTypeProperty);
  if (prop_type == properties.end()) {
    // Set to Unknown if not present.
    *out_type = NetworkConnectionType::kUnknown;
    return false;
  }

  string type_str = prop_type->second.TryGet<string>();
  if (type_str == shill::kTypeVPN) {
    const auto& prop_physical =
        properties.find(shill::kPhysicalTechnologyProperty);
    if (prop_physical == properties.end()) {
      LOG(ERROR) << "No PhysicalTechnology property found for a VPN"
                    " connection (service: "
                 << path.value() << "). Returning default kUnknown value.";
      *out_type = NetworkConnectionType::kUnknown;
    } else {
      *out_type = ParseConnectionType(prop_physical->second.TryGet<string>());
    }
  } else {
    *out_type = ParseConnectionType(type_str);
  }
  return true;
}

}  // namespace chromeos_update_engine

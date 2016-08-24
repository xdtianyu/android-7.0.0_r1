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

#include "shill/wifi/wifi_driver_hal.h"

#include <base/logging.h>
#include <hardware_brillo/wifi_driver_hal.h>
#include <hardware/hardware.h>

using std::string;

namespace shill {

namespace {

base::LazyInstance<WiFiDriverHal> g_wifi_driver_hal = LAZY_INSTANCE_INITIALIZER;

bool WiFiDriverInit(wifi_driver_device_t** out_driver) {
  const hw_module_t* module;
  wifi_driver_device_t* driver;
  int ret;

  ret = hw_get_module(WIFI_DRIVER_DEVICE_ID_MAIN, &module);
  if (ret != 0) {
    LOG(ERROR) << "Failed to find HAL module";
    return false;
  }

  if (wifi_driver_open(module, &driver) != 0) {
    LOG(ERROR) << "Failed to open WiFi HAL module";
    return false;
  }

  wifi_driver_error error;
  error = (*driver->wifi_driver_initialize)();
  if (error != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to initialize WiFi driver";
    wifi_driver_close(driver);
    return false;
  }

  *out_driver = driver;
  return true;
}

void WiFiDriverShutdown(wifi_driver_device_t* driver) {
  wifi_driver_close(driver);
}

string WiFiDriverSetupInterface(wifi_driver_mode mode) {
  wifi_driver_device_t* driver;
  if (!WiFiDriverInit(&driver)) {
    return "";
  }

  string name_str;
  char device_name[DEFAULT_WIFI_DEVICE_NAME_SIZE];
  wifi_driver_error error =
      (*driver->wifi_driver_set_mode)(mode, device_name, sizeof(device_name));
  if (error != WIFI_SUCCESS) {
    LOG(ERROR) << "WiFi driver setup for mode " << mode << " failed: " << error;
  } else {
    name_str = string(device_name);
  }

  WiFiDriverShutdown(driver);
  return name_str;
}

}  // namespace

WiFiDriverHal::WiFiDriverHal() {}

WiFiDriverHal::~WiFiDriverHal() {}

WiFiDriverHal* WiFiDriverHal::GetInstance() {
  return g_wifi_driver_hal.Pointer();
}

string WiFiDriverHal::SetupStationModeInterface() {
  return WiFiDriverSetupInterface(WIFI_MODE_STATION);
}

string WiFiDriverHal::SetupApModeInterface() {
  return WiFiDriverSetupInterface(WIFI_MODE_AP);
}

}  // namespace shill

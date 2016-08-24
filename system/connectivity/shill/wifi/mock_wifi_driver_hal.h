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

#ifndef SHILL_WIFI_MOCK_WIFI_DRIVER_HAL_H_
#define SHILL_WIFI_MOCK_WIFI_DRIVER_HAL_H_

#include <string>

#include <base/macros.h>

#include "shill/wifi/wifi_driver_hal.h"

namespace shill {

class MockWiFiDriverHal : public WiFiDriverHal {
 public:
  MockWiFiDriverHal() {}
  ~MockWiFiDriverHal() override {}

  MOCK_METHOD0(SetupStationModeInterface, std::string());
  MOCK_METHOD0(SetupApModeInterface, std::string());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiFiDriverHal);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_WIFI_DRIVER_HAL_H_

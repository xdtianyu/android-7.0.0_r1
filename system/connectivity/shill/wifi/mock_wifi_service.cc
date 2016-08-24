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

#include "shill/wifi/mock_wifi_service.h"

using std::string;
using std::vector;

namespace shill {

class ControlInterface;
class EventDispatcher;
class Manager;

MockWiFiService::MockWiFiService(ControlInterface* control_interface,
                                 EventDispatcher* dispatcher,
                                 Metrics* metrics,
                                 Manager* manager,
                                 WiFiProvider* provider,
                                 const vector<uint8_t>& ssid,
                                 const string& mode,
                                 const string& security,
                                 bool hidden_ssid)
    : WiFiService(
        control_interface, dispatcher, metrics, manager, provider, ssid, mode,
        security, hidden_ssid) {
  ON_CALL(*this, GetSupplicantConfigurationParameters())
      .WillByDefault(testing::Return(KeyValueStore()));
}

MockWiFiService::~MockWiFiService() {}

}  // namespace shill

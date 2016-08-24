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

#ifndef SHILL_WIFI_MOCK_WIFI_H_
#define SHILL_WIFI_MOCK_WIFI_H_

#include <map>
#include <string>

#include <base/memory/ref_counted.h>
#include <gmock/gmock.h>

#include "shill/key_value_store.h"
#include "shill/refptr_types.h"
#include "shill/wifi/wifi.h"
#include "shill/wifi/wifi_endpoint.h"
#include "shill/wifi/wifi_service.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;

class MockWiFi : public WiFi {
 public:
  MockWiFi(ControlInterface* control_interface,
           EventDispatcher* dispatcher,
           Metrics* metrics,
           Manager* manager,
           const std::string& link_name,
           const std::string& address,
           int interface_index);
  ~MockWiFi() override;

  MOCK_METHOD2(Start, void(Error* error,
                           const EnabledStateChangedCallback& callback));
  MOCK_METHOD2(Stop, void(Error* error,
                          const EnabledStateChangedCallback& callback));
  MOCK_METHOD3(Scan, void(ScanType scan_type, Error* error,
                          const std::string& reason));
  MOCK_METHOD1(DisconnectFromIfActive, void(WiFiService* service));
  MOCK_METHOD1(DisconnectFrom, void(WiFiService* service));
  MOCK_METHOD1(ClearCachedCredentials, void(const WiFiService* service));
  MOCK_METHOD1(ConnectTo, void(WiFiService* service));
  MOCK_CONST_METHOD0(IsIdle, bool());
  MOCK_METHOD1(NotifyEndpointChanged,
               void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(DestroyIPConfigLease, void(const std::string&));
  MOCK_CONST_METHOD0(IsConnectedViaTether, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiFi);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_WIFI_H_

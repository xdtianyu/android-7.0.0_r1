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

#ifndef SHILL_WIFI_MOCK_WIFI_SERVICE_H_
#define SHILL_WIFI_MOCK_WIFI_SERVICE_H_

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include "shill/wifi/wifi_service.h"

namespace shill {

class MockWiFiService : public WiFiService {
 public:
  MockWiFiService(ControlInterface* control_interface,
                  EventDispatcher* dispatcher,
                  Metrics* metrics,
                  Manager* manager,
                  WiFiProvider* provider,
                  const std::vector<uint8_t>& ssid,
                  const std::string& mode,
                  const std::string& security,
                  bool hidden_ssid);
  ~MockWiFiService() override;

  MOCK_METHOD2(Configure, void(const KeyValueStore& args, Error* error));
  MOCK_METHOD1(SetFailure, void(ConnectFailure failure));
  MOCK_METHOD1(SetFailureSilent, void(ConnectFailure failure));
  MOCK_METHOD1(SetState, void(ConnectState state));
  MOCK_METHOD2(AddEAPCertification, bool(const std::string& name,
                                         size_t depth));
  MOCK_METHOD0(HasRecentConnectionIssues, bool());
  MOCK_METHOD0(AddSuspectedCredentialFailure, bool());
  MOCK_METHOD0(ResetSuspectedCredentialFailures, void());
  MOCK_METHOD1(AddEndpoint,
               void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(RemoveEndpoint,
               void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(NotifyCurrentEndpoint,
               void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(NotifyEndpointUpdated,
               void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD3(DisconnectWithFailure,
               void(ConnectFailure failure, Error* error, const char* reason));
  MOCK_METHOD1(IsActive, bool(Error* error));
  MOCK_CONST_METHOD0(IsConnected, bool());
  MOCK_CONST_METHOD0(IsConnecting, bool());
  MOCK_CONST_METHOD0(GetEndpointCount, int());
  MOCK_CONST_METHOD0(HasEndpoints, bool());
  MOCK_CONST_METHOD0(IsRemembered, bool());
  MOCK_METHOD0(ResetWiFi, void());
  MOCK_CONST_METHOD0(GetSupplicantConfigurationParameters, KeyValueStore());
  MOCK_CONST_METHOD1(IsAutoConnectable, bool(const char** reason));
  MOCK_CONST_METHOD0(HasStaticIPAddress, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiFiService);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_WIFI_SERVICE_H_

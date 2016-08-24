//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_WIFI_MOCK_WIFI_PROVIDER_H_
#define SHILL_WIFI_MOCK_WIFI_PROVIDER_H_

#include <gmock/gmock.h>

#include "shill/wifi/wifi_endpoint.h"
#include "shill/wifi/wifi_provider.h"

namespace shill {

class MockWiFiProvider : public WiFiProvider {
 public:
  MockWiFiProvider();
  ~MockWiFiProvider() override;

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD1(CreateServicesFromProfile, void(const ProfileRefPtr& profile));
  MOCK_CONST_METHOD2(FindSimilarService,
                     ServiceRefPtr(const KeyValueStore& args,
                                   Error* error));
  MOCK_METHOD2(CreateTemporaryService,
               ServiceRefPtr(const KeyValueStore& args, Error* error));
  MOCK_METHOD2(GetService, ServiceRefPtr(const KeyValueStore& args,
                                         Error* error));
  MOCK_METHOD1(FindServiceForEndpoint,
               WiFiServiceRefPtr(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(OnEndpointAdded, void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(OnEndpointRemoved,
               WiFiServiceRefPtr(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(OnEndpointUpdated,
               void(const WiFiEndpointConstRefPtr& endpoint));
  MOCK_METHOD1(OnServiceUnloaded, bool(const WiFiServiceRefPtr& service));
  MOCK_METHOD0(GetHiddenSSIDList, ByteArrays());
  MOCK_METHOD1(LoadAndFixupServiceEntries, void(Profile* storage));
  MOCK_CONST_METHOD1(Save, bool(StoreInterface* storage));
  MOCK_METHOD1(IncrementConnectCount, void(uint16_t frequency));
  MOCK_METHOD0(NumAutoConnectableServices, int());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWiFiProvider);
};

}  // namespace shill

#endif  // SHILL_WIFI_MOCK_WIFI_PROVIDER_H_

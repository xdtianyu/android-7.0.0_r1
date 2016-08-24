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

#ifndef SHILL_MOCK_PROFILE_H_
#define SHILL_MOCK_PROFILE_H_

#include <string>

#include <gmock/gmock.h>

#include "shill/profile.h"
#include "shill/wifi/wifi_provider.h"

namespace shill {

class MockProfile : public Profile {
 public:
  MockProfile(ControlInterface* control, Metrics* metrics, Manager* manager);
  MockProfile(ControlInterface* control,
              Metrics* metrics,
              Manager* manager,
              const std::string& identifier);
  ~MockProfile() override;

  MOCK_METHOD1(AdoptService, bool(const ServiceRefPtr& service));
  MOCK_METHOD1(AbandonService, bool(const ServiceRefPtr& service));
  MOCK_METHOD1(LoadService,  bool(const ServiceRefPtr& service));
  MOCK_METHOD1(ConfigureService,  bool(const ServiceRefPtr& service));
  MOCK_METHOD1(ConfigureDevice, bool(const DeviceRefPtr& device));
  MOCK_METHOD2(DeleteEntry,  void(const std::string& entry_name, Error* error));
  MOCK_METHOD0(GetRpcIdentifier, std::string());
  MOCK_METHOD1(UpdateService, bool(const ServiceRefPtr& service));
  MOCK_METHOD1(UpdateDevice, bool(const DeviceRefPtr& device));
  MOCK_METHOD1(UpdateWiFiProvider, bool(const WiFiProvider& wifi_provider));
  MOCK_METHOD0(Save, bool());
  MOCK_METHOD0(GetStorage, StoreInterface*());
  MOCK_CONST_METHOD0(GetConstStorage, const StoreInterface*());
  MOCK_CONST_METHOD0(IsDefault, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockProfile);
};

}  // namespace shill

#endif  // SHILL_MOCK_PROFILE_H_

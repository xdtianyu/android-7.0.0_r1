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

#ifndef APMANAGER_MOCK_DEVICE_H_
#define APMANAGER_MOCK_DEVICE_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "apmanager/device.h"

namespace apmanager {

class MockDevice : public Device {
 public:
  explicit MockDevice(Manager* manager);
  ~MockDevice() override;

  MOCK_METHOD1(RegisterInterface,
               void(const WiFiInterface& interface));
  MOCK_METHOD1(DeregisterInterface,
               void(const WiFiInterface& interface));
  MOCK_METHOD1(ParseWiphyCapability,
               void(const shill::Nl80211Message& msg));
  MOCK_METHOD1(ClaimDevice, bool(bool full_control));
  MOCK_METHOD0(ReleaseDevice, bool());
  MOCK_METHOD1(InterfaceExists, bool(const std::string& interface_name));
  MOCK_METHOD2(GetHTCapability, bool(uint16_t channel, std::string* ht_capab));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDevice);
};

}  // namespace apmanager

#endif  // APMANAGER_MOCK_DEVICE_H_

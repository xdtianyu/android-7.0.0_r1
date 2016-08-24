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

#ifndef SHILL_MOCK_DEVICE_CLAIMER_H_
#define SHILL_MOCK_DEVICE_CLAIMER_H_

#include "shill/device_claimer.h"

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

namespace shill {

class MockDeviceClaimer : public DeviceClaimer {
 public:
  explicit MockDeviceClaimer(const std::string& dbus_service_name);
  ~MockDeviceClaimer() override;

  MOCK_METHOD2(Claim, bool(const std::string& device_name, Error* error));
  MOCK_METHOD2(Release, bool(const std::string& device_name, Error* error));
  MOCK_METHOD0(DevicesClaimed, bool());
  MOCK_METHOD1(IsDeviceReleased, bool(const std::string& device_name));
  MOCK_CONST_METHOD0(default_claimer, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDeviceClaimer);
};

}  // namespace shill

#endif  // SHILL_MOCK_DEVICE_CLAIMER_H_

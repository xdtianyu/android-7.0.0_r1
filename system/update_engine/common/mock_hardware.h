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

#ifndef UPDATE_ENGINE_COMMON_MOCK_HARDWARE_H_
#define UPDATE_ENGINE_COMMON_MOCK_HARDWARE_H_

#include <string>

#include "update_engine/common/fake_hardware.h"

#include <gmock/gmock.h>

namespace chromeos_update_engine {

// A mocked, fake implementation of HardwareInterface.
class MockHardware : public HardwareInterface {
 public:
  MockHardware() {
    // Delegate all calls to the fake instance
    ON_CALL(*this, IsOfficialBuild())
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::IsOfficialBuild));
    ON_CALL(*this, IsNormalBootMode())
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::IsNormalBootMode));
    ON_CALL(*this, IsOOBEComplete(testing::_))
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::IsOOBEComplete));
    ON_CALL(*this, GetHardwareClass())
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::GetHardwareClass));
    ON_CALL(*this, GetFirmwareVersion())
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::GetFirmwareVersion));
    ON_CALL(*this, GetECVersion())
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::GetECVersion));
    ON_CALL(*this, GetPowerwashCount())
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::GetPowerwashCount));
    ON_CALL(*this, GetNonVolatileDirectory(testing::_))
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::GetNonVolatileDirectory));
    ON_CALL(*this, GetPowerwashSafeDirectory(testing::_))
      .WillByDefault(testing::Invoke(&fake_,
            &FakeHardware::GetPowerwashSafeDirectory));
  }

  ~MockHardware() override = default;

  // Hardware overrides.
  MOCK_CONST_METHOD0(IsOfficialBuild, bool());
  MOCK_CONST_METHOD0(IsNormalBootMode, bool());
  MOCK_CONST_METHOD1(IsOOBEComplete, bool(base::Time* out_time_of_oobe));
  MOCK_CONST_METHOD0(GetHardwareClass, std::string());
  MOCK_CONST_METHOD0(GetFirmwareVersion, std::string());
  MOCK_CONST_METHOD0(GetECVersion, std::string());
  MOCK_CONST_METHOD0(GetPowerwashCount, int());
  MOCK_CONST_METHOD1(GetNonVolatileDirectory, bool(base::FilePath*));
  MOCK_CONST_METHOD1(GetPowerwashSafeDirectory, bool(base::FilePath*));

  // Returns a reference to the underlying FakeHardware.
  FakeHardware& fake() {
    return fake_;
  }

 private:
  // The underlying FakeHardware.
  FakeHardware fake_;

  DISALLOW_COPY_AND_ASSIGN(MockHardware);
};


}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_COMMON_MOCK_HARDWARE_H_

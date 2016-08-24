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

#include "update_engine/update_manager/real_system_provider.h"

#include <memory>

#include <base/time/time.h>
#include <gtest/gtest.h>

#include "update_engine/common/fake_boot_control.h"
#include "update_engine/common/fake_hardware.h"
#include "update_engine/update_manager/umtest_utils.h"

using std::unique_ptr;

namespace chromeos_update_manager {

class UmRealSystemProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    provider_.reset(
        new RealSystemProvider(&fake_hardware_, &fake_boot_control_));
    EXPECT_TRUE(provider_->Init());
  }

  chromeos_update_engine::FakeHardware fake_hardware_;
  chromeos_update_engine::FakeBootControl fake_boot_control_;
  unique_ptr<RealSystemProvider> provider_;
};

TEST_F(UmRealSystemProviderTest, InitTest) {
  EXPECT_NE(nullptr, provider_->var_is_normal_boot_mode());
  EXPECT_NE(nullptr, provider_->var_is_official_build());
  EXPECT_NE(nullptr, provider_->var_is_oobe_complete());
}

TEST_F(UmRealSystemProviderTest, IsOOBECompleteTrue) {
  fake_hardware_.SetIsOOBEComplete(base::Time());
  UmTestUtils::ExpectVariableHasValue(true, provider_->var_is_oobe_complete());
}

TEST_F(UmRealSystemProviderTest, IsOOBECompleteFalse) {
  fake_hardware_.UnsetIsOOBEComplete();
  UmTestUtils::ExpectVariableHasValue(false, provider_->var_is_oobe_complete());
}

}  // namespace chromeos_update_manager

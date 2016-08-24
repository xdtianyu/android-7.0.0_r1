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

#include "update_engine/update_manager/real_config_provider.h"

#include <memory>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/fake_hardware.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/update_manager/umtest_utils.h"

using base::TimeDelta;
using chromeos_update_engine::test_utils::WriteFileString;
using std::string;
using std::unique_ptr;

namespace chromeos_update_manager {

class UmRealConfigProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(root_dir_.CreateUniqueTempDir());
    provider_.reset(new RealConfigProvider(&fake_hardware_));
    provider_->SetRootPrefix(root_dir_.path().value());
  }

  void WriteStatefulConfig(const string& config) {
    base::FilePath kFile(root_dir_.path().value()
                         + chromeos_update_engine::kStatefulPartition
                         + "/etc/update_manager.conf");
    ASSERT_TRUE(base::CreateDirectory(kFile.DirName()));
    ASSERT_TRUE(WriteFileString(kFile.value(), config));
  }

  void WriteRootfsConfig(const string& config) {
    base::FilePath kFile(root_dir_.path().value()
                         + "/etc/update_manager.conf");
    ASSERT_TRUE(base::CreateDirectory(kFile.DirName()));
    ASSERT_TRUE(WriteFileString(kFile.value(), config));
  }

  unique_ptr<RealConfigProvider> provider_;
  chromeos_update_engine::FakeHardware fake_hardware_;
  TimeDelta default_timeout_ = TimeDelta::FromSeconds(1);
  base::ScopedTempDir root_dir_;
};

TEST_F(UmRealConfigProviderTest, InitTest) {
  EXPECT_TRUE(provider_->Init());
  EXPECT_NE(nullptr, provider_->var_is_oobe_enabled());
}

TEST_F(UmRealConfigProviderTest, NoFileFoundReturnsDefault) {
  EXPECT_TRUE(provider_->Init());
  UmTestUtils::ExpectVariableHasValue(true, provider_->var_is_oobe_enabled());
}

TEST_F(UmRealConfigProviderTest, DontReadStatefulInNormalMode) {
  fake_hardware_.SetIsNormalBootMode(true);
  WriteStatefulConfig("is_oobe_enabled=false");

  EXPECT_TRUE(provider_->Init());
  UmTestUtils::ExpectVariableHasValue(true, provider_->var_is_oobe_enabled());
}

TEST_F(UmRealConfigProviderTest, ReadStatefulInDevMode) {
  fake_hardware_.SetIsNormalBootMode(false);
  WriteRootfsConfig("is_oobe_enabled=true");
  // Since the stateful is present, this should read that one.
  WriteStatefulConfig("is_oobe_enabled=false");

  EXPECT_TRUE(provider_->Init());
  UmTestUtils::ExpectVariableHasValue(false, provider_->var_is_oobe_enabled());
}

TEST_F(UmRealConfigProviderTest, ReadRootfsIfStatefulNotFound) {
  fake_hardware_.SetIsNormalBootMode(false);
  WriteRootfsConfig("is_oobe_enabled=false");

  EXPECT_TRUE(provider_->Init());
  UmTestUtils::ExpectVariableHasValue(false, provider_->var_is_oobe_enabled());
}

}  // namespace chromeos_update_manager

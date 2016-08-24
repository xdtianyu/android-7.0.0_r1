//
// Copyright (C) 2011 The Android Open Source Project
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

#include "update_engine/omaha_request_params.h"

#include <stdio.h>

#include <string>

#include <base/files/file_util.h>
#include <gtest/gtest.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/common/utils.h"
#include "update_engine/fake_system_state.h"
#include "update_engine/payload_consumer/install_plan.h"

using chromeos_update_engine::test_utils::WriteFileString;
using std::string;

namespace chromeos_update_engine {

class OmahaRequestParamsTest : public ::testing::Test {
 public:
  OmahaRequestParamsTest() : params_(&fake_system_state_) {}

 protected:
  // Return true iff the OmahaRequestParams::Init succeeded. If
  // out is non-null, it's set w/ the generated data.
  bool DoTest(OmahaRequestParams* out, const string& app_version,
              const string& omaha_url);

  void SetUp() override {
    // Create a uniquely named test directory.
    ASSERT_TRUE(utils::MakeTempDirectory(kTestDirTemplate, &test_dir_));
    EXPECT_TRUE(base::CreateDirectory(base::FilePath(test_dir_ + "/etc")));
    EXPECT_TRUE(base::CreateDirectory(
        base::FilePath(test_dir_ + kStatefulPartition + "/etc")));
    // Create a fresh copy of the params for each test, so there's no
    // unintended reuse of state across tests.
    OmahaRequestParams new_params(&fake_system_state_);
    params_ = new_params;
    params_.set_root(test_dir_);
    SetLockDown(false);
  }

  void TearDown() override {
    EXPECT_TRUE(base::DeleteFile(base::FilePath(test_dir_), true));
  }

  void SetLockDown(bool locked_down) {
    fake_system_state_.fake_hardware()->SetIsOfficialBuild(locked_down);
    fake_system_state_.fake_hardware()->SetIsNormalBootMode(locked_down);
  }

  OmahaRequestParams params_;
  FakeSystemState fake_system_state_;

  static const char* kTestDirTemplate;
  string test_dir_;
};

const char* OmahaRequestParamsTest::kTestDirTemplate =
  "omaha_request_params-test-XXXXXX";

bool OmahaRequestParamsTest::DoTest(OmahaRequestParams* out,
                                    const string& app_version,
                                    const string& omaha_url) {
  bool success = params_.Init(app_version, omaha_url, false);
  if (out)
    *out = params_;
  return success;
}

namespace {
string GetMachineType() {
  string machine_type;
  if (!utils::ReadPipe("uname -m", &machine_type))
    return "";
  // Strip anything from the first newline char.
  size_t newline_pos = machine_type.find('\n');
  if (newline_pos != string::npos)
    machine_type.erase(newline_pos);
  return machine_type;
}
}  // namespace

TEST_F(OmahaRequestParamsTest, SimpleTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_EQ(fake_system_state_.hardware()->GetHardwareClass(), out.hwid());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_EQ("http://www.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, AppIDTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_RELEASE_APPID={58c35cef-9d30-476e-9098-ce20377d535d}\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{58c35cef-9d30-476e-9098-ce20377d535d}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_EQ(fake_system_state_.hardware()->GetHardwareClass(), out.hwid());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_EQ("http://www.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, MissingChannelTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRXCK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  // By default, if no channel is set, we should track the stable-channel.
  EXPECT_EQ("stable-channel", out.target_channel());
}

TEST_F(OmahaRequestParamsTest, ConfusingReleaseTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_FOO=CHROMEOS_RELEASE_VERSION=1.2.3.4\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRXCK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_EQ("stable-channel", out.target_channel());
}

TEST_F(OmahaRequestParamsTest, MissingVersionTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
}

TEST_F(OmahaRequestParamsTest, ForceVersionTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "ForcedVersion", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("ForcedVersion_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("ForcedVersion", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
}

TEST_F(OmahaRequestParamsTest, ForcedURLTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", "http://forced.google.com"));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_EQ("http://forced.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, MissingURLTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_EQ(constants::kOmahaDefaultProductionURL, out.update_url());
}

TEST_F(OmahaRequestParamsTest, NoDeltasTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_FOO=CHROMEOS_RELEASE_VERSION=1.2.3.4\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRXCK=dev-channel"));
  ASSERT_TRUE(WriteFileString(test_dir_ + "/.nodelta", ""));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_FALSE(out.delta_okay());
}

TEST_F(OmahaRequestParamsTest, OverrideTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  ASSERT_TRUE(WriteFileString(
      test_dir_ + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=x86-generic\n"
      "CHROMEOS_RELEASE_TRACK=beta-channel\n"
      "CHROMEOS_AUSERVER=https://www.google.com"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("x86-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_EQ(fake_system_state_.hardware()->GetHardwareClass(), out.hwid());
  EXPECT_FALSE(out.delta_okay());
  EXPECT_EQ("beta-channel", out.target_channel());
  EXPECT_EQ("https://www.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, OverrideLockDownTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=https://www.google.com"));
  ASSERT_TRUE(WriteFileString(
      test_dir_ + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=x86-generic\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  SetLockDown(true);
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ(fake_system_state_.hardware()->GetHardwareClass(), out.hwid());
  EXPECT_FALSE(out.delta_okay());
  EXPECT_EQ("stable-channel", out.target_channel());
  EXPECT_EQ("https://www.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, OverrideSameChannelTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  ASSERT_TRUE(WriteFileString(
      test_dir_ + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=x86-generic\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("x86-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ(fake_system_state_.hardware()->GetHardwareClass(), out.hwid());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_EQ("http://www.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, SetTargetChannelTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  {
    OmahaRequestParams params(&fake_system_state_);
    params.set_root(test_dir_);
    EXPECT_TRUE(params.Init("", "", false));
    params.SetTargetChannel("canary-channel", false, nullptr);
    EXPECT_FALSE(params.is_powerwash_allowed());
  }
  OmahaRequestParams out(&fake_system_state_);
  out.set_root(test_dir_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("canary-channel", out.target_channel());
  EXPECT_FALSE(out.is_powerwash_allowed());
}

TEST_F(OmahaRequestParamsTest, SetIsPowerwashAllowedTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  {
    OmahaRequestParams params(&fake_system_state_);
    params.set_root(test_dir_);
    EXPECT_TRUE(params.Init("", "", false));
    params.SetTargetChannel("canary-channel", true, nullptr);
    EXPECT_TRUE(params.is_powerwash_allowed());
  }
  OmahaRequestParams out(&fake_system_state_);
  out.set_root(test_dir_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("canary-channel", out.target_channel());
  EXPECT_TRUE(out.is_powerwash_allowed());
}

TEST_F(OmahaRequestParamsTest, SetTargetChannelInvalidTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  {
    OmahaRequestParams params(&fake_system_state_);
    params.set_root(test_dir_);
    SetLockDown(true);
    EXPECT_TRUE(params.Init("", "", false));
    string error_message;
    EXPECT_FALSE(
        params.SetTargetChannel("dogfood-channel", true, &error_message));
    // The error message should include a message about the valid channels.
    EXPECT_NE(string::npos, error_message.find("stable-channel"));
    EXPECT_FALSE(params.is_powerwash_allowed());
  }
  OmahaRequestParams out(&fake_system_state_);
  out.set_root(test_dir_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_FALSE(out.is_powerwash_allowed());
}

TEST_F(OmahaRequestParamsTest, IsValidChannelTest) {
  EXPECT_TRUE(params_.IsValidChannel("canary-channel"));
  EXPECT_TRUE(params_.IsValidChannel("stable-channel"));
  EXPECT_TRUE(params_.IsValidChannel("beta-channel"));
  EXPECT_TRUE(params_.IsValidChannel("dev-channel"));
  EXPECT_FALSE(params_.IsValidChannel("testimage-channel"));
  EXPECT_FALSE(params_.IsValidChannel("dogfood-channel"));
  EXPECT_FALSE(params_.IsValidChannel("some-channel"));
  EXPECT_FALSE(params_.IsValidChannel(""));
}

TEST_F(OmahaRequestParamsTest, ValidChannelTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  SetLockDown(true);
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("Chrome OS", out.os_platform());
  EXPECT_EQ(string("0.2.2.3_") + GetMachineType(), out.os_sp());
  EXPECT_EQ("arm-generic", out.os_board());
  EXPECT_EQ("{87efface-864d-49a5-9bb3-4b050a7c227a}", out.GetAppId());
  EXPECT_EQ("0.2.2.3", out.app_version());
  EXPECT_EQ("en-US", out.app_lang());
  EXPECT_EQ(fake_system_state_.hardware()->GetHardwareClass(), out.hwid());
  EXPECT_TRUE(out.delta_okay());
  EXPECT_EQ("dev-channel", out.target_channel());
  EXPECT_EQ("http://www.google.com", out.update_url());
}

TEST_F(OmahaRequestParamsTest, SetTargetChannelWorks) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=dev-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));

  // Check LSB value is used by default when SetTargetChannel is not called.
  params_.Init("", "", false);
  EXPECT_EQ("dev-channel", params_.target_channel());

  // When an invalid value is set, it should be ignored and the
  // value from lsb-release should be used instead.
  params_.Init("", "", false);
  EXPECT_FALSE(params_.SetTargetChannel("invalid-channel", false, nullptr));
  EXPECT_EQ("dev-channel", params_.target_channel());

  // When set to a valid value, it should take effect.
  params_.Init("", "", false);
  EXPECT_TRUE(params_.SetTargetChannel("beta-channel", true, nullptr));
  EXPECT_EQ("beta-channel", params_.target_channel());

  // When set to the same value, it should be idempotent.
  params_.Init("", "", false);
  EXPECT_TRUE(params_.SetTargetChannel("beta-channel", true, nullptr));
  EXPECT_EQ("beta-channel", params_.target_channel());

  // When set to a valid value while a change is already pending, it should
  // succeed.
  params_.Init("", "", false);
  EXPECT_TRUE(params_.SetTargetChannel("stable-channel", true, nullptr));
  EXPECT_EQ("stable-channel", params_.target_channel());

  // Set a different channel in stateful LSB release.
  ASSERT_TRUE(WriteFileString(
      test_dir_ + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"
      "CHROMEOS_IS_POWERWASH_ALLOWED=true\n"));

  // When set to a valid value while a change is already pending, it should
  // succeed.
  params_.Init("", "", false);
  EXPECT_TRUE(params_.SetTargetChannel("beta-channel", true, nullptr));
  // The target channel should reflect the change, but the download channel
  // should continue to retain the old value ...
  EXPECT_EQ("beta-channel", params_.target_channel());
  EXPECT_EQ("stable-channel", params_.download_channel());

  // ... until we update the download channel explicitly.
  params_.UpdateDownloadChannel();
  EXPECT_EQ("beta-channel", params_.download_channel());
  EXPECT_EQ("beta-channel", params_.target_channel());
}

TEST_F(OmahaRequestParamsTest, ChannelIndexTest) {
  int canary = params_.GetChannelIndex("canary-channel");
  int dev = params_.GetChannelIndex("dev-channel");
  int beta = params_.GetChannelIndex("beta-channel");
  int stable = params_.GetChannelIndex("stable-channel");
  EXPECT_LE(canary, dev);
  EXPECT_LE(dev, beta);
  EXPECT_LE(beta, stable);

  // testimage-channel or other names are not recognized, so index will be -1.
  int testimage = params_.GetChannelIndex("testimage-channel");
  int bogus = params_.GetChannelIndex("bogus-channel");
  EXPECT_EQ(-1, testimage);
  EXPECT_EQ(-1, bogus);
}

TEST_F(OmahaRequestParamsTest, ToMoreStableChannelFlagTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=arm-generic\n"
      "CHROMEOS_RELEASE_FOO=bar\n"
      "CHROMEOS_RELEASE_VERSION=0.2.2.3\n"
      "CHROMEOS_RELEASE_TRACK=canary-channel\n"
      "CHROMEOS_AUSERVER=http://www.google.com"));
  ASSERT_TRUE(WriteFileString(
      test_dir_ + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_BOARD=x86-generic\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"
      "CHROMEOS_AUSERVER=https://www.google.com"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("https://www.google.com", out.update_url());
  EXPECT_FALSE(out.delta_okay());
  EXPECT_EQ("stable-channel", out.target_channel());
  EXPECT_TRUE(out.to_more_stable_channel());
}

TEST_F(OmahaRequestParamsTest, BoardAppIdUsedForNonCanaryChannelTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_APPID=r\n"
      "CHROMEOS_BOARD_APPID=b\n"
      "CHROMEOS_CANARY_APPID=c\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("stable-channel", out.download_channel());
  EXPECT_EQ("b", out.GetAppId());
}

TEST_F(OmahaRequestParamsTest, CanaryAppIdUsedForCanaryChannelTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_APPID=r\n"
      "CHROMEOS_BOARD_APPID=b\n"
      "CHROMEOS_CANARY_APPID=c\n"
      "CHROMEOS_RELEASE_TRACK=canary-channel\n"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("canary-channel", out.download_channel());
  EXPECT_EQ("c", out.GetAppId());
}

TEST_F(OmahaRequestParamsTest, ReleaseAppIdUsedAsDefaultTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_APPID=r\n"
      "CHROMEOS_CANARY_APPID=c\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));
  OmahaRequestParams out(&fake_system_state_);
  EXPECT_TRUE(DoTest(&out, "", ""));
  EXPECT_EQ("stable-channel", out.download_channel());
  EXPECT_EQ("r", out.GetAppId());
}

TEST_F(OmahaRequestParamsTest, CollectECFWVersionsTest) {
  ASSERT_TRUE(WriteFileString(
      test_dir_ + "/etc/lsb-release",
      "CHROMEOS_RELEASE_APPID=r\n"
      "CHROMEOS_CANARY_APPID=c\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));
  OmahaRequestParams out(&fake_system_state_);
  out.hwid_ = string("STUMPY ALEX 12345");
  EXPECT_FALSE(out.CollectECFWVersions());

  out.hwid_ = string("SNOW 12345");
  EXPECT_TRUE(out.CollectECFWVersions());

  out.hwid_ = string("SAMS ALEX 12345");
  EXPECT_TRUE(out.CollectECFWVersions());
}

}  // namespace chromeos_update_engine

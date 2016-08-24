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

#include "update_engine/omaha_response_handler_action.h"

#include <string>

#include <base/files/file_util.h>
#include <gtest/gtest.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/common/utils.h"
#include "update_engine/fake_system_state.h"
#include "update_engine/mock_payload_state.h"
#include "update_engine/payload_consumer/payload_constants.h"

using chromeos_update_engine::test_utils::System;
using chromeos_update_engine::test_utils::WriteFileString;
using std::string;
using testing::Return;

namespace chromeos_update_engine {

class OmahaResponseHandlerActionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FakeBootControl* fake_boot_control = fake_system_state_.fake_boot_control();
    fake_boot_control->SetPartitionDevice(
        kLegacyPartitionNameKernel, 0, "/dev/sdz2");
    fake_boot_control->SetPartitionDevice(
        kLegacyPartitionNameRoot, 0, "/dev/sdz3");
    fake_boot_control->SetPartitionDevice(
        kLegacyPartitionNameKernel, 1, "/dev/sdz4");
    fake_boot_control->SetPartitionDevice(
        kLegacyPartitionNameRoot, 1, "/dev/sdz5");
  }

  // Return true iff the OmahaResponseHandlerAction succeeded.
  // If out is non-null, it's set w/ the response from the action.
  bool DoTest(const OmahaResponse& in,
              const string& deadline_file,
              InstallPlan* out);

  FakeSystemState fake_system_state_;
};

class OmahaResponseHandlerActionProcessorDelegate
    : public ActionProcessorDelegate {
 public:
  OmahaResponseHandlerActionProcessorDelegate()
      : code_(ErrorCode::kError),
        code_set_(false) {}
  void ActionCompleted(ActionProcessor* processor,
                       AbstractAction* action,
                       ErrorCode code) {
    if (action->Type() == OmahaResponseHandlerAction::StaticType()) {
      code_ = code;
      code_set_ = true;
    }
  }
  ErrorCode code_;
  bool code_set_;
};

namespace {
const char* const kLongName =
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "-the_update_a.b.c.d_DELTA_.tgz";
const char* const kBadVersion = "don't update me";
}  // namespace

bool OmahaResponseHandlerActionTest::DoTest(
    const OmahaResponse& in,
    const string& test_deadline_file,
    InstallPlan* out) {
  ActionProcessor processor;
  OmahaResponseHandlerActionProcessorDelegate delegate;
  processor.set_delegate(&delegate);

  ObjectFeederAction<OmahaResponse> feeder_action;
  feeder_action.set_obj(in);
  if (in.update_exists && in.version != kBadVersion) {
    EXPECT_CALL(*(fake_system_state_.mock_prefs()),
                SetString(kPrefsUpdateCheckResponseHash, in.hash))
        .WillOnce(Return(true));

    int slot = 1 - fake_system_state_.fake_boot_control()->GetCurrentSlot();
    string key = kPrefsChannelOnSlotPrefix + std::to_string(slot);
    EXPECT_CALL(*(fake_system_state_.mock_prefs()), SetString(key, testing::_))
        .WillOnce(Return(true));
  }

  string current_url = in.payload_urls.size() ? in.payload_urls[0] : "";
  EXPECT_CALL(*(fake_system_state_.mock_payload_state()), GetCurrentUrl())
      .WillRepeatedly(Return(current_url));

  OmahaResponseHandlerAction response_handler_action(
      &fake_system_state_,
      (test_deadline_file.empty() ?
       constants::kOmahaResponseDeadlineFile : test_deadline_file));
  BondActions(&feeder_action, &response_handler_action);
  ObjectCollectorAction<InstallPlan> collector_action;
  BondActions(&response_handler_action, &collector_action);
  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&response_handler_action);
  processor.EnqueueAction(&collector_action);
  processor.StartProcessing();
  EXPECT_TRUE(!processor.IsRunning())
      << "Update test to handle non-async actions";
  if (out)
    *out = collector_action.object();
  EXPECT_TRUE(delegate.code_set_);
  return delegate.code_ == ErrorCode::kSuccess;
}

TEST_F(OmahaResponseHandlerActionTest, SimpleTest) {
  string test_deadline_file;
  CHECK(utils::MakeTempFile(
          "omaha_response_handler_action_unittest-XXXXXX",
          &test_deadline_file, nullptr));
  ScopedPathUnlinker deadline_unlinker(test_deadline_file);
  {
    OmahaResponse in;
    in.update_exists = true;
    in.version = "a.b.c.d";
    in.payload_urls.push_back("http://foo/the_update_a.b.c.d.tgz");
    in.more_info_url = "http://more/info";
    in.hash = "HASH+";
    in.size = 12;
    in.prompt = false;
    in.deadline = "20101020";
    InstallPlan install_plan;
    EXPECT_TRUE(DoTest(in, test_deadline_file, &install_plan));
    EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
    EXPECT_EQ(in.hash, install_plan.payload_hash);
    EXPECT_EQ(1U, install_plan.target_slot);
    string deadline;
    EXPECT_TRUE(utils::ReadFile(test_deadline_file, &deadline));
    EXPECT_EQ("20101020", deadline);
    struct stat deadline_stat;
    EXPECT_EQ(0, stat(test_deadline_file.c_str(), &deadline_stat));
    EXPECT_EQ(
        static_cast<mode_t>(S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
        deadline_stat.st_mode);
    EXPECT_EQ(in.version, install_plan.version);
  }
  {
    OmahaResponse in;
    in.update_exists = true;
    in.version = "a.b.c.d";
    in.payload_urls.push_back("http://foo/the_update_a.b.c.d.tgz");
    in.more_info_url = "http://more/info";
    in.hash = "HASHj+";
    in.size = 12;
    in.prompt = true;
    InstallPlan install_plan;
    // Set the other slot as current.
    fake_system_state_.fake_boot_control()->SetCurrentSlot(1);
    EXPECT_TRUE(DoTest(in, test_deadline_file, &install_plan));
    EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
    EXPECT_EQ(in.hash, install_plan.payload_hash);
    EXPECT_EQ(0U, install_plan.target_slot);
    string deadline;
    EXPECT_TRUE(utils::ReadFile(test_deadline_file, &deadline) &&
                deadline.empty());
    EXPECT_EQ(in.version, install_plan.version);
  }
  {
    OmahaResponse in;
    in.update_exists = true;
    in.version = "a.b.c.d";
    in.payload_urls.push_back(kLongName);
    in.more_info_url = "http://more/info";
    in.hash = "HASHj+";
    in.size = 12;
    in.prompt = true;
    in.deadline = "some-deadline";
    InstallPlan install_plan;
    fake_system_state_.fake_boot_control()->SetCurrentSlot(0);
    EXPECT_TRUE(DoTest(in, test_deadline_file, &install_plan));
    EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
    EXPECT_EQ(in.hash, install_plan.payload_hash);
    EXPECT_EQ(1U, install_plan.target_slot);
    string deadline;
    EXPECT_TRUE(utils::ReadFile(test_deadline_file, &deadline));
    EXPECT_EQ("some-deadline", deadline);
    EXPECT_EQ(in.version, install_plan.version);
  }
}

TEST_F(OmahaResponseHandlerActionTest, NoUpdatesTest) {
  OmahaResponse in;
  in.update_exists = false;
  InstallPlan install_plan;
  EXPECT_FALSE(DoTest(in, "", &install_plan));
  EXPECT_TRUE(install_plan.partitions.empty());
}

TEST_F(OmahaResponseHandlerActionTest, HashChecksForHttpTest) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("http://test.should/need/hash.checks.signed");
  in.more_info_url = "http://more/info";
  in.hash = "HASHj+";
  in.size = 12;
  // Hash checks are always skipped for non-official update URLs.
  EXPECT_CALL(*(fake_system_state_.mock_request_params()),
              IsUpdateUrlOfficial())
      .WillRepeatedly(Return(true));
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
  EXPECT_EQ(in.hash, install_plan.payload_hash);
  EXPECT_TRUE(install_plan.hash_checks_mandatory);
  EXPECT_EQ(in.version, install_plan.version);
}

TEST_F(OmahaResponseHandlerActionTest, HashChecksForUnofficialUpdateUrl) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("http://url.normally/needs/hash.checks.signed");
  in.more_info_url = "http://more/info";
  in.hash = "HASHj+";
  in.size = 12;
  EXPECT_CALL(*(fake_system_state_.mock_request_params()),
              IsUpdateUrlOfficial())
      .WillRepeatedly(Return(false));
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
  EXPECT_EQ(in.hash, install_plan.payload_hash);
  EXPECT_FALSE(install_plan.hash_checks_mandatory);
  EXPECT_EQ(in.version, install_plan.version);
}

TEST_F(OmahaResponseHandlerActionTest,
       HashChecksForOfficialUrlUnofficialBuildTest) {
  // Official URLs for unofficial builds (dev/test images) don't require hash.
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("http://url.normally/needs/hash.checks.signed");
  in.more_info_url = "http://more/info";
  in.hash = "HASHj+";
  in.size = 12;
  EXPECT_CALL(*(fake_system_state_.mock_request_params()),
              IsUpdateUrlOfficial())
      .WillRepeatedly(Return(true));
  fake_system_state_.fake_hardware()->SetIsOfficialBuild(false);
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
  EXPECT_EQ(in.hash, install_plan.payload_hash);
  EXPECT_FALSE(install_plan.hash_checks_mandatory);
  EXPECT_EQ(in.version, install_plan.version);
}

TEST_F(OmahaResponseHandlerActionTest, HashChecksForHttpsTest) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("https://test.should.not/need/hash.checks.signed");
  in.more_info_url = "http://more/info";
  in.hash = "HASHj+";
  in.size = 12;
  EXPECT_CALL(*(fake_system_state_.mock_request_params()),
              IsUpdateUrlOfficial())
      .WillRepeatedly(Return(true));
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
  EXPECT_EQ(in.hash, install_plan.payload_hash);
  EXPECT_FALSE(install_plan.hash_checks_mandatory);
  EXPECT_EQ(in.version, install_plan.version);
}

TEST_F(OmahaResponseHandlerActionTest, HashChecksForBothHttpAndHttpsTest) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("http://test.should.still/need/hash.checks");
  in.payload_urls.push_back("https://test.should.still/need/hash.checks");
  in.more_info_url = "http://more/info";
  in.hash = "HASHj+";
  in.size = 12;
  EXPECT_CALL(*(fake_system_state_.mock_request_params()),
              IsUpdateUrlOfficial())
      .WillRepeatedly(Return(true));
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
  EXPECT_EQ(in.hash, install_plan.payload_hash);
  EXPECT_TRUE(install_plan.hash_checks_mandatory);
  EXPECT_EQ(in.version, install_plan.version);
}

TEST_F(OmahaResponseHandlerActionTest, ChangeToMoreStableChannelTest) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("https://MoreStableChannelTest");
  in.more_info_url = "http://more/info";
  in.hash = "HASHjk";
  in.size = 15;

  // Create a uniquely named test directory.
  string test_dir;
  ASSERT_TRUE(utils::MakeTempDirectory(
          "omaha_response_handler_action-test-XXXXXX", &test_dir));

  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir + "/etc"));
  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir +
                      kStatefulPartition + "/etc"));
  ASSERT_TRUE(WriteFileString(
      test_dir + "/etc/lsb-release",
      "CHROMEOS_RELEASE_TRACK=canary-channel\n"));
  ASSERT_TRUE(WriteFileString(
      test_dir + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_IS_POWERWASH_ALLOWED=true\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));

  OmahaRequestParams params(&fake_system_state_);
  fake_system_state_.fake_hardware()->SetIsOfficialBuild(false);
  params.set_root(test_dir);
  params.Init("1.2.3.4", "", 0);
  EXPECT_EQ("canary-channel", params.current_channel());
  EXPECT_EQ("stable-channel", params.target_channel());
  EXPECT_TRUE(params.to_more_stable_channel());
  EXPECT_TRUE(params.is_powerwash_allowed());

  fake_system_state_.set_request_params(&params);
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_TRUE(install_plan.powerwash_required);

  ASSERT_TRUE(base::DeleteFile(base::FilePath(test_dir), true));
}

TEST_F(OmahaResponseHandlerActionTest, ChangeToLessStableChannelTest) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("https://LessStableChannelTest");
  in.more_info_url = "http://more/info";
  in.hash = "HASHjk";
  in.size = 15;

  // Create a uniquely named test directory.
  string test_dir;
  ASSERT_TRUE(utils::MakeTempDirectory(
          "omaha_response_handler_action-test-XXXXXX", &test_dir));

  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir + "/etc"));
  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir +
                      kStatefulPartition + "/etc"));
  ASSERT_TRUE(WriteFileString(
      test_dir + "/etc/lsb-release",
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));
  ASSERT_TRUE(WriteFileString(
      test_dir + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_TRACK=canary-channel\n"));

  OmahaRequestParams params(&fake_system_state_);
  fake_system_state_.fake_hardware()->SetIsOfficialBuild(false);
  params.set_root(test_dir);
  params.Init("5.6.7.8", "", 0);
  EXPECT_EQ("stable-channel", params.current_channel());
  params.SetTargetChannel("canary-channel", false, nullptr);
  EXPECT_EQ("canary-channel", params.target_channel());
  EXPECT_FALSE(params.to_more_stable_channel());
  EXPECT_FALSE(params.is_powerwash_allowed());

  fake_system_state_.set_request_params(&params);
  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_FALSE(install_plan.powerwash_required);

  ASSERT_TRUE(base::DeleteFile(base::FilePath(test_dir), true));
}

TEST_F(OmahaResponseHandlerActionTest, P2PUrlIsUsedAndHashChecksMandatory) {
  OmahaResponse in;
  in.update_exists = true;
  in.version = "a.b.c.d";
  in.payload_urls.push_back("https://would.not/cause/hash/checks");
  in.more_info_url = "http://more/info";
  in.hash = "HASHj+";
  in.size = 12;

  OmahaRequestParams params(&fake_system_state_);
  // We're using a real OmahaRequestParams object here so we can't mock
  // IsUpdateUrlOfficial(), but setting the update URL to the AutoUpdate test
  // server will cause IsUpdateUrlOfficial() to return true.
  params.set_update_url(constants::kOmahaDefaultAUTestURL);
  fake_system_state_.set_request_params(&params);

  EXPECT_CALL(*fake_system_state_.mock_payload_state(),
              SetUsingP2PForDownloading(true));

  string p2p_url = "http://9.8.7.6/p2p";
  EXPECT_CALL(*fake_system_state_.mock_payload_state(), GetP2PUrl())
      .WillRepeatedly(Return(p2p_url));
  EXPECT_CALL(*fake_system_state_.mock_payload_state(),
              GetUsingP2PForDownloading()).WillRepeatedly(Return(true));

  InstallPlan install_plan;
  EXPECT_TRUE(DoTest(in, "", &install_plan));
  EXPECT_EQ(in.hash, install_plan.payload_hash);
  EXPECT_EQ(install_plan.download_url, p2p_url);
  EXPECT_TRUE(install_plan.hash_checks_mandatory);
}

}  // namespace chromeos_update_engine

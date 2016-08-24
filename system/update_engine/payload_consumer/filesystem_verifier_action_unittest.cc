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

#include "update_engine/payload_consumer/filesystem_verifier_action.h"

#include <fcntl.h>

#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/posix/eintr_wrapper.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/bind_lambda.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "update_engine/common/fake_boot_control.h"
#include "update_engine/common/hash_calculator.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/payload_constants.h"

using brillo::MessageLoop;
using std::set;
using std::string;
using std::vector;

namespace chromeos_update_engine {

class FilesystemVerifierActionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_.SetAsCurrent();
  }

  void TearDown() override {
    EXPECT_EQ(0, brillo::MessageLoopRunMaxIterations(&loop_, 1));
  }

  // Returns true iff test has completed successfully.
  bool DoTest(bool terminate_early,
              bool hash_fail,
              VerifierMode verifier_mode);

  brillo::FakeMessageLoop loop_{nullptr};
  FakeBootControl fake_boot_control_;
};

class FilesystemVerifierActionTestDelegate : public ActionProcessorDelegate {
 public:
  explicit FilesystemVerifierActionTestDelegate(
      FilesystemVerifierAction* action)
      : action_(action), ran_(false), code_(ErrorCode::kError) {}
  void ExitMainLoop() {
    // We need to wait for the Action to call Cleanup.
    if (action_->IsCleanupPending()) {
      LOG(INFO) << "Waiting for Cleanup() to be called.";
      MessageLoop::current()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&FilesystemVerifierActionTestDelegate::ExitMainLoop,
                     base::Unretained(this)),
          base::TimeDelta::FromMilliseconds(100));
    } else {
      MessageLoop::current()->BreakLoop();
    }
  }
  void ProcessingDone(const ActionProcessor* processor, ErrorCode code) {
    ExitMainLoop();
  }
  void ProcessingStopped(const ActionProcessor* processor) {
    ExitMainLoop();
  }
  void ActionCompleted(ActionProcessor* processor,
                       AbstractAction* action,
                       ErrorCode code) {
    if (action->Type() == FilesystemVerifierAction::StaticType()) {
      ran_ = true;
      code_ = code;
    }
  }
  bool ran() const { return ran_; }
  ErrorCode code() const { return code_; }

 private:
  FilesystemVerifierAction* action_;
  bool ran_;
  ErrorCode code_;
};

void StartProcessorInRunLoop(ActionProcessor* processor,
                             FilesystemVerifierAction* filesystem_copier_action,
                             bool terminate_early) {
  processor->StartProcessing();
  if (terminate_early) {
    EXPECT_NE(nullptr, filesystem_copier_action);
    processor->StopProcessing();
  }
}

// TODO(garnold) Temporarily disabling this test, see chromium-os:31082 for
// details; still trying to track down the root cause for these rare write
// failures and whether or not they are due to the test setup or an inherent
// issue with the chroot environment, library versions we use, etc.
TEST_F(FilesystemVerifierActionTest, DISABLED_RunAsRootSimpleTest) {
  ASSERT_EQ(0U, getuid());
  bool test = DoTest(false, false, VerifierMode::kComputeSourceHash);
  EXPECT_TRUE(test);
  if (!test)
    return;
  test = DoTest(false, false, VerifierMode::kVerifyTargetHash);
  EXPECT_TRUE(test);
}

bool FilesystemVerifierActionTest::DoTest(bool terminate_early,
                                          bool hash_fail,
                                          VerifierMode verifier_mode) {
  string a_loop_file;

  if (!(utils::MakeTempFile("a_loop_file.XXXXXX", &a_loop_file, nullptr))) {
    ADD_FAILURE();
    return false;
  }
  ScopedPathUnlinker a_loop_file_unlinker(a_loop_file);

  // Make random data for a.
  const size_t kLoopFileSize = 10 * 1024 * 1024 + 512;
  brillo::Blob a_loop_data(kLoopFileSize);
  test_utils::FillWithData(&a_loop_data);

  // Write data to disk
  if (!(test_utils::WriteFileVector(a_loop_file, a_loop_data))) {
    ADD_FAILURE();
    return false;
  }

  // Attach loop devices to the files
  string a_dev;
  test_utils::ScopedLoopbackDeviceBinder a_dev_releaser(
      a_loop_file, false, &a_dev);
  if (!(a_dev_releaser.is_bound())) {
    ADD_FAILURE();
    return false;
  }

  LOG(INFO) << "verifying: "  << a_loop_file << " (" << a_dev << ")";

  bool success = true;

  // Set up the action objects
  InstallPlan install_plan;
  install_plan.source_slot = 0;
  install_plan.target_slot = 1;
  InstallPlan::Partition part;
  part.name = "part";
  if (verifier_mode == VerifierMode::kVerifyTargetHash) {
    part.target_size = kLoopFileSize - (hash_fail ? 1 : 0);
    part.target_path = a_dev;
    fake_boot_control_.SetPartitionDevice(
        part.name, install_plan.target_slot, a_dev);
    if (!HashCalculator::RawHashOfData(a_loop_data, &part.target_hash)) {
      ADD_FAILURE();
      success = false;
    }
  }
  part.source_size = kLoopFileSize;
  part.source_path = a_dev;
  fake_boot_control_.SetPartitionDevice(
      part.name, install_plan.source_slot, a_dev);
  if (!HashCalculator::RawHashOfData(a_loop_data, &part.source_hash)) {
    ADD_FAILURE();
    success = false;
  }
  install_plan.partitions = {part};

  ActionProcessor processor;

  ObjectFeederAction<InstallPlan> feeder_action;
  FilesystemVerifierAction copier_action(&fake_boot_control_, verifier_mode);
  ObjectCollectorAction<InstallPlan> collector_action;

  BondActions(&feeder_action, &copier_action);
  BondActions(&copier_action, &collector_action);

  FilesystemVerifierActionTestDelegate delegate(&copier_action);
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&copier_action);
  processor.EnqueueAction(&collector_action);

  feeder_action.set_obj(install_plan);

  loop_.PostTask(FROM_HERE, base::Bind(&StartProcessorInRunLoop,
                                       &processor,
                                       &copier_action,
                                       terminate_early));
  loop_.Run();

  if (!terminate_early) {
    bool is_delegate_ran = delegate.ran();
    EXPECT_TRUE(is_delegate_ran);
    success = success && is_delegate_ran;
  } else {
    EXPECT_EQ(ErrorCode::kError, delegate.code());
    return (ErrorCode::kError == delegate.code());
  }
  if (hash_fail) {
    ErrorCode expected_exit_code = ErrorCode::kNewRootfsVerificationError;
    EXPECT_EQ(expected_exit_code, delegate.code());
    return (expected_exit_code == delegate.code());
  }
  EXPECT_EQ(ErrorCode::kSuccess, delegate.code());

  // Make sure everything in the out_image is there
  brillo::Blob a_out;
  if (!utils::ReadFile(a_dev, &a_out)) {
    ADD_FAILURE();
    return false;
  }
  const bool is_a_file_reading_eq =
      test_utils::ExpectVectorsEq(a_loop_data, a_out);
  EXPECT_TRUE(is_a_file_reading_eq);
  success = success && is_a_file_reading_eq;

  bool is_install_plan_eq = (collector_action.object() == install_plan);
  EXPECT_TRUE(is_install_plan_eq);
  success = success && is_install_plan_eq;
  return success;
}

class FilesystemVerifierActionTest2Delegate : public ActionProcessorDelegate {
 public:
  void ActionCompleted(ActionProcessor* processor,
                       AbstractAction* action,
                       ErrorCode code) {
    if (action->Type() == FilesystemVerifierAction::StaticType()) {
      ran_ = true;
      code_ = code;
    }
  }
  bool ran_;
  ErrorCode code_;
};

TEST_F(FilesystemVerifierActionTest, MissingInputObjectTest) {
  ActionProcessor processor;
  FilesystemVerifierActionTest2Delegate delegate;

  processor.set_delegate(&delegate);

  FilesystemVerifierAction copier_action(&fake_boot_control_,
                                         VerifierMode::kVerifyTargetHash);
  ObjectCollectorAction<InstallPlan> collector_action;

  BondActions(&copier_action, &collector_action);

  processor.EnqueueAction(&copier_action);
  processor.EnqueueAction(&collector_action);
  processor.StartProcessing();
  EXPECT_FALSE(processor.IsRunning());
  EXPECT_TRUE(delegate.ran_);
  EXPECT_EQ(ErrorCode::kError, delegate.code_);
}

TEST_F(FilesystemVerifierActionTest, NonExistentDriveTest) {
  ActionProcessor processor;
  FilesystemVerifierActionTest2Delegate delegate;

  processor.set_delegate(&delegate);

  ObjectFeederAction<InstallPlan> feeder_action;
  InstallPlan install_plan;
  InstallPlan::Partition part;
  part.name = "nope";
  part.source_path = "/no/such/file";
  part.target_path = "/no/such/file";
  install_plan.partitions = {part};

  feeder_action.set_obj(install_plan);
  FilesystemVerifierAction verifier_action(&fake_boot_control_,
                                           VerifierMode::kVerifyTargetHash);
  ObjectCollectorAction<InstallPlan> collector_action;

  BondActions(&verifier_action, &collector_action);

  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&verifier_action);
  processor.EnqueueAction(&collector_action);
  processor.StartProcessing();
  EXPECT_FALSE(processor.IsRunning());
  EXPECT_TRUE(delegate.ran_);
  EXPECT_EQ(ErrorCode::kError, delegate.code_);
}

TEST_F(FilesystemVerifierActionTest, RunAsRootVerifyHashTest) {
  ASSERT_EQ(0U, getuid());
  EXPECT_TRUE(DoTest(false, false, VerifierMode::kVerifyTargetHash));
  EXPECT_TRUE(DoTest(false, false, VerifierMode::kComputeSourceHash));
}

TEST_F(FilesystemVerifierActionTest, RunAsRootVerifyHashFailTest) {
  ASSERT_EQ(0U, getuid());
  EXPECT_TRUE(DoTest(false, true, VerifierMode::kVerifyTargetHash));
}

TEST_F(FilesystemVerifierActionTest, RunAsRootTerminateEarlyTest) {
  ASSERT_EQ(0U, getuid());
  EXPECT_TRUE(DoTest(true, false, VerifierMode::kVerifyTargetHash));
  // TerminateEarlyTest may leak some null callbacks from the Stream class.
  while (loop_.RunOnce(false)) {}
}

// Disabled as we switched to minor version 3, so this test is obsolete, will be
// deleted when we delete the corresponding code in PerformAction().
// Test that the rootfs and kernel size used for hashing in delta payloads for
// major version 1 is properly read.
TEST_F(FilesystemVerifierActionTest,
       DISABLED_RunAsRootDetermineLegacySizeTest) {
  string img;
  EXPECT_TRUE(utils::MakeTempFile("img.XXXXXX", &img, nullptr));
  ScopedPathUnlinker img_unlinker(img);
  test_utils::CreateExtImageAtPath(img, nullptr);
  // Extend the "partition" holding the file system from 10MiB to 20MiB.
  EXPECT_EQ(0, truncate(img.c_str(), 20 * 1024 * 1024));

  InstallPlan install_plan;
  install_plan.source_slot = 1;

  fake_boot_control_.SetPartitionDevice(
      kLegacyPartitionNameRoot, install_plan.source_slot, img);
  fake_boot_control_.SetPartitionDevice(
      kLegacyPartitionNameKernel, install_plan.source_slot, img);
  FilesystemVerifierAction action(&fake_boot_control_,
                                  VerifierMode::kComputeSourceHash);

  ObjectFeederAction<InstallPlan> feeder_action;
  feeder_action.set_obj(install_plan);

  ObjectCollectorAction<InstallPlan> collector_action;

  BondActions(&feeder_action, &action);
  BondActions(&action, &collector_action);
  ActionProcessor processor;
  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&action);
  processor.EnqueueAction(&collector_action);

  loop_.PostTask(FROM_HERE,
                 base::Bind([&processor]{ processor.StartProcessing(); }));
  loop_.Run();
  install_plan = collector_action.object();

  ASSERT_EQ(2U, install_plan.partitions.size());
  // When computing the size of the rootfs on legacy delta updates we use the
  // size of the filesystem, but when updating the kernel we use the whole
  // partition.
  EXPECT_EQ(10U << 20, install_plan.partitions[0].source_size);
  EXPECT_EQ(kLegacyPartitionNameRoot, install_plan.partitions[0].name);
  EXPECT_EQ(20U << 20, install_plan.partitions[1].source_size);
  EXPECT_EQ(kLegacyPartitionNameKernel, install_plan.partitions[1].name);
}

}  // namespace chromeos_update_engine

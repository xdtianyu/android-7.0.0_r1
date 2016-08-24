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

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_FILESYSTEM_VERIFIER_ACTION_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_FILESYSTEM_VERIFIER_ACTION_H_

#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include <brillo/streams/stream.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "update_engine/common/action.h"
#include "update_engine/common/hash_calculator.h"
#include "update_engine/payload_consumer/install_plan.h"

// This action will hash all the partitions of a single slot involved in the
// update (either source or target slot). The hashes are then either stored in
// the InstallPlan (for source partitions) or verified against it (for target
// partitions).

namespace chromeos_update_engine {

// The mode we are running the FilesystemVerifier on. On kComputeSourceHash mode
// it computes the source_hash of all the partitions in the InstallPlan, based
// on the already populated source_size values. On kVerifyTargetHash it computes
// the hash on the target partitions based on the already populated size and
// verifies it matches the one in the target_hash in the InstallPlan.
enum class VerifierMode {
  kComputeSourceHash,
  kVerifyTargetHash,
  kVerifySourceHash,
};

class FilesystemVerifierAction : public InstallPlanAction {
 public:
  FilesystemVerifierAction(const BootControlInterface* boot_control,
                           VerifierMode verifier_mode);

  void PerformAction() override;
  void TerminateProcessing() override;

  // Used for testing. Return true if Cleanup() has not yet been called due
  // to a callback upon the completion or cancellation of the verifier action.
  // A test should wait until IsCleanupPending() returns false before
  // terminating the main loop.
  bool IsCleanupPending() const;

  // Debugging/logging
  static std::string StaticType() { return "FilesystemVerifierAction"; }
  std::string Type() const override { return StaticType(); }

 private:
  friend class FilesystemVerifierActionTest;
  FRIEND_TEST(FilesystemVerifierActionTest,
              RunAsRootDetermineFilesystemSizeTest);

  // Starts the hashing of the current partition. If there aren't any partitions
  // remaining to be hashed, if finishes the action.
  void StartPartitionHashing();

  // Schedules the asynchronous read of the filesystem.
  void ScheduleRead();

  // Called from the main loop when a single read from |src_stream_| succeeds or
  // fails, calling OnReadDoneCallback() and OnReadErrorCallback() respectively.
  void OnReadDoneCallback(size_t bytes_read);
  void OnReadErrorCallback(const brillo::Error* error);

  // When the read is done, finalize the hash checking of the current partition
  // and continue checking the next one.
  void FinishPartitionHashing();

  // Cleans up all the variables we use for async operations and tells the
  // ActionProcessor we're done w/ |code| as passed in. |cancelled_| should be
  // true if TerminateProcessing() was called.
  void Cleanup(ErrorCode code);

  // The type of the partition that we are verifying.
  VerifierMode verifier_mode_;

  // The BootControlInterface used to get the partitions based on the slots.
  const BootControlInterface* const boot_control_;

  // The index in the install_plan_.partitions vector of the partition currently
  // being hashed.
  size_t partition_index_{0};

  // If not null, the FileStream used to read from the device.
  brillo::StreamPtr src_stream_;

  // Buffer for storing data we read.
  brillo::Blob buffer_;

  bool read_done_{false};  // true if reached EOF on the input stream.
  bool cancelled_{false};  // true if the action has been cancelled.

  // The install plan we're passed in via the input pipe.
  InstallPlan install_plan_;

  // Calculates the hash of the data.
  std::unique_ptr<HashCalculator> hasher_;

  // Reads and hashes this many bytes from the head of the input stream. This
  // field is initialized from the corresponding InstallPlan::Partition size,
  // when the partition starts to be hashed.
  int64_t remaining_size_{0};

  DISALLOW_COPY_AND_ASSIGN(FilesystemVerifierAction);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_FILESYSTEM_VERIFIER_ACTION_H_

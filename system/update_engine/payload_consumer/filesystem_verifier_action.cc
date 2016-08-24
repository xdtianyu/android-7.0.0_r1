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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdlib>
#include <string>

#include <base/bind.h>
#include <brillo/streams/file_stream.h>

#include "update_engine/common/boot_control_interface.h"
#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/delta_performer.h"
#include "update_engine/payload_consumer/payload_constants.h"

using std::string;

namespace chromeos_update_engine {

namespace {
const off_t kReadFileBufferSize = 128 * 1024;
}  // namespace

FilesystemVerifierAction::FilesystemVerifierAction(
    const BootControlInterface* boot_control,
    VerifierMode verifier_mode)
    : verifier_mode_(verifier_mode),
      boot_control_(boot_control) {}

void FilesystemVerifierAction::PerformAction() {
  // Will tell the ActionProcessor we've failed if we return.
  ScopedActionCompleter abort_action_completer(processor_, this);

  if (!HasInputObject()) {
    LOG(ERROR) << "FilesystemVerifierAction missing input object.";
    return;
  }
  install_plan_ = GetInputObject();

  // For delta updates (major version 1) we need to populate the source
  // partition hash if not pre-populated.
  if (install_plan_.payload_type == InstallPayloadType::kDelta &&
      install_plan_.partitions.empty() &&
      verifier_mode_ == VerifierMode::kComputeSourceHash &&
      DeltaPerformer::kSupportedMinorPayloadVersion <
          kOpSrcHashMinorPayloadVersion) {
    LOG(INFO) << "Using legacy partition names.";
    InstallPlan::Partition part;
    string part_path;

    part.name = kLegacyPartitionNameRoot;
    if (!boot_control_->GetPartitionDevice(
        part.name, install_plan_.source_slot, &part_path))
      return;
    int block_count = 0, block_size = 0;
    if (utils::GetFilesystemSize(part_path, &block_count, &block_size)) {
      part.source_size = static_cast<int64_t>(block_count) * block_size;
      LOG(INFO) << "Partition " << part.name << " size: " << part.source_size
                << " bytes (" << block_count << "x" << block_size << ").";
    }
    install_plan_.partitions.push_back(part);

    part.name = kLegacyPartitionNameKernel;
    if (!boot_control_->GetPartitionDevice(
        part.name, install_plan_.source_slot, &part_path))
      return;
    off_t kernel_part_size = utils::FileSize(part_path);
    if (kernel_part_size < 0)
      return;
    LOG(INFO) << "Partition " << part.name << " size: " << kernel_part_size
              << " bytes.";
    part.source_size = kernel_part_size;
    install_plan_.partitions.push_back(part);
  }

  if (install_plan_.partitions.empty()) {
    LOG(INFO) << "No partitions to verify.";
    if (HasOutputPipe())
      SetOutputObject(install_plan_);
    abort_action_completer.set_code(ErrorCode::kSuccess);
    return;
  }

  StartPartitionHashing();
  abort_action_completer.set_should_complete(false);
}

void FilesystemVerifierAction::TerminateProcessing() {
  cancelled_ = true;
  Cleanup(ErrorCode::kSuccess);  // error code is ignored if canceled_ is true.
}

bool FilesystemVerifierAction::IsCleanupPending() const {
  return src_stream_ != nullptr;
}

void FilesystemVerifierAction::Cleanup(ErrorCode code) {
  src_stream_.reset();
  // This memory is not used anymore.
  buffer_.clear();

  if (cancelled_)
    return;
  if (code == ErrorCode::kSuccess && HasOutputPipe())
    SetOutputObject(install_plan_);
  processor_->ActionComplete(this, code);
}

void FilesystemVerifierAction::StartPartitionHashing() {
  if (partition_index_ == install_plan_.partitions.size()) {
    // We never called this action with kVerifySourceHash directly, if we are in
    // this mode, it means the target partition verification has failed, so we
    // should set the error code to reflect the error in target.
    if (verifier_mode_ == VerifierMode::kVerifySourceHash)
      Cleanup(ErrorCode::kNewRootfsVerificationError);
    else
      Cleanup(ErrorCode::kSuccess);
    return;
  }
  InstallPlan::Partition& partition =
      install_plan_.partitions[partition_index_];

  string part_path;
  switch (verifier_mode_) {
    case VerifierMode::kComputeSourceHash:
    case VerifierMode::kVerifySourceHash:
      boot_control_->GetPartitionDevice(
          partition.name, install_plan_.source_slot, &part_path);
      remaining_size_ = partition.source_size;
      break;
    case VerifierMode::kVerifyTargetHash:
      boot_control_->GetPartitionDevice(
          partition.name, install_plan_.target_slot, &part_path);
      remaining_size_ = partition.target_size;
      break;
  }
  LOG(INFO) << "Hashing partition " << partition_index_ << " ("
            << partition.name << ") on device " << part_path;
  if (part_path.empty())
    return Cleanup(ErrorCode::kFilesystemVerifierError);

  brillo::ErrorPtr error;
  src_stream_ = brillo::FileStream::Open(
      base::FilePath(part_path),
      brillo::Stream::AccessMode::READ,
      brillo::FileStream::Disposition::OPEN_EXISTING,
      &error);

  if (!src_stream_) {
    LOG(ERROR) << "Unable to open " << part_path << " for reading";
    return Cleanup(ErrorCode::kFilesystemVerifierError);
  }

  buffer_.resize(kReadFileBufferSize);
  read_done_ = false;
  hasher_.reset(new HashCalculator());

  // Start the first read.
  ScheduleRead();
}

void FilesystemVerifierAction::ScheduleRead() {
  size_t bytes_to_read = std::min(static_cast<int64_t>(buffer_.size()),
                                  remaining_size_);
  if (!bytes_to_read) {
    OnReadDoneCallback(0);
    return;
  }

  bool read_async_ok = src_stream_->ReadAsync(
    buffer_.data(),
    bytes_to_read,
    base::Bind(&FilesystemVerifierAction::OnReadDoneCallback,
               base::Unretained(this)),
    base::Bind(&FilesystemVerifierAction::OnReadErrorCallback,
               base::Unretained(this)),
    nullptr);

  if (!read_async_ok) {
    LOG(ERROR) << "Unable to schedule an asynchronous read from the stream.";
    Cleanup(ErrorCode::kError);
  }
}

void FilesystemVerifierAction::OnReadDoneCallback(size_t bytes_read) {
  if (bytes_read == 0) {
    read_done_ = true;
  } else {
    remaining_size_ -= bytes_read;
    CHECK(!read_done_);
    if (!hasher_->Update(buffer_.data(), bytes_read)) {
      LOG(ERROR) << "Unable to update the hash.";
      Cleanup(ErrorCode::kError);
      return;
    }
  }

  // We either terminate the current partition or have more data to read.
  if (cancelled_)
    return Cleanup(ErrorCode::kError);

  if (read_done_ || remaining_size_ == 0) {
    if (remaining_size_ != 0) {
      LOG(ERROR) << "Failed to read the remaining " << remaining_size_
                 << " bytes from partition "
                 << install_plan_.partitions[partition_index_].name;
      return Cleanup(ErrorCode::kFilesystemVerifierError);
    }
    return FinishPartitionHashing();
  }
  ScheduleRead();
}

void FilesystemVerifierAction::OnReadErrorCallback(
      const brillo::Error* error) {
  // TODO(deymo): Transform the read-error into an specific ErrorCode.
  LOG(ERROR) << "Asynchronous read failed.";
  Cleanup(ErrorCode::kError);
}

void FilesystemVerifierAction::FinishPartitionHashing() {
  if (!hasher_->Finalize()) {
    LOG(ERROR) << "Unable to finalize the hash.";
    return Cleanup(ErrorCode::kError);
  }
  InstallPlan::Partition& partition =
      install_plan_.partitions[partition_index_];
  LOG(INFO) << "Hash of " << partition.name << ": " << hasher_->hash();

  switch (verifier_mode_) {
    case VerifierMode::kComputeSourceHash:
      partition.source_hash = hasher_->raw_hash();
      partition_index_++;
      break;
    case VerifierMode::kVerifyTargetHash:
      if (partition.target_hash != hasher_->raw_hash()) {
        LOG(ERROR) << "New '" << partition.name
                   << "' partition verification failed.";
        if (DeltaPerformer::kSupportedMinorPayloadVersion <
            kOpSrcHashMinorPayloadVersion)
          return Cleanup(ErrorCode::kNewRootfsVerificationError);
        // If we support per-operation source hash, then we skipped source
        // filesystem verification, now that the target partition does not
        // match, we need to switch to kVerifySourceHash mode to check if it's
        // because the source partition does not match either.
        verifier_mode_ = VerifierMode::kVerifySourceHash;
        partition_index_ = 0;
      } else {
        partition_index_++;
      }
      break;
    case VerifierMode::kVerifySourceHash:
      if (partition.source_hash != hasher_->raw_hash()) {
        LOG(ERROR) << "Old '" << partition.name
                   << "' partition verification failed.";
        return Cleanup(ErrorCode::kDownloadStateInitializationError);
      }
      partition_index_++;
      break;
  }
  // Start hashing the next partition, if any.
  hasher_.reset();
  buffer_.clear();
  src_stream_->CloseBlocking(nullptr);
  StartPartitionHashing();
}

}  // namespace chromeos_update_engine

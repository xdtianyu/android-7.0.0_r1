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

#include "update_engine/payload_consumer/delta_performer.h"

#include <endian.h>
#include <errno.h>
#include <linux/fs.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/format_macros.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/data_encoding.h>
#include <brillo/make_unique_ptr.h>
#include <google/protobuf/repeated_field.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/hardware_interface.h"
#include "update_engine/common/prefs_interface.h"
#include "update_engine/common/subprocess.h"
#include "update_engine/common/terminator.h"
#include "update_engine/payload_consumer/bzip_extent_writer.h"
#include "update_engine/payload_consumer/download_action.h"
#include "update_engine/payload_consumer/extent_writer.h"
#if USE_MTD
#include "update_engine/payload_consumer/mtd_file_descriptor.h"
#endif
#include "update_engine/payload_consumer/payload_constants.h"
#include "update_engine/payload_consumer/payload_verifier.h"
#include "update_engine/payload_consumer/xz_extent_writer.h"

using google::protobuf::RepeatedPtrField;
using std::min;
using std::string;
using std::vector;

namespace chromeos_update_engine {

const uint64_t DeltaPerformer::kDeltaVersionOffset = sizeof(kDeltaMagic);
const uint64_t DeltaPerformer::kDeltaVersionSize = 8;
const uint64_t DeltaPerformer::kDeltaManifestSizeOffset =
    kDeltaVersionOffset + kDeltaVersionSize;
const uint64_t DeltaPerformer::kDeltaManifestSizeSize = 8;
const uint64_t DeltaPerformer::kDeltaMetadataSignatureSizeSize = 4;
const uint64_t DeltaPerformer::kMaxPayloadHeaderSize = 24;
const uint64_t DeltaPerformer::kSupportedMajorPayloadVersion = 2;
const uint32_t DeltaPerformer::kSupportedMinorPayloadVersion = 3;

const unsigned DeltaPerformer::kProgressLogMaxChunks = 10;
const unsigned DeltaPerformer::kProgressLogTimeoutSeconds = 30;
const unsigned DeltaPerformer::kProgressDownloadWeight = 50;
const unsigned DeltaPerformer::kProgressOperationsWeight = 50;

namespace {
const int kUpdateStateOperationInvalid = -1;
const int kMaxResumedUpdateFailures = 10;
#if USE_MTD
const int kUbiVolumeAttachTimeout = 5 * 60;
#endif

FileDescriptorPtr CreateFileDescriptor(const char* path) {
  FileDescriptorPtr ret;
#if USE_MTD
  if (strstr(path, "/dev/ubi") == path) {
    if (!UbiFileDescriptor::IsUbi(path)) {
      // The volume might not have been attached at boot time.
      int volume_no;
      if (utils::SplitPartitionName(path, nullptr, &volume_no)) {
        utils::TryAttachingUbiVolume(volume_no, kUbiVolumeAttachTimeout);
      }
    }
    if (UbiFileDescriptor::IsUbi(path)) {
      LOG(INFO) << path << " is a UBI device.";
      ret.reset(new UbiFileDescriptor);
    }
  } else if (MtdFileDescriptor::IsMtd(path)) {
    LOG(INFO) << path << " is an MTD device.";
    ret.reset(new MtdFileDescriptor);
  } else {
    LOG(INFO) << path << " is not an MTD nor a UBI device.";
#endif
    ret.reset(new EintrSafeFileDescriptor);
#if USE_MTD
  }
#endif
  return ret;
}

// Opens path for read/write. On success returns an open FileDescriptor
// and sets *err to 0. On failure, sets *err to errno and returns nullptr.
FileDescriptorPtr OpenFile(const char* path, int mode, int* err) {
  // Try to mark the block device read-only based on the mode. Ignore any
  // failure since this won't work when passing regular files.
  utils::SetBlockDeviceReadOnly(path, (mode & O_ACCMODE) == O_RDONLY);

  FileDescriptorPtr fd = CreateFileDescriptor(path);
#if USE_MTD
  // On NAND devices, we can either read, or write, but not both. So here we
  // use O_WRONLY.
  if (UbiFileDescriptor::IsUbi(path) || MtdFileDescriptor::IsMtd(path)) {
    mode = O_WRONLY;
  }
#endif
  if (!fd->Open(path, mode, 000)) {
    *err = errno;
    PLOG(ERROR) << "Unable to open file " << path;
    return nullptr;
  }
  *err = 0;
  return fd;
}
}  // namespace


// Computes the ratio of |part| and |total|, scaled to |norm|, using integer
// arithmetic.
static uint64_t IntRatio(uint64_t part, uint64_t total, uint64_t norm) {
  return part * norm / total;
}

void DeltaPerformer::LogProgress(const char* message_prefix) {
  // Format operations total count and percentage.
  string total_operations_str("?");
  string completed_percentage_str("");
  if (num_total_operations_) {
    total_operations_str = std::to_string(num_total_operations_);
    // Upcasting to 64-bit to avoid overflow, back to size_t for formatting.
    completed_percentage_str =
        base::StringPrintf(" (%" PRIu64 "%%)",
                           IntRatio(next_operation_num_, num_total_operations_,
                                    100));
  }

  // Format download total count and percentage.
  size_t payload_size = install_plan_->payload_size;
  string payload_size_str("?");
  string downloaded_percentage_str("");
  if (payload_size) {
    payload_size_str = std::to_string(payload_size);
    // Upcasting to 64-bit to avoid overflow, back to size_t for formatting.
    downloaded_percentage_str =
        base::StringPrintf(" (%" PRIu64 "%%)",
                           IntRatio(total_bytes_received_, payload_size, 100));
  }

  LOG(INFO) << (message_prefix ? message_prefix : "") << next_operation_num_
            << "/" << total_operations_str << " operations"
            << completed_percentage_str << ", " << total_bytes_received_
            << "/" << payload_size_str << " bytes downloaded"
            << downloaded_percentage_str << ", overall progress "
            << overall_progress_ << "%";
}

void DeltaPerformer::UpdateOverallProgress(bool force_log,
                                           const char* message_prefix) {
  // Compute our download and overall progress.
  unsigned new_overall_progress = 0;
  static_assert(kProgressDownloadWeight + kProgressOperationsWeight == 100,
                "Progress weights don't add up");
  // Only consider download progress if its total size is known; otherwise
  // adjust the operations weight to compensate for the absence of download
  // progress. Also, make sure to cap the download portion at
  // kProgressDownloadWeight, in case we end up downloading more than we
  // initially expected (this indicates a problem, but could generally happen).
  // TODO(garnold) the correction of operations weight when we do not have the
  // total payload size, as well as the conditional guard below, should both be
  // eliminated once we ensure that the payload_size in the install plan is
  // always given and is non-zero. This currently isn't the case during unit
  // tests (see chromium-os:37969).
  size_t payload_size = install_plan_->payload_size;
  unsigned actual_operations_weight = kProgressOperationsWeight;
  if (payload_size)
    new_overall_progress += min(
        static_cast<unsigned>(IntRatio(total_bytes_received_, payload_size,
                                       kProgressDownloadWeight)),
        kProgressDownloadWeight);
  else
    actual_operations_weight += kProgressDownloadWeight;

  // Only add completed operations if their total number is known; we definitely
  // expect an update to have at least one operation, so the expectation is that
  // this will eventually reach |actual_operations_weight|.
  if (num_total_operations_)
    new_overall_progress += IntRatio(next_operation_num_, num_total_operations_,
                                     actual_operations_weight);

  // Progress ratio cannot recede, unless our assumptions about the total
  // payload size, total number of operations, or the monotonicity of progress
  // is breached.
  if (new_overall_progress < overall_progress_) {
    LOG(WARNING) << "progress counter receded from " << overall_progress_
                 << "% down to " << new_overall_progress << "%; this is a bug";
    force_log = true;
  }
  overall_progress_ = new_overall_progress;

  // Update chunk index, log as needed: if forced by called, or we completed a
  // progress chunk, or a timeout has expired.
  base::Time curr_time = base::Time::Now();
  unsigned curr_progress_chunk =
      overall_progress_ * kProgressLogMaxChunks / 100;
  if (force_log || curr_progress_chunk > last_progress_chunk_ ||
      curr_time > forced_progress_log_time_) {
    forced_progress_log_time_ = curr_time + forced_progress_log_wait_;
    LogProgress(message_prefix);
  }
  last_progress_chunk_ = curr_progress_chunk;
}


size_t DeltaPerformer::CopyDataToBuffer(const char** bytes_p, size_t* count_p,
                                        size_t max) {
  const size_t count = *count_p;
  if (!count)
    return 0;  // Special case shortcut.
  size_t read_len = min(count, max - buffer_.size());
  const char* bytes_start = *bytes_p;
  const char* bytes_end = bytes_start + read_len;
  buffer_.insert(buffer_.end(), bytes_start, bytes_end);
  *bytes_p = bytes_end;
  *count_p = count - read_len;
  return read_len;
}


bool DeltaPerformer::HandleOpResult(bool op_result, const char* op_type_name,
                                    ErrorCode* error) {
  if (op_result)
    return true;

  LOG(ERROR) << "Failed to perform " << op_type_name << " operation "
             << next_operation_num_;
  *error = ErrorCode::kDownloadOperationExecutionError;
  return false;
}

int DeltaPerformer::Close() {
  int err = -CloseCurrentPartition();
  LOG_IF(ERROR, !payload_hash_calculator_.Finalize() ||
                !signed_hash_calculator_.Finalize())
      << "Unable to finalize the hash.";
  if (!buffer_.empty()) {
    LOG(INFO) << "Discarding " << buffer_.size() << " unused downloaded bytes";
    if (err >= 0)
      err = 1;
  }
  return -err;
}

int DeltaPerformer::CloseCurrentPartition() {
  int err = 0;
  if (source_fd_ && !source_fd_->Close()) {
    err = errno;
    PLOG(ERROR) << "Error closing source partition";
    if (!err)
      err = 1;
  }
  source_fd_.reset();
  source_path_.clear();

  if (target_fd_ && !target_fd_->Close()) {
    err = errno;
    PLOG(ERROR) << "Error closing target partition";
    if (!err)
      err = 1;
  }
  target_fd_.reset();
  target_path_.clear();
  return -err;
}

bool DeltaPerformer::OpenCurrentPartition() {
  if (current_partition_ >= partitions_.size())
    return false;

  const PartitionUpdate& partition = partitions_[current_partition_];
  // Open source fds if we have a delta payload with minor version >= 2.
  if (install_plan_->payload_type == InstallPayloadType::kDelta &&
      GetMinorVersion() != kInPlaceMinorPayloadVersion) {
    source_path_ = install_plan_->partitions[current_partition_].source_path;
    int err;
    source_fd_ = OpenFile(source_path_.c_str(), O_RDONLY, &err);
    if (!source_fd_) {
      LOG(ERROR) << "Unable to open source partition "
                 << partition.partition_name() << " on slot "
                 << BootControlInterface::SlotName(install_plan_->source_slot)
                 << ", file " << source_path_;
      return false;
    }
  }

  target_path_ = install_plan_->partitions[current_partition_].target_path;
  int err;
  target_fd_ = OpenFile(target_path_.c_str(), O_RDWR, &err);
  if (!target_fd_) {
    LOG(ERROR) << "Unable to open target partition "
               << partition.partition_name() << " on slot "
               << BootControlInterface::SlotName(install_plan_->target_slot)
               << ", file " << target_path_;
    return false;
  }
  return true;
}

namespace {

void LogPartitionInfoHash(const PartitionInfo& info, const string& tag) {
  string sha256 = brillo::data_encoding::Base64Encode(info.hash());
  LOG(INFO) << "PartitionInfo " << tag << " sha256: " << sha256
            << " size: " << info.size();
}

void LogPartitionInfo(const vector<PartitionUpdate>& partitions) {
  for (const PartitionUpdate& partition : partitions) {
    LogPartitionInfoHash(partition.old_partition_info(),
                         "old " + partition.partition_name());
    LogPartitionInfoHash(partition.new_partition_info(),
                         "new " + partition.partition_name());
  }
}

}  // namespace

bool DeltaPerformer::GetMetadataSignatureSizeOffset(
    uint64_t* out_offset) const {
  if (GetMajorVersion() == kBrilloMajorPayloadVersion) {
    *out_offset = kDeltaManifestSizeOffset + kDeltaManifestSizeSize;
    return true;
  }
  return false;
}

bool DeltaPerformer::GetManifestOffset(uint64_t* out_offset) const {
  // Actual manifest begins right after the manifest size field or
  // metadata signature size field if major version >= 2.
  if (major_payload_version_ == kChromeOSMajorPayloadVersion) {
    *out_offset = kDeltaManifestSizeOffset + kDeltaManifestSizeSize;
    return true;
  }
  if (major_payload_version_ == kBrilloMajorPayloadVersion) {
    *out_offset = kDeltaManifestSizeOffset + kDeltaManifestSizeSize +
                  kDeltaMetadataSignatureSizeSize;
    return true;
  }
  LOG(ERROR) << "Unknown major payload version: " << major_payload_version_;
  return false;
}

uint64_t DeltaPerformer::GetMetadataSize() const {
  return metadata_size_;
}

uint64_t DeltaPerformer::GetMajorVersion() const {
  return major_payload_version_;
}

uint32_t DeltaPerformer::GetMinorVersion() const {
  if (manifest_.has_minor_version()) {
    return manifest_.minor_version();
  } else {
    return install_plan_->payload_type == InstallPayloadType::kDelta
               ? kSupportedMinorPayloadVersion
               : kFullPayloadMinorVersion;
  }
}

bool DeltaPerformer::GetManifest(DeltaArchiveManifest* out_manifest_p) const {
  if (!manifest_parsed_)
    return false;
  *out_manifest_p = manifest_;
  return true;
}

bool DeltaPerformer::IsHeaderParsed() const {
  return metadata_size_ != 0;
}

DeltaPerformer::MetadataParseResult DeltaPerformer::ParsePayloadMetadata(
    const brillo::Blob& payload, ErrorCode* error) {
  *error = ErrorCode::kSuccess;
  uint64_t manifest_offset;

  if (!IsHeaderParsed()) {
    // Ensure we have data to cover the major payload version.
    if (payload.size() < kDeltaManifestSizeOffset)
      return kMetadataParseInsufficientData;

    // Validate the magic string.
    if (memcmp(payload.data(), kDeltaMagic, sizeof(kDeltaMagic)) != 0) {
      LOG(ERROR) << "Bad payload format -- invalid delta magic.";
      *error = ErrorCode::kDownloadInvalidMetadataMagicString;
      return kMetadataParseError;
    }

    // Extract the payload version from the metadata.
    static_assert(sizeof(major_payload_version_) == kDeltaVersionSize,
                  "Major payload version size mismatch");
    memcpy(&major_payload_version_,
           &payload[kDeltaVersionOffset],
           kDeltaVersionSize);
    // switch big endian to host
    major_payload_version_ = be64toh(major_payload_version_);

    if (major_payload_version_ != supported_major_version_ &&
        major_payload_version_ != kChromeOSMajorPayloadVersion) {
      LOG(ERROR) << "Bad payload format -- unsupported payload version: "
          << major_payload_version_;
      *error = ErrorCode::kUnsupportedMajorPayloadVersion;
      return kMetadataParseError;
    }

    // Get the manifest offset now that we have payload version.
    if (!GetManifestOffset(&manifest_offset)) {
      *error = ErrorCode::kUnsupportedMajorPayloadVersion;
      return kMetadataParseError;
    }
    // Check again with the manifest offset.
    if (payload.size() < manifest_offset)
      return kMetadataParseInsufficientData;

    // Next, parse the manifest size.
    static_assert(sizeof(manifest_size_) == kDeltaManifestSizeSize,
                  "manifest_size size mismatch");
    memcpy(&manifest_size_,
           &payload[kDeltaManifestSizeOffset],
           kDeltaManifestSizeSize);
    manifest_size_ = be64toh(manifest_size_);  // switch big endian to host

    if (GetMajorVersion() == kBrilloMajorPayloadVersion) {
      // Parse the metadata signature size.
      static_assert(sizeof(metadata_signature_size_) ==
                    kDeltaMetadataSignatureSizeSize,
                    "metadata_signature_size size mismatch");
      uint64_t metadata_signature_size_offset;
      if (!GetMetadataSignatureSizeOffset(&metadata_signature_size_offset)) {
        *error = ErrorCode::kError;
        return kMetadataParseError;
      }
      memcpy(&metadata_signature_size_,
             &payload[metadata_signature_size_offset],
             kDeltaMetadataSignatureSizeSize);
      metadata_signature_size_ = be32toh(metadata_signature_size_);
    }

    // If the metadata size is present in install plan, check for it immediately
    // even before waiting for that many number of bytes to be downloaded in the
    // payload. This will prevent any attack which relies on us downloading data
    // beyond the expected metadata size.
    metadata_size_ = manifest_offset + manifest_size_;
    if (install_plan_->hash_checks_mandatory) {
      if (install_plan_->metadata_size != metadata_size_) {
        LOG(ERROR) << "Mandatory metadata size in Omaha response ("
                   << install_plan_->metadata_size
                   << ") is missing/incorrect, actual = " << metadata_size_;
        *error = ErrorCode::kDownloadInvalidMetadataSize;
        return kMetadataParseError;
      }
    }
  }

  // Now that we have validated the metadata size, we should wait for the full
  // metadata and its signature (if exist) to be read in before we can parse it.
  if (payload.size() < metadata_size_ + metadata_signature_size_)
    return kMetadataParseInsufficientData;

  // Log whether we validated the size or simply trusting what's in the payload
  // here. This is logged here (after we received the full metadata data) so
  // that we just log once (instead of logging n times) if it takes n
  // DeltaPerformer::Write calls to download the full manifest.
  if (install_plan_->metadata_size == metadata_size_) {
    LOG(INFO) << "Manifest size in payload matches expected value from Omaha";
  } else {
    // For mandatory-cases, we'd have already returned a kMetadataParseError
    // above. We'll be here only for non-mandatory cases. Just send a UMA stat.
    LOG(WARNING) << "Ignoring missing/incorrect metadata size ("
                 << install_plan_->metadata_size
                 << ") in Omaha response as validation is not mandatory. "
                 << "Trusting metadata size in payload = " << metadata_size_;
  }

  // We have the full metadata in |payload|. Verify its integrity
  // and authenticity based on the information we have in Omaha response.
  *error = ValidateMetadataSignature(payload);
  if (*error != ErrorCode::kSuccess) {
    if (install_plan_->hash_checks_mandatory) {
      // The autoupdate_CatchBadSignatures test checks for this string
      // in log-files. Keep in sync.
      LOG(ERROR) << "Mandatory metadata signature validation failed";
      return kMetadataParseError;
    }

    // For non-mandatory cases, just send a UMA stat.
    LOG(WARNING) << "Ignoring metadata signature validation failures";
    *error = ErrorCode::kSuccess;
  }

  if (!GetManifestOffset(&manifest_offset)) {
    *error = ErrorCode::kUnsupportedMajorPayloadVersion;
    return kMetadataParseError;
  }
  // The payload metadata is deemed valid, it's safe to parse the protobuf.
  if (!manifest_.ParseFromArray(&payload[manifest_offset], manifest_size_)) {
    LOG(ERROR) << "Unable to parse manifest in update file.";
    *error = ErrorCode::kDownloadManifestParseError;
    return kMetadataParseError;
  }

  manifest_parsed_ = true;
  return kMetadataParseSuccess;
}

// Wrapper around write. Returns true if all requested bytes
// were written, or false on any error, regardless of progress
// and stores an action exit code in |error|.
bool DeltaPerformer::Write(const void* bytes, size_t count, ErrorCode *error) {
  *error = ErrorCode::kSuccess;

  const char* c_bytes = reinterpret_cast<const char*>(bytes);

  // Update the total byte downloaded count and the progress logs.
  total_bytes_received_ += count;
  UpdateOverallProgress(false, "Completed ");

  while (!manifest_valid_) {
    // Read data up to the needed limit; this is either maximium payload header
    // size, or the full metadata size (once it becomes known).
    const bool do_read_header = !IsHeaderParsed();
    CopyDataToBuffer(&c_bytes, &count,
                     (do_read_header ? kMaxPayloadHeaderSize :
                      metadata_size_ + metadata_signature_size_));

    MetadataParseResult result = ParsePayloadMetadata(buffer_, error);
    if (result == kMetadataParseError)
      return false;
    if (result == kMetadataParseInsufficientData) {
      // If we just processed the header, make an attempt on the manifest.
      if (do_read_header && IsHeaderParsed())
        continue;

      return true;
    }

    // Checks the integrity of the payload manifest.
    if ((*error = ValidateManifest()) != ErrorCode::kSuccess)
      return false;
    manifest_valid_ = true;

    // Clear the download buffer.
    DiscardBuffer(false, metadata_size_);

    // This populates |partitions_| and the |install_plan.partitions| with the
    // list of partitions from the manifest.
    if (!ParseManifestPartitions(error))
      return false;

    num_total_operations_ = 0;
    for (const auto& partition : partitions_) {
      num_total_operations_ += partition.operations_size();
      acc_num_operations_.push_back(num_total_operations_);
    }

    LOG_IF(WARNING, !prefs_->SetInt64(kPrefsManifestMetadataSize,
                                      metadata_size_))
        << "Unable to save the manifest metadata size.";
    LOG_IF(WARNING, !prefs_->SetInt64(kPrefsManifestSignatureSize,
                                      metadata_signature_size_))
        << "Unable to save the manifest signature size.";

    if (!PrimeUpdateState()) {
      *error = ErrorCode::kDownloadStateInitializationError;
      LOG(ERROR) << "Unable to prime the update state.";
      return false;
    }

    if (!OpenCurrentPartition()) {
      *error = ErrorCode::kInstallDeviceOpenError;
      return false;
    }

    if (next_operation_num_ > 0)
      UpdateOverallProgress(true, "Resuming after ");
    LOG(INFO) << "Starting to apply update payload operations";
  }

  while (next_operation_num_ < num_total_operations_) {
    // Check if we should cancel the current attempt for any reason.
    // In this case, *error will have already been populated with the reason
    // why we're canceling.
    if (download_delegate_ && download_delegate_->ShouldCancel(error))
      return false;

    // We know there are more operations to perform because we didn't reach the
    // |num_total_operations_| limit yet.
    while (next_operation_num_ >= acc_num_operations_[current_partition_]) {
      CloseCurrentPartition();
      current_partition_++;
      if (!OpenCurrentPartition()) {
        *error = ErrorCode::kInstallDeviceOpenError;
        return false;
      }
    }
    const size_t partition_operation_num = next_operation_num_ - (
        current_partition_ ? acc_num_operations_[current_partition_ - 1] : 0);

    const InstallOperation& op =
        partitions_[current_partition_].operations(partition_operation_num);

    CopyDataToBuffer(&c_bytes, &count, op.data_length());

    // Check whether we received all of the next operation's data payload.
    if (!CanPerformInstallOperation(op))
      return true;

    // Validate the operation only if the metadata signature is present.
    // Otherwise, keep the old behavior. This serves as a knob to disable
    // the validation logic in case we find some regression after rollout.
    // NOTE: If hash checks are mandatory and if metadata_signature is empty,
    // we would have already failed in ParsePayloadMetadata method and thus not
    // even be here. So no need to handle that case again here.
    if (!install_plan_->metadata_signature.empty()) {
      // Note: Validate must be called only if CanPerformInstallOperation is
      // called. Otherwise, we might be failing operations before even if there
      // isn't sufficient data to compute the proper hash.
      *error = ValidateOperationHash(op);
      if (*error != ErrorCode::kSuccess) {
        if (install_plan_->hash_checks_mandatory) {
          LOG(ERROR) << "Mandatory operation hash check failed";
          return false;
        }

        // For non-mandatory cases, just send a UMA stat.
        LOG(WARNING) << "Ignoring operation validation errors";
        *error = ErrorCode::kSuccess;
      }
    }

    // Makes sure we unblock exit when this operation completes.
    ScopedTerminatorExitUnblocker exit_unblocker =
        ScopedTerminatorExitUnblocker();  // Avoids a compiler unused var bug.

    bool op_result;
    switch (op.type()) {
      case InstallOperation::REPLACE:
      case InstallOperation::REPLACE_BZ:
      case InstallOperation::REPLACE_XZ:
        op_result = PerformReplaceOperation(op);
        break;
      case InstallOperation::ZERO:
      case InstallOperation::DISCARD:
        op_result = PerformZeroOrDiscardOperation(op);
        break;
      case InstallOperation::MOVE:
        op_result = PerformMoveOperation(op);
        break;
      case InstallOperation::BSDIFF:
        op_result = PerformBsdiffOperation(op);
        break;
      case InstallOperation::SOURCE_COPY:
        op_result = PerformSourceCopyOperation(op);
        break;
      case InstallOperation::SOURCE_BSDIFF:
        op_result = PerformSourceBsdiffOperation(op);
        break;
      default:
       op_result = false;
    }
    if (!HandleOpResult(op_result, InstallOperationTypeName(op.type()), error))
      return false;

    next_operation_num_++;
    UpdateOverallProgress(false, "Completed ");
    CheckpointUpdateProgress();
  }

  // In major version 2, we don't add dummy operation to the payload.
  // If we already extracted the signature we should skip this step.
  if (major_payload_version_ == kBrilloMajorPayloadVersion &&
      manifest_.has_signatures_offset() && manifest_.has_signatures_size() &&
      signatures_message_data_.empty()) {
    if (manifest_.signatures_offset() != buffer_offset_) {
      LOG(ERROR) << "Payload signatures offset points to blob offset "
                 << manifest_.signatures_offset()
                 << " but signatures are expected at offset "
                 << buffer_offset_;
      *error = ErrorCode::kDownloadPayloadVerificationError;
      return false;
    }
    CopyDataToBuffer(&c_bytes, &count, manifest_.signatures_size());
    // Needs more data to cover entire signature.
    if (buffer_.size() < manifest_.signatures_size())
      return true;
    if (!ExtractSignatureMessage()) {
      LOG(ERROR) << "Extract payload signature failed.";
      *error = ErrorCode::kDownloadPayloadVerificationError;
      return false;
    }
    DiscardBuffer(true, 0);
    // Since we extracted the SignatureMessage we need to advance the
    // checkpoint, otherwise we would reload the signature and try to extract
    // it again.
    CheckpointUpdateProgress();
  }

  return true;
}

bool DeltaPerformer::IsManifestValid() {
  return manifest_valid_;
}

bool DeltaPerformer::ParseManifestPartitions(ErrorCode* error) {
  if (major_payload_version_ == kBrilloMajorPayloadVersion) {
    partitions_.clear();
    for (const PartitionUpdate& partition : manifest_.partitions()) {
      partitions_.push_back(partition);
    }
    manifest_.clear_partitions();
  } else if (major_payload_version_ == kChromeOSMajorPayloadVersion) {
    LOG(INFO) << "Converting update information from old format.";
    PartitionUpdate root_part;
    root_part.set_partition_name(kLegacyPartitionNameRoot);
#ifdef __ANDROID__
    LOG(WARNING) << "Legacy payload major version provided to an Android "
                    "build. Assuming no post-install. Please use major version "
                    "2 or newer.";
    root_part.set_run_postinstall(false);
#else
    root_part.set_run_postinstall(true);
#endif  // __ANDROID__
    if (manifest_.has_old_rootfs_info()) {
      *root_part.mutable_old_partition_info() = manifest_.old_rootfs_info();
      manifest_.clear_old_rootfs_info();
    }
    if (manifest_.has_new_rootfs_info()) {
      *root_part.mutable_new_partition_info() = manifest_.new_rootfs_info();
      manifest_.clear_new_rootfs_info();
    }
    *root_part.mutable_operations() = manifest_.install_operations();
    manifest_.clear_install_operations();
    partitions_.push_back(std::move(root_part));

    PartitionUpdate kern_part;
    kern_part.set_partition_name(kLegacyPartitionNameKernel);
    kern_part.set_run_postinstall(false);
    if (manifest_.has_old_kernel_info()) {
      *kern_part.mutable_old_partition_info() = manifest_.old_kernel_info();
      manifest_.clear_old_kernel_info();
    }
    if (manifest_.has_new_kernel_info()) {
      *kern_part.mutable_new_partition_info() = manifest_.new_kernel_info();
      manifest_.clear_new_kernel_info();
    }
    *kern_part.mutable_operations() = manifest_.kernel_install_operations();
    manifest_.clear_kernel_install_operations();
    partitions_.push_back(std::move(kern_part));
  }

  // TODO(deymo): Remove this block of code once we switched to optional
  // source partition verification. This list of partitions in the InstallPlan
  // is initialized with the expected hashes in the payload major version 1,
  // so we need to check those now if already set. See b/23182225.
  if (!install_plan_->partitions.empty()) {
    if (!VerifySourcePartitions()) {
      *error = ErrorCode::kDownloadStateInitializationError;
      return false;
    }
  }

  // Fill in the InstallPlan::partitions based on the partitions from the
  // payload.
  install_plan_->partitions.clear();
  for (const auto& partition : partitions_) {
    InstallPlan::Partition install_part;
    install_part.name = partition.partition_name();
    install_part.run_postinstall =
        partition.has_run_postinstall() && partition.run_postinstall();
    if (install_part.run_postinstall) {
      install_part.postinstall_path =
          (partition.has_postinstall_path() ? partition.postinstall_path()
                                            : kPostinstallDefaultScript);
      install_part.filesystem_type = partition.filesystem_type();
    }

    if (partition.has_old_partition_info()) {
      const PartitionInfo& info = partition.old_partition_info();
      install_part.source_size = info.size();
      install_part.source_hash.assign(info.hash().begin(), info.hash().end());
    }

    if (!partition.has_new_partition_info()) {
      LOG(ERROR) << "Unable to get new partition hash info on partition "
                 << install_part.name << ".";
      *error = ErrorCode::kDownloadNewPartitionInfoError;
      return false;
    }
    const PartitionInfo& info = partition.new_partition_info();
    install_part.target_size = info.size();
    install_part.target_hash.assign(info.hash().begin(), info.hash().end());

    install_plan_->partitions.push_back(install_part);
  }

  if (!install_plan_->LoadPartitionsFromSlots(boot_control_)) {
    LOG(ERROR) << "Unable to determine all the partition devices.";
    *error = ErrorCode::kInstallDeviceOpenError;
    return false;
  }
  LogPartitionInfo(partitions_);
  return true;
}

bool DeltaPerformer::CanPerformInstallOperation(
    const chromeos_update_engine::InstallOperation& operation) {
  // If we don't have a data blob we can apply it right away.
  if (!operation.has_data_offset() && !operation.has_data_length())
    return true;

  // See if we have the entire data blob in the buffer
  if (operation.data_offset() < buffer_offset_) {
    LOG(ERROR) << "we threw away data it seems?";
    return false;
  }

  return (operation.data_offset() + operation.data_length() <=
          buffer_offset_ + buffer_.size());
}

bool DeltaPerformer::PerformReplaceOperation(
    const InstallOperation& operation) {
  CHECK(operation.type() == InstallOperation::REPLACE ||
        operation.type() == InstallOperation::REPLACE_BZ ||
        operation.type() == InstallOperation::REPLACE_XZ);

  // Since we delete data off the beginning of the buffer as we use it,
  // the data we need should be exactly at the beginning of the buffer.
  TEST_AND_RETURN_FALSE(buffer_offset_ == operation.data_offset());
  TEST_AND_RETURN_FALSE(buffer_.size() >= operation.data_length());

  // Extract the signature message if it's in this operation.
  if (ExtractSignatureMessageFromOperation(operation)) {
    // If this is dummy replace operation, we ignore it after extracting the
    // signature.
    DiscardBuffer(true, 0);
    return true;
  }

  // Setup the ExtentWriter stack based on the operation type.
  std::unique_ptr<ExtentWriter> writer =
    brillo::make_unique_ptr(new ZeroPadExtentWriter(
      brillo::make_unique_ptr(new DirectExtentWriter())));

  if (operation.type() == InstallOperation::REPLACE_BZ) {
    writer.reset(new BzipExtentWriter(std::move(writer)));
  } else if (operation.type() == InstallOperation::REPLACE_XZ) {
    writer.reset(new XzExtentWriter(std::move(writer)));
  }

  // Create a vector of extents to pass to the ExtentWriter.
  vector<Extent> extents;
  for (int i = 0; i < operation.dst_extents_size(); i++) {
    extents.push_back(operation.dst_extents(i));
  }

  TEST_AND_RETURN_FALSE(writer->Init(target_fd_, extents, block_size_));
  TEST_AND_RETURN_FALSE(writer->Write(buffer_.data(), operation.data_length()));
  TEST_AND_RETURN_FALSE(writer->End());

  // Update buffer
  DiscardBuffer(true, buffer_.size());
  return true;
}

bool DeltaPerformer::PerformZeroOrDiscardOperation(
    const InstallOperation& operation) {
  CHECK(operation.type() == InstallOperation::DISCARD ||
        operation.type() == InstallOperation::ZERO);

  // These operations have no blob.
  TEST_AND_RETURN_FALSE(!operation.has_data_offset());
  TEST_AND_RETURN_FALSE(!operation.has_data_length());

#ifdef BLKZEROOUT
  bool attempt_ioctl = true;
  int request =
      (operation.type() == InstallOperation::ZERO ? BLKZEROOUT : BLKDISCARD);
#else  // !defined(BLKZEROOUT)
  bool attempt_ioctl = false;
  int request = 0;
#endif  // !defined(BLKZEROOUT)

  brillo::Blob zeros;
  for (int i = 0; i < operation.dst_extents_size(); i++) {
    Extent extent = operation.dst_extents(i);
    const uint64_t start = extent.start_block() * block_size_;
    const uint64_t length = extent.num_blocks() * block_size_;
    if (attempt_ioctl) {
      int result = 0;
      if (target_fd_->BlkIoctl(request, start, length, &result) && result == 0)
        continue;
      attempt_ioctl = false;
      zeros.resize(16 * block_size_);
    }
    // In case of failure, we fall back to writing 0 to the selected region.
    for (uint64_t offset = 0; offset < length; offset += zeros.size()) {
      uint64_t chunk_length = min(length - offset,
                                  static_cast<uint64_t>(zeros.size()));
      TEST_AND_RETURN_FALSE(
          utils::PWriteAll(target_fd_, zeros.data(), chunk_length, start + offset));
    }
  }
  return true;
}

bool DeltaPerformer::PerformMoveOperation(const InstallOperation& operation) {
  // Calculate buffer size. Note, this function doesn't do a sliding
  // window to copy in case the source and destination blocks overlap.
  // If we wanted to do a sliding window, we could program the server
  // to generate deltas that effectively did a sliding window.

  uint64_t blocks_to_read = 0;
  for (int i = 0; i < operation.src_extents_size(); i++)
    blocks_to_read += operation.src_extents(i).num_blocks();

  uint64_t blocks_to_write = 0;
  for (int i = 0; i < operation.dst_extents_size(); i++)
    blocks_to_write += operation.dst_extents(i).num_blocks();

  DCHECK_EQ(blocks_to_write, blocks_to_read);
  brillo::Blob buf(blocks_to_write * block_size_);

  // Read in bytes.
  ssize_t bytes_read = 0;
  for (int i = 0; i < operation.src_extents_size(); i++) {
    ssize_t bytes_read_this_iteration = 0;
    const Extent& extent = operation.src_extents(i);
    const size_t bytes = extent.num_blocks() * block_size_;
    TEST_AND_RETURN_FALSE(extent.start_block() != kSparseHole);
    TEST_AND_RETURN_FALSE(utils::PReadAll(target_fd_,
                                          &buf[bytes_read],
                                          bytes,
                                          extent.start_block() * block_size_,
                                          &bytes_read_this_iteration));
    TEST_AND_RETURN_FALSE(
        bytes_read_this_iteration == static_cast<ssize_t>(bytes));
    bytes_read += bytes_read_this_iteration;
  }

  // Write bytes out.
  ssize_t bytes_written = 0;
  for (int i = 0; i < operation.dst_extents_size(); i++) {
    const Extent& extent = operation.dst_extents(i);
    const size_t bytes = extent.num_blocks() * block_size_;
    TEST_AND_RETURN_FALSE(extent.start_block() != kSparseHole);
    TEST_AND_RETURN_FALSE(utils::PWriteAll(target_fd_,
                                           &buf[bytes_written],
                                           bytes,
                                           extent.start_block() * block_size_));
    bytes_written += bytes;
  }
  DCHECK_EQ(bytes_written, bytes_read);
  DCHECK_EQ(bytes_written, static_cast<ssize_t>(buf.size()));
  return true;
}

namespace {

// Takes |extents| and fills an empty vector |blocks| with a block index for
// each block in |extents|. For example, [(3, 2), (8, 1)] would give [3, 4, 8].
void ExtentsToBlocks(const RepeatedPtrField<Extent>& extents,
                     vector<uint64_t>* blocks) {
  for (Extent ext : extents) {
    for (uint64_t j = 0; j < ext.num_blocks(); j++)
      blocks->push_back(ext.start_block() + j);
  }
}

// Takes |extents| and returns the number of blocks in those extents.
uint64_t GetBlockCount(const RepeatedPtrField<Extent>& extents) {
  uint64_t sum = 0;
  for (Extent ext : extents) {
    sum += ext.num_blocks();
  }
  return sum;
}

// Compare |calculated_hash| with source hash in |operation|, return false and
// dump hash if don't match.
bool ValidateSourceHash(const brillo::Blob& calculated_hash,
                        const InstallOperation& operation) {
  brillo::Blob expected_source_hash(operation.src_sha256_hash().begin(),
                                    operation.src_sha256_hash().end());
  if (calculated_hash != expected_source_hash) {
    LOG(ERROR) << "Hash verification failed. Expected hash = ";
    utils::HexDumpVector(expected_source_hash);
    LOG(ERROR) << "Calculated hash = ";
    utils::HexDumpVector(calculated_hash);
    return false;
  }
  return true;
}

}  // namespace

bool DeltaPerformer::PerformSourceCopyOperation(
    const InstallOperation& operation) {
  if (operation.has_src_length())
    TEST_AND_RETURN_FALSE(operation.src_length() % block_size_ == 0);
  if (operation.has_dst_length())
    TEST_AND_RETURN_FALSE(operation.dst_length() % block_size_ == 0);

  uint64_t blocks_to_read = GetBlockCount(operation.src_extents());
  uint64_t blocks_to_write = GetBlockCount(operation.dst_extents());
  TEST_AND_RETURN_FALSE(blocks_to_write ==  blocks_to_read);

  // Create vectors of all the individual src/dst blocks.
  vector<uint64_t> src_blocks;
  vector<uint64_t> dst_blocks;
  ExtentsToBlocks(operation.src_extents(), &src_blocks);
  ExtentsToBlocks(operation.dst_extents(), &dst_blocks);
  DCHECK_EQ(src_blocks.size(), blocks_to_read);
  DCHECK_EQ(src_blocks.size(), dst_blocks.size());

  brillo::Blob buf(block_size_);
  ssize_t bytes_read = 0;
  HashCalculator source_hasher;
  // Read/write one block at a time.
  for (uint64_t i = 0; i < blocks_to_read; i++) {
    ssize_t bytes_read_this_iteration = 0;
    uint64_t src_block = src_blocks[i];
    uint64_t dst_block = dst_blocks[i];

    // Read in bytes.
    TEST_AND_RETURN_FALSE(
        utils::PReadAll(source_fd_,
                        buf.data(),
                        block_size_,
                        src_block * block_size_,
                        &bytes_read_this_iteration));

    // Write bytes out.
    TEST_AND_RETURN_FALSE(
        utils::PWriteAll(target_fd_,
                         buf.data(),
                         block_size_,
                         dst_block * block_size_));

    bytes_read += bytes_read_this_iteration;
    TEST_AND_RETURN_FALSE(bytes_read_this_iteration ==
                          static_cast<ssize_t>(block_size_));

    if (operation.has_src_sha256_hash())
      TEST_AND_RETURN_FALSE(source_hasher.Update(buf.data(), buf.size()));
  }

  if (operation.has_src_sha256_hash()) {
    TEST_AND_RETURN_FALSE(source_hasher.Finalize());
    TEST_AND_RETURN_FALSE(
        ValidateSourceHash(source_hasher.raw_hash(), operation));
  }

  DCHECK_EQ(bytes_read, static_cast<ssize_t>(blocks_to_read * block_size_));
  return true;
}

bool DeltaPerformer::ExtentsToBsdiffPositionsString(
    const RepeatedPtrField<Extent>& extents,
    uint64_t block_size,
    uint64_t full_length,
    string* positions_string) {
  string ret;
  uint64_t length = 0;
  for (int i = 0; i < extents.size(); i++) {
    Extent extent = extents.Get(i);
    int64_t start = extent.start_block() * block_size;
    uint64_t this_length = min(full_length - length,
                               extent.num_blocks() * block_size);
    ret += base::StringPrintf("%" PRIi64 ":%" PRIu64 ",", start, this_length);
    length += this_length;
  }
  TEST_AND_RETURN_FALSE(length == full_length);
  if (!ret.empty())
    ret.resize(ret.size() - 1);  // Strip trailing comma off
  *positions_string = ret;
  return true;
}

bool DeltaPerformer::PerformBsdiffOperation(const InstallOperation& operation) {
  // Since we delete data off the beginning of the buffer as we use it,
  // the data we need should be exactly at the beginning of the buffer.
  TEST_AND_RETURN_FALSE(buffer_offset_ == operation.data_offset());
  TEST_AND_RETURN_FALSE(buffer_.size() >= operation.data_length());

  string input_positions;
  TEST_AND_RETURN_FALSE(ExtentsToBsdiffPositionsString(operation.src_extents(),
                                                       block_size_,
                                                       operation.src_length(),
                                                       &input_positions));
  string output_positions;
  TEST_AND_RETURN_FALSE(ExtentsToBsdiffPositionsString(operation.dst_extents(),
                                                       block_size_,
                                                       operation.dst_length(),
                                                       &output_positions));

  string temp_filename;
  TEST_AND_RETURN_FALSE(utils::MakeTempFile("au_patch.XXXXXX",
                                            &temp_filename,
                                            nullptr));
  ScopedPathUnlinker path_unlinker(temp_filename);
  {
    int fd = open(temp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ScopedFdCloser fd_closer(&fd);
    TEST_AND_RETURN_FALSE(
        utils::WriteAll(fd, buffer_.data(), operation.data_length()));
  }

  // Update the buffer to release the patch data memory as soon as the patch
  // file is written out.
  DiscardBuffer(true, buffer_.size());

  vector<string> cmd{kBspatchPath, target_path_, target_path_, temp_filename,
                     input_positions, output_positions};

  int return_code = 0;
  TEST_AND_RETURN_FALSE(
      Subprocess::SynchronousExecFlags(cmd, Subprocess::kSearchPath,
                                       &return_code, nullptr));
  TEST_AND_RETURN_FALSE(return_code == 0);

  if (operation.dst_length() % block_size_) {
    // Zero out rest of final block.
    // TODO(adlr): build this into bspatch; it's more efficient that way.
    const Extent& last_extent =
        operation.dst_extents(operation.dst_extents_size() - 1);
    const uint64_t end_byte =
        (last_extent.start_block() + last_extent.num_blocks()) * block_size_;
    const uint64_t begin_byte =
        end_byte - (block_size_ - operation.dst_length() % block_size_);
    brillo::Blob zeros(end_byte - begin_byte);
    TEST_AND_RETURN_FALSE(
        utils::PWriteAll(target_fd_, zeros.data(), end_byte - begin_byte, begin_byte));
  }
  return true;
}

bool DeltaPerformer::PerformSourceBsdiffOperation(
    const InstallOperation& operation) {
  // Since we delete data off the beginning of the buffer as we use it,
  // the data we need should be exactly at the beginning of the buffer.
  TEST_AND_RETURN_FALSE(buffer_offset_ == operation.data_offset());
  TEST_AND_RETURN_FALSE(buffer_.size() >= operation.data_length());
  if (operation.has_src_length())
    TEST_AND_RETURN_FALSE(operation.src_length() % block_size_ == 0);
  if (operation.has_dst_length())
    TEST_AND_RETURN_FALSE(operation.dst_length() % block_size_ == 0);

  if (operation.has_src_sha256_hash()) {
    HashCalculator source_hasher;
    const uint64_t kMaxBlocksToRead = 512;  // 2MB if block size is 4KB
    brillo::Blob buf(kMaxBlocksToRead * block_size_);
    for (const Extent& extent : operation.src_extents()) {
      for (uint64_t i = 0; i < extent.num_blocks(); i += kMaxBlocksToRead) {
        uint64_t blocks_to_read =
            min(kMaxBlocksToRead, extent.num_blocks() - i);
        ssize_t bytes_to_read = blocks_to_read * block_size_;
        ssize_t bytes_read_this_iteration = 0;
        TEST_AND_RETURN_FALSE(
            utils::PReadAll(source_fd_, buf.data(), bytes_to_read,
                            (extent.start_block() + i) * block_size_,
                            &bytes_read_this_iteration));
        TEST_AND_RETURN_FALSE(bytes_read_this_iteration == bytes_to_read);
        TEST_AND_RETURN_FALSE(source_hasher.Update(buf.data(), bytes_to_read));
      }
    }
    TEST_AND_RETURN_FALSE(source_hasher.Finalize());
    TEST_AND_RETURN_FALSE(
        ValidateSourceHash(source_hasher.raw_hash(), operation));
  }

  string input_positions;
  TEST_AND_RETURN_FALSE(ExtentsToBsdiffPositionsString(operation.src_extents(),
                                                       block_size_,
                                                       operation.src_length(),
                                                       &input_positions));
  string output_positions;
  TEST_AND_RETURN_FALSE(ExtentsToBsdiffPositionsString(operation.dst_extents(),
                                                       block_size_,
                                                       operation.dst_length(),
                                                       &output_positions));

  string temp_filename;
  TEST_AND_RETURN_FALSE(utils::MakeTempFile("au_patch.XXXXXX",
                                            &temp_filename,
                                            nullptr));
  ScopedPathUnlinker path_unlinker(temp_filename);
  {
    int fd = open(temp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ScopedFdCloser fd_closer(&fd);
    TEST_AND_RETURN_FALSE(
        utils::WriteAll(fd, buffer_.data(), operation.data_length()));
  }

  // Update the buffer to release the patch data memory as soon as the patch
  // file is written out.
  DiscardBuffer(true, buffer_.size());

  vector<string> cmd{kBspatchPath, source_path_, target_path_, temp_filename,
                     input_positions, output_positions};

  int return_code = 0;
  TEST_AND_RETURN_FALSE(
      Subprocess::SynchronousExecFlags(cmd, Subprocess::kSearchPath,
                                       &return_code, nullptr));
  TEST_AND_RETURN_FALSE(return_code == 0);
  return true;
}

bool DeltaPerformer::ExtractSignatureMessageFromOperation(
    const InstallOperation& operation) {
  if (operation.type() != InstallOperation::REPLACE ||
      !manifest_.has_signatures_offset() ||
      manifest_.signatures_offset() != operation.data_offset()) {
    return false;
  }
  TEST_AND_RETURN_FALSE(manifest_.has_signatures_size() &&
                        manifest_.signatures_size() == operation.data_length());
  TEST_AND_RETURN_FALSE(ExtractSignatureMessage());
  return true;
}

bool DeltaPerformer::ExtractSignatureMessage() {
  TEST_AND_RETURN_FALSE(signatures_message_data_.empty());
  TEST_AND_RETURN_FALSE(buffer_offset_ == manifest_.signatures_offset());
  TEST_AND_RETURN_FALSE(buffer_.size() >= manifest_.signatures_size());
  signatures_message_data_.assign(
      buffer_.begin(),
      buffer_.begin() + manifest_.signatures_size());

  // Save the signature blob because if the update is interrupted after the
  // download phase we don't go through this path anymore. Some alternatives to
  // consider:
  //
  // 1. On resume, re-download the signature blob from the server and re-verify
  // it.
  //
  // 2. Verify the signature as soon as it's received and don't checkpoint the
  // blob and the signed sha-256 context.
  LOG_IF(WARNING, !prefs_->SetString(kPrefsUpdateStateSignatureBlob,
                                     string(signatures_message_data_.begin(),
                                            signatures_message_data_.end())))
      << "Unable to store the signature blob.";

  LOG(INFO) << "Extracted signature data of size "
            << manifest_.signatures_size() << " at "
            << manifest_.signatures_offset();
  return true;
}

bool DeltaPerformer::GetPublicKeyFromResponse(base::FilePath *out_tmp_key) {
  if (hardware_->IsOfficialBuild() ||
      utils::FileExists(public_key_path_.c_str()) ||
      install_plan_->public_key_rsa.empty())
    return false;

  if (!utils::DecodeAndStoreBase64String(install_plan_->public_key_rsa,
                                         out_tmp_key))
    return false;

  return true;
}

ErrorCode DeltaPerformer::ValidateMetadataSignature(
    const brillo::Blob& payload) {
  if (payload.size() < metadata_size_ + metadata_signature_size_)
    return ErrorCode::kDownloadMetadataSignatureError;

  brillo::Blob metadata_signature_blob, metadata_signature_protobuf_blob;
  if (!install_plan_->metadata_signature.empty()) {
    // Convert base64-encoded signature to raw bytes.
    if (!brillo::data_encoding::Base64Decode(
        install_plan_->metadata_signature, &metadata_signature_blob)) {
      LOG(ERROR) << "Unable to decode base64 metadata signature: "
                 << install_plan_->metadata_signature;
      return ErrorCode::kDownloadMetadataSignatureError;
    }
  } else if (major_payload_version_ == kBrilloMajorPayloadVersion) {
    metadata_signature_protobuf_blob.assign(payload.begin() + metadata_size_,
                                            payload.begin() + metadata_size_ +
                                            metadata_signature_size_);
  }

  if (metadata_signature_blob.empty() &&
      metadata_signature_protobuf_blob.empty()) {
    if (install_plan_->hash_checks_mandatory) {
      LOG(ERROR) << "Missing mandatory metadata signature in both Omaha "
                 << "response and payload.";
      return ErrorCode::kDownloadMetadataSignatureMissingError;
    }

    LOG(WARNING) << "Cannot validate metadata as the signature is empty";
    return ErrorCode::kSuccess;
  }

  // See if we should use the public RSA key in the Omaha response.
  base::FilePath path_to_public_key(public_key_path_);
  base::FilePath tmp_key;
  if (GetPublicKeyFromResponse(&tmp_key))
    path_to_public_key = tmp_key;
  ScopedPathUnlinker tmp_key_remover(tmp_key.value());
  if (tmp_key.empty())
    tmp_key_remover.set_should_remove(false);

  LOG(INFO) << "Verifying metadata hash signature using public key: "
            << path_to_public_key.value();

  HashCalculator metadata_hasher;
  metadata_hasher.Update(payload.data(), metadata_size_);
  if (!metadata_hasher.Finalize()) {
    LOG(ERROR) << "Unable to compute actual hash of manifest";
    return ErrorCode::kDownloadMetadataSignatureVerificationError;
  }

  brillo::Blob calculated_metadata_hash = metadata_hasher.raw_hash();
  PayloadVerifier::PadRSA2048SHA256Hash(&calculated_metadata_hash);
  if (calculated_metadata_hash.empty()) {
    LOG(ERROR) << "Computed actual hash of metadata is empty.";
    return ErrorCode::kDownloadMetadataSignatureVerificationError;
  }

  if (!metadata_signature_blob.empty()) {
    brillo::Blob expected_metadata_hash;
    if (!PayloadVerifier::GetRawHashFromSignature(metadata_signature_blob,
                                                  path_to_public_key.value(),
                                                  &expected_metadata_hash)) {
      LOG(ERROR) << "Unable to compute expected hash from metadata signature";
      return ErrorCode::kDownloadMetadataSignatureError;
    }
    if (calculated_metadata_hash != expected_metadata_hash) {
      LOG(ERROR) << "Manifest hash verification failed. Expected hash = ";
      utils::HexDumpVector(expected_metadata_hash);
      LOG(ERROR) << "Calculated hash = ";
      utils::HexDumpVector(calculated_metadata_hash);
      return ErrorCode::kDownloadMetadataSignatureMismatch;
    }
  } else {
    if (!PayloadVerifier::VerifySignature(metadata_signature_protobuf_blob,
                                          path_to_public_key.value(),
                                          calculated_metadata_hash)) {
      LOG(ERROR) << "Manifest hash verification failed.";
      return ErrorCode::kDownloadMetadataSignatureMismatch;
    }
  }

  // The autoupdate_CatchBadSignatures test checks for this string in
  // log-files. Keep in sync.
  LOG(INFO) << "Metadata hash signature matches value in Omaha response.";
  return ErrorCode::kSuccess;
}

ErrorCode DeltaPerformer::ValidateManifest() {
  // Perform assorted checks to sanity check the manifest, make sure it
  // matches data from other sources, and that it is a supported version.

  bool has_old_fields =
      (manifest_.has_old_kernel_info() || manifest_.has_old_rootfs_info());
  for (const PartitionUpdate& partition : manifest_.partitions()) {
    has_old_fields = has_old_fields || partition.has_old_partition_info();
  }

  // The presence of an old partition hash is the sole indicator for a delta
  // update.
  InstallPayloadType actual_payload_type =
      has_old_fields ? InstallPayloadType::kDelta : InstallPayloadType::kFull;

  if (install_plan_->payload_type == InstallPayloadType::kUnknown) {
    LOG(INFO) << "Detected a '"
              << InstallPayloadTypeToString(actual_payload_type)
              << "' payload.";
    install_plan_->payload_type = actual_payload_type;
  } else if (install_plan_->payload_type != actual_payload_type) {
    LOG(ERROR) << "InstallPlan expected a '"
               << InstallPayloadTypeToString(install_plan_->payload_type)
               << "' payload but the downloaded manifest contains a '"
               << InstallPayloadTypeToString(actual_payload_type)
               << "' payload.";
    return ErrorCode::kPayloadMismatchedType;
  }

  // Check that the minor version is compatible.
  if (actual_payload_type == InstallPayloadType::kFull) {
    if (manifest_.minor_version() != kFullPayloadMinorVersion) {
      LOG(ERROR) << "Manifest contains minor version "
                 << manifest_.minor_version()
                 << ", but all full payloads should have version "
                 << kFullPayloadMinorVersion << ".";
      return ErrorCode::kUnsupportedMinorPayloadVersion;
    }
  } else {
    if (manifest_.minor_version() != supported_minor_version_) {
      LOG(ERROR) << "Manifest contains minor version "
                 << manifest_.minor_version()
                 << " not the supported "
                 << supported_minor_version_;
      return ErrorCode::kUnsupportedMinorPayloadVersion;
    }
  }

  if (major_payload_version_ != kChromeOSMajorPayloadVersion) {
    if (manifest_.has_old_rootfs_info() ||
        manifest_.has_new_rootfs_info() ||
        manifest_.has_old_kernel_info() ||
        manifest_.has_new_kernel_info() ||
        manifest_.install_operations_size() != 0 ||
        manifest_.kernel_install_operations_size() != 0) {
      LOG(ERROR) << "Manifest contains deprecated field only supported in "
                 << "major payload version 1, but the payload major version is "
                 << major_payload_version_;
      return ErrorCode::kPayloadMismatchedType;
    }
  }

  // TODO(garnold) we should be adding more and more manifest checks, such as
  // partition boundaries etc (see chromium-os:37661).

  return ErrorCode::kSuccess;
}

ErrorCode DeltaPerformer::ValidateOperationHash(
    const InstallOperation& operation) {
  if (!operation.data_sha256_hash().size()) {
    if (!operation.data_length()) {
      // Operations that do not have any data blob won't have any operation hash
      // either. So, these operations are always considered validated since the
      // metadata that contains all the non-data-blob portions of the operation
      // has already been validated. This is true for both HTTP and HTTPS cases.
      return ErrorCode::kSuccess;
    }

    // No hash is present for an operation that has data blobs. This shouldn't
    // happen normally for any client that has this code, because the
    // corresponding update should have been produced with the operation
    // hashes. So if it happens it means either we've turned operation hash
    // generation off in DeltaDiffGenerator or it's a regression of some sort.
    // One caveat though: The last operation is a dummy signature operation
    // that doesn't have a hash at the time the manifest is created. So we
    // should not complaint about that operation. This operation can be
    // recognized by the fact that it's offset is mentioned in the manifest.
    if (manifest_.signatures_offset() &&
        manifest_.signatures_offset() == operation.data_offset()) {
      LOG(INFO) << "Skipping hash verification for signature operation "
                << next_operation_num_ + 1;
    } else {
      if (install_plan_->hash_checks_mandatory) {
        LOG(ERROR) << "Missing mandatory operation hash for operation "
                   << next_operation_num_ + 1;
        return ErrorCode::kDownloadOperationHashMissingError;
      }

      LOG(WARNING) << "Cannot validate operation " << next_operation_num_ + 1
                   << " as there's no operation hash in manifest";
    }
    return ErrorCode::kSuccess;
  }

  brillo::Blob expected_op_hash;
  expected_op_hash.assign(operation.data_sha256_hash().data(),
                          (operation.data_sha256_hash().data() +
                           operation.data_sha256_hash().size()));

  HashCalculator operation_hasher;
  operation_hasher.Update(buffer_.data(), operation.data_length());
  if (!operation_hasher.Finalize()) {
    LOG(ERROR) << "Unable to compute actual hash of operation "
               << next_operation_num_;
    return ErrorCode::kDownloadOperationHashVerificationError;
  }

  brillo::Blob calculated_op_hash = operation_hasher.raw_hash();
  if (calculated_op_hash != expected_op_hash) {
    LOG(ERROR) << "Hash verification failed for operation "
               << next_operation_num_ << ". Expected hash = ";
    utils::HexDumpVector(expected_op_hash);
    LOG(ERROR) << "Calculated hash over " << operation.data_length()
               << " bytes at offset: " << operation.data_offset() << " = ";
    utils::HexDumpVector(calculated_op_hash);
    return ErrorCode::kDownloadOperationHashMismatch;
  }

  return ErrorCode::kSuccess;
}

#define TEST_AND_RETURN_VAL(_retval, _condition)                \
  do {                                                          \
    if (!(_condition)) {                                        \
      LOG(ERROR) << "VerifyPayload failure: " << #_condition;   \
      return _retval;                                           \
    }                                                           \
  } while (0);

ErrorCode DeltaPerformer::VerifyPayload(
    const string& update_check_response_hash,
    const uint64_t update_check_response_size) {

  // See if we should use the public RSA key in the Omaha response.
  base::FilePath path_to_public_key(public_key_path_);
  base::FilePath tmp_key;
  if (GetPublicKeyFromResponse(&tmp_key))
    path_to_public_key = tmp_key;
  ScopedPathUnlinker tmp_key_remover(tmp_key.value());
  if (tmp_key.empty())
    tmp_key_remover.set_should_remove(false);

  LOG(INFO) << "Verifying payload using public key: "
            << path_to_public_key.value();

  // Verifies the download size.
  TEST_AND_RETURN_VAL(ErrorCode::kPayloadSizeMismatchError,
                      update_check_response_size ==
                      metadata_size_ + metadata_signature_size_ +
                      buffer_offset_);

  // Verifies the payload hash.
  const string& payload_hash_data = payload_hash_calculator_.hash();
  TEST_AND_RETURN_VAL(ErrorCode::kDownloadPayloadVerificationError,
                      !payload_hash_data.empty());
  TEST_AND_RETURN_VAL(ErrorCode::kPayloadHashMismatchError,
                      payload_hash_data == update_check_response_hash);

  // Verifies the signed payload hash.
  if (!utils::FileExists(path_to_public_key.value().c_str())) {
    LOG(WARNING) << "Not verifying signed delta payload -- missing public key.";
    return ErrorCode::kSuccess;
  }
  TEST_AND_RETURN_VAL(ErrorCode::kSignedDeltaPayloadExpectedError,
                      !signatures_message_data_.empty());
  brillo::Blob hash_data = signed_hash_calculator_.raw_hash();
  TEST_AND_RETURN_VAL(ErrorCode::kDownloadPayloadPubKeyVerificationError,
                      PayloadVerifier::PadRSA2048SHA256Hash(&hash_data));
  TEST_AND_RETURN_VAL(ErrorCode::kDownloadPayloadPubKeyVerificationError,
                      !hash_data.empty());

  if (!PayloadVerifier::VerifySignature(
      signatures_message_data_, path_to_public_key.value(), hash_data)) {
    // The autoupdate_CatchBadSignatures test checks for this string
    // in log-files. Keep in sync.
    LOG(ERROR) << "Public key verification failed, thus update failed.";
    return ErrorCode::kDownloadPayloadPubKeyVerificationError;
  }

  LOG(INFO) << "Payload hash matches value in payload.";

  // At this point, we are guaranteed to have downloaded a full payload, i.e
  // the one whose size matches the size mentioned in Omaha response. If any
  // errors happen after this, it's likely a problem with the payload itself or
  // the state of the system and not a problem with the URL or network.  So,
  // indicate that to the download delegate so that AU can backoff
  // appropriately.
  if (download_delegate_)
    download_delegate_->DownloadComplete();

  return ErrorCode::kSuccess;
}

namespace {
void LogVerifyError(const string& type,
                    const string& device,
                    uint64_t size,
                    const string& local_hash,
                    const string& expected_hash) {
  LOG(ERROR) << "This is a server-side error due to "
             << "mismatched delta update image!";
  LOG(ERROR) << "The delta I've been given contains a " << type << " delta "
             << "update that must be applied over a " << type << " with "
             << "a specific checksum, but the " << type << " we're starting "
             << "with doesn't have that checksum! This means that "
             << "the delta I've been given doesn't match my existing "
             << "system. The " << type << " partition I have has hash: "
             << local_hash << " but the update expected me to have "
             << expected_hash << " .";
  LOG(INFO) << "To get the checksum of the " << type << " partition run this"
               "command: dd if=" << device << " bs=1M count=" << size
            << " iflag=count_bytes 2>/dev/null | openssl dgst -sha256 -binary "
               "| openssl base64";
  LOG(INFO) << "To get the checksum of partitions in a bin file, "
            << "run: .../src/scripts/sha256_partitions.sh .../file.bin";
}

string StringForHashBytes(const void* bytes, size_t size) {
  return brillo::data_encoding::Base64Encode(bytes, size);
}
}  // namespace

bool DeltaPerformer::VerifySourcePartitions() {
  LOG(INFO) << "Verifying source partitions.";
  CHECK(manifest_valid_);
  CHECK(install_plan_);
  if (install_plan_->partitions.size() != partitions_.size()) {
    DLOG(ERROR) << "The list of partitions in the InstallPlan doesn't match the "
                   "list received in the payload. The InstallPlan has "
                << install_plan_->partitions.size()
                << " partitions while the payload has " << partitions_.size()
                << " partitions.";
    return false;
  }
  for (size_t i = 0; i < partitions_.size(); ++i) {
    if (partitions_[i].partition_name() != install_plan_->partitions[i].name) {
      DLOG(ERROR) << "The InstallPlan's partition " << i << " is \""
                  << install_plan_->partitions[i].name
                  << "\" but the payload expects it to be \""
                  << partitions_[i].partition_name()
                  << "\". This is an error in the DeltaPerformer setup.";
      return false;
    }
    if (!partitions_[i].has_old_partition_info())
      continue;
    const PartitionInfo& info = partitions_[i].old_partition_info();
    const InstallPlan::Partition& plan_part = install_plan_->partitions[i];
    bool valid =
        !plan_part.source_hash.empty() &&
        plan_part.source_hash.size() == info.hash().size() &&
        memcmp(plan_part.source_hash.data(),
               info.hash().data(),
               plan_part.source_hash.size()) == 0;
    if (!valid) {
      LogVerifyError(partitions_[i].partition_name(),
                     plan_part.source_path,
                     info.hash().size(),
                     StringForHashBytes(plan_part.source_hash.data(),
                                        plan_part.source_hash.size()),
                     StringForHashBytes(info.hash().data(),
                                        info.hash().size()));
      return false;
    }
  }
  return true;
}

void DeltaPerformer::DiscardBuffer(bool do_advance_offset,
                                   size_t signed_hash_buffer_size) {
  // Update the buffer offset.
  if (do_advance_offset)
    buffer_offset_ += buffer_.size();

  // Hash the content.
  payload_hash_calculator_.Update(buffer_.data(), buffer_.size());
  signed_hash_calculator_.Update(buffer_.data(), signed_hash_buffer_size);

  // Swap content with an empty vector to ensure that all memory is released.
  brillo::Blob().swap(buffer_);
}

bool DeltaPerformer::CanResumeUpdate(PrefsInterface* prefs,
                                     string update_check_response_hash) {
  int64_t next_operation = kUpdateStateOperationInvalid;
  if (!(prefs->GetInt64(kPrefsUpdateStateNextOperation, &next_operation) &&
        next_operation != kUpdateStateOperationInvalid &&
        next_operation > 0))
    return false;

  string interrupted_hash;
  if (!(prefs->GetString(kPrefsUpdateCheckResponseHash, &interrupted_hash) &&
        !interrupted_hash.empty() &&
        interrupted_hash == update_check_response_hash))
    return false;

  int64_t resumed_update_failures;
  // Note that storing this value is optional, but if it is there it should not
  // be more than the limit.
  if (prefs->GetInt64(kPrefsResumedUpdateFailures, &resumed_update_failures) &&
      resumed_update_failures > kMaxResumedUpdateFailures)
    return false;

  // Sanity check the rest.
  int64_t next_data_offset = -1;
  if (!(prefs->GetInt64(kPrefsUpdateStateNextDataOffset, &next_data_offset) &&
        next_data_offset >= 0))
    return false;

  string sha256_context;
  if (!(prefs->GetString(kPrefsUpdateStateSHA256Context, &sha256_context) &&
        !sha256_context.empty()))
    return false;

  int64_t manifest_metadata_size = 0;
  if (!(prefs->GetInt64(kPrefsManifestMetadataSize, &manifest_metadata_size) &&
        manifest_metadata_size > 0))
    return false;

  int64_t manifest_signature_size = 0;
  if (!(prefs->GetInt64(kPrefsManifestSignatureSize,
                        &manifest_signature_size) &&
        manifest_signature_size >= 0))
    return false;

  return true;
}

bool DeltaPerformer::ResetUpdateProgress(PrefsInterface* prefs, bool quick) {
  TEST_AND_RETURN_FALSE(prefs->SetInt64(kPrefsUpdateStateNextOperation,
                                        kUpdateStateOperationInvalid));
  if (!quick) {
    prefs->SetString(kPrefsUpdateCheckResponseHash, "");
    prefs->SetInt64(kPrefsUpdateStateNextDataOffset, -1);
    prefs->SetInt64(kPrefsUpdateStateNextDataLength, 0);
    prefs->SetString(kPrefsUpdateStateSHA256Context, "");
    prefs->SetString(kPrefsUpdateStateSignedSHA256Context, "");
    prefs->SetString(kPrefsUpdateStateSignatureBlob, "");
    prefs->SetInt64(kPrefsManifestMetadataSize, -1);
    prefs->SetInt64(kPrefsManifestSignatureSize, -1);
    prefs->SetInt64(kPrefsResumedUpdateFailures, 0);
  }
  return true;
}

bool DeltaPerformer::CheckpointUpdateProgress() {
  Terminator::set_exit_blocked(true);
  if (last_updated_buffer_offset_ != buffer_offset_) {
    // Resets the progress in case we die in the middle of the state update.
    ResetUpdateProgress(prefs_, true);
    TEST_AND_RETURN_FALSE(
        prefs_->SetString(kPrefsUpdateStateSHA256Context,
                          payload_hash_calculator_.GetContext()));
    TEST_AND_RETURN_FALSE(
        prefs_->SetString(kPrefsUpdateStateSignedSHA256Context,
                          signed_hash_calculator_.GetContext()));
    TEST_AND_RETURN_FALSE(prefs_->SetInt64(kPrefsUpdateStateNextDataOffset,
                                           buffer_offset_));
    last_updated_buffer_offset_ = buffer_offset_;

    if (next_operation_num_ < num_total_operations_) {
      size_t partition_index = current_partition_;
      while (next_operation_num_ >= acc_num_operations_[partition_index])
        partition_index++;
      const size_t partition_operation_num = next_operation_num_ - (
          partition_index ? acc_num_operations_[partition_index - 1] : 0);
      const InstallOperation& op =
          partitions_[partition_index].operations(partition_operation_num);
      TEST_AND_RETURN_FALSE(prefs_->SetInt64(kPrefsUpdateStateNextDataLength,
                                             op.data_length()));
    } else {
      TEST_AND_RETURN_FALSE(prefs_->SetInt64(kPrefsUpdateStateNextDataLength,
                                             0));
    }
  }
  TEST_AND_RETURN_FALSE(prefs_->SetInt64(kPrefsUpdateStateNextOperation,
                                         next_operation_num_));
  return true;
}

bool DeltaPerformer::PrimeUpdateState() {
  CHECK(manifest_valid_);
  block_size_ = manifest_.block_size();

  int64_t next_operation = kUpdateStateOperationInvalid;
  if (!prefs_->GetInt64(kPrefsUpdateStateNextOperation, &next_operation) ||
      next_operation == kUpdateStateOperationInvalid ||
      next_operation <= 0) {
    // Initiating a new update, no more state needs to be initialized.
    return true;
  }
  next_operation_num_ = next_operation;

  // Resuming an update -- load the rest of the update state.
  int64_t next_data_offset = -1;
  TEST_AND_RETURN_FALSE(prefs_->GetInt64(kPrefsUpdateStateNextDataOffset,
                                         &next_data_offset) &&
                        next_data_offset >= 0);
  buffer_offset_ = next_data_offset;

  // The signed hash context and the signature blob may be empty if the
  // interrupted update didn't reach the signature.
  string signed_hash_context;
  if (prefs_->GetString(kPrefsUpdateStateSignedSHA256Context,
                        &signed_hash_context)) {
    TEST_AND_RETURN_FALSE(
        signed_hash_calculator_.SetContext(signed_hash_context));
  }

  string signature_blob;
  if (prefs_->GetString(kPrefsUpdateStateSignatureBlob, &signature_blob)) {
    signatures_message_data_.assign(signature_blob.begin(),
                                    signature_blob.end());
  }

  string hash_context;
  TEST_AND_RETURN_FALSE(prefs_->GetString(kPrefsUpdateStateSHA256Context,
                                          &hash_context) &&
                        payload_hash_calculator_.SetContext(hash_context));

  int64_t manifest_metadata_size = 0;
  TEST_AND_RETURN_FALSE(prefs_->GetInt64(kPrefsManifestMetadataSize,
                                         &manifest_metadata_size) &&
                        manifest_metadata_size > 0);
  metadata_size_ = manifest_metadata_size;

  int64_t manifest_signature_size = 0;
  TEST_AND_RETURN_FALSE(
      prefs_->GetInt64(kPrefsManifestSignatureSize, &manifest_signature_size) &&
      manifest_signature_size >= 0);
  metadata_signature_size_ = manifest_signature_size;

  // Advance the download progress to reflect what doesn't need to be
  // re-downloaded.
  total_bytes_received_ += buffer_offset_;

  // Speculatively count the resume as a failure.
  int64_t resumed_update_failures;
  if (prefs_->GetInt64(kPrefsResumedUpdateFailures, &resumed_update_failures)) {
    resumed_update_failures++;
  } else {
    resumed_update_failures = 1;
  }
  prefs_->SetInt64(kPrefsResumedUpdateFailures, resumed_update_failures);
  return true;
}

}  // namespace chromeos_update_engine

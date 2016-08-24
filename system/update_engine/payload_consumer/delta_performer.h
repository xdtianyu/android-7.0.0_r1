//
// Copyright (C) 2010 The Android Open Source Project
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

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_DELTA_PERFORMER_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_DELTA_PERFORMER_H_

#include <inttypes.h>

#include <string>
#include <vector>

#include <base/time/time.h>
#include <brillo/secure_blob.h>
#include <google/protobuf/repeated_field.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "update_engine/common/hash_calculator.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/payload_consumer/file_descriptor.h"
#include "update_engine/payload_consumer/file_writer.h"
#include "update_engine/payload_consumer/install_plan.h"
#include "update_engine/update_metadata.pb.h"

namespace chromeos_update_engine {

class DownloadActionDelegate;
class BootControlInterface;
class HardwareInterface;
class PrefsInterface;

// This class performs the actions in a delta update synchronously. The delta
// update itself should be passed in in chunks as it is received.

class DeltaPerformer : public FileWriter {
 public:
  enum MetadataParseResult {
    kMetadataParseSuccess,
    kMetadataParseError,
    kMetadataParseInsufficientData,
  };

  static const uint64_t kDeltaVersionOffset;
  static const uint64_t kDeltaVersionSize;
  static const uint64_t kDeltaManifestSizeOffset;
  static const uint64_t kDeltaManifestSizeSize;
  static const uint64_t kDeltaMetadataSignatureSizeSize;
  static const uint64_t kMaxPayloadHeaderSize;
  static const uint64_t kSupportedMajorPayloadVersion;
  static const uint32_t kSupportedMinorPayloadVersion;

  // Defines the granularity of progress logging in terms of how many "completed
  // chunks" we want to report at the most.
  static const unsigned kProgressLogMaxChunks;
  // Defines a timeout since the last progress was logged after which we want to
  // force another log message (even if the current chunk was not completed).
  static const unsigned kProgressLogTimeoutSeconds;
  // These define the relative weights (0-100) we give to the different work
  // components associated with an update when computing an overall progress.
  // Currently they include the download progress and the number of completed
  // operations. They must add up to one hundred (100).
  static const unsigned kProgressDownloadWeight;
  static const unsigned kProgressOperationsWeight;

  DeltaPerformer(PrefsInterface* prefs,
                 BootControlInterface* boot_control,
                 HardwareInterface* hardware,
                 DownloadActionDelegate* download_delegate,
                 InstallPlan* install_plan)
      : prefs_(prefs),
        boot_control_(boot_control),
        hardware_(hardware),
        download_delegate_(download_delegate),
        install_plan_(install_plan) {}

  // FileWriter's Write implementation where caller doesn't care about
  // error codes.
  bool Write(const void* bytes, size_t count) override {
    ErrorCode error;
    return Write(bytes, count, &error);
  }

  // FileWriter's Write implementation that returns a more specific |error| code
  // in case of failures in Write operation.
  bool Write(const void* bytes, size_t count, ErrorCode *error) override;

  // Wrapper around close. Returns 0 on success or -errno on error.
  // Closes both 'path' given to Open() and the kernel path.
  int Close() override;

  // Open the target and source (if delta payload) file descriptors for the
  // |current_partition_|. The manifest needs to be already parsed for this to
  // work. Returns whether the required file descriptors were successfully open.
  bool OpenCurrentPartition();

  // Closes the current partition file descriptors if open. Returns 0 on success
  // or -errno on error.
  int CloseCurrentPartition();

  // Returns |true| only if the manifest has been processed and it's valid.
  bool IsManifestValid();

  // Verifies the downloaded payload against the signed hash included in the
  // payload, against the update check hash (which is in base64 format)  and
  // size using the public key and returns ErrorCode::kSuccess on success, an
  // error code on failure.  This method should be called after closing the
  // stream. Note this method skips the signed hash check if the public key is
  // unavailable; it returns ErrorCode::kSignedDeltaPayloadExpectedError if the
  // public key is available but the delta payload doesn't include a signature.
  ErrorCode VerifyPayload(const std::string& update_check_response_hash,
                          const uint64_t update_check_response_size);

  // Converts an ordered collection of Extent objects which contain data of
  // length full_length to a comma-separated string. For each Extent, the
  // string will have the start offset and then the length in bytes.
  // The length value of the last extent in the string may be short, since
  // the full length of all extents in the string is capped to full_length.
  // Also, an extent starting at kSparseHole, appears as -1 in the string.
  // For example, if the Extents are {1, 1}, {4, 2}, {kSparseHole, 1},
  // {0, 1}, block_size is 4096, and full_length is 5 * block_size - 13,
  // the resulting string will be: "4096:4096,16384:8192,-1:4096,0:4083"
  static bool ExtentsToBsdiffPositionsString(
      const google::protobuf::RepeatedPtrField<Extent>& extents,
      uint64_t block_size,
      uint64_t full_length,
      std::string* positions_string);

  // Returns true if a previous update attempt can be continued based on the
  // persistent preferences and the new update check response hash.
  static bool CanResumeUpdate(PrefsInterface* prefs,
                              std::string update_check_response_hash);

  // Resets the persistent update progress state to indicate that an update
  // can't be resumed. Performs a quick update-in-progress reset if |quick| is
  // true, otherwise resets all progress-related update state. Returns true on
  // success, false otherwise.
  static bool ResetUpdateProgress(PrefsInterface* prefs, bool quick);

  // Attempts to parse the update metadata starting from the beginning of
  // |payload|. On success, returns kMetadataParseSuccess. Returns
  // kMetadataParseInsufficientData if more data is needed to parse the complete
  // metadata. Returns kMetadataParseError if the metadata can't be parsed given
  // the payload.
  MetadataParseResult ParsePayloadMetadata(const brillo::Blob& payload,
                                           ErrorCode* error);

  void set_public_key_path(const std::string& public_key_path) {
    public_key_path_ = public_key_path;
  }

  // Set |*out_offset| to the byte offset where the size of the metadata signature
  // is stored in a payload. Return true on success, if this field is not
  // present in the payload, return false.
  bool GetMetadataSignatureSizeOffset(uint64_t* out_offset) const;

  // Set |*out_offset| to the byte offset at which the manifest protobuf begins
  // in a payload. Return true on success, false if the offset is unknown.
  bool GetManifestOffset(uint64_t* out_offset) const;

  // Returns the size of the payload metadata, which includes the payload header
  // and the manifest. If the header was not yet parsed, returns zero.
  uint64_t GetMetadataSize() const;

  // If the manifest was successfully parsed, copies it to |*out_manifest_p|.
  // Returns true on success.
  bool GetManifest(DeltaArchiveManifest* out_manifest_p) const;

  // Return true if header parsing is finished and no errors occurred.
  bool IsHeaderParsed() const;

  // Returns the major payload version. If the version was not yet parsed,
  // returns zero.
  uint64_t GetMajorVersion() const;

  // Returns the delta minor version. If this value is defined in the manifest,
  // it returns that value, otherwise it returns the default value.
  uint32_t GetMinorVersion() const;

 private:
  friend class DeltaPerformerTest;
  friend class DeltaPerformerIntegrationTest;
  FRIEND_TEST(DeltaPerformerTest, BrilloMetadataSignatureSizeTest);
  FRIEND_TEST(DeltaPerformerTest, BrilloVerifyMetadataSignatureTest);
  FRIEND_TEST(DeltaPerformerTest, UsePublicKeyFromResponse);

  // Parse and move the update instructions of all partitions into our local
  // |partitions_| variable based on the version of the payload. Requires the
  // manifest to be parsed and valid.
  bool ParseManifestPartitions(ErrorCode* error);

  // Appends up to |*count_p| bytes from |*bytes_p| to |buffer_|, but only to
  // the extent that the size of |buffer_| does not exceed |max|. Advances
  // |*cbytes_p| and decreases |*count_p| by the actual number of bytes copied,
  // and returns this number.
  size_t CopyDataToBuffer(const char** bytes_p, size_t* count_p, size_t max);

  // If |op_result| is false, emits an error message using |op_type_name| and
  // sets |*error| accordingly. Otherwise does nothing. Returns |op_result|.
  bool HandleOpResult(bool op_result, const char* op_type_name,
                      ErrorCode* error);

  // Logs the progress of downloading/applying an update.
  void LogProgress(const char* message_prefix);

  // Update overall progress metrics, log as necessary.
  void UpdateOverallProgress(bool force_log, const char* message_prefix);

  // Verifies that the expected source partition hashes (if present) match the
  // hashes for the current partitions. Returns true if there are no expected
  // hashes in the payload (e.g., if it's a new-style full update) or if the
  // hashes match; returns false otherwise.
  bool VerifySourcePartitions();

  // Returns true if enough of the delta file has been passed via Write()
  // to be able to perform a given install operation.
  bool CanPerformInstallOperation(const InstallOperation& operation);

  // Checks the integrity of the payload manifest. Returns true upon success,
  // false otherwise.
  ErrorCode ValidateManifest();

  // Validates that the hash of the blobs corresponding to the given |operation|
  // matches what's specified in the manifest in the payload.
  // Returns ErrorCode::kSuccess on match or a suitable error code otherwise.
  ErrorCode ValidateOperationHash(const InstallOperation& operation);

  // Given the |payload|, verifies that the signed hash of its metadata matches
  // what's specified in the install plan from Omaha (if present) or the
  // metadata signature in payload itself (if present). Returns
  // ErrorCode::kSuccess on match or a suitable error code otherwise. This
  // method must be called before any part of the metadata is parsed so that a
  // man-in-the-middle attack on the SSL connection to the payload server
  // doesn't exploit any vulnerability in the code that parses the protocol
  // buffer.
  ErrorCode ValidateMetadataSignature(const brillo::Blob& payload);

  // Returns true on success.
  bool PerformInstallOperation(const InstallOperation& operation);

  // These perform a specific type of operation and return true on success.
  bool PerformReplaceOperation(const InstallOperation& operation);
  bool PerformZeroOrDiscardOperation(const InstallOperation& operation);
  bool PerformMoveOperation(const InstallOperation& operation);
  bool PerformBsdiffOperation(const InstallOperation& operation);
  bool PerformSourceCopyOperation(const InstallOperation& operation);
  bool PerformSourceBsdiffOperation(const InstallOperation& operation);

  // Extracts the payload signature message from the blob on the |operation| if
  // the offset matches the one specified by the manifest. Returns whether the
  // signature was extracted.
  bool ExtractSignatureMessageFromOperation(const InstallOperation& operation);

  // Extracts the payload signature message from the current |buffer_| if the
  // offset matches the one specified by the manifest. Returns whether the
  // signature was extracted.
  bool ExtractSignatureMessage();

  // Updates the payload hash calculator with the bytes in |buffer_|, also
  // updates the signed hash calculator with the first |signed_hash_buffer_size|
  // bytes in |buffer_|. Then discard the content, ensuring that memory is being
  // deallocated. If |do_advance_offset|, advances the internal offset counter
  // accordingly.
  void DiscardBuffer(bool do_advance_offset, size_t signed_hash_buffer_size);

  // Checkpoints the update progress into persistent storage to allow this
  // update attempt to be resumed after reboot.
  bool CheckpointUpdateProgress();

  // Primes the required update state. Returns true if the update state was
  // successfully initialized to a saved resume state or if the update is a new
  // update. Returns false otherwise.
  bool PrimeUpdateState();

  // If the Omaha response contains a public RSA key and we're allowed
  // to use it (e.g. if we're in developer mode), extract the key from
  // the response and store it in a temporary file and return true. In
  // the affirmative the path to the temporary file is stored in
  // |out_tmp_key| and it is the responsibility of the caller to clean
  // it up.
  bool GetPublicKeyFromResponse(base::FilePath *out_tmp_key);

  // Update Engine preference store.
  PrefsInterface* prefs_;

  // BootControl and Hardware interface references.
  BootControlInterface* boot_control_;
  HardwareInterface* hardware_;

  // The DownloadActionDelegate instance monitoring the DownloadAction, or a
  // nullptr if not used.
  DownloadActionDelegate* download_delegate_;

  // Install Plan based on Omaha Response.
  InstallPlan* install_plan_;

  // File descriptor of the source partition. Only set while updating a
  // partition when using a delta payload.
  FileDescriptorPtr source_fd_{nullptr};

  // File descriptor of the target partition. Only set while performing the
  // operations of a given partition.
  FileDescriptorPtr target_fd_{nullptr};

  // Paths the |source_fd_| and |target_fd_| refer to.
  std::string source_path_;
  std::string target_path_;

  // Parsed manifest. Set after enough bytes to parse the manifest were
  // downloaded.
  DeltaArchiveManifest manifest_;
  bool manifest_parsed_{false};
  bool manifest_valid_{false};
  uint64_t metadata_size_{0};
  uint64_t manifest_size_{0};
  uint32_t metadata_signature_size_{0};
  uint64_t major_payload_version_{0};

  // Accumulated number of operations per partition. The i-th element is the
  // sum of the number of operations for all the partitions from 0 to i
  // inclusive. Valid when |manifest_valid_| is true.
  std::vector<size_t> acc_num_operations_;

  // The total operations in a payload. Valid when |manifest_valid_| is true,
  // otherwise 0.
  size_t num_total_operations_{0};

  // The list of partitions to update as found in the manifest major version 2.
  // When parsing an older manifest format, the information is converted over to
  // this format instead.
  std::vector<PartitionUpdate> partitions_;

  // Index in the list of partitions (|partitions_| member) of the current
  // partition being processed.
  size_t current_partition_{0};

  // Index of the next operation to perform in the manifest. The index is linear
  // on the total number of operation on the manifest.
  size_t next_operation_num_{0};

  // A buffer used for accumulating downloaded data. Initially, it stores the
  // payload metadata; once that's downloaded and parsed, it stores data for the
  // next update operation.
  brillo::Blob buffer_;
  // Offset of buffer_ in the binary blobs section of the update.
  uint64_t buffer_offset_{0};

  // Last |buffer_offset_| value updated as part of the progress update.
  uint64_t last_updated_buffer_offset_{std::numeric_limits<uint64_t>::max()};

  // The block size (parsed from the manifest).
  uint32_t block_size_{0};

  // Calculates the whole payload file hash, including headers and signatures.
  HashCalculator payload_hash_calculator_;

  // Calculates the hash of the portion of the payload signed by the payload
  // signature. This hash skips the metadata signature portion, located after
  // the metadata and doesn't include the payload signature itself.
  HashCalculator signed_hash_calculator_;

  // Signatures message blob extracted directly from the payload.
  brillo::Blob signatures_message_data_;

  // The public key to be used. Provided as a member so that tests can
  // override with test keys.
  std::string public_key_path_{constants::kUpdatePayloadPublicKeyPath};

  // The number of bytes received so far, used for progress tracking.
  size_t total_bytes_received_{0};

  // An overall progress counter, which should reflect both download progress
  // and the ratio of applied operations. Range is 0-100.
  unsigned overall_progress_{0};

  // The last progress chunk recorded.
  unsigned last_progress_chunk_{0};

  // The timeout after which we should force emitting a progress log (constant),
  // and the actual point in time for the next forced log to be emitted.
  const base::TimeDelta forced_progress_log_wait_{
      base::TimeDelta::FromSeconds(kProgressLogTimeoutSeconds)};
  base::Time forced_progress_log_time_;

  // The payload major payload version supported by DeltaPerformer.
  uint64_t supported_major_version_{kSupportedMajorPayloadVersion};

  // The delta minor payload version supported by DeltaPerformer.
  uint32_t supported_minor_version_{kSupportedMinorPayloadVersion};

  DISALLOW_COPY_AND_ASSIGN(DeltaPerformer);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_DELTA_PERFORMER_H_

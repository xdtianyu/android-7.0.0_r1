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
#include <inttypes.h>

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <gmock/gmock.h>
#include <google/protobuf/repeated_field.h>
#include <gtest/gtest.h>

#include "update_engine/common/constants.h"
#include "update_engine/common/fake_boot_control.h"
#include "update_engine/common/fake_hardware.h"
#include "update_engine/common/fake_prefs.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/mock_download_action.h"
#include "update_engine/payload_consumer/payload_constants.h"
#include "update_engine/payload_generator/bzip.h"
#include "update_engine/payload_generator/extent_ranges.h"
#include "update_engine/payload_generator/payload_file.h"
#include "update_engine/payload_generator/payload_signer.h"
#include "update_engine/update_metadata.pb.h"

namespace chromeos_update_engine {

using std::string;
using std::vector;
using test_utils::System;
using test_utils::kRandomString;
using testing::_;

extern const char* kUnittestPrivateKeyPath;
extern const char* kUnittestPublicKeyPath;

namespace {

const char kBogusMetadataSignature1[] =
    "awSFIUdUZz2VWFiR+ku0Pj00V7bPQPQFYQSXjEXr3vaw3TE4xHV5CraY3/YrZpBv"
    "J5z4dSBskoeuaO1TNC/S6E05t+yt36tE4Fh79tMnJ/z9fogBDXWgXLEUyG78IEQr"
    "YH6/eBsQGT2RJtBgXIXbZ9W+5G9KmGDoPOoiaeNsDuqHiBc/58OFsrxskH8E6vMS"
    "BmMGGk82mvgzic7ApcoURbCGey1b3Mwne/hPZ/bb9CIyky8Og9IfFMdL2uAweOIR"
    "fjoTeLYZpt+WN65Vu7jJ0cQN8e1y+2yka5112wpRf/LLtPgiAjEZnsoYpLUd7CoV"
    "pLRtClp97kN2+tXGNBQqkA==";

#ifdef __ANDROID__
const char kZlibFingerprintPath[] =
    "/data/nativetest/update_engine_unittests/zlib_fingerprint";
#else
const char kZlibFingerprintPath[] = "/etc/zlib_fingerprint";
#endif  // __ANDROID__

// Different options that determine what we should fill into the
// install_plan.metadata_signature to simulate the contents received in the
// Omaha response.
enum MetadataSignatureTest {
  kEmptyMetadataSignature,
  kInvalidMetadataSignature,
  kValidMetadataSignature,
};

// Compressed data without checksum, generated with:
// echo -n a | xz -9 --check=none | hexdump -v -e '"    " 12/1 "0x%02x, " "\n"'
const uint8_t kXzCompressedData[] = {
    0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00, 0x00, 0x00, 0xff, 0x12, 0xd9, 0x41,
    0x02, 0x00, 0x21, 0x01, 0x1c, 0x00, 0x00, 0x00, 0x10, 0xcf, 0x58, 0xcc,
    0x01, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x01,
    0xad, 0xa6, 0x58, 0x04, 0x06, 0x72, 0x9e, 0x7a, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x59, 0x5a,
};

}  // namespace

class DeltaPerformerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    install_plan_.source_slot = 0;
    install_plan_.target_slot = 1;
    EXPECT_CALL(mock_delegate_, ShouldCancel(_))
        .WillRepeatedly(testing::Return(false));
  }

  // Test helper placed where it can easily be friended from DeltaPerformer.
  void RunManifestValidation(const DeltaArchiveManifest& manifest,
                             uint64_t major_version,
                             InstallPayloadType payload_type,
                             ErrorCode expected) {
    install_plan_.payload_type = payload_type;

    // The Manifest we are validating.
    performer_.manifest_.CopyFrom(manifest);
    performer_.major_payload_version_ = major_version;

    EXPECT_EQ(expected, performer_.ValidateManifest());
  }

  brillo::Blob GeneratePayload(const brillo::Blob& blob_data,
                               const vector<AnnotatedOperation>& aops,
                               bool sign_payload) {
    return GeneratePayload(blob_data, aops, sign_payload,
                           DeltaPerformer::kSupportedMajorPayloadVersion,
                           DeltaPerformer::kSupportedMinorPayloadVersion);
  }

  brillo::Blob GeneratePayload(const brillo::Blob& blob_data,
                               const vector<AnnotatedOperation>& aops,
                               bool sign_payload,
                               uint64_t major_version,
                               uint32_t minor_version) {
    string blob_path;
    EXPECT_TRUE(utils::MakeTempFile("Blob-XXXXXX", &blob_path, nullptr));
    ScopedPathUnlinker blob_unlinker(blob_path);
    EXPECT_TRUE(utils::WriteFile(blob_path.c_str(),
                                 blob_data.data(),
                                 blob_data.size()));

    PayloadGenerationConfig config;
    config.version.major = major_version;
    config.version.minor = minor_version;

    PayloadFile payload;
    EXPECT_TRUE(payload.Init(config));

    PartitionConfig old_part(kLegacyPartitionNameRoot);
    if (minor_version != kFullPayloadMinorVersion) {
      // When generating a delta payload we need to include the old partition
      // information to mark it as a delta payload.
      old_part.path = "/dev/null";
      old_part.size = 0;
    }
    PartitionConfig new_part(kLegacyPartitionNameRoot);
    new_part.path = "/dev/zero";
    new_part.size = 1234;

    payload.AddPartition(old_part, new_part, aops);

    // We include a kernel partition without operations.
    old_part.name = kLegacyPartitionNameKernel;
    new_part.name = kLegacyPartitionNameKernel;
    new_part.size = 0;
    payload.AddPartition(old_part, new_part, {});

    string payload_path;
    EXPECT_TRUE(utils::MakeTempFile("Payload-XXXXXX", &payload_path, nullptr));
    ScopedPathUnlinker payload_unlinker(payload_path);
    EXPECT_TRUE(payload.WritePayload(payload_path, blob_path,
        sign_payload ? kUnittestPrivateKeyPath : "",
        &install_plan_.metadata_size));

    brillo::Blob payload_data;
    EXPECT_TRUE(utils::ReadFile(payload_path, &payload_data));
    return payload_data;
  }

  // Apply |payload_data| on partition specified in |source_path|.
  // Expect result of performer_.Write() to be |expect_success|.
  // Returns the result of the payload application.
  brillo::Blob ApplyPayload(const brillo::Blob& payload_data,
                            const string& source_path,
                            bool expect_success) {
    return ApplyPayloadToData(payload_data, source_path, brillo::Blob(),
                              expect_success);
  }

  // Apply the payload provided in |payload_data| reading from the |source_path|
  // file and writing the contents to a new partition. The existing data in the
  // new target file are set to |target_data| before applying the payload.
  // Expect result of performer_.Write() to be |expect_success|.
  // Returns the result of the payload application.
  brillo::Blob ApplyPayloadToData(const brillo::Blob& payload_data,
                                  const string& source_path,
                                  const brillo::Blob& target_data,
                                  bool expect_success) {
    string new_part;
    EXPECT_TRUE(utils::MakeTempFile("Partition-XXXXXX", &new_part, nullptr));
    ScopedPathUnlinker partition_unlinker(new_part);
    EXPECT_TRUE(utils::WriteFile(new_part.c_str(), target_data.data(),
                                 target_data.size()));

    // We installed the operations only in the rootfs partition, but the
    // delta performer needs to access all the partitions.
    fake_boot_control_.SetPartitionDevice(
        kLegacyPartitionNameRoot, install_plan_.target_slot, new_part);
    fake_boot_control_.SetPartitionDevice(
        kLegacyPartitionNameRoot, install_plan_.source_slot, source_path);
    fake_boot_control_.SetPartitionDevice(
        kLegacyPartitionNameKernel, install_plan_.target_slot, "/dev/null");
    fake_boot_control_.SetPartitionDevice(
        kLegacyPartitionNameKernel, install_plan_.source_slot, "/dev/null");

    EXPECT_EQ(expect_success,
              performer_.Write(payload_data.data(), payload_data.size()));
    EXPECT_EQ(0, performer_.Close());

    brillo::Blob partition_data;
    EXPECT_TRUE(utils::ReadFile(new_part, &partition_data));
    return partition_data;
  }

  // Calls delta performer's Write method by pretending to pass in bytes from a
  // delta file whose metadata size is actual_metadata_size and tests if all
  // checks are correctly performed if the install plan contains
  // expected_metadata_size and that the result of the parsing are as per
  // hash_checks_mandatory flag.
  void DoMetadataSizeTest(uint64_t expected_metadata_size,
                          uint64_t actual_metadata_size,
                          bool hash_checks_mandatory) {
    install_plan_.hash_checks_mandatory = hash_checks_mandatory;

    // Set a valid magic string and version number 1.
    EXPECT_TRUE(performer_.Write("CrAU", 4));
    uint64_t version = htobe64(kChromeOSMajorPayloadVersion);
    EXPECT_TRUE(performer_.Write(&version, 8));

    install_plan_.metadata_size = expected_metadata_size;
    ErrorCode error_code;
    // When filling in size in manifest, exclude the size of the 20-byte header.
    uint64_t size_in_manifest = htobe64(actual_metadata_size - 20);
    bool result = performer_.Write(&size_in_manifest, 8, &error_code);
    if (expected_metadata_size == actual_metadata_size ||
        !hash_checks_mandatory) {
      EXPECT_TRUE(result);
    } else {
      EXPECT_FALSE(result);
      EXPECT_EQ(ErrorCode::kDownloadInvalidMetadataSize, error_code);
    }

    EXPECT_LT(performer_.Close(), 0);
  }

  // Generates a valid delta file but tests the delta performer by suppling
  // different metadata signatures as per metadata_signature_test flag and
  // sees if the result of the parsing are as per hash_checks_mandatory flag.
  void DoMetadataSignatureTest(MetadataSignatureTest metadata_signature_test,
                               bool sign_payload,
                               bool hash_checks_mandatory) {

    // Loads the payload and parses the manifest.
    brillo::Blob payload = GeneratePayload(brillo::Blob(),
        vector<AnnotatedOperation>(), sign_payload,
        kChromeOSMajorPayloadVersion, kFullPayloadMinorVersion);

    LOG(INFO) << "Payload size: " << payload.size();

    install_plan_.hash_checks_mandatory = hash_checks_mandatory;

    DeltaPerformer::MetadataParseResult expected_result, actual_result;
    ErrorCode expected_error, actual_error;

    // Fill up the metadata signature in install plan according to the test.
    switch (metadata_signature_test) {
      case kEmptyMetadataSignature:
        install_plan_.metadata_signature.clear();
        expected_result = DeltaPerformer::kMetadataParseError;
        expected_error = ErrorCode::kDownloadMetadataSignatureMissingError;
        break;

      case kInvalidMetadataSignature:
        install_plan_.metadata_signature = kBogusMetadataSignature1;
        expected_result = DeltaPerformer::kMetadataParseError;
        expected_error = ErrorCode::kDownloadMetadataSignatureMismatch;
        break;

      case kValidMetadataSignature:
      default:
        // Set the install plan's metadata size to be the same as the one
        // in the manifest so that we pass the metadata size checks. Only
        // then we can get to manifest signature checks.
        ASSERT_TRUE(PayloadSigner::GetMetadataSignature(
            payload.data(),
            install_plan_.metadata_size,
            kUnittestPrivateKeyPath,
            &install_plan_.metadata_signature));
        EXPECT_FALSE(install_plan_.metadata_signature.empty());
        expected_result = DeltaPerformer::kMetadataParseSuccess;
        expected_error = ErrorCode::kSuccess;
        break;
    }

    // Ignore the expected result/error if hash checks are not mandatory.
    if (!hash_checks_mandatory) {
      expected_result = DeltaPerformer::kMetadataParseSuccess;
      expected_error = ErrorCode::kSuccess;
    }

    // Use the public key corresponding to the private key used above to
    // sign the metadata.
    EXPECT_TRUE(utils::FileExists(kUnittestPublicKeyPath));
    performer_.set_public_key_path(kUnittestPublicKeyPath);

    // Init actual_error with an invalid value so that we make sure
    // ParsePayloadMetadata properly populates it in all cases.
    actual_error = ErrorCode::kUmaReportedMax;
    actual_result = performer_.ParsePayloadMetadata(payload, &actual_error);

    EXPECT_EQ(expected_result, actual_result);
    EXPECT_EQ(expected_error, actual_error);

    // Check that the parsed metadata size is what's expected. This test
    // implicitly confirms that the metadata signature is valid, if required.
    EXPECT_EQ(install_plan_.metadata_size, performer_.GetMetadataSize());
  }

  void SetSupportedMajorVersion(uint64_t major_version) {
    performer_.supported_major_version_ = major_version;
  }
  FakePrefs prefs_;
  InstallPlan install_plan_;
  FakeBootControl fake_boot_control_;
  FakeHardware fake_hardware_;
  MockDownloadActionDelegate mock_delegate_;
  DeltaPerformer performer_{
      &prefs_, &fake_boot_control_, &fake_hardware_, &mock_delegate_, &install_plan_};
};

TEST_F(DeltaPerformerTest, FullPayloadWriteTest) {
  install_plan_.payload_type = InstallPayloadType::kFull;
  brillo::Blob expected_data = brillo::Blob(std::begin(kRandomString),
                                            std::end(kRandomString));
  expected_data.resize(4096);  // block size
  vector<AnnotatedOperation> aops;
  AnnotatedOperation aop;
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_data_offset(0);
  aop.op.set_data_length(expected_data.size());
  aop.op.set_type(InstallOperation::REPLACE);
  aops.push_back(aop);

  brillo::Blob payload_data = GeneratePayload(expected_data, aops, false,
      kChromeOSMajorPayloadVersion, kFullPayloadMinorVersion);

  EXPECT_EQ(expected_data, ApplyPayload(payload_data, "/dev/null", true));
}

TEST_F(DeltaPerformerTest, ShouldCancelTest) {
  install_plan_.payload_type = InstallPayloadType::kFull;
  brillo::Blob expected_data = brillo::Blob(std::begin(kRandomString),
                                            std::end(kRandomString));
  expected_data.resize(4096);  // block size
  vector<AnnotatedOperation> aops;
  AnnotatedOperation aop;
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_data_offset(0);
  aop.op.set_data_length(expected_data.size());
  aop.op.set_type(InstallOperation::REPLACE);
  aops.push_back(aop);

  brillo::Blob payload_data = GeneratePayload(expected_data, aops, false,
      kChromeOSMajorPayloadVersion, kFullPayloadMinorVersion);

  testing::Mock::VerifyAndClearExpectations(&mock_delegate_);
  EXPECT_CALL(mock_delegate_, ShouldCancel(_))
      .WillOnce(
          testing::DoAll(testing::SetArgumentPointee<0>(ErrorCode::kError),
                         testing::Return(true)));

  ApplyPayload(payload_data, "/dev/null", false);
}

TEST_F(DeltaPerformerTest, ReplaceOperationTest) {
  brillo::Blob expected_data = brillo::Blob(std::begin(kRandomString),
                                            std::end(kRandomString));
  expected_data.resize(4096);  // block size
  vector<AnnotatedOperation> aops;
  AnnotatedOperation aop;
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_data_offset(0);
  aop.op.set_data_length(expected_data.size());
  aop.op.set_type(InstallOperation::REPLACE);
  aops.push_back(aop);

  brillo::Blob payload_data = GeneratePayload(expected_data, aops, false);

  EXPECT_EQ(expected_data, ApplyPayload(payload_data, "/dev/null", true));
}

TEST_F(DeltaPerformerTest, ReplaceBzOperationTest) {
  brillo::Blob expected_data = brillo::Blob(std::begin(kRandomString),
                                            std::end(kRandomString));
  expected_data.resize(4096);  // block size
  brillo::Blob bz_data;
  EXPECT_TRUE(BzipCompress(expected_data, &bz_data));

  vector<AnnotatedOperation> aops;
  AnnotatedOperation aop;
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_data_offset(0);
  aop.op.set_data_length(bz_data.size());
  aop.op.set_type(InstallOperation::REPLACE_BZ);
  aops.push_back(aop);

  brillo::Blob payload_data = GeneratePayload(bz_data, aops, false);

  EXPECT_EQ(expected_data, ApplyPayload(payload_data, "/dev/null", true));
}

TEST_F(DeltaPerformerTest, ReplaceXzOperationTest) {
  brillo::Blob xz_data(std::begin(kXzCompressedData),
                         std::end(kXzCompressedData));
  // The compressed xz data contains only a single "a", but the operation should
  // pad the rest of the two blocks with zeros.
  brillo::Blob expected_data = brillo::Blob(4096, 0);
  expected_data[0] = 'a';

  AnnotatedOperation aop;
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_data_offset(0);
  aop.op.set_data_length(xz_data.size());
  aop.op.set_type(InstallOperation::REPLACE_XZ);
  vector<AnnotatedOperation> aops = {aop};

  brillo::Blob payload_data = GeneratePayload(xz_data, aops, false);

  EXPECT_EQ(expected_data, ApplyPayload(payload_data, "/dev/null", true));
}

TEST_F(DeltaPerformerTest, ZeroOperationTest) {
  brillo::Blob existing_data = brillo::Blob(4096 * 10, 'a');
  brillo::Blob expected_data = existing_data;
  // Blocks 4, 5 and 7 should have zeros instead of 'a' after the operation is
  // applied.
  std::fill(expected_data.data() + 4096 * 4, expected_data.data() + 4096 * 6,
            0);
  std::fill(expected_data.data() + 4096 * 7, expected_data.data() + 4096 * 8,
            0);

  AnnotatedOperation aop;
  *(aop.op.add_dst_extents()) = ExtentForRange(4, 2);
  *(aop.op.add_dst_extents()) = ExtentForRange(7, 1);
  aop.op.set_type(InstallOperation::ZERO);
  vector<AnnotatedOperation> aops = {aop};

  brillo::Blob payload_data = GeneratePayload(brillo::Blob(), aops, false);

  EXPECT_EQ(expected_data,
            ApplyPayloadToData(payload_data, "/dev/null", existing_data, true));
}

TEST_F(DeltaPerformerTest, SourceCopyOperationTest) {
  brillo::Blob expected_data(std::begin(kRandomString),
                             std::end(kRandomString));
  expected_data.resize(4096);  // block size
  AnnotatedOperation aop;
  *(aop.op.add_src_extents()) = ExtentForRange(0, 1);
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_type(InstallOperation::SOURCE_COPY);
  brillo::Blob src_hash;
  EXPECT_TRUE(HashCalculator::RawHashOfData(expected_data, &src_hash));
  aop.op.set_src_sha256_hash(src_hash.data(), src_hash.size());

  brillo::Blob payload_data = GeneratePayload(brillo::Blob(), {aop}, false);

  string source_path;
  EXPECT_TRUE(utils::MakeTempFile("Source-XXXXXX",
                                  &source_path, nullptr));
  ScopedPathUnlinker path_unlinker(source_path);
  EXPECT_TRUE(utils::WriteFile(source_path.c_str(),
                               expected_data.data(),
                               expected_data.size()));

  EXPECT_EQ(expected_data, ApplyPayload(payload_data, source_path, true));
}

TEST_F(DeltaPerformerTest, SourceHashMismatchTest) {
  brillo::Blob expected_data = {'f', 'o', 'o'};
  brillo::Blob actual_data = {'b', 'a', 'r'};
  expected_data.resize(4096);  // block size
  actual_data.resize(4096);    // block size

  AnnotatedOperation aop;
  *(aop.op.add_src_extents()) = ExtentForRange(0, 1);
  *(aop.op.add_dst_extents()) = ExtentForRange(0, 1);
  aop.op.set_type(InstallOperation::SOURCE_COPY);
  brillo::Blob src_hash;
  EXPECT_TRUE(HashCalculator::RawHashOfData(expected_data, &src_hash));
  aop.op.set_src_sha256_hash(src_hash.data(), src_hash.size());

  brillo::Blob payload_data = GeneratePayload(brillo::Blob(), {aop}, false);

  string source_path;
  EXPECT_TRUE(utils::MakeTempFile("Source-XXXXXX", &source_path, nullptr));
  ScopedPathUnlinker path_unlinker(source_path);
  EXPECT_TRUE(utils::WriteFile(source_path.c_str(), actual_data.data(),
                               actual_data.size()));

  EXPECT_EQ(actual_data, ApplyPayload(payload_data, source_path, false));
}

TEST_F(DeltaPerformerTest, ExtentsToByteStringTest) {
  uint64_t test[] = {1, 1, 4, 2, 0, 1};
  static_assert(arraysize(test) % 2 == 0, "Array size uneven");
  const uint64_t block_size = 4096;
  const uint64_t file_length = 4 * block_size - 13;

  google::protobuf::RepeatedPtrField<Extent> extents;
  for (size_t i = 0; i < arraysize(test); i += 2) {
    *(extents.Add()) = ExtentForRange(test[i], test[i + 1]);
  }

  string expected_output = "4096:4096,16384:8192,0:4083";
  string actual_output;
  EXPECT_TRUE(DeltaPerformer::ExtentsToBsdiffPositionsString(extents,
                                                             block_size,
                                                             file_length,
                                                             &actual_output));
  EXPECT_EQ(expected_output, actual_output);
}

TEST_F(DeltaPerformerTest, ValidateManifestFullGoodTest) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;
  manifest.mutable_new_kernel_info();
  manifest.mutable_new_rootfs_info();
  manifest.set_minor_version(kFullPayloadMinorVersion);

  RunManifestValidation(manifest,
                        kChromeOSMajorPayloadVersion,
                        InstallPayloadType::kFull,
                        ErrorCode::kSuccess);
}

TEST_F(DeltaPerformerTest, ValidateManifestDeltaGoodTest) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;
  manifest.mutable_old_kernel_info();
  manifest.mutable_old_rootfs_info();
  manifest.mutable_new_kernel_info();
  manifest.mutable_new_rootfs_info();
  manifest.set_minor_version(DeltaPerformer::kSupportedMinorPayloadVersion);

  RunManifestValidation(manifest,
                        kChromeOSMajorPayloadVersion,
                        InstallPayloadType::kDelta,
                        ErrorCode::kSuccess);
}

TEST_F(DeltaPerformerTest, ValidateManifestFullUnsetMinorVersion) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;

  RunManifestValidation(manifest,
                        DeltaPerformer::kSupportedMajorPayloadVersion,
                        InstallPayloadType::kFull,
                        ErrorCode::kSuccess);
}

TEST_F(DeltaPerformerTest, ValidateManifestDeltaUnsetMinorVersion) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;
  // Add an empty old_rootfs_info() to trick the DeltaPerformer into think that
  // this is a delta payload manifest with a missing minor version.
  manifest.mutable_old_rootfs_info();

  RunManifestValidation(manifest,
                        DeltaPerformer::kSupportedMajorPayloadVersion,
                        InstallPayloadType::kDelta,
                        ErrorCode::kUnsupportedMinorPayloadVersion);
}

TEST_F(DeltaPerformerTest, ValidateManifestFullOldKernelTest) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;
  manifest.mutable_old_kernel_info();
  manifest.mutable_new_kernel_info();
  manifest.mutable_new_rootfs_info();
  manifest.set_minor_version(DeltaPerformer::kSupportedMinorPayloadVersion);

  RunManifestValidation(manifest,
                        kChromeOSMajorPayloadVersion,
                        InstallPayloadType::kFull,
                        ErrorCode::kPayloadMismatchedType);
}

TEST_F(DeltaPerformerTest, ValidateManifestFullOldRootfsTest) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;
  manifest.mutable_old_rootfs_info();
  manifest.mutable_new_kernel_info();
  manifest.mutable_new_rootfs_info();
  manifest.set_minor_version(DeltaPerformer::kSupportedMinorPayloadVersion);

  RunManifestValidation(manifest,
                        kChromeOSMajorPayloadVersion,
                        InstallPayloadType::kFull,
                        ErrorCode::kPayloadMismatchedType);
}

TEST_F(DeltaPerformerTest, ValidateManifestFullPartitionUpdateTest) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;
  PartitionUpdate* partition = manifest.add_partitions();
  partition->mutable_old_partition_info();
  partition->mutable_new_partition_info();
  manifest.set_minor_version(DeltaPerformer::kSupportedMinorPayloadVersion);

  RunManifestValidation(manifest,
                        kBrilloMajorPayloadVersion,
                        InstallPayloadType::kFull,
                        ErrorCode::kPayloadMismatchedType);
}

TEST_F(DeltaPerformerTest, ValidateManifestBadMinorVersion) {
  // The Manifest we are validating.
  DeltaArchiveManifest manifest;

  // Generate a bad version number.
  manifest.set_minor_version(DeltaPerformer::kSupportedMinorPayloadVersion +
                             10000);
  // Mark the manifest as a delta payload by setting old_rootfs_info.
  manifest.mutable_old_rootfs_info();

  RunManifestValidation(manifest,
                        DeltaPerformer::kSupportedMajorPayloadVersion,
                        InstallPayloadType::kDelta,
                        ErrorCode::kUnsupportedMinorPayloadVersion);
}

TEST_F(DeltaPerformerTest, BrilloMetadataSignatureSizeTest) {
  EXPECT_TRUE(performer_.Write(kDeltaMagic, sizeof(kDeltaMagic)));

  uint64_t major_version = htobe64(kBrilloMajorPayloadVersion);
  EXPECT_TRUE(performer_.Write(&major_version, 8));

  uint64_t manifest_size = rand() % 256;
  uint64_t manifest_size_be = htobe64(manifest_size);
  EXPECT_TRUE(performer_.Write(&manifest_size_be, 8));

  uint32_t metadata_signature_size = rand() % 256;
  uint32_t metadata_signature_size_be = htobe32(metadata_signature_size);
  EXPECT_TRUE(performer_.Write(&metadata_signature_size_be, 4));

  EXPECT_LT(performer_.Close(), 0);

  EXPECT_TRUE(performer_.IsHeaderParsed());
  EXPECT_EQ(kBrilloMajorPayloadVersion, performer_.GetMajorVersion());
  uint64_t manifest_offset;
  EXPECT_TRUE(performer_.GetManifestOffset(&manifest_offset));
  EXPECT_EQ(24U, manifest_offset);  // 4 + 8 + 8 + 4
  EXPECT_EQ(manifest_offset + manifest_size, performer_.GetMetadataSize());
  EXPECT_EQ(metadata_signature_size, performer_.metadata_signature_size_);
}

TEST_F(DeltaPerformerTest, BrilloVerifyMetadataSignatureTest) {
  brillo::Blob payload_data = GeneratePayload({}, {}, true,
                                              kBrilloMajorPayloadVersion,
                                              kSourceMinorPayloadVersion);
  install_plan_.hash_checks_mandatory = true;
  // Just set these value so that we can use ValidateMetadataSignature directly.
  performer_.major_payload_version_ = kBrilloMajorPayloadVersion;
  performer_.metadata_size_ = install_plan_.metadata_size;
  uint64_t signature_length;
  EXPECT_TRUE(PayloadSigner::SignatureBlobLength({kUnittestPrivateKeyPath},
                                                 &signature_length));
  performer_.metadata_signature_size_ = signature_length;
  performer_.set_public_key_path(kUnittestPublicKeyPath);
  EXPECT_EQ(ErrorCode::kSuccess,
            performer_.ValidateMetadataSignature(payload_data));
}

TEST_F(DeltaPerformerTest, BadDeltaMagicTest) {
  EXPECT_TRUE(performer_.Write("junk", 4));
  EXPECT_FALSE(performer_.Write("morejunk", 8));
  EXPECT_LT(performer_.Close(), 0);
}

TEST_F(DeltaPerformerTest, MissingMandatoryMetadataSizeTest) {
  DoMetadataSizeTest(0, 75456, true);
}

TEST_F(DeltaPerformerTest, MissingNonMandatoryMetadataSizeTest) {
  DoMetadataSizeTest(0, 123456, false);
}

TEST_F(DeltaPerformerTest, InvalidMandatoryMetadataSizeTest) {
  DoMetadataSizeTest(13000, 140000, true);
}

TEST_F(DeltaPerformerTest, InvalidNonMandatoryMetadataSizeTest) {
  DoMetadataSizeTest(40000, 50000, false);
}

TEST_F(DeltaPerformerTest, ValidMandatoryMetadataSizeTest) {
  DoMetadataSizeTest(85376, 85376, true);
}

TEST_F(DeltaPerformerTest, MandatoryEmptyMetadataSignatureTest) {
  DoMetadataSignatureTest(kEmptyMetadataSignature, true, true);
}

TEST_F(DeltaPerformerTest, NonMandatoryEmptyMetadataSignatureTest) {
  DoMetadataSignatureTest(kEmptyMetadataSignature, true, false);
}

TEST_F(DeltaPerformerTest, MandatoryInvalidMetadataSignatureTest) {
  DoMetadataSignatureTest(kInvalidMetadataSignature, true, true);
}

TEST_F(DeltaPerformerTest, NonMandatoryInvalidMetadataSignatureTest) {
  DoMetadataSignatureTest(kInvalidMetadataSignature, true, false);
}

TEST_F(DeltaPerformerTest, MandatoryValidMetadataSignature1Test) {
  DoMetadataSignatureTest(kValidMetadataSignature, false, true);
}

TEST_F(DeltaPerformerTest, MandatoryValidMetadataSignature2Test) {
  DoMetadataSignatureTest(kValidMetadataSignature, true, true);
}

TEST_F(DeltaPerformerTest, NonMandatoryValidMetadataSignatureTest) {
  DoMetadataSignatureTest(kValidMetadataSignature, true, false);
}

TEST_F(DeltaPerformerTest, UsePublicKeyFromResponse) {
  base::FilePath key_path;

  // The result of the GetPublicKeyResponse() method is based on three things
  //
  //  1. Whether it's an official build; and
  //  2. Whether the Public RSA key to be used is in the root filesystem; and
  //  3. Whether the response has a public key
  //
  // We test all eight combinations to ensure that we only use the
  // public key in the response if
  //
  //  a. it's not an official build; and
  //  b. there is no key in the root filesystem.

  string temp_dir;
  EXPECT_TRUE(utils::MakeTempDirectory("PublicKeyFromResponseTests.XXXXXX",
                                       &temp_dir));
  string non_existing_file = temp_dir + "/non-existing";
  string existing_file = temp_dir + "/existing";
  EXPECT_EQ(0, System(base::StringPrintf("touch %s", existing_file.c_str())));

  // Non-official build, non-existing public-key, key in response -> true
  fake_hardware_.SetIsOfficialBuild(false);
  performer_.public_key_path_ = non_existing_file;
  install_plan_.public_key_rsa = "VGVzdAo="; // result of 'echo "Test" | base64'
  EXPECT_TRUE(performer_.GetPublicKeyFromResponse(&key_path));
  EXPECT_FALSE(key_path.empty());
  EXPECT_EQ(unlink(key_path.value().c_str()), 0);
  // Same with official build -> false
  fake_hardware_.SetIsOfficialBuild(true);
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));

  // Non-official build, existing public-key, key in response -> false
  fake_hardware_.SetIsOfficialBuild(false);
  performer_.public_key_path_ = existing_file;
  install_plan_.public_key_rsa = "VGVzdAo="; // result of 'echo "Test" | base64'
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));
  // Same with official build -> false
  fake_hardware_.SetIsOfficialBuild(true);
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));

  // Non-official build, non-existing public-key, no key in response -> false
  fake_hardware_.SetIsOfficialBuild(false);
  performer_.public_key_path_ = non_existing_file;
  install_plan_.public_key_rsa = "";
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));
  // Same with official build -> false
  fake_hardware_.SetIsOfficialBuild(true);
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));

  // Non-official build, existing public-key, no key in response -> false
  fake_hardware_.SetIsOfficialBuild(false);
  performer_.public_key_path_ = existing_file;
  install_plan_.public_key_rsa = "";
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));
  // Same with official build -> false
  fake_hardware_.SetIsOfficialBuild(true);
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));

  // Non-official build, non-existing public-key, key in response
  // but invalid base64 -> false
  fake_hardware_.SetIsOfficialBuild(false);
  performer_.public_key_path_ = non_existing_file;
  install_plan_.public_key_rsa = "not-valid-base64";
  EXPECT_FALSE(performer_.GetPublicKeyFromResponse(&key_path));

  EXPECT_TRUE(base::DeleteFile(base::FilePath(temp_dir), true));
}

TEST_F(DeltaPerformerTest, ConfVersionsMatch) {
  // Test that the versions in update_engine.conf that is installed to the
  // image match the supported delta versions in the update engine.
  uint32_t minor_version;
  brillo::KeyValueStore store;
  EXPECT_TRUE(store.Load(base::FilePath("update_engine.conf")));
  EXPECT_TRUE(utils::GetMinorVersion(store, &minor_version));
  EXPECT_EQ(DeltaPerformer::kSupportedMinorPayloadVersion, minor_version);

  string major_version_str;
  uint64_t major_version;
  EXPECT_TRUE(store.GetString("PAYLOAD_MAJOR_VERSION", &major_version_str));
  EXPECT_TRUE(base::StringToUint64(major_version_str, &major_version));
  EXPECT_EQ(DeltaPerformer::kSupportedMajorPayloadVersion, major_version);
}

// Test that we recognize our own zlib compressor implementation as supported.
// All other equivalent implementations should be added to
// kCompatibleZlibFingerprint.
TEST_F(DeltaPerformerTest, ZlibFingerprintMatch) {
  string fingerprint;
  EXPECT_TRUE(base::ReadFileToString(base::FilePath(kZlibFingerprintPath),
                                     &fingerprint));
  EXPECT_TRUE(utils::IsZlibCompatible(fingerprint));
}

}  // namespace chromeos_update_engine

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

#include "update_engine/payload_generator/payload_signer.h"

#include <endian.h>

#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <brillo/data_encoding.h>
#include <brillo/streams/file_stream.h>
#include <brillo/streams/stream.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include "update_engine/common/hash_calculator.h"
#include "update_engine/common/subprocess.h"
#include "update_engine/common/utils.h"
#include "update_engine/payload_consumer/delta_performer.h"
#include "update_engine/payload_consumer/payload_constants.h"
#include "update_engine/payload_consumer/payload_verifier.h"
#include "update_engine/payload_generator/delta_diff_generator.h"
#include "update_engine/payload_generator/payload_file.h"
#include "update_engine/update_metadata.pb.h"

using std::string;
using std::vector;

namespace chromeos_update_engine {

namespace {

// The payload verifier will check all the signatures included in the payload
// regardless of the version field. Old version of the verifier require the
// version field to be included and be 1.
const uint32_t kSignatureMessageLegacyVersion = 1;

// Given raw |signatures|, packs them into a protobuf and serializes it into a
// binary blob. Returns true on success, false otherwise.
bool ConvertSignatureToProtobufBlob(const vector<brillo::Blob>& signatures,
                                    brillo::Blob* out_signature_blob) {
  // Pack it into a protobuf
  Signatures out_message;
  for (const brillo::Blob& signature : signatures) {
    Signatures_Signature* sig_message = out_message.add_signatures();
    // Set all the signatures with the same version number.
    sig_message->set_version(kSignatureMessageLegacyVersion);
    sig_message->set_data(signature.data(), signature.size());
  }

  // Serialize protobuf
  string serialized;
  TEST_AND_RETURN_FALSE(out_message.AppendToString(&serialized));
  out_signature_blob->insert(out_signature_blob->end(),
                             serialized.begin(),
                             serialized.end());
  LOG(INFO) << "Signature blob size: " << out_signature_blob->size();
  return true;
}

// Given an unsigned payload under |payload_path| and the |signature_blob| and
// |metadata_signature_blob| generates an updated payload that includes the
// signatures. It populates |out_metadata_size| with the size of the final
// manifest after adding the dummy signature operation, and
// |out_signatures_offset| with the expected offset for the new blob, and
// |out_metadata_signature_size| which will be size of |metadata_signature_blob|
// if the payload major version supports metadata signature, 0 otherwise.
// Returns true on success, false otherwise.
bool AddSignatureBlobToPayload(const string& payload_path,
                               const brillo::Blob& signature_blob,
                               const brillo::Blob& metadata_signature_blob,
                               brillo::Blob* out_payload,
                               uint64_t* out_metadata_size,
                               uint32_t* out_metadata_signature_size,
                               uint64_t* out_signatures_offset) {
  uint64_t manifest_offset = 20;
  const int kProtobufSizeOffset = 12;

  DeltaArchiveManifest manifest;
  uint64_t metadata_size, major_version;
  uint32_t metadata_signature_size;
  TEST_AND_RETURN_FALSE(
      PayloadSigner::LoadPayloadMetadata(payload_path,
                                         nullptr,
                                         &manifest,
                                         &major_version,
                                         &metadata_size,
                                         &metadata_signature_size));

  brillo::Blob payload;
  TEST_AND_RETURN_FALSE(utils::ReadFile(payload_path, &payload));

  if (major_version == kBrilloMajorPayloadVersion) {
    // Write metadata signature size in header.
    uint32_t metadata_signature_size_be =
        htobe32(metadata_signature_blob.size());
    memcpy(payload.data() + manifest_offset, &metadata_signature_size_be,
           sizeof(metadata_signature_size_be));
    manifest_offset += sizeof(metadata_signature_size_be);
    // Replace metadata signature.
    payload.erase(payload.begin() + metadata_size,
                  payload.begin() + metadata_size + metadata_signature_size);
    payload.insert(payload.begin() + metadata_size,
                   metadata_signature_blob.begin(),
                   metadata_signature_blob.end());
    metadata_signature_size = metadata_signature_blob.size();
    LOG(INFO) << "Metadata signature size: " << metadata_signature_size;
  }

  // Is there already a signature op in place?
  if (manifest.has_signatures_size()) {
    // The signature op is tied to the size of the signature blob, but not it's
    // contents. We don't allow the manifest to change if there is already an op
    // present, because that might invalidate previously generated
    // hashes/signatures.
    if (manifest.signatures_size() != signature_blob.size()) {
      LOG(ERROR) << "Attempt to insert different signature sized blob. "
                 << "(current:" << manifest.signatures_size()
                 << "new:" << signature_blob.size() << ")";
      return false;
    }

    LOG(INFO) << "Matching signature sizes already present.";
  } else {
    // Updates the manifest to include the signature operation.
    PayloadSigner::AddSignatureToManifest(
        payload.size() - metadata_size - metadata_signature_size,
        signature_blob.size(),
        major_version == kChromeOSMajorPayloadVersion,
        &manifest);

    // Updates the payload to include the new manifest.
    string serialized_manifest;
    TEST_AND_RETURN_FALSE(manifest.AppendToString(&serialized_manifest));
    LOG(INFO) << "Updated protobuf size: " << serialized_manifest.size();
    payload.erase(payload.begin() + manifest_offset,
                  payload.begin() + metadata_size);
    payload.insert(payload.begin() + manifest_offset,
                   serialized_manifest.begin(),
                   serialized_manifest.end());

    // Updates the protobuf size.
    uint64_t size_be = htobe64(serialized_manifest.size());
    memcpy(&payload[kProtobufSizeOffset], &size_be, sizeof(size_be));
    metadata_size = serialized_manifest.size() + manifest_offset;

    LOG(INFO) << "Updated payload size: " << payload.size();
    LOG(INFO) << "Updated metadata size: " << metadata_size;
  }
  uint64_t signatures_offset = metadata_size + metadata_signature_size +
                               manifest.signatures_offset();
  LOG(INFO) << "Signature Blob Offset: " << signatures_offset;
  payload.resize(signatures_offset);
  payload.insert(payload.begin() + signatures_offset,
                 signature_blob.begin(),
                 signature_blob.end());

  *out_payload = std::move(payload);
  *out_metadata_size = metadata_size;
  *out_metadata_signature_size = metadata_signature_size;
  *out_signatures_offset = signatures_offset;
  return true;
}

// Given a |payload| with correct signature op and metadata signature size in
// header and |metadata_size|, |metadata_signature_size|, |signatures_offset|,
// calculate hash for payload and metadata, save it to |out_hash_data| and
// |out_metadata_hash|.
bool CalculateHashFromPayload(const brillo::Blob& payload,
                              const uint64_t metadata_size,
                              const uint32_t metadata_signature_size,
                              const uint64_t signatures_offset,
                              brillo::Blob* out_hash_data,
                              brillo::Blob* out_metadata_hash) {
  if (out_metadata_hash) {
    // Calculates the hash on the manifest.
    TEST_AND_RETURN_FALSE(
        HashCalculator::RawHashOfBytes(payload.data(), metadata_size,
                                       out_metadata_hash));
  }
  if (out_hash_data) {
    // Calculates the hash on the updated payload. Note that we skip metadata
    // signature and payload signature.
    HashCalculator calc;
    TEST_AND_RETURN_FALSE(calc.Update(payload.data(), metadata_size));
    TEST_AND_RETURN_FALSE(signatures_offset >=
                          metadata_size + metadata_signature_size);
    TEST_AND_RETURN_FALSE(calc.Update(
        payload.data() + metadata_size + metadata_signature_size,
        signatures_offset - metadata_size - metadata_signature_size));
    TEST_AND_RETURN_FALSE(calc.Finalize());
    *out_hash_data = calc.raw_hash();
  }
  return true;
}

}  // namespace

void PayloadSigner::AddSignatureToManifest(uint64_t signature_blob_offset,
                                           uint64_t signature_blob_length,
                                           bool add_dummy_op,
                                           DeltaArchiveManifest* manifest) {
  LOG(INFO) << "Making room for signature in file";
  manifest->set_signatures_offset(signature_blob_offset);
  LOG(INFO) << "set? " << manifest->has_signatures_offset();
  manifest->set_signatures_offset(signature_blob_offset);
  manifest->set_signatures_size(signature_blob_length);
  // Add a dummy op at the end to appease older clients
  if (add_dummy_op) {
    InstallOperation* dummy_op = manifest->add_kernel_install_operations();
    dummy_op->set_type(InstallOperation::REPLACE);
    dummy_op->set_data_offset(signature_blob_offset);
    dummy_op->set_data_length(signature_blob_length);
    Extent* dummy_extent = dummy_op->add_dst_extents();
    // Tell the dummy op to write this data to a big sparse hole
    dummy_extent->set_start_block(kSparseHole);
    dummy_extent->set_num_blocks((signature_blob_length + kBlockSize - 1) /
                                 kBlockSize);
  }
}

bool PayloadSigner::LoadPayloadMetadata(const string& payload_path,
                                        brillo::Blob* out_payload_metadata,
                                        DeltaArchiveManifest* out_manifest,
                                        uint64_t* out_major_version,
                                        uint64_t* out_metadata_size,
                                        uint32_t* out_metadata_signature_size) {
  brillo::StreamPtr payload_file =
      brillo::FileStream::Open(base::FilePath(payload_path),
                               brillo::Stream::AccessMode::READ,
                               brillo::FileStream::Disposition::OPEN_EXISTING,
                               nullptr);
  TEST_AND_RETURN_FALSE(payload_file);
  brillo::Blob payload_metadata;

  payload_metadata.resize(DeltaPerformer::kMaxPayloadHeaderSize);
  TEST_AND_RETURN_FALSE(payload_file->ReadAllBlocking(
      payload_metadata.data(), payload_metadata.size(), nullptr));

  const uint8_t* read_pointer = payload_metadata.data();
  TEST_AND_RETURN_FALSE(
      memcmp(read_pointer, kDeltaMagic, sizeof(kDeltaMagic)) == 0);
  read_pointer += sizeof(kDeltaMagic);

  uint64_t major_version;
  memcpy(&major_version, read_pointer, sizeof(major_version));
  read_pointer += sizeof(major_version);
  major_version = be64toh(major_version);
  TEST_AND_RETURN_FALSE(major_version == kChromeOSMajorPayloadVersion ||
                        major_version == kBrilloMajorPayloadVersion);
  if (out_major_version)
    *out_major_version = major_version;

  uint64_t manifest_size = 0;
  memcpy(&manifest_size, read_pointer, sizeof(manifest_size));
  read_pointer += sizeof(manifest_size);
  manifest_size = be64toh(manifest_size);

  uint32_t metadata_signature_size = 0;
  if (major_version == kBrilloMajorPayloadVersion) {
    memcpy(&metadata_signature_size, read_pointer,
           sizeof(metadata_signature_size));
    read_pointer += sizeof(metadata_signature_size);
    metadata_signature_size = be32toh(metadata_signature_size);
  }
  if (out_metadata_signature_size)
    *out_metadata_signature_size = metadata_signature_size;

  uint64_t header_size = read_pointer - payload_metadata.data();
  uint64_t metadata_size = header_size + manifest_size;
  if (out_metadata_size)
    *out_metadata_size = metadata_size;

  size_t bytes_read = payload_metadata.size();
  payload_metadata.resize(metadata_size);
  TEST_AND_RETURN_FALSE(
      payload_file->ReadAllBlocking(payload_metadata.data() + bytes_read,
                                    payload_metadata.size() - bytes_read,
                                    nullptr));
  if (out_manifest) {
    TEST_AND_RETURN_FALSE(out_manifest->ParseFromArray(
        payload_metadata.data() + header_size, manifest_size));
  }
  if (out_payload_metadata)
    *out_payload_metadata = std::move(payload_metadata);
  return true;
}

bool PayloadSigner::VerifySignedPayload(const string& payload_path,
                                        const string& public_key_path) {
  DeltaArchiveManifest manifest;
  uint64_t metadata_size;
  uint32_t metadata_signature_size;
  TEST_AND_RETURN_FALSE(LoadPayloadMetadata(payload_path,
                                            nullptr,
                                            &manifest,
                                            nullptr,
                                            &metadata_size,
                                            &metadata_signature_size));
  brillo::Blob payload;
  TEST_AND_RETURN_FALSE(utils::ReadFile(payload_path, &payload));
  TEST_AND_RETURN_FALSE(manifest.has_signatures_offset() &&
                        manifest.has_signatures_size());
  uint64_t signatures_offset = metadata_size + metadata_signature_size +
                               manifest.signatures_offset();
  CHECK_EQ(payload.size(), signatures_offset + manifest.signatures_size());
  brillo::Blob payload_hash, metadata_hash;
  TEST_AND_RETURN_FALSE(CalculateHashFromPayload(payload,
                                                 metadata_size,
                                                 metadata_signature_size,
                                                 signatures_offset,
                                                 &payload_hash,
                                                 &metadata_hash));
  brillo::Blob signature_blob(payload.begin() + signatures_offset,
                              payload.end());
  TEST_AND_RETURN_FALSE(PayloadVerifier::PadRSA2048SHA256Hash(&payload_hash));
  TEST_AND_RETURN_FALSE(PayloadVerifier::VerifySignature(
      signature_blob, public_key_path, payload_hash));
  if (metadata_signature_size) {
    signature_blob.assign(payload.begin() + metadata_size,
                          payload.begin() + metadata_size +
                          metadata_signature_size);
    TEST_AND_RETURN_FALSE(
        PayloadVerifier::PadRSA2048SHA256Hash(&metadata_hash));
    TEST_AND_RETURN_FALSE(PayloadVerifier::VerifySignature(
        signature_blob, public_key_path, metadata_hash));
  }
  return true;
}

bool PayloadSigner::SignHash(const brillo::Blob& hash,
                             const string& private_key_path,
                             brillo::Blob* out_signature) {
  LOG(INFO) << "Signing hash with private key: " << private_key_path;
  // We expect unpadded SHA256 hash coming in
  TEST_AND_RETURN_FALSE(hash.size() == 32);
  brillo::Blob padded_hash(hash);
  PayloadVerifier::PadRSA2048SHA256Hash(&padded_hash);

  // The code below executes the equivalent of:
  //
  // openssl rsautl -raw -sign -inkey |private_key_path|
  //   -in |padded_hash| -out |out_signature|

  FILE* fprikey = fopen(private_key_path.c_str(), "rb");
  TEST_AND_RETURN_FALSE(fprikey != nullptr);
  RSA* rsa = PEM_read_RSAPrivateKey(fprikey, nullptr, nullptr, nullptr);
  fclose(fprikey);
  TEST_AND_RETURN_FALSE(rsa != nullptr);
  brillo::Blob signature(RSA_size(rsa));
  ssize_t signature_size = RSA_private_encrypt(padded_hash.size(),
                                               padded_hash.data(),
                                               signature.data(),
                                               rsa,
                                               RSA_NO_PADDING);
  RSA_free(rsa);
  if (signature_size < 0) {
    LOG(ERROR) << "Signing hash failed: "
               << ERR_error_string(ERR_get_error(), nullptr);
    return false;
  }
  TEST_AND_RETURN_FALSE(static_cast<size_t>(signature_size) ==
                        signature.size());
  out_signature->swap(signature);
  return true;
}

bool PayloadSigner::SignHashWithKeys(const brillo::Blob& hash_data,
                                     const vector<string>& private_key_paths,
                                     brillo::Blob* out_signature_blob) {
  vector<brillo::Blob> signatures;
  for (const string& path : private_key_paths) {
    brillo::Blob signature;
    TEST_AND_RETURN_FALSE(SignHash(hash_data, path, &signature));
    signatures.push_back(signature);
  }
  TEST_AND_RETURN_FALSE(ConvertSignatureToProtobufBlob(signatures,
                                                       out_signature_blob));
  return true;
}

bool PayloadSigner::SignPayload(const string& unsigned_payload_path,
                                const vector<string>& private_key_paths,
                                const uint64_t metadata_size,
                                const uint32_t metadata_signature_size,
                                const uint64_t signatures_offset,
                                brillo::Blob* out_signature_blob) {
  brillo::Blob payload;
  TEST_AND_RETURN_FALSE(utils::ReadFile(unsigned_payload_path, &payload));
  brillo::Blob hash_data;
  TEST_AND_RETURN_FALSE(CalculateHashFromPayload(payload,
                                                 metadata_size,
                                                 metadata_signature_size,
                                                 signatures_offset,
                                                 &hash_data,
                                                 nullptr));
  TEST_AND_RETURN_FALSE(SignHashWithKeys(hash_data,
                                         private_key_paths,
                                         out_signature_blob));
  return true;
}

bool PayloadSigner::SignatureBlobLength(const vector<string>& private_key_paths,
                                        uint64_t* out_length) {
  DCHECK(out_length);
  brillo::Blob x_blob(1, 'x'), hash_blob, sig_blob;
  TEST_AND_RETURN_FALSE(HashCalculator::RawHashOfBytes(x_blob.data(),
                                                       x_blob.size(),
                                                       &hash_blob));
  TEST_AND_RETURN_FALSE(
      SignHashWithKeys(hash_blob, private_key_paths, &sig_blob));
  *out_length = sig_blob.size();
  return true;
}

bool PayloadSigner::HashPayloadForSigning(const string& payload_path,
                                          const vector<int>& signature_sizes,
                                          brillo::Blob* out_payload_hash_data,
                                          brillo::Blob* out_metadata_hash) {
  // Create a signature blob with signatures filled with 0.
  // Will be used for both payload signature and metadata signature.
  vector<brillo::Blob> signatures;
  for (int signature_size : signature_sizes) {
    signatures.emplace_back(signature_size, 0);
  }
  brillo::Blob signature_blob;
  TEST_AND_RETURN_FALSE(ConvertSignatureToProtobufBlob(signatures,
                                                       &signature_blob));

  brillo::Blob payload;
  uint64_t metadata_size, signatures_offset;
  uint32_t metadata_signature_size;
  // Prepare payload for hashing.
  TEST_AND_RETURN_FALSE(AddSignatureBlobToPayload(payload_path,
                                                  signature_blob,
                                                  signature_blob,
                                                  &payload,
                                                  &metadata_size,
                                                  &metadata_signature_size,
                                                  &signatures_offset));
  TEST_AND_RETURN_FALSE(CalculateHashFromPayload(payload,
                                                 metadata_size,
                                                 metadata_signature_size,
                                                 signatures_offset,
                                                 out_payload_hash_data,
                                                 out_metadata_hash));
  return true;
}

bool PayloadSigner::AddSignatureToPayload(
    const string& payload_path,
    const vector<brillo::Blob>& payload_signatures,
    const vector<brillo::Blob>& metadata_signatures,
    const string& signed_payload_path,
    uint64_t *out_metadata_size) {
  // TODO(petkov): Reduce memory usage -- the payload is manipulated in memory.

  // Loads the payload and adds the signature op to it.
  brillo::Blob signature_blob, metadata_signature_blob;
  TEST_AND_RETURN_FALSE(ConvertSignatureToProtobufBlob(payload_signatures,
                                                       &signature_blob));
  if (!metadata_signatures.empty()) {
    TEST_AND_RETURN_FALSE(
        ConvertSignatureToProtobufBlob(metadata_signatures,
                                       &metadata_signature_blob));
  }
  brillo::Blob payload;
  uint64_t signatures_offset;
  uint32_t metadata_signature_size;
  TEST_AND_RETURN_FALSE(AddSignatureBlobToPayload(payload_path,
                                                  signature_blob,
                                                  metadata_signature_blob,
                                                  &payload,
                                                  out_metadata_size,
                                                  &metadata_signature_size,
                                                  &signatures_offset));

  LOG(INFO) << "Signed payload size: " << payload.size();
  TEST_AND_RETURN_FALSE(utils::WriteFile(signed_payload_path.c_str(),
                                         payload.data(),
                                         payload.size()));
  return true;
}

bool PayloadSigner::GetMetadataSignature(const void* const metadata,
                                         size_t metadata_size,
                                         const string& private_key_path,
                                         string* out_signature) {
  // Calculates the hash on the updated payload. Note that the payload includes
  // the signature op but doesn't include the signature blob at the end.
  brillo::Blob metadata_hash;
  TEST_AND_RETURN_FALSE(HashCalculator::RawHashOfBytes(metadata,
                                                       metadata_size,
                                                       &metadata_hash));

  brillo::Blob signature;
  TEST_AND_RETURN_FALSE(SignHash(metadata_hash,
                                 private_key_path,
                                 &signature));

  *out_signature = brillo::data_encoding::Base64Encode(signature);
  return true;
}

bool PayloadSigner::ExtractPayloadProperties(
    const string& payload_path, brillo::KeyValueStore* properties) {
  DeltaArchiveManifest manifest;
  brillo::Blob payload_metadata;
  uint64_t major_version, metadata_size;
  uint32_t metadata_signature_size;
  uint64_t file_size = utils::FileSize(payload_path);

  TEST_AND_RETURN_FALSE(
      PayloadSigner::LoadPayloadMetadata(payload_path,
                                         &payload_metadata,
                                         &manifest,
                                         &major_version,
                                         &metadata_size,
                                         &metadata_signature_size));

  properties->SetString(kPayloadPropertyFileSize, std::to_string(file_size));
  properties->SetString(kPayloadPropertyMetadataSize,
                        std::to_string(metadata_size));

  brillo::Blob file_hash, metadata_hash;
  TEST_AND_RETURN_FALSE(
      HashCalculator::RawHashOfFile(payload_path, file_size, &file_hash) ==
      static_cast<off_t>(file_size));
  TEST_AND_RETURN_FALSE(HashCalculator::RawHashOfBytes(
      payload_metadata.data(), payload_metadata.size(), &metadata_hash));

  properties->SetString(kPayloadPropertyFileHash,
                        brillo::data_encoding::Base64Encode(file_hash));
  properties->SetString(kPayloadPropertyMetadataHash,
                        brillo::data_encoding::Base64Encode(metadata_hash));
  return true;
}

}  // namespace chromeos_update_engine

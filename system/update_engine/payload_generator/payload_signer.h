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

#ifndef UPDATE_ENGINE_PAYLOAD_GENERATOR_PAYLOAD_SIGNER_H_
#define UPDATE_ENGINE_PAYLOAD_GENERATOR_PAYLOAD_SIGNER_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <brillo/key_value_store.h>
#include <brillo/secure_blob.h>

#include "update_engine/update_metadata.pb.h"

// This class encapsulates methods used for payload signing.
// See update_metadata.proto for more info.

namespace chromeos_update_engine {

class PayloadSigner {
 public:
  // Reads the payload metadata from the given |payload_path| into the
  // |out_payload_metadata| vector if not null. It also parses the manifest
  // protobuf in the payload and returns it in |out_manifest| if not null, along
  // with the major version of the payload in |out_major_version| if not null,
  // the size of the entire metadata in |out_metadata_size| and the size of
  // metadata signature in |out_metadata_signature_size| if not null. Returns
  // whether a valid payload metadata was found and parsed.
  static bool LoadPayloadMetadata(const std::string& payload_path,
                                  brillo::Blob* out_payload_metadata,
                                  DeltaArchiveManifest* out_manifest,
                                  uint64_t* out_major_version,
                                  uint64_t* out_metadata_size,
                                  uint32_t* out_metadata_signature_size);

  // Returns true if the payload in |payload_path| is signed and its hash can be
  // verified using the public key in |public_key_path| with the signature
  // of a given version in the signature blob. Returns false otherwise.
  static bool VerifySignedPayload(const std::string& payload_path,
                                  const std::string& public_key_path);

  // Adds specified signature offset/length to given |manifest|, also adds a
  // dummy operation that points to a signature blob located at the specified
  // offset/length if |add_dummy_op| is true.
  static void AddSignatureToManifest(uint64_t signature_blob_offset,
                                     uint64_t signature_blob_length,
                                     bool add_dummy_op,
                                     DeltaArchiveManifest* manifest);

  // Given a raw |hash| and a private key in |private_key_path| calculates the
  // raw signature in |out_signature|. Returns true on success, false otherwise.
  static bool SignHash(const brillo::Blob& hash,
                       const std::string& private_key_path,
                       brillo::Blob* out_signature);

  // Sign |hash_data| blob with all private keys in |private_key_paths|, then
  // convert the signatures to protobuf blob.
  static bool SignHashWithKeys(
      const brillo::Blob& hash_data,
      const std::vector<std::string>& private_key_paths,
      brillo::Blob* out_signature_blob);

  // Given an unsigned payload in |unsigned_payload_path|, private keys in
  // |private_key_path|, metadata size in |metadata_size|, metadata signature
  // size in |metadata_signature_size| and signatures offset in
  // |signatures_offset|, calculates the payload signature blob into
  // |out_signature_blob|. Note that the payload must already have an
  // updated manifest that includes the dummy signature op and correct metadata
  // signature size in header. Returns true on success, false otherwise.
  static bool SignPayload(const std::string& unsigned_payload_path,
                          const std::vector<std::string>& private_key_paths,
                          const uint64_t metadata_size,
                          const uint32_t metadata_signature_size,
                          const uint64_t signatures_offset,
                          brillo::Blob* out_signature_blob);

  // Returns the length of out_signature_blob that will result in a call
  // to SignPayload with the given private keys. Returns true on success.
  static bool SignatureBlobLength(
      const std::vector<std::string>& private_key_paths,
      uint64_t* out_length);

  // Given an unsigned payload in |payload_path|,
  // this method does two things:
  // 1. It loads the payload into memory, and inserts placeholder signature
  //    operations and placeholder metadata signature to make the header and
  //    the manifest match what the final signed payload will look like based
  //    on |signatures_sizes|, if needed.
  // 2. It calculates the raw SHA256 hash of the payload and the metadata in
  //    |payload_path| (except signatures) and returns the result in
  //    |out_hash_data| and |out_metadata_hash| respectively.
  //
  // The changes to payload are not preserved or written to disk.
  static bool HashPayloadForSigning(const std::string& payload_path,
                                    const std::vector<int>& signature_sizes,
                                    brillo::Blob* out_payload_hash_data,
                                    brillo::Blob* out_metadata_hash);

  // Given an unsigned payload in |payload_path| (with no dummy signature op)
  // and the raw |payload_signatures| and |metadata_signatures| updates the
  // payload to include the signature thus turning it into a signed payload. The
  // new payload is stored in |signed_payload_path|. |payload_path| and
  // |signed_payload_path| can point to the same file. Populates
  // |out_metadata_size| with the size of the metadata after adding the
  // signature operation in the manifest. Returns true on success, false
  // otherwise.
  static bool AddSignatureToPayload(
      const std::string& payload_path,
      const std::vector<brillo::Blob>& payload_signatures,
      const std::vector<brillo::Blob>& metadata_signatures,
      const std::string& signed_payload_path,
      uint64_t* out_metadata_size);

  // Computes the SHA256 hash of the first metadata_size bytes of |metadata|
  // and signs the hash with the given private_key_path and writes the signed
  // hash in |out_signature|. Returns true if successful or false if there was
  // any error in the computations.
  static bool GetMetadataSignature(const void* const metadata,
                                   size_t metadata_size,
                                   const std::string& private_key_path,
                                   std::string* out_signature);

  static bool ExtractPayloadProperties(const std::string& payload_path,
                                       brillo::KeyValueStore* properties);

 private:
  // This should never be constructed
  DISALLOW_IMPLICIT_CONSTRUCTORS(PayloadSigner);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_GENERATOR_PAYLOAD_SIGNER_H_

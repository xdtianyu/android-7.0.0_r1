//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_PAYLOAD_VERIFIER_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_PAYLOAD_VERIFIER_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <brillo/secure_blob.h>

#include "update_engine/update_metadata.pb.h"

// This class encapsulates methods used for payload signature verification.
// See payload_generator/payload_signer.h for payload signing.

namespace chromeos_update_engine {

class PayloadVerifier {
 public:
  // Interprets |signature_blob| as a protocol buffer containing the Signatures
  // message and decrypts each signature data using the |public_key_path|.
  // Returns whether *any* of the decrypted hashes matches the |hash_data|.
  // In case of any error parsing the signatures or the public key, returns
  // false.
  static bool VerifySignature(const brillo::Blob& signature_blob,
                              const std::string& public_key_path,
                              const brillo::Blob& hash_data);

  // Decrypts sig_data with the given public_key_path and populates
  // out_hash_data with the decoded raw hash. Returns true if successful,
  // false otherwise.
  static bool GetRawHashFromSignature(const brillo::Blob& sig_data,
                                      const std::string& public_key_path,
                                      brillo::Blob* out_hash_data);

  // Pads a SHA256 hash so that it may be encrypted/signed with RSA2048
  // using the PKCS#1 v1.5 scheme.
  // hash should be a pointer to vector of exactly 256 bits. The vector
  // will be modified in place and will result in having a length of
  // 2048 bits. Returns true on success, false otherwise.
  static bool PadRSA2048SHA256Hash(brillo::Blob* hash);

 private:
  // This should never be constructed
  DISALLOW_IMPLICIT_CONSTRUCTORS(PayloadVerifier);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_PAYLOAD_VERIFIER_H_

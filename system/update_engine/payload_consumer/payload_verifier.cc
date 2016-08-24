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

#include "update_engine/payload_consumer/payload_verifier.h"

#include <base/logging.h>
#include <openssl/pem.h>

#include "update_engine/common/hash_calculator.h"
#include "update_engine/common/utils.h"
#include "update_engine/update_metadata.pb.h"

using std::string;

namespace chromeos_update_engine {

namespace {

// The following is a standard PKCS1-v1_5 padding for SHA256 signatures, as
// defined in RFC3447. It is prepended to the actual signature (32 bytes) to
// form a sequence of 256 bytes (2048 bits) that is amenable to RSA signing. The
// padded hash will look as follows:
//
//    0x00 0x01 0xff ... 0xff 0x00  ASN1HEADER  SHA256HASH
//   |--------------205-----------||----19----||----32----|
//
// where ASN1HEADER is the ASN.1 description of the signed data. The complete 51
// bytes of actual data (i.e. the ASN.1 header complete with the hash) are
// packed as follows:
//
//  SEQUENCE(2+49) {
//   SEQUENCE(2+13) {
//    OBJECT(2+9) id-sha256
//    NULL(2+0)
//   }
//   OCTET STRING(2+32) <actual signature bytes...>
//  }
const uint8_t kRSA2048SHA256Padding[] = {
  // PKCS1-v1_5 padding
  0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x00,
  // ASN.1 header
  0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
  0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
  0x00, 0x04, 0x20,
};

}  // namespace

bool PayloadVerifier::VerifySignature(const brillo::Blob& signature_blob,
                                      const string& public_key_path,
                                      const brillo::Blob& hash_data) {
  TEST_AND_RETURN_FALSE(!public_key_path.empty());

  Signatures signatures;
  LOG(INFO) << "signature blob size = " <<  signature_blob.size();
  TEST_AND_RETURN_FALSE(signatures.ParseFromArray(signature_blob.data(),
                                                  signature_blob.size()));

  if (!signatures.signatures_size()) {
    LOG(ERROR) << "No signatures stored in the blob.";
    return false;
  }

  std::vector<brillo::Blob> tested_hashes;
  // Tries every signature in the signature blob.
  for (int i = 0; i < signatures.signatures_size(); i++) {
    const Signatures_Signature& signature = signatures.signatures(i);
    brillo::Blob sig_data(signature.data().begin(), signature.data().end());
    brillo::Blob sig_hash_data;
    if (!GetRawHashFromSignature(sig_data, public_key_path, &sig_hash_data))
      continue;

    if (hash_data == sig_hash_data) {
      LOG(INFO) << "Verified correct signature " << i + 1 << " out of "
                << signatures.signatures_size() << " signatures.";
      return true;
    }
    tested_hashes.push_back(sig_hash_data);
  }
  LOG(ERROR) << "None of the " << signatures.signatures_size()
             << " signatures is correct. Expected:";
  utils::HexDumpVector(hash_data);
  LOG(ERROR) << "But found decrypted hashes:";
  for (const auto& sig_hash_data : tested_hashes) {
    utils::HexDumpVector(sig_hash_data);
  }
  return false;
}


bool PayloadVerifier::GetRawHashFromSignature(
    const brillo::Blob& sig_data,
    const string& public_key_path,
    brillo::Blob* out_hash_data) {
  TEST_AND_RETURN_FALSE(!public_key_path.empty());

  // The code below executes the equivalent of:
  //
  // openssl rsautl -verify -pubin -inkey |public_key_path|
  //   -in |sig_data| -out |out_hash_data|

  // Loads the public key.
  FILE* fpubkey = fopen(public_key_path.c_str(), "rb");
  if (!fpubkey) {
    LOG(ERROR) << "Unable to open public key file: " << public_key_path;
    return false;
  }

  char dummy_password[] = { ' ', 0 };  // Ensure no password is read from stdin.
  RSA* rsa = PEM_read_RSA_PUBKEY(fpubkey, nullptr, nullptr, dummy_password);
  fclose(fpubkey);
  TEST_AND_RETURN_FALSE(rsa != nullptr);
  unsigned int keysize = RSA_size(rsa);
  if (sig_data.size() > 2 * keysize) {
    LOG(ERROR) << "Signature size is too big for public key size.";
    RSA_free(rsa);
    return false;
  }

  // Decrypts the signature.
  brillo::Blob hash_data(keysize);
  int decrypt_size = RSA_public_decrypt(sig_data.size(),
                                        sig_data.data(),
                                        hash_data.data(),
                                        rsa,
                                        RSA_NO_PADDING);
  RSA_free(rsa);
  TEST_AND_RETURN_FALSE(decrypt_size > 0 &&
                        decrypt_size <= static_cast<int>(hash_data.size()));
  hash_data.resize(decrypt_size);
  out_hash_data->swap(hash_data);
  return true;
}

bool PayloadVerifier::PadRSA2048SHA256Hash(brillo::Blob* hash) {
  TEST_AND_RETURN_FALSE(hash->size() == 32);
  hash->insert(hash->begin(),
               reinterpret_cast<const char*>(kRSA2048SHA256Padding),
               reinterpret_cast<const char*>(kRSA2048SHA256Padding +
                                             sizeof(kRSA2048SHA256Padding)));
  TEST_AND_RETURN_FALSE(hash->size() == 256);
  return true;
}

}  // namespace chromeos_update_engine

//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef ATTESTATION_COMMON_CRYPTO_UTILITY_IMPL_H_
#define ATTESTATION_COMMON_CRYPTO_UTILITY_IMPL_H_

#include "attestation/common/crypto_utility.h"

#include <string>

#include <openssl/rsa.h>

#include "attestation/common/tpm_utility.h"

namespace attestation {

// An implementation of CryptoUtility.
class CryptoUtilityImpl : public CryptoUtility {
 public:
  // Does not take ownership of pointers.
  explicit CryptoUtilityImpl(TpmUtility* tpm_utility);
  ~CryptoUtilityImpl() override;

  // CryptoUtility methods.
  bool GetRandom(size_t num_bytes, std::string* random_data) const override;
  bool CreateSealedKey(std::string* aes_key, std::string* sealed_key) override;
  bool EncryptData(const std::string& data,
                   const std::string& aes_key,
                   const std::string& sealed_key,
                   std::string* encrypted_data) override;
  bool UnsealKey(const std::string& encrypted_data,
                 std::string* aes_key,
                 std::string* sealed_key) override;
  bool DecryptData(const std::string& encrypted_data,
                   const std::string& aes_key,
                   std::string* data) override;
  bool GetRSASubjectPublicKeyInfo(const std::string& public_key,
                                  std::string* spki) override;
  bool GetRSAPublicKey(const std::string& public_key_info,
                       std::string* public_key) override;
  bool EncryptIdentityCredential(
      const std::string& credential,
      const std::string& ek_public_key_info,
      const std::string& aik_public_key,
      EncryptedIdentityCredential* encrypted) override;
  bool EncryptForUnbind(const std::string& public_key,
                        const std::string& data,
                        std::string* encrypted_data) override;
  bool VerifySignature(const std::string& public_key,
                       const std::string& data,
                       const std::string& signature) override;

 private:
  // Encrypts |data| using |key| and |iv| for AES in CBC mode with PKCS #5
  // padding and produces the |encrypted_data|. Returns true on success.
  bool AesEncrypt(const std::string& data,
                  const std::string& key,
                  const std::string& iv,
                  std::string* encrypted_data);

  // Decrypts |encrypted_data| using |key| and |iv| for AES in CBC mode with
  // PKCS #5 padding and produces the decrypted |data|. Returns true on success.
  bool AesDecrypt(const std::string& encrypted_data,
                  const std::string& key,
                  const std::string& iv,
                  std::string* data);

  // Computes and returns an HMAC of |data| using |key| and SHA-512.
  std::string HmacSha512(const std::string& data, const std::string& key);

  // Encrypt like trousers does. This is like AesEncrypt but a random IV is
  // included in the output.
  bool TssCompatibleEncrypt(const std::string& input,
                            const std::string& key,
                            std::string* output);

  // Encrypts using RSA-OAEP and the TPM-specific OAEP parameter.
  bool TpmCompatibleOAEPEncrypt(const std::string& input,
                                RSA* key,
                                std::string* output);

  TpmUtility* tpm_utility_;
};

}  // namespace attestation

#endif  // ATTESTATION_COMMON_CRYPTO_UTILITY_IMPL_H_

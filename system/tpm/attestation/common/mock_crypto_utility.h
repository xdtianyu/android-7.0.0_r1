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

#ifndef ATTESTATION_COMMON_MOCK_CRYPTO_UTILITY_H_
#define ATTESTATION_COMMON_MOCK_CRYPTO_UTILITY_H_

#include "attestation/common/crypto_utility.h"

#include <string>

#include <gmock/gmock.h>

namespace attestation {

class MockCryptoUtility : public CryptoUtility {
 public:
  MockCryptoUtility();
  ~MockCryptoUtility() override;

  MOCK_CONST_METHOD2(GetRandom, bool(size_t, std::string*));

  MOCK_METHOD2(CreateSealedKey, bool(std::string* aes_key,
                                     std::string* sealed_key));

  MOCK_METHOD4(EncryptData, bool(const std::string& data,
                                 const std::string& aes_key,
                                 const std::string& sealed_key,
                                 std::string* encrypted_data));

  MOCK_METHOD3(UnsealKey, bool(const std::string& encrypted_data,
                               std::string* aes_key,
                               std::string* sealed_key));

  MOCK_METHOD3(DecryptData, bool(const std::string& encrypted_data,
                                 const std::string& aes_key,
                                 std::string* data));
  MOCK_METHOD2(GetRSASubjectPublicKeyInfo, bool(const std::string&,
                                                std::string*));
  MOCK_METHOD2(GetRSAPublicKey, bool(const std::string&, std::string*));
  MOCK_METHOD4(EncryptIdentityCredential, bool(const std::string&,
                                               const std::string&,
                                               const std::string&,
                                               EncryptedIdentityCredential*));
  MOCK_METHOD3(EncryptForUnbind, bool(const std::string&,
                                      const std::string&,
                                      std::string*));
  MOCK_METHOD3(VerifySignature, bool(const std::string&,
                                     const std::string&,
                                     const std::string&));
};

}  // namespace attestation

#endif  // ATTESTATION_COMMON_MOCK_CRYPTO_UTILITY_H_

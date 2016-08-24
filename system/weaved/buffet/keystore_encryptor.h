// Copyright 2015 The Android Open Source Project
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

#ifndef BUFFET_KEYSTORE_ENCRYPTOR_H_
#define BUFFET_KEYSTORE_ENCRYPTOR_H_

#include "buffet/encryptor.h"

#include <memory>

#include <keystore/keystore_client.h>

namespace buffet {

// An Encryptor implementation backed by Brillo Keystore. This class is intended
// to be the default encryptor on platforms that support it. An implementation
// of Encryptor::CreateDefaultEncryptor is provided for this class.
class KeystoreEncryptor : public Encryptor {
 public:
  explicit KeystoreEncryptor(
      std::unique_ptr<keystore::KeystoreClient> keystore);
  ~KeystoreEncryptor() override = default;

  bool EncryptWithAuthentication(const std::string& plaintext,
                                 std::string* ciphertext) override;
  bool DecryptWithAuthentication(const std::string& ciphertext,
                                 std::string* plaintext) override;

 private:
  std::unique_ptr<keystore::KeystoreClient> keystore_;
};

}  // namespace buffet

#endif  // BUFFET_KEYSTORE_ENCRYPTOR_H_

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

#include "buffet/keystore_encryptor.h"

#include <memory>

#include <keystore/keystore_client_impl.h>

namespace {

const char kBuffetKeyName[] = "buffet_config_b4f594c3";

}  // namespace

namespace buffet {

std::unique_ptr<Encryptor> Encryptor::CreateDefaultEncryptor() {
  return std::unique_ptr<Encryptor>(
      new KeystoreEncryptor(std::unique_ptr<keystore::KeystoreClient>(
          new keystore::KeystoreClientImpl)));
}

KeystoreEncryptor::KeystoreEncryptor(
    std::unique_ptr<keystore::KeystoreClient> keystore)
    : keystore_(std::move(keystore)) {}

bool KeystoreEncryptor::EncryptWithAuthentication(const std::string& plaintext,
                                                  std::string* ciphertext) {
  return keystore_->encryptWithAuthentication(kBuffetKeyName, plaintext,
                                              ciphertext);
}

bool KeystoreEncryptor::DecryptWithAuthentication(const std::string& ciphertext,
                                                  std::string* plaintext) {
  return keystore_->decryptWithAuthentication(kBuffetKeyName, ciphertext,
                                              plaintext);
}

}  // namespace buffet

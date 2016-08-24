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

#include "buffet/encryptor.h"

#include <memory>

#include <brillo/data_encoding.h>

namespace buffet {

class FakeEncryptor : public Encryptor {
 public:
  bool EncryptWithAuthentication(const std::string& plaintext,
                                 std::string* ciphertext) override {
    *ciphertext = brillo::data_encoding::Base64Encode(plaintext);
    return true;
  }

  bool DecryptWithAuthentication(const std::string& ciphertext,
                                 std::string* plaintext) override {
    return brillo::data_encoding::Base64Decode(ciphertext, plaintext);
  }
};

std::unique_ptr<Encryptor> Encryptor::CreateDefaultEncryptor() {
  return std::unique_ptr<Encryptor>{new FakeEncryptor};
}

}  // namespace buffet

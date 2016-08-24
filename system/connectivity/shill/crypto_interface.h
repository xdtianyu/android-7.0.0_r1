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

#ifndef SHILL_CRYPTO_INTERFACE_H_
#define SHILL_CRYPTO_INTERFACE_H_

#include <string>

namespace shill {

// An interface to an encryption/decryption module.
class CryptoInterface {
 public:
  virtual ~CryptoInterface() {}

  // Returns a unique identifier for this crypto module.
  virtual std::string GetID() = 0;

  // Encrypts |plaintext| into |ciphertext|. Returns true on success.
  virtual bool Encrypt(const std::string& plaintext,
                       std::string* ciphertext) = 0;

  // Decrypts |ciphertext| into |plaintext|. Returns true on success.
  virtual bool Decrypt(const std::string& ciphertext,
                       std::string* plaintext) = 0;
};

}  // namespace shill

#endif  // SHILL_CRYPTO_INTERFACE_H_

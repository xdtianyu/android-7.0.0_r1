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

#ifndef SHILL_CRYPTO_DES_CBC_H_
#define SHILL_CRYPTO_DES_CBC_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/crypto_interface.h"

namespace base {

class FilePath;

}  // namespace base

namespace shill {

// DES-CBC crypto module implementation.
class CryptoDESCBC : public CryptoInterface {
 public:
  static const char kID[];

  CryptoDESCBC();

  // Sets the DES key to the last |kBlockSize| bytes of |key_matter_path| and
  // the DES initialization vector to the second to last |kBlockSize| bytes of
  // |key_matter_path|. Returns true on success.
  bool LoadKeyMatter(const base::FilePath& path);

  // Inherited from CryptoInterface.
  virtual std::string GetID();
  virtual bool Encrypt(const std::string& plaintext, std::string* ciphertext);
  virtual bool Decrypt(const std::string& ciphertext, std::string* plaintext);

  const std::vector<char>& key() const { return key_; }
  const std::vector<char>& iv() const { return iv_; }

 private:
  FRIEND_TEST(CryptoDESCBCTest, Decrypt);
  FRIEND_TEST(CryptoDESCBCTest, Encrypt);

  static const unsigned int kBlockSize;
  static const char kSentinel[];
  static const char kVersion2Prefix[];

  std::vector<char> key_;
  std::vector<char> iv_;

  DISALLOW_COPY_AND_ASSIGN(CryptoDESCBC);
};

}  // namespace shill

#endif  // SHILL_CRYPTO_DES_CBC_H_

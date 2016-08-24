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

#ifndef SHILL_CRYPTO_PROVIDER_H_
#define SHILL_CRYPTO_PROVIDER_H_

#include <string>

#include <base/files/file_path.h>
#include <base/memory/scoped_vector.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/crypto_interface.h"

namespace shill {

class CryptoProvider {
 public:
  CryptoProvider();

  void Init();

  // Returns |plaintext| encrypted by the highest priority available crypto
  // module capable of performing the operation. If no module succeeds, returns
  // |plaintext| as is.
  std::string Encrypt(const std::string& plaintext);

  // Returns |ciphertext| decrypted by the highest priority available crypto
  // module capable of performing the operation. If no module succeeds, returns
  // |ciphertext| as is.
  std::string Decrypt(const std::string& ciphertext);

  void set_key_matter_file(const base::FilePath& path) {
    key_matter_file_ = path;
  }

 private:
  FRIEND_TEST(CryptoProviderTest, Init);
  FRIEND_TEST(KeyFileStoreTest, OpenClose);
  typedef ScopedVector<CryptoInterface> Cryptos;

  static const char kKeyMatterFile[];

  // Registered crypto modules in high to low priority order.
  Cryptos cryptos_;

  base::FilePath key_matter_file_;

  DISALLOW_COPY_AND_ASSIGN(CryptoProvider);
};

}  // namespace shill

#endif  // SHILL_CRYPTO_PROVIDER_H_

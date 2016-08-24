//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/crypto_provider.h"

#include <memory>

#include <base/strings/string_util.h>

#include "shill/crypto_des_cbc.h"
#include "shill/crypto_rot47.h"
#include "shill/logging.h"

using std::string;

namespace shill {

const char CryptoProvider::kKeyMatterFile[] = "/var/lib/whitelist/owner.key";

CryptoProvider::CryptoProvider()
    : key_matter_file_(kKeyMatterFile) {}

void CryptoProvider::Init() {
  cryptos_.clear();

  // Register the crypto modules in priority order -- highest priority first.
  std::unique_ptr<CryptoDESCBC> des_cbc(new CryptoDESCBC());
  if (des_cbc->LoadKeyMatter(key_matter_file_)) {
    cryptos_.push_back(des_cbc.release());
  }
  cryptos_.push_back(new CryptoROT47());
}

string CryptoProvider::Encrypt(const string& plaintext) {
  for (auto crypto : cryptos_) {
    string ciphertext;
    if (crypto->Encrypt(plaintext, &ciphertext)) {
      const string prefix = crypto->GetID() + ":";
      return prefix + ciphertext;
    }
  }
  LOG(WARNING) << "Unable to encrypt text, returning as is.";
  return plaintext;
}

string CryptoProvider::Decrypt(const string& ciphertext) {
  for (auto crypto : cryptos_) {
    const string prefix = crypto->GetID() + ":";
    if (base::StartsWith(ciphertext, prefix, base::CompareCase::SENSITIVE)) {
      string to_decrypt = ciphertext;
      to_decrypt.erase(0, prefix.size());
      string plaintext;
      if (!crypto->Decrypt(to_decrypt, &plaintext)) {
        LOG(WARNING) << "Crypto module " << crypto->GetID()
                     << " failed to decrypt.";
      }
      return plaintext;
    }
  }
  LOG(WARNING) << "Unable to decrypt text, returning as is.";
  return ciphertext;
}

}  // namespace shill

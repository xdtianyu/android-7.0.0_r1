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

#include "shill/crypto_des_cbc.h"

#include <rpc/des_crypt.h>

#include <base/files/file_util.h>
#include <base/strings/string_util.h>
#include <brillo/data_encoding.h>

using base::FilePath;
using std::string;
using std::vector;

namespace shill {

const unsigned int CryptoDESCBC::kBlockSize = 8;
const char CryptoDESCBC::kID[] = "des-cbc";
const char CryptoDESCBC::kSentinel[] = "[ok]";
const char CryptoDESCBC::kVersion2Prefix[] = "02:";

CryptoDESCBC::CryptoDESCBC() {}

string CryptoDESCBC::GetID() {
  return kID;
}

bool CryptoDESCBC::Encrypt(const string& plaintext, string* ciphertext) {
  // Never encrypt. We'll fall back to rot47 which doesn't depend on
  // the owner key which may change due to rotation.
  return false;
}

bool CryptoDESCBC::Decrypt(const string& ciphertext, string* plaintext) {
  CHECK_EQ(kBlockSize, key_.size());
  CHECK_EQ(kBlockSize, iv_.size());
  int version = 1;
  string b64_ciphertext = ciphertext;
  if (base::StartsWith(ciphertext, kVersion2Prefix,
                       base::CompareCase::SENSITIVE)) {
    version = 2;
    b64_ciphertext.erase(0, strlen(kVersion2Prefix));
  }

  string decoded_data;
  if (!brillo::data_encoding::Base64Decode(b64_ciphertext, &decoded_data)) {
    LOG(ERROR) << "Unable to base64-decode DEC-CBC ciphertext.";
    return false;
  }

  vector<char> data(decoded_data.c_str(),
                    decoded_data.c_str() + decoded_data.length());
  if (data.empty() || (data.size() % kBlockSize != 0)) {
    LOG(ERROR) << "Invalid DES-CBC ciphertext size: " << data.size();
    return false;
  }

  // The IV is modified in place.
  vector<char> iv = iv_;
  int rv =
      cbc_crypt(key_.data(), data.data(), data.size(), DES_DECRYPT, iv.data());
  if (DES_FAILED(rv)) {
    LOG(ERROR) << "DES-CBC decryption failed.";
    return false;
  }
  if (data.back() != '\0') {
    LOG(ERROR) << "DEC-CBC decryption resulted in invalid plain text.";
    return false;
  }
  string text = data.data();
  if (version == 2) {
    if (!base::EndsWith(text, kSentinel, base::CompareCase::SENSITIVE)) {
      LOG(ERROR) << "DES-CBC decrypted text missing sentinel -- bad key?";
      return false;
    }
    text.erase(text.size() - strlen(kSentinel), strlen(kSentinel));
  }
  *plaintext = text;
  return true;
}

bool CryptoDESCBC::LoadKeyMatter(const FilePath& path) {
  key_.clear();
  iv_.clear();
  string matter;
  // TODO(petkov): This mimics current flimflam behavior. Fix it so that it
  // doesn't read the whole file.
  if (!base::ReadFileToString(path, &matter)) {
    LOG(ERROR) << "Unable to load key matter from " << path.value();
    return false;
  }
  if (matter.size() < 2 * kBlockSize) {
    LOG(ERROR) << "Key matter data not enough " << matter.size() << " < "
               << 2 * kBlockSize;
    return false;
  }
  string::const_iterator matter_start =
      matter.begin() + (matter.size() - 2 * kBlockSize);
  key_.assign(matter_start + kBlockSize, matter_start + 2 * kBlockSize);
  iv_.assign(matter_start, matter_start + kBlockSize);
  return true;
}

}  // namespace shill

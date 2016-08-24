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

#include "shill/crypto_rot47.h"

using std::string;

namespace shill {

const char CryptoROT47::kID[] = "rot47";

CryptoROT47::CryptoROT47() {}

string CryptoROT47::GetID() {
  return kID;
}

bool CryptoROT47::Encrypt(const string& plaintext, string* ciphertext) {
  const int kRotSize = 94;
  const int kRotHalf = kRotSize / 2;
  const char kRotMin = '!';
  const char kRotMax = kRotMin + kRotSize - 1;

  *ciphertext = plaintext;
  for (auto& ch : *ciphertext) {
    if (ch < kRotMin || ch > kRotMax) {
      continue;
    }
    int rot = ch + kRotHalf;
    ch = (rot > kRotMax) ? rot - kRotSize : rot;
  }
  return true;
}

bool CryptoROT47::Decrypt(const string& ciphertext, string* plaintext) {
  // ROT47 is self-reciprocal.
  return Encrypt(ciphertext, plaintext);
}

}  // namespace shill

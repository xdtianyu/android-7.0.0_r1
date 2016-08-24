// Copyright 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/chromium/crypto/sha2.h"

#include <algorithm>
#include <openssl/sha.h>

#include <base/memory/scoped_ptr.h>

namespace crypto {

void SHA256HashString(const std::string& str, uint8_t* output, size_t len) {
  std::string hash = SHA256HashString(str);
  len = std::min(hash.size(), len);
  std::copy(hash.begin(), hash.begin() + len, output);
}

std::string SHA256HashString(const std::string& str) {
  SHA256_CTX sha_context;
  SHA256_Init(&sha_context);
  SHA256_Update(&sha_context, str.data(), str.size());

  std::string hash(kSHA256Length, 0);
  SHA256_Final(reinterpret_cast<uint8_t*>(&hash[0]), &sha_context);
  return hash;
}

}  // namespace crypto

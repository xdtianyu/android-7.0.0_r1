// Copyright 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_THIRD_PARTY_CHROMIUM_SHA2_H_
#define LIBWEAVE_THIRD_PARTY_CHROMIUM_SHA2_H_

#include <string>

namespace crypto {

// These functions perform SHA-256 operations.
//
// Functions for SHA-384 and SHA-512 can be added when the need arises.

static const size_t kSHA256Length = 32;  // Length in bytes of a SHA-256 hash.

// Computes the SHA-256 hash of the input string 'str' and stores the first
// 'len' bytes of the hash in the output buffer 'output'.  If 'len' > 32,
// only 32 bytes (the full hash) are stored in the 'output' buffer.
void SHA256HashString(const std::string& str, uint8_t* output, size_t len);

// Convenience version of the above that returns the result in a 32-byte
// string.
std::string SHA256HashString(const std::string& str);

}  // namespace crypto

#endif  // LIBWEAVE_THIRD_PARTY_CHROMIUM_SHA2_H_

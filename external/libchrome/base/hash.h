// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HASH_H_
#define BASE_HASH_H_

#include <cstdint>
#include <functional>
#include <string>

namespace base {

// Deprecated: just use std::hash directly
//
// Computes a hash of a string |str|.
// WARNING: This hash function should not be used for any cryptographic purpose.
inline uint32_t Hash(const std::string& str) {
  std::hash<std::string> hash_fn;
  return hash_fn(str);
}

}  // namespace base

#endif  // BASE_HASH_H_

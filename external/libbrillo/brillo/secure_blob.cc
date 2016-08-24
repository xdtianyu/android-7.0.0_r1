// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>  // memcpy

#include <base/stl_util.h>

#include "brillo/secure_blob.h"

namespace brillo {

SecureBlob::SecureBlob(const std::string& data)
    : SecureBlob(data.begin(), data.end()) {}

SecureBlob::~SecureBlob() {
  clear();
}

void SecureBlob::resize(size_type count) {
  if (count < size()) {
    SecureMemset(data() + count, 0, capacity() - count);
  }
  Blob::resize(count);
}

void SecureBlob::resize(size_type count, const value_type& value) {
  if (count < size()) {
    SecureMemset(data() + count, 0, capacity() - count);
  }
  Blob::resize(count, value);
}

void SecureBlob::clear() {
  SecureMemset(data(), 0, capacity());
  Blob::clear();
}

std::string SecureBlob::to_string() const {
  return std::string(data(), data() + size());
}

SecureBlob SecureBlob::Combine(const SecureBlob& blob1,
                               const SecureBlob& blob2) {
  SecureBlob result;
  result.reserve(blob1.size() + blob2.size());
  result.insert(result.end(), blob1.begin(), blob1.end());
  result.insert(result.end(), blob2.begin(), blob2.end());
  return result;
}

void* SecureMemset(void* v, int c, size_t n) {
  volatile uint8_t* p = reinterpret_cast<volatile uint8_t*>(v);
  while (n--)
    *p++ = c;
  return v;
}

int SecureMemcmp(const void* s1, const void* s2, size_t n) {
  const uint8_t* us1 = reinterpret_cast<const uint8_t*>(s1);
  const uint8_t* us2 = reinterpret_cast<const uint8_t*>(s2);
  int result = 0;

  if (0 == n)
    return 1;

  /* Code snippet without data-dependent branch due to
   * Nate Lawson (nate@root.org) of Root Labs. */
  while (n--)
    result |= *us1++ ^ *us2++;

  return result != 0;
}

}  // namespace brillo

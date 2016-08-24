// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_POINTER_UTILS_H_
#define LIBBRILLO_BRILLO_POINTER_UTILS_H_

#include <cstdint>
#include <sys/types.h>

namespace brillo {

// AdvancePointer() is a helper function to advance void pointer by
// |byte_offset| bytes. Both const and non-const overloads are provided.
inline void* AdvancePointer(void* pointer, ssize_t byte_offset) {
  return reinterpret_cast<uint8_t*>(pointer) + byte_offset;
}
inline const void* AdvancePointer(const void* pointer, ssize_t byte_offset) {
  return reinterpret_cast<const uint8_t*>(pointer) + byte_offset;
}

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_POINTER_UTILS_H_

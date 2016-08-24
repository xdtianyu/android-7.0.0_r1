// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/crypto_utils.h"

bool uw_crypto_utils_equal_(const uint8_t* arr1,
                            const uint8_t* arr2,
                            size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= arr1[i] ^ arr2[i];
  }

  return 0 == diff;
}

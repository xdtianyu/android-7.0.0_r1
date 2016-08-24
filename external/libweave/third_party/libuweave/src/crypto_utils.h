// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_UTILS_H_
#define LIBUWEAVE_SRC_CRYPTO_UTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Check if two byte arrays are the same in constant time (the running time
 * should only depend on the length of the given arrays). It's critical to use
 * constant-time methods to compare secret data. Timing information can lead to
 * full recovery of the secret data.
 */
bool uw_crypto_utils_equal_(const uint8_t* arr1,
                            const uint8_t* arr2,
                            size_t len);

#endif  // LIBUWEAVE_SRC_CRYPTO_UTILS_H_

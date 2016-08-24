// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_CRYPTO_HMAC_H_
#define LIBUWEAVE_SRC_CRYPTO_HMAC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t* bytes;
  size_t num_bytes;
} UwCryptoHmacMsg;

/**
 * Compute HMAC over a list of messages, which is equivalent to computing HMAC
 * over the concatenation of all the messages. The HMAC output will be truncated
 * to the desired length truncated_digest_len, and written into trucated_digest.
 */
bool uw_crypto_hmac_(const uint8_t* key,
                     size_t key_len,
                     const UwCryptoHmacMsg messages[],
                     size_t num_messages,
                     uint8_t* truncated_digest,
                     size_t truncated_digest_len);

#endif  // LIBUWEAVE_SRC_CRYPTO_HMAC_H_

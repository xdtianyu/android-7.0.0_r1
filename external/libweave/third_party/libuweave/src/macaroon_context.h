// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_CONTEXT_
#define LIBUWEAVE_SRC_MACAROON_CONTEXT_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "src/macaroon_caveat.h"

typedef struct {
  uint32_t current_time;  // In number of seconds since Jan 1st 2000 00:00:00
  const uint8_t* ble_session_id;  // Only for BLE
  size_t ble_session_id_len;
} UwMacaroonContext;

bool uw_macaroon_context_create_(uint32_t current_time,
                                 const uint8_t* ble_session_id,
                                 size_t ble_session_id_len,
                                 UwMacaroonContext* new_context);

#endif  // LIBUWEAVE_SRC_MACAROON_CONTEXT_

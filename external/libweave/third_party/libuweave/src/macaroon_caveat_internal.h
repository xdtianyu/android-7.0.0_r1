// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_CAVEAT_INTERNAL_H_
#define LIBUWEAVE_SRC_MACAROON_CAVEAT_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "src/macaroon.h"
#include "src/macaroon_caveat.h"

bool uw_macaroon_caveat_sign_(const uint8_t* key,
                              size_t key_len,
                              const UwMacaroonContext* context,
                              const UwMacaroonCaveat* caveat,
                              uint8_t* mac_tag,
                              size_t mac_tag_size);

typedef struct {
  uint32_t issued_time;  // 0 when invalid or not set.
} UwMacaroonValidationState;

bool uw_macaroon_caveat_init_validation_state_(
    UwMacaroonValidationState* state);

bool uw_macaroon_caveat_validate_(const UwMacaroonCaveat* caveat,
                                  const UwMacaroonContext* context,
                                  UwMacaroonValidationState* state,
                                  UwMacaroonValidationResult* result);

bool uw_macaroon_caveat_get_value_uint_(const UwMacaroonCaveat* caveat,
                                        uint32_t* unsigned_int);
bool uw_macaroon_caveat_get_value_bstr_(const UwMacaroonCaveat* caveat,
                                        const uint8_t** str,
                                        size_t* str_len);

#endif  // LIBUWEAVE_SRC_MACAROON_CAVEAT_INTERNAL_H_

// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBUWEAVE_SRC_MACAROON_CAVEAT_H_
#define LIBUWEAVE_SRC_MACAROON_CAVEAT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t num_bytes;
  const uint8_t* bytes;
} UwMacaroonCaveat;

typedef enum {
  kUwMacaroonCaveatTypeNonce = 0,                // bstr
  kUwMacaroonCaveatTypeScope = 1,                // uint
  kUwMacaroonCaveatTypeExpirationAbsolute = 5,   // uint
  kUwMacaroonCaveatTypeTTL1Hour = 6,             // no value
  kUwMacaroonCaveatTypeTTL24Hour = 7,            // no value
  kUwMacaroonCaveatTypeDelegationTimestamp = 8,  // uint

  kUwMacaroonCaveatTypeDelegateeUser = 9,      // bstr
  kUwMacaroonCaveatTypeDelegateeApp = 10,      // bstr
  kUwMacaroonCaveatTypeDelegateeService = 12,  // bstr

  kUwMacaroonCaveatTypeAppCommandsOnly = 11,                 // no value
  kUwMacaroonCaveatTypeBleSessionID = 16,                    // no value
  kUwMacaroonCaveatTypeLanSessionID = 17,                    // bstr
  kUwMacaroonCaveatTypeClientAuthorizationTokenV1 = 8193,    // bstr (0x2001)
  kUwMacaroonCaveatTypeServerAuthenticationTokenV1 = 12289,  // bstr (0x3001)
} UwMacaroonCaveatType;

typedef enum {
  kUwMacaroonCaveatScopeTypeOwner = 2,
  kUwMacaroonCaveatScopeTypeManager = 8,
  kUwMacaroonCaveatScopeTypeUser = 14,
  kUwMacaroonCaveatScopeTypeViewer = 20,
} UwMacaroonCaveatScopeType;

// For security sanity checks
#define UW_MACAROON_CAVEAT_SCOPE_LOWEST_POSSIBLE 127

/** Compute the buffer sizes that are enough for caveat creation functions. */
size_t uw_macaroon_caveat_creation_get_buffsize_(UwMacaroonCaveatType type,
                                                 size_t str_len);

// Caveat creation functions
bool uw_macaroon_caveat_create_nonce_(const uint8_t* nonce,
                                      size_t nonce_size,
                                      uint8_t* buffer,
                                      size_t buffer_size,
                                      UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_scope_(UwMacaroonCaveatScopeType scope,
                                      uint8_t* buffer,
                                      size_t buffer_size,
                                      UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_expiration_absolute_(
    uint32_t expiration_time,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_ttl_1_hour_(uint8_t* buffer,
                                           size_t buffer_size,
                                           UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_ttl_24_hour_(uint8_t* buffer,
                                            size_t buffer_size,
                                            UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_delegation_timestamp_(
    uint32_t timestamp,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_delegatee_user_(const uint8_t* id_str,
                                               size_t id_str_len,
                                               uint8_t* buffer,
                                               size_t buffer_size,
                                               UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_delegatee_app_(const uint8_t* id_str,
                                              size_t id_str_len,
                                              uint8_t* buffer,
                                              size_t buffer_size,
                                              UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_delegatee_service_(const uint8_t* id_str,
                                                  size_t id_str_len,
                                                  uint8_t* buffer,
                                                  size_t buffer_size,
                                                  UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_app_commands_only_(uint8_t* buffer,
                                                  size_t buffer_size,
                                                  UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_ble_session_id_(uint8_t* buffer,
                                               size_t buffer_size,
                                               UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_lan_session_id_(const uint8_t* session_id,
                                               size_t session_id_len,
                                               uint8_t* buffer,
                                               size_t buffer_size,
                                               UwMacaroonCaveat* new_caveat);

// The string values for these two token types are optional.
// Use str_len = 0 to indicate creating the caveats without string values.
bool uw_macaroon_caveat_create_client_authorization_token_(
    const uint8_t* str,
    size_t str_len,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat);
bool uw_macaroon_caveat_create_server_authentication_token_(
    const uint8_t* str,
    size_t str_len,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat);

/** Get the type for the given caveat. */
bool uw_macaroon_caveat_get_type_(const UwMacaroonCaveat* caveat,
                                  UwMacaroonCaveatType* type);

#endif  // LIBUWEAVE_SRC_MACAROON_CAVEAT_H_

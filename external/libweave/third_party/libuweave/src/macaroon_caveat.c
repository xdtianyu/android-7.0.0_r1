// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon_caveat.h"
#include "src/macaroon_caveat_internal.h"

#include <string.h>

#include "src/crypto_hmac.h"
#include "src/macaroon.h"
#include "src/macaroon_context.h"
#include "src/macaroon_encoding.h"

static bool is_valid_caveat_type_(UwMacaroonCaveatType type) {
  switch (type) {
    case kUwMacaroonCaveatTypeNonce:
    case kUwMacaroonCaveatTypeScope:
    case kUwMacaroonCaveatTypeExpirationAbsolute:
    case kUwMacaroonCaveatTypeTTL1Hour:
    case kUwMacaroonCaveatTypeTTL24Hour:
    case kUwMacaroonCaveatTypeDelegationTimestamp:
    case kUwMacaroonCaveatTypeDelegateeUser:
    case kUwMacaroonCaveatTypeDelegateeApp:
    case kUwMacaroonCaveatTypeAppCommandsOnly:
    case kUwMacaroonCaveatTypeDelegateeService:
    case kUwMacaroonCaveatTypeBleSessionID:
    case kUwMacaroonCaveatTypeLanSessionID:
    case kUwMacaroonCaveatTypeClientAuthorizationTokenV1:
    case kUwMacaroonCaveatTypeServerAuthenticationTokenV1:
      return true;
  }
  return false;
}

static bool is_valid_scope_type_(UwMacaroonCaveatScopeType type) {
  switch (type) {
    case kUwMacaroonCaveatScopeTypeOwner:
    case kUwMacaroonCaveatScopeTypeManager:
    case kUwMacaroonCaveatScopeTypeUser:
    case kUwMacaroonCaveatScopeTypeViewer:
      return true;
  }
  return false;
}

static bool create_caveat_no_value_(UwMacaroonCaveatType type,
                                    uint8_t* buffer,
                                    size_t buffer_size,
                                    UwMacaroonCaveat* new_caveat) {
  // (buffer_size == 0 || get_buffer_size_() > buffer_size) will conver the case
  // that get_buffer_size_() returns 0 (for errors), so there is no need to
  // check get_buffer_size_() == 0 again.
  if (buffer == NULL || buffer_size == 0 || new_caveat == NULL ||
      uw_macaroon_caveat_creation_get_buffsize_(type, 0) > buffer_size) {
    return false;
  }

  size_t encoded_str_len = 0, total_str_len = 0;
  if (!uw_macaroon_encoding_encode_uint_((uint32_t)type, buffer, buffer_size,
                                         &encoded_str_len)) {
    return false;
  }
  total_str_len += encoded_str_len;

  new_caveat->bytes = buffer;
  new_caveat->num_bytes = total_str_len;
  return true;
}

static bool create_caveat_uint_value_(UwMacaroonCaveatType type,
                                      uint32_t unsigned_int,
                                      uint8_t* buffer,
                                      size_t buffer_size,
                                      UwMacaroonCaveat* new_caveat) {
  if (buffer == NULL || buffer_size == 0 || new_caveat == NULL ||
      uw_macaroon_caveat_creation_get_buffsize_(type, 0) > buffer_size) {
    return false;
  }

  size_t encoded_str_len = 0, total_str_len = 0;
  if (!uw_macaroon_encoding_encode_uint_((uint32_t)type, buffer, buffer_size,
                                         &encoded_str_len)) {
    return false;
  }
  total_str_len += encoded_str_len;
  if (!uw_macaroon_encoding_encode_uint_(unsigned_int, buffer + total_str_len,
                                         buffer_size - total_str_len,
                                         &encoded_str_len)) {
    return false;
  }
  total_str_len += encoded_str_len;

  new_caveat->bytes = buffer;
  new_caveat->num_bytes = total_str_len;
  return true;
}

static bool create_caveat_bstr_value_(UwMacaroonCaveatType type,
                                      const uint8_t* str,
                                      size_t str_len,
                                      uint8_t* buffer,
                                      size_t buffer_size,
                                      UwMacaroonCaveat* new_caveat) {
  if ((str == NULL && str_len != 0) || buffer == NULL || buffer_size == 0 ||
      new_caveat == NULL ||
      uw_macaroon_caveat_creation_get_buffsize_(type, str_len) > buffer_size) {
    return false;
  }

  size_t encoded_str_len = 0, total_str_len = 0;
  if (!uw_macaroon_encoding_encode_uint_((uint32_t)type, buffer, buffer_size,
                                         &encoded_str_len)) {
    return false;
  }
  total_str_len += encoded_str_len;
  if (!uw_macaroon_encoding_encode_byte_str_(
          str, str_len, buffer + total_str_len, buffer_size - total_str_len,
          &encoded_str_len)) {
    return false;
  }
  total_str_len += encoded_str_len;

  new_caveat->bytes = buffer;
  new_caveat->num_bytes = total_str_len;
  return true;
}

size_t uw_macaroon_caveat_creation_get_buffsize_(UwMacaroonCaveatType type,
                                                 size_t str_len) {
  switch (type) {
    // No values
    case kUwMacaroonCaveatTypeTTL1Hour:
    case kUwMacaroonCaveatTypeTTL24Hour:
    case kUwMacaroonCaveatTypeAppCommandsOnly:
    case kUwMacaroonCaveatTypeBleSessionID:
      return UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN;

    // Unsigned integers
    case kUwMacaroonCaveatTypeScope:
    case kUwMacaroonCaveatTypeExpirationAbsolute:
    case kUwMacaroonCaveatTypeDelegationTimestamp:
      return 2 * UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN;

    // Byte strings
    case kUwMacaroonCaveatTypeNonce:
    case kUwMacaroonCaveatTypeDelegateeUser:
    case kUwMacaroonCaveatTypeDelegateeApp:
    case kUwMacaroonCaveatTypeDelegateeService:
    case kUwMacaroonCaveatTypeLanSessionID:
    case kUwMacaroonCaveatTypeClientAuthorizationTokenV1:
    case kUwMacaroonCaveatTypeServerAuthenticationTokenV1:
      return str_len + UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN;

    default:
      return 0;  // For errors
  }
}

bool uw_macaroon_caveat_create_nonce_(const uint8_t* nonce,
                                      size_t nonce_size,
                                      uint8_t* buffer,
                                      size_t buffer_size,
                                      UwMacaroonCaveat* new_caveat) {
  return create_caveat_bstr_value_(kUwMacaroonCaveatTypeNonce, nonce,
                                   nonce_size, buffer, buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_scope_(UwMacaroonCaveatScopeType scope,
                                      uint8_t* buffer,
                                      size_t buffer_size,
                                      UwMacaroonCaveat* new_caveat) {
  if (!is_valid_scope_type_(scope)) {
    return false;
  }

  return create_caveat_uint_value_(kUwMacaroonCaveatTypeScope, scope, buffer,
                                   buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_expiration_absolute_(
    uint32_t expiration_time,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat) {
  return create_caveat_uint_value_(kUwMacaroonCaveatTypeExpirationAbsolute,
                                   expiration_time, buffer, buffer_size,
                                   new_caveat);
}

bool uw_macaroon_caveat_create_ttl_1_hour_(uint8_t* buffer,
                                           size_t buffer_size,
                                           UwMacaroonCaveat* new_caveat) {
  return create_caveat_no_value_(kUwMacaroonCaveatTypeTTL1Hour, buffer,
                                 buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_ttl_24_hour_(uint8_t* buffer,
                                            size_t buffer_size,
                                            UwMacaroonCaveat* new_caveat) {
  return create_caveat_no_value_(kUwMacaroonCaveatTypeTTL24Hour, buffer,
                                 buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_delegation_timestamp_(
    uint32_t timestamp,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat) {
  return create_caveat_uint_value_(kUwMacaroonCaveatTypeDelegationTimestamp,
                                   timestamp, buffer, buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_delegatee_user_(const uint8_t* id_str,
                                               size_t id_str_len,
                                               uint8_t* buffer,
                                               size_t buffer_size,
                                               UwMacaroonCaveat* new_caveat) {
  return create_caveat_bstr_value_(kUwMacaroonCaveatTypeDelegateeUser, id_str,
                                   id_str_len, buffer, buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_delegatee_app_(const uint8_t* id_str,
                                              size_t id_str_len,
                                              uint8_t* buffer,
                                              size_t buffer_size,
                                              UwMacaroonCaveat* new_caveat) {
  return create_caveat_bstr_value_(kUwMacaroonCaveatTypeDelegateeApp, id_str,
                                   id_str_len, buffer, buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_app_commands_only_(
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat) {
  return create_caveat_no_value_(kUwMacaroonCaveatTypeAppCommandsOnly, buffer,
                                 buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_delegatee_service_(
    const uint8_t* id_str,
    size_t id_str_len,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat) {
  return create_caveat_bstr_value_(kUwMacaroonCaveatTypeDelegateeService,
                                   id_str, id_str_len, buffer, buffer_size,
                                   new_caveat);
}

bool uw_macaroon_caveat_create_ble_session_id_(uint8_t* buffer,
                                               size_t buffer_size,
                                               UwMacaroonCaveat* new_caveat) {
  return create_caveat_no_value_(kUwMacaroonCaveatTypeBleSessionID, buffer,
                                 buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_lan_session_id_(const uint8_t* session_id,
                                               size_t session_id_len,
                                               uint8_t* buffer,
                                               size_t buffer_size,
                                               UwMacaroonCaveat* new_caveat) {
  return create_caveat_bstr_value_(kUwMacaroonCaveatTypeLanSessionID,
                                   session_id, session_id_len, buffer,
                                   buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_client_authorization_token_(
    const uint8_t* str,
    size_t str_len,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat) {
  if (str_len == 0) {
    return create_caveat_no_value_(
        kUwMacaroonCaveatTypeClientAuthorizationTokenV1, buffer, buffer_size,
        new_caveat);
  }
  return create_caveat_bstr_value_(
      kUwMacaroonCaveatTypeClientAuthorizationTokenV1, str, str_len, buffer,
      buffer_size, new_caveat);
}

bool uw_macaroon_caveat_create_server_authentication_token_(
    const uint8_t* str,
    size_t str_len,
    uint8_t* buffer,
    size_t buffer_size,
    UwMacaroonCaveat* new_caveat) {
  if (str_len == 0) {
    return create_caveat_no_value_(
        kUwMacaroonCaveatTypeServerAuthenticationTokenV1, buffer, buffer_size,
        new_caveat);
  }
  return create_caveat_bstr_value_(
      kUwMacaroonCaveatTypeServerAuthenticationTokenV1, str, str_len, buffer,
      buffer_size, new_caveat);
}

bool uw_macaroon_caveat_get_type_(const UwMacaroonCaveat* caveat,
                                  UwMacaroonCaveatType* type) {
  if (caveat == NULL || type == NULL) {
    return false;
  }

  uint32_t unsigned_int;
  if (!uw_macaroon_encoding_decode_uint_(caveat->bytes, caveat->num_bytes,
                                         &unsigned_int)) {
    return false;
  }

  *type = (UwMacaroonCaveatType)unsigned_int;
  return is_valid_caveat_type_(*type);
}

/* === Some internal functions defined in macaroon_caveat_internal.h === */

bool uw_macaroon_caveat_sign_(const uint8_t* key,
                              size_t key_len,
                              const UwMacaroonContext* context,
                              const UwMacaroonCaveat* caveat,
                              uint8_t* mac_tag,
                              size_t mac_tag_size) {
  if (key == NULL || key_len == 0 || context == NULL || caveat == NULL ||
      mac_tag == NULL || mac_tag_size == 0) {
    return false;
  }

  UwMacaroonCaveatType caveat_type;
  if (!uw_macaroon_caveat_get_type_(caveat, &caveat_type) ||
      !is_valid_caveat_type_(caveat_type)) {
    return false;
  }

  // Need to encode the whole caveat as a byte string and then sign it

  // If there is no additional value from the context, just compute the HMAC on
  // the current byte string.
  uint8_t bstr_cbor_prefix[UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN] = {0};
  size_t bstr_cbor_prefix_len = 0;
  if (caveat_type != kUwMacaroonCaveatTypeBleSessionID) {
    if (!uw_macaroon_encoding_encode_byte_str_len_(
            (uint32_t)(caveat->num_bytes), bstr_cbor_prefix,
            sizeof(bstr_cbor_prefix), &bstr_cbor_prefix_len)) {
      return false;
    }

    UwCryptoHmacMsg messages[] = {
        {bstr_cbor_prefix, bstr_cbor_prefix_len},
        {caveat->bytes, caveat->num_bytes},
    };

    return uw_crypto_hmac_(key, key_len, messages,
                           sizeof(messages) / sizeof(messages[0]), mac_tag,
                           mac_tag_size);
  }

  // If there is additional value from the context.
  if (context->ble_session_id == NULL || context->ble_session_id_len == 0) {
    return false;
  }

  // The length here includes the length of the BLE session ID string.
  if (!uw_macaroon_encoding_encode_byte_str_len_(
          (uint32_t)(context->ble_session_id_len + caveat->num_bytes),
          bstr_cbor_prefix, sizeof(bstr_cbor_prefix), &bstr_cbor_prefix_len)) {
    return false;
  }

  uint8_t value_cbor_prefix[UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN] = {0};
  size_t value_cbor_prefix_len = 0;
  if (!uw_macaroon_encoding_encode_byte_str_len_(
          (uint32_t)(context->ble_session_id_len), value_cbor_prefix,
          sizeof(value_cbor_prefix), &value_cbor_prefix_len)) {
    return false;
  }

  UwCryptoHmacMsg messages[] = {
      {bstr_cbor_prefix, bstr_cbor_prefix_len},
      {caveat->bytes, caveat->num_bytes},
      {value_cbor_prefix, value_cbor_prefix_len},
      {context->ble_session_id, context->ble_session_id_len},
  };

  return uw_crypto_hmac_(key, key_len, messages,
                         sizeof(messages) / sizeof(messages[0]), mac_tag,
                         mac_tag_size);
}

static bool update_and_check_expiration_time(
    uint32_t current_time,
    uint32_t new_expiration_time,
    UwMacaroonValidationResult* result) {
  if (result->expiration_time > new_expiration_time) {
    result->expiration_time = new_expiration_time;
  }

  return current_time <= result->expiration_time;
}

static bool update_delegatee_list(UwMacaroonCaveatType caveat_type,
                                  const UwMacaroonCaveat* caveat,
                                  uint32_t issued_time,
                                  UwMacaroonValidationResult* result) {
  if (result->num_delegatees >= MAX_NUM_DELEGATEES || issued_time == 0) {
    return false;
  }

  UwMacaroonDelegateeType delegatee_type = kUwMacaroonDelegateeTypeNone;
  switch (caveat_type) {
    case kUwMacaroonCaveatTypeDelegateeUser:
      delegatee_type = kUwMacaroonDelegateeTypeUser;
      break;

    case kUwMacaroonCaveatTypeDelegateeApp:
      delegatee_type = kUwMacaroonDelegateeTypeApp;
      break;

    case kUwMacaroonCaveatTypeDelegateeService:
      delegatee_type = kUwMacaroonDelegateeTypeService;
      break;

    default:
      return false;
  }

  if (caveat_type != kUwMacaroonCaveatTypeDelegateeUser) {
    for (size_t i = 0; i < result->num_delegatees; i++) {
      // There must have at most one DelegateeApp or DelegateeService
      if (result->delegatees[i].type == delegatee_type) {
        return false;
      }
    }
  }

  if (!uw_macaroon_caveat_get_value_bstr_(
          caveat, &(result->delegatees[result->num_delegatees].id),
          &(result->delegatees[result->num_delegatees].id_len))) {
    return false;
  }
  result->delegatees[result->num_delegatees].type = delegatee_type;
  result->delegatees[result->num_delegatees].timestamp = issued_time;
  result->num_delegatees++;
  return true;
}

bool uw_macaroon_caveat_validate_(const UwMacaroonCaveat* caveat,
                                  const UwMacaroonContext* context,
                                  UwMacaroonValidationState* state,
                                  UwMacaroonValidationResult* result) {
  if (caveat == NULL || context == NULL || state == NULL || result == NULL) {
    return false;
  }

  uint32_t expiration_time = 0;
  uint32_t issued_time = 0;
  uint32_t scope = UW_MACAROON_CAVEAT_SCOPE_LOWEST_POSSIBLE;

  UwMacaroonCaveatType caveat_type;
  if (!uw_macaroon_caveat_get_type_(caveat, &caveat_type)) {
    return false;
  }

  switch (caveat_type) {
    // The types that always validate
    case kUwMacaroonCaveatTypeClientAuthorizationTokenV1:
    case kUwMacaroonCaveatTypeServerAuthenticationTokenV1:
    case kUwMacaroonCaveatTypeNonce:
    case kUwMacaroonCaveatTypeBleSessionID:
      return true;

    case kUwMacaroonCaveatTypeDelegationTimestamp:
      if (!uw_macaroon_caveat_get_value_uint_(caveat, &issued_time) ||
          issued_time < state->issued_time) {
        return false;
      }
      state->issued_time = issued_time;
      return true;

    case kUwMacaroonCaveatTypeTTL1Hour:
      if (state->issued_time == 0) {
        return false;
      }
      return update_and_check_expiration_time(
          context->current_time, state->issued_time + 60 * 60, result);

    case kUwMacaroonCaveatTypeTTL24Hour:
      if (state->issued_time == 0) {
        return false;
      }
      return update_and_check_expiration_time(
          context->current_time, state->issued_time + 24 * 60 * 60, result);

    // Need to create a list of delegatees
    case kUwMacaroonCaveatTypeDelegateeUser:
    case kUwMacaroonCaveatTypeDelegateeApp:
    case kUwMacaroonCaveatTypeDelegateeService:
      return update_delegatee_list(caveat_type, caveat, state->issued_time,
                                   result);

    // Time related caveats
    case kUwMacaroonCaveatTypeExpirationAbsolute:
      if (!uw_macaroon_caveat_get_value_uint_(caveat, &expiration_time)) {
        return false;
      }
      return update_and_check_expiration_time(context->current_time,
                                              expiration_time, result);

    // The caveats that update the values of the result object
    case kUwMacaroonCaveatTypeScope:
      if (!uw_macaroon_caveat_get_value_uint_(caveat, &scope) ||
          // Larger value means less priviledge
          scope > UW_MACAROON_CAVEAT_SCOPE_LOWEST_POSSIBLE) {
        return false;
      }
      if (scope > (uint32_t)(result->granted_scope)) {
        result->granted_scope = (UwMacaroonCaveatScopeType)scope;
      }
      return true;

    case kUwMacaroonCaveatTypeAppCommandsOnly:
      result->weave_app_restricted = true;
      return true;

    case kUwMacaroonCaveatTypeLanSessionID:
      return uw_macaroon_caveat_get_value_bstr_(
          caveat, &(result->lan_session_id), &(result->lan_session_id_len));
  }

  return false;
}

bool uw_macaroon_caveat_get_value_uint_(const UwMacaroonCaveat* caveat,
                                        uint32_t* unsigned_int) {
  if (caveat == NULL || unsigned_int == NULL) {
    return false;
  }

  UwMacaroonCaveatType type;
  if (!uw_macaroon_caveat_get_type_(caveat, &type)) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeScope &&
      type != kUwMacaroonCaveatTypeExpirationAbsolute &&
      type != kUwMacaroonCaveatTypeDelegationTimestamp) {
    // Wrong type
    return false;
  }

  // Skip the portion for CBOR type
  size_t offset;
  if (!uw_macaroon_encoding_get_item_len_(caveat->bytes, caveat->num_bytes,
                                          &offset)) {
    return false;
  }

  return uw_macaroon_encoding_decode_uint_(
      caveat->bytes + offset, caveat->num_bytes - offset, unsigned_int);
}

bool uw_macaroon_caveat_get_value_bstr_(const UwMacaroonCaveat* caveat,
                                        const uint8_t** str,
                                        size_t* str_len) {
  if (caveat == NULL || str == NULL || str_len == NULL) {
    return false;
  }

  UwMacaroonCaveatType type;
  if (!uw_macaroon_caveat_get_type_(caveat, &type)) {
    return false;
  }
  if (type != kUwMacaroonCaveatTypeNonce &&
      type != kUwMacaroonCaveatTypeDelegateeUser &&
      type != kUwMacaroonCaveatTypeDelegateeApp &&
      type != kUwMacaroonCaveatTypeDelegateeService &&
      type != kUwMacaroonCaveatTypeLanSessionID &&
      type != kUwMacaroonCaveatTypeClientAuthorizationTokenV1 &&
      type != kUwMacaroonCaveatTypeServerAuthenticationTokenV1) {
    // Wrong type
    return false;
  }

  size_t offset;
  if (!uw_macaroon_encoding_get_item_len_(caveat->bytes, caveat->num_bytes,
                                          &offset)) {
    return false;
  }

  return uw_macaroon_encoding_decode_byte_str_(
      caveat->bytes + offset, caveat->num_bytes - offset, str, str_len);
}

bool uw_macaroon_caveat_init_validation_state_(
    UwMacaroonValidationState* state) {
  if (state == NULL) {
    return false;
  }

  state->issued_time = 0;
  return true;
}

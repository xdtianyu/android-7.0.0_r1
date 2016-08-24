// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon.h"

#include <string.h>

#include "src/crypto_utils.h"
#include "src/macaroon_caveat.h"
#include "src/macaroon_caveat_internal.h"
#include "src/macaroon_encoding.h"

static bool create_mac_tag_(const uint8_t* key,
                            size_t key_len,
                            const UwMacaroonContext* context,
                            const UwMacaroonCaveat* const caveats[],
                            size_t num_caveats,
                            uint8_t mac_tag[UW_MACAROON_MAC_LEN]) {
  if (key == NULL || key_len == 0 || context == NULL || caveats == NULL ||
      num_caveats == 0 || mac_tag == NULL) {
    return false;
  }

  // Store the intermediate MAC tags in an internal buffer before we finish the
  // whole computation.
  // If we use the output buffer mac_tag directly and certain errors happen in
  // the middle of this computation, mac_tag will probably contain a valid
  // macaroon tag with large scope than expected.
  uint8_t mac_tag_buff[UW_MACAROON_MAC_LEN];

  // Compute the first tag by using the key
  if (!uw_macaroon_caveat_sign_(key, key_len, context, caveats[0], mac_tag_buff,
                                UW_MACAROON_MAC_LEN)) {
    return false;
  }

  // Compute the rest of the tags by using the tag as the key
  for (size_t i = 1; i < num_caveats; i++) {
    if (!uw_macaroon_caveat_sign_(mac_tag_buff, UW_MACAROON_MAC_LEN, context,
                                  caveats[i], mac_tag_buff,
                                  UW_MACAROON_MAC_LEN)) {
      return false;
    }
  }

  memcpy(mac_tag, mac_tag_buff, UW_MACAROON_MAC_LEN);
  return true;
}

static bool verify_mac_tag_(const uint8_t* root_key,
                            size_t root_key_len,
                            const UwMacaroonContext* context,
                            const UwMacaroonCaveat* const caveats[],
                            size_t num_caveats,
                            const uint8_t mac_tag[UW_MACAROON_MAC_LEN]) {
  if (root_key == NULL || root_key_len == 0 || context == NULL ||
      caveats == NULL || num_caveats == 0 || mac_tag == 0) {
    return false;
  }

  uint8_t computed_mac_tag[UW_MACAROON_MAC_LEN] = {0};
  if (!create_mac_tag_(root_key, root_key_len, context, caveats, num_caveats,
                       computed_mac_tag)) {
    return false;
  }

  return uw_crypto_utils_equal_(mac_tag, computed_mac_tag, UW_MACAROON_MAC_LEN);
}

bool uw_macaroon_create_from_root_key_(UwMacaroon* new_macaroon,
                                       const uint8_t* root_key,
                                       size_t root_key_len,
                                       const UwMacaroonContext* context,
                                       const UwMacaroonCaveat* const caveats[],
                                       size_t num_caveats) {
  if (new_macaroon == NULL || root_key == NULL || context == NULL ||
      root_key_len == 0 || caveats == NULL || num_caveats == 0) {
    return false;
  }

  if (!create_mac_tag_(root_key, root_key_len, context, caveats, num_caveats,
                       new_macaroon->mac_tag)) {
    return false;
  }

  new_macaroon->num_caveats = num_caveats;
  new_macaroon->caveats = caveats;

  return true;
}

bool uw_macaroon_extend_(const UwMacaroon* old_macaroon,
                         UwMacaroon* new_macaroon,
                         const UwMacaroonContext* context,
                         const UwMacaroonCaveat* additional_caveat,
                         uint8_t* buffer,
                         size_t buffer_size) {
  if (old_macaroon == NULL || new_macaroon == NULL || context == NULL ||
      additional_caveat == NULL || buffer == NULL || buffer_size == 0) {
    return false;
  }

  new_macaroon->num_caveats = old_macaroon->num_caveats + 1;

  // Extend the caveat pointer list
  if ((new_macaroon->num_caveats) * sizeof(UwMacaroonCaveat*) > buffer_size) {
    // Not enough memory to store the extended caveat pointer list
    return false;
  }
  const UwMacaroonCaveat** extended_list = (const UwMacaroonCaveat**)buffer;
  if (new_macaroon->caveats != old_macaroon->caveats) {
    memcpy(extended_list, old_macaroon->caveats,
           old_macaroon->num_caveats * sizeof(old_macaroon->caveats[0]));
  }
  extended_list[old_macaroon->num_caveats] = additional_caveat;
  new_macaroon->caveats = (const UwMacaroonCaveat* const*)extended_list;

  // Compute the new MAC tag
  return create_mac_tag_(old_macaroon->mac_tag, UW_MACAROON_MAC_LEN, context,
                         new_macaroon->caveats + old_macaroon->num_caveats, 1,
                         new_macaroon->mac_tag);
}

static void init_validation_result(UwMacaroonValidationResult* result) {
  // Start from the largest scope
  *result = (UwMacaroonValidationResult){
      .granted_scope = kUwMacaroonCaveatScopeTypeOwner,
      .expiration_time = UINT32_MAX,
  };
}

/** Reset the result object to the lowest scope when encountering errors */
static void reset_validation_result(UwMacaroonValidationResult* result) {
  *result = (UwMacaroonValidationResult){
      .weave_app_restricted = true,
      .granted_scope = UW_MACAROON_CAVEAT_SCOPE_LOWEST_POSSIBLE};
}

/** Get the next closest scope (to the narrower side). */
static UwMacaroonCaveatScopeType get_closest_scope(
    UwMacaroonCaveatScopeType scope) {
  if (scope <= kUwMacaroonCaveatScopeTypeOwner) {
    return kUwMacaroonCaveatScopeTypeOwner;
  } else if (scope <= kUwMacaroonCaveatScopeTypeManager) {
    return kUwMacaroonCaveatScopeTypeManager;
  } else if (scope <= kUwMacaroonCaveatScopeTypeUser) {
    return kUwMacaroonCaveatScopeTypeUser;
  } else if (scope <= kUwMacaroonCaveatScopeTypeViewer) {
    return kUwMacaroonCaveatScopeTypeViewer;
  }
  return scope;
}

bool uw_macaroon_validate_(const UwMacaroon* macaroon,
                           const uint8_t* root_key,
                           size_t root_key_len,
                           const UwMacaroonContext* context,
                           UwMacaroonValidationResult* result) {
  if (result == NULL) {
    return false;
  }
  init_validation_result(result);

  if (root_key == NULL || root_key_len == 0 || macaroon == NULL ||
      context == NULL || result == NULL ||
      !verify_mac_tag_(root_key, root_key_len, context, macaroon->caveats,
                       macaroon->num_caveats, macaroon->mac_tag)) {
    return false;
  }

  UwMacaroonValidationState state;
  if (!uw_macaroon_caveat_init_validation_state_(&state)) {
    return false;
  }
  for (size_t i = 0; i < macaroon->num_caveats; i++) {
    if (!uw_macaroon_caveat_validate_(macaroon->caveats[i], context, &state,
                                      result)) {
      reset_validation_result(result);  // Reset the result object
      return false;
    }
  }

  result->granted_scope = get_closest_scope(result->granted_scope);
  return true;
}

// Encode a Macaroon to a byte string
bool uw_macaroon_serialize_(const UwMacaroon* macaroon,
                            uint8_t* out,
                            size_t out_len,
                            size_t* resulting_str_len) {
  if (macaroon == NULL || out == NULL ||
      out_len < UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN ||
      resulting_str_len == NULL) {
    return false;
  }

  // Need to encode the whole Macaroon again into a byte string.

  // First encode the part without the overall byte string header to the buffer
  // to get the total length.
  size_t item_len = 0;
  // Start with an offset
  size_t offset = UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN;
  if (!uw_macaroon_encoding_encode_array_len_((uint32_t)(macaroon->num_caveats),
                                              out + offset, out_len - offset,
                                              &item_len)) {
    return false;
  }
  offset += item_len;

  for (size_t i = 0; i < macaroon->num_caveats; i++) {
    if (!uw_macaroon_encoding_encode_byte_str_(
            macaroon->caveats[i]->bytes, macaroon->caveats[i]->num_bytes,
            out + offset, out_len - offset, &item_len)) {
      return false;
    }
    offset += item_len;
  }

  if (!uw_macaroon_encoding_encode_byte_str_(macaroon->mac_tag,
                                             UW_MACAROON_MAC_LEN, out + offset,
                                             out_len - offset, &item_len)) {
    return false;
  }
  offset += item_len;

  // Encode the length of the body at the beginning of the buffer
  size_t bstr_len = offset - UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN;
  if (!uw_macaroon_encoding_encode_byte_str_len_(
          bstr_len, out, UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN, &item_len)) {
    return false;
  }

  // Move the body part to be adjacent to the byte string header part
  memmove(out + item_len, out + UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN,
          bstr_len);

  *resulting_str_len = item_len + bstr_len;
  return true;
}

// Decode a byte string to a Macaroon
bool uw_macaroon_deserialize_(const uint8_t* in,
                              size_t in_len,
                              uint8_t* buffer,
                              size_t buffer_size,
                              UwMacaroon* macaroon) {
  if (in == NULL || in_len == 0 || buffer == NULL || buffer_size == 0 ||
      macaroon == NULL) {
    return false;
  }

  size_t offset = 0;
  size_t item_len = 0;

  const uint8_t* bstr = NULL;
  size_t bstr_len = 0;
  if (!uw_macaroon_encoding_decode_byte_str_(in + offset, in_len - offset,
                                             &bstr, &bstr_len)) {
    return false;
  }
  item_len = bstr - in;  // The length of the first byte string header
  offset += item_len;

  if (item_len + bstr_len != in_len) {
    // The string length doesn't match
    return false;
  }

  uint32_t array_len = 0;
  if (!uw_macaroon_encoding_decode_array_len_(in + offset, in_len - offset,
                                              &array_len)) {
    return false;
  }
  macaroon->num_caveats = (size_t)array_len;
  if (buffer_size <
      (array_len * (sizeof(UwMacaroonCaveat) + sizeof(UwMacaroonCaveat*)))) {
    // Need two levels of abstraction, one for structs and one for pointers
    return false;
  }

  if (!uw_macaroon_encoding_get_item_len_(in + offset, in_len - offset,
                                          &item_len)) {
    return false;
  }
  offset += item_len;

  const UwMacaroonCaveat** caveat_pointers = (const UwMacaroonCaveat**)buffer;
  buffer += array_len * sizeof(UwMacaroonCaveat*);
  UwMacaroonCaveat* caveat_structs = (UwMacaroonCaveat*)buffer;
  for (size_t i = 0; i < array_len; i++) {
    caveat_pointers[i] = &(caveat_structs[i]);

    if (!uw_macaroon_encoding_decode_byte_str_(
            in + offset, in_len - offset, &(caveat_structs[i].bytes),
            &(caveat_structs[i].num_bytes))) {
      return false;
    }

    if (!uw_macaroon_encoding_get_item_len_(in + offset, in_len - offset,
                                            &item_len)) {
      return false;
    }
    offset += item_len;
  }
  macaroon->caveats = caveat_pointers;

  const uint8_t* tag;
  size_t tag_len;
  if (!uw_macaroon_encoding_decode_byte_str_(in + offset, in_len - offset, &tag,
                                             &tag_len) ||
      tag_len != UW_MACAROON_MAC_LEN) {
    return false;
  }
  memcpy(macaroon->mac_tag, tag, UW_MACAROON_MAC_LEN);

  return true;
}

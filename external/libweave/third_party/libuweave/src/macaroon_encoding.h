// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UW_LIBUWEAVE_SRC_MACAROON_ENCODING_
#define UW_LIBUWEAVE_SRC_MACAROON_ENCODING_

/*
 * Utility functions to encode and decode canonical CBOR representations for
 * cryptographic use, such as signatures. We only need to support a very small
 * subset of the CBOR standard, since only these are used in our cryptographic
 * designs. The supported data types are: unsigned integers (maximum 32 bits),
 * byte strings, text strings, and arrays.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UW_MACAROON_ENCODING_MAX_UINT_CBOR_LEN 5

/**
 * Gets the number of bytes that is occupied by the first data item in the give
 * CBOR string.
 */
bool uw_macaroon_encoding_get_item_len_(const uint8_t* cbor,
                                        size_t cbor_len,
                                        size_t* first_item_len);

bool uw_macaroon_encoding_encode_uint_(const uint32_t unsigned_int,
                                       uint8_t* buffer,
                                       size_t buffer_size,
                                       size_t* resulting_cbor_len);
bool uw_macaroon_encoding_encode_array_len_(const uint32_t array_len,
                                            uint8_t* buffer,
                                            size_t buffer_size,
                                            size_t* resulting_cbor_len);
bool uw_macaroon_encoding_encode_byte_str_(const uint8_t* str,
                                           size_t str_len,
                                           uint8_t* buffer,
                                           size_t buffer_size,
                                           size_t* resulting_cbor_len);
bool uw_macaroon_encoding_encode_text_str_(const uint8_t* str,
                                           size_t str_len,
                                           uint8_t* buffer,
                                           size_t buffer_size,
                                           size_t* resulting_cbor_len);

/** Only encode the header (major type and length) of the byte string */
bool uw_macaroon_encoding_encode_byte_str_len_(size_t str_len,
                                               uint8_t* buffer,
                                               size_t buffer_size,
                                               size_t* resulting_cbor_len);

bool uw_macaroon_encoding_decode_uint_(const uint8_t* cbor,
                                       size_t cbor_len,
                                       uint32_t* unsigned_int);
bool uw_macaroon_encoding_decode_array_len_(const uint8_t* cbor,
                                            size_t cbor_len,
                                            uint32_t* array_len);
bool uw_macaroon_encoding_decode_byte_str_(const uint8_t* cbor,
                                           size_t cbor_len,
                                           const uint8_t** str,
                                           size_t* str_len);
bool uw_macaroon_encoding_decode_text_str_(const uint8_t* cbor,
                                           size_t cbor_len,
                                           const uint8_t** str,
                                           size_t* str_len);

#endif  // UW_LIBUWEAVE_SRC_MACAROON_ENCODING_

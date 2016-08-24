// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/macaroon_encoding.h"

#include <string.h>

#define MAJOR_TYPE_MASK 0xE0       // 0b11100000
#define ADDITIONAL_DATA_MASK 0x1F  // 0b00011111

#define FLAG_1BYTE_UINT 24
#define FLAG_2BYTE_UINT 25
#define FLAG_4BYTE_UINT 26
// #define FLAG_8BYTE_UINT 27  // Do not support 8-byte

typedef enum {
  kCborMajorTypeUint = 0,          // type 0 -- unsigned integers
  kCborMajorTypeByteStr = 2 << 5,  // type 2 -- byte strings
  kCborMajorTypeTextStr = 3 << 5,  // type 3 -- text strings
  kCborMajorTypeArray = 4 << 5,    // type 4 -- arrays
} CborMajorType;

static inline CborMajorType get_type_(const uint8_t* cbor);
static inline uint8_t get_addtl_data_(const uint8_t* cbor);
static inline void set_type_(CborMajorType type, uint8_t* cbor);
static inline void set_addtl_data_(uint8_t addtl_data, uint8_t* cbor);

/** Computes the minimum number of bytes to store the unsigned integer. */
static inline size_t uint_min_len_(uint32_t unsigned_int);

/** Encoding or decoding without checking types */
static bool blindly_encode_uint_(uint32_t unsigned_int,
                                 uint8_t* buffer,
                                 size_t buffer_size,
                                 size_t* result_len);
static bool blindly_encode_str_(const uint8_t* str,
                                size_t str_len,
                                uint8_t* buffer,
                                size_t buffer_size,
                                size_t* result_len);
static bool blindly_decode_uint_(const uint8_t* cbor,
                                 size_t cbor_len,
                                 uint32_t* unsigned_int);
static bool blindly_decode_str_(const uint8_t* cbor,
                                size_t cbor_len,
                                const uint8_t** out_str,
                                size_t* out_str_len);

bool uw_macaroon_encoding_get_item_len_(const uint8_t* cbor,
                                        size_t cbor_len,
                                        size_t* first_item_len) {
  if (cbor == NULL || cbor_len == 0 || first_item_len == NULL) {
    return false;
  }

  CborMajorType type = get_type_(cbor);
  if (type != kCborMajorTypeUint && type != kCborMajorTypeByteStr &&
      type != kCborMajorTypeTextStr && type != kCborMajorTypeArray) {
    // Other types are not supported
    return false;
  }

  uint32_t unsigned_int;
  if (!blindly_decode_uint_(cbor, cbor_len, &unsigned_int)) {
    return false;
  }

  *first_item_len = uint_min_len_(unsigned_int) + 1;

  // For arrays, it returns only the length of the array length portion, not the
  // length of the whole array
  if (type == kCborMajorTypeByteStr || type == kCborMajorTypeTextStr) {
    *first_item_len += (size_t)unsigned_int;
  }

  if (*first_item_len > cbor_len) {
    // Something is wrong. The CBOR string isn't long enough.
    return false;
  }
  return true;
}

bool uw_macaroon_encoding_encode_uint_(const uint32_t unsigned_int,
                                       uint8_t* buffer,
                                       size_t buffer_size,
                                       size_t* resulting_cbor_len) {
  if (buffer == NULL || buffer_size == 0 || resulting_cbor_len == NULL) {
    return false;
  }

  set_type_(kCborMajorTypeUint, buffer);
  return blindly_encode_uint_(unsigned_int, buffer, buffer_size,
                              resulting_cbor_len);
}

bool uw_macaroon_encoding_encode_array_len_(const uint32_t array_len,
                                            uint8_t* buffer,
                                            size_t buffer_size,
                                            size_t* resulting_cbor_len) {
  if (buffer == NULL || buffer_size == 0 || resulting_cbor_len == NULL) {
    return false;
  }

  set_type_(kCborMajorTypeArray, buffer);
  return blindly_encode_uint_(array_len, buffer, buffer_size,
                              resulting_cbor_len);
}

bool uw_macaroon_encoding_encode_byte_str_(const uint8_t* str,
                                           size_t str_len,
                                           uint8_t* buffer,
                                           size_t buffer_size,
                                           size_t* resulting_cbor_len) {
  if (buffer == NULL || buffer_size == 0 || resulting_cbor_len == NULL) {
    return false;
  }

  set_type_(kCborMajorTypeByteStr, buffer);
  return blindly_encode_str_(str, str_len, buffer, buffer_size,
                             resulting_cbor_len);
}

bool uw_macaroon_encoding_encode_text_str_(const uint8_t* str,
                                           size_t str_len,
                                           uint8_t* buffer,
                                           size_t buffer_size,
                                           size_t* resulting_cbor_len) {
  if (buffer == NULL || buffer_size == 0 || resulting_cbor_len == NULL) {
    return false;
  }

  set_type_(kCborMajorTypeTextStr, buffer);
  return blindly_encode_str_(str, str_len, buffer, buffer_size,
                             resulting_cbor_len);
}

bool uw_macaroon_encoding_encode_byte_str_len_(size_t str_len,
                                               uint8_t* buffer,
                                               size_t buffer_size,
                                               size_t* resulting_cbor_len) {
  if (buffer == NULL || buffer_size == 0 || resulting_cbor_len == NULL) {
    return false;
  }
  set_type_(kCborMajorTypeByteStr, buffer);
  return blindly_encode_uint_(str_len, buffer, buffer_size, resulting_cbor_len);
}

bool uw_macaroon_encoding_decode_uint_(const uint8_t* cbor,
                                       size_t cbor_len,
                                       uint32_t* unsigned_int) {
  if (cbor == NULL || cbor_len == 0 || unsigned_int == NULL ||
      get_type_(cbor) != kCborMajorTypeUint) {
    return false;
  }

  return blindly_decode_uint_(cbor, cbor_len, unsigned_int);
}

bool uw_macaroon_encoding_decode_array_len_(const uint8_t* cbor,
                                            size_t cbor_len,
                                            uint32_t* array_len) {
  if (cbor == NULL || cbor_len == 0 || array_len == NULL ||
      get_type_(cbor) != kCborMajorTypeArray) {
    return false;
  }

  return blindly_decode_uint_(cbor, cbor_len, array_len);
}

bool uw_macaroon_encoding_decode_byte_str_(const uint8_t* cbor,
                                           size_t cbor_len,
                                           const uint8_t** out_str,
                                           size_t* out_str_len) {
  if (cbor == NULL || cbor_len == 0 || out_str == NULL || out_str_len == NULL ||
      get_type_(cbor) != kCborMajorTypeByteStr) {
    return false;
  }

  return blindly_decode_str_(cbor, cbor_len, out_str, out_str_len);
}

bool uw_macaroon_encoding_decode_text_str_(const uint8_t* cbor,
                                           size_t cbor_len,
                                           const uint8_t** out_str,
                                           size_t* out_str_len) {
  if (cbor == NULL || cbor_len == 0 || out_str == NULL || out_str_len == NULL ||
      get_type_(cbor) != kCborMajorTypeTextStr) {
    return false;
  }

  return blindly_decode_str_(cbor, cbor_len, out_str, out_str_len);
}

static inline CborMajorType get_type_(const uint8_t* cbor) {
  return (CborMajorType)((*cbor) & MAJOR_TYPE_MASK);
}

static inline uint8_t get_addtl_data_(const uint8_t* cbor) {
  return (*cbor) & ADDITIONAL_DATA_MASK;
}

static inline void set_type_(CborMajorType type, uint8_t* cbor) {
  *cbor = ((uint8_t)type) | ((*cbor) & ADDITIONAL_DATA_MASK);
}

static inline void set_addtl_data_(uint8_t addtl_data, uint8_t* cbor) {
  *cbor = ((*cbor) & MAJOR_TYPE_MASK) | (addtl_data & ADDITIONAL_DATA_MASK);
}

static inline size_t uint_min_len_(uint32_t unsigned_int) {
  if (unsigned_int < FLAG_1BYTE_UINT) {
    return 0;  // Should be stored in the 5-bit additional data part
  } else if (unsigned_int <= 0xFF) {
    return 1;
  } else if (unsigned_int <= 0xFFFF) {
    return 2;
  }
  return 4;
}

/**
 * Writes the unsigned int in the big-endian fashion by using the minimum number
 * of bytes in CBOR
 */
static inline bool write_uint_big_endian_(uint32_t unsigned_int,
                                          uint8_t* buff,
                                          size_t buff_len) {
  if (buff == NULL || buff_len == 0) {
    return false;
  }

  size_t num_bytes = uint_min_len_(unsigned_int);
  if (num_bytes > buff_len) {
    // Not enough memory
    return false;
  }

  switch (num_bytes) {
    // Falling through intentionally
    case 4:
      *(buff++) = (uint8_t)(0xFF & (unsigned_int >> 24));
      *(buff++) = (uint8_t)(0xFF & (unsigned_int >> 16));
    case 2:
      *(buff++) = (uint8_t)(0xFF & (unsigned_int >> 8));
    case 1:
      *(buff++) = (uint8_t)(0xFF & (unsigned_int));
      break;

    default:
      return false;
  }

  return true;
}

/** Reads the unsigned int written in big-endian. */
static inline bool read_uint_big_endian_(const uint8_t* bytes,
                                         size_t num_bytes,
                                         uint32_t* unsigned_int) {
  if (bytes == NULL || num_bytes == 0 || num_bytes > 4 ||
      unsigned_int == NULL) {
    return false;
  }

  *unsigned_int = 0;
  switch (num_bytes) {
    // Falling through intentionally
    case 4:
      *unsigned_int |= ((uint32_t)(*(bytes++))) << 24;
      *unsigned_int |= ((uint32_t)(*(bytes++))) << 16;
    case 2:
      *unsigned_int |= ((uint32_t)(*(bytes++))) << 8;
    case 1:
      *unsigned_int |= ((uint32_t)(*(bytes++)));
      break;

    default:
      return false;
  }

  return true;
}

static bool blindly_encode_uint_(uint32_t unsigned_int,
                                 uint8_t* buffer,
                                 size_t buffer_size,
                                 size_t* result_len) {
  if (buffer == NULL || buffer_size == 0 || result_len == NULL) {
    return false;
  }

  // Don't need to set the data type in this function

  *result_len = uint_min_len_(unsigned_int) + 1;

  if (*result_len > buffer_size) {
    // Not enough memory
    return false;
  }

  switch (*result_len) {
    case 1:
      set_addtl_data_(unsigned_int, buffer);
      return true;
    case 2:  // 1 + 1
      set_addtl_data_(FLAG_1BYTE_UINT, buffer);
      break;
    case 3:  // 1 + 2
      set_addtl_data_(FLAG_2BYTE_UINT, buffer);
      break;
    case 5:  // 1 + 4
      set_addtl_data_(FLAG_4BYTE_UINT, buffer);
      break;
    default:
      // Wrong length
      return false;
  }

  return write_uint_big_endian_(unsigned_int, buffer + 1, buffer_size - 1);
}

static bool blindly_encode_str_(const uint8_t* str,
                                size_t str_len,
                                uint8_t* buffer,
                                size_t buffer_size,
                                size_t* result_len) {
  if (buffer == NULL || buffer_size == 0) {
    return false;
  }
  if (str == NULL && str_len != 0) {
    // str_len should be 0 for empty strings
    return false;
  }

  // Don't need to set the data type in this function

  if (!blindly_encode_uint_((uint32_t)str_len, buffer, buffer_size,
                            result_len)) {
    return false;
  }

  if (str_len == 0) {
    return true;
  }

  if (str_len + (*result_len) > buffer_size) {
    // Not enough memory
    return false;
  }

  memcpy(buffer + (*result_len), str, str_len);
  *result_len += str_len;
  return true;
}

static bool blindly_decode_uint_(const uint8_t* cbor,
                                 size_t cbor_len,
                                 uint32_t* unsigned_int) {
  if (cbor == NULL || cbor_len == 0 || unsigned_int == NULL) {
    return false;
  }

  uint8_t addtl_data = get_addtl_data_(cbor);
  if (addtl_data < FLAG_1BYTE_UINT) {
    *unsigned_int = (uint32_t)addtl_data;
    return true;
  }
  if (addtl_data > FLAG_4BYTE_UINT) {
    return false;
  }

  size_t uint_num_bytes = 1 << (addtl_data - (uint8_t)FLAG_1BYTE_UINT);
  if (uint_num_bytes + 1 > cbor_len) {
    // The CBOR string isn't long enough.
    return false;
  }

  return read_uint_big_endian_(cbor + 1, uint_num_bytes, unsigned_int);
}

static bool blindly_decode_str_(const uint8_t* cbor,
                                size_t cbor_len,
                                const uint8_t** out_str,
                                size_t* out_str_len) {
  if (cbor == NULL || cbor_len == 0 || out_str == NULL || out_str == NULL) {
    return false;
  }

  uint32_t unsigned_int;
  if (!blindly_decode_uint_(cbor, cbor_len, &unsigned_int)) {
    return false;
  }

  size_t offset = 1 + uint_min_len_(unsigned_int);
  if (unsigned_int > (uint32_t)(cbor_len - offset)) {
    // The CBOR string isn't long enough
    return false;
  }

  *out_str = cbor + offset;
  *out_str_len = unsigned_int;
  return true;
}

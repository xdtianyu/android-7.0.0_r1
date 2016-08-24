// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "polo/util/poloutil.h"

#include <openssl/rand.h>

namespace polo {
namespace util {

const std::string PoloUtil::BytesToHexString(const uint8_t* bytes,
                                             size_t length) {
  // Use OpenSSL BigNum functions to perform the conversion.
  BIGNUM* bn = BN_bin2bn(bytes, length, NULL);
  char* hex = BN_bn2hex(bn);
  std::string hex_string(hex);
  BN_free(bn);
  OPENSSL_free(hex);
  return hex_string;
}

const size_t PoloUtil::HexStringToBytes(const std::string hex_string,
                                        uint8_t*& bytes) {
  // Use OpenSSL BigNum functions to perform the conversion.
  BIGNUM* bn = NULL;
  BN_hex2bn(&bn, hex_string.c_str());
  int length = BN_num_bytes(bn);
  bytes = new uint8_t[length];
  BN_bn2bin(bn, &bytes[0]);
  BN_free(bn);
  return length;
}

const void PoloUtil::IntToBigEndianBytes(uint32_t value,
                                         uint8_t*& bytes) {
  // Use OpenSSL BigNum functions to perform the conversion.
  BIGNUM* bn = BN_new();
  BN_add_word(bn, value);

  // Initialize the array to 0 so there will be leading null bytes if the
  // number is less than 4 bytes long.
  bytes = new uint8_t[4];
  for (int i = 0; i < 4; i++) {
    bytes[i] = 0;
  }

  int length = BN_num_bytes(bn);
  BN_bn2bin(bn, &bytes[4 - length]);
  BN_free(bn);
}

const uint32_t PoloUtil::BigEndianBytesToInt(
    const uint8_t* bytes) {
  // Use OpenSSL BigNum functions to perform the conversion.
  BIGNUM* bn = BN_bin2bn(bytes, 4, NULL);
  BN_ULONG value = bn->d[0];
  BN_free(bn);
  return value;
}

uint8_t* PoloUtil::GenerateRandomBytes(size_t length) {
  // Use the OpenSSL library to generate the random byte array. The RAND_bytes
  // function is guaranteed to provide secure random bytes.
  uint8_t* buffer = new uint8_t[length];
  if (RAND_bytes(buffer, length)) {
    return buffer;
  } else {
    delete buffer;
    return NULL;
  }
}

}  // namespace util
}  // namespace polo

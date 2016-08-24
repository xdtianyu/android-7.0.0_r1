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

#ifndef POLO_UTIL_POLOUTIL_H_
#define POLO_UTIL_POLOUTIL_H_

#include <openssl/ssl.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace polo {
namespace util {

// Utilites used for the Polo protocol.
class PoloUtil {
 public:
  // Converts an array of big-endian bytes to a hex string.
  // @param bytes an array of big-endian bytes
  // @param length the length of the given byte array
  // @return a hex string representing the given byte array
  static const std::string BytesToHexString(const uint8_t* bytes,
                                            size_t length);

  // Converts a hex string to an array of big-endian bytes. A new byte array
  // is created at the given bytes pointer, and the number of bytes is returned.
  // The byte array must be freed using: delete[] bytes.
  // @param hex_string the hex string to convert
  // @param bytes pointer to a byte array that will be created with the
  //              big-endian result
  // @return the number of bytes in the resulting byte array
  static const size_t HexStringToBytes(const std::string hex_string,
                                       uint8_t*& bytes);

  // Converts an integer value to a big-endian array of bytes. A new byte array
  // is created at the given bytes pointer. There are always 4 bytes in the
  // array. The byte array must be freed using: delete[] bytes.
  // @param value the integer value to convert
  // @param bytes pointer to a byte array that will be created with the 4-byte
  //              big-endian array
  static const void IntToBigEndianBytes(uint32_t value,
                                        uint8_t*& bytes);

  // Converts a big-endian array of bytes to an unsigned-integer. The given byte
  // array must contain 4 bytes.
  // @param bytes a big-endian array of bytes
  // @return the unsigned integer representation of the given byte array
  static const uint32_t BigEndianBytesToInt(const uint8_t* bytes);

  // Generates a random array of bytes with the given length. NULL is returned
  // if a random number could not be generated. The returned array must be freed
  // using: delete[] bytes.
  // @param length the number of random bytes to generate
  // @return an array of random bytes of the given length
  static uint8_t* GenerateRandomBytes(size_t length);
};

}  // namespace util
}  // namespace polo

#endif  // POLO_UTIL_POLOUTIL_H_

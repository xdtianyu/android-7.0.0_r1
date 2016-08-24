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

#ifndef POLO_ENCODING_SECRETENCODER_H_
#define POLO_ENCODING_SECRETENCODER_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace polo {
namespace encoding {

// Encodes and decodes secret challenges. The decoded secret is displayed to the
// user on the display device, and entered by the user on the input device. The
// secret is encoded for transmission on the wire and used for computing pairing
// keys.
class SecretEncoder {
 public:
  virtual ~SecretEncoder() {}

  // Encodes a byte array representation of a secret to a string.
  // @param secret the secret bytes
  // @return a string representation of the given secret
  virtual std::string EncodeToString(
      const std::vector<uint8_t>& secret) const = 0;

  // Decodes the string representation of the secret to a byte array.
  // @param secret a string representation of the secret
  // @return the decoded secret as a byte array
  virtual std::vector<uint8_t> DecodeToBytes(
      const std::string& secret) const = 0;

  // The number of symbols contained in each byte of data. For example, a
  // hexadecimal encoding has 4 bytes per symbol and therefore 2 symbols per
  // byte.
  virtual size_t symbols_per_byte() const = 0;
};

}  // namespace encoding
}  // namespace polo

#endif  // POLO_ENCODING_SECRETENCODER_H_

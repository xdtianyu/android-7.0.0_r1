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

#ifndef POLO_ENCODING_HEXADECIMALENCODER_H_
#define POLO_ENCODING_HEXADECIMALENCODER_H_

#include <string>
#include <vector>

#include "polo/encoding/secretencoder.h"

namespace polo {
namespace encoding {

// Encodes and decodes secret challenges as hexadecimal strings.
class HexadecimalEncoder : public SecretEncoder {
 public:
  // @override
  virtual std::string EncodeToString(
      const std::vector<uint8_t>& secret) const;

  // @override
  virtual std::vector<uint8_t> DecodeToBytes(
      const std::string& secret) const;

  // @override
  virtual size_t symbols_per_byte() const;\
 private:
  enum {
    // Hex encoding has 2 symbols per byte since each hex character uses 4 bits.
    HEX_SYMBOLS_PER_BYTE = 2
  };
};

}  // namespace encoding
}  // namespace polo

#endif  // POLO_ENCODING_HEXADECIMALENCODER_H_

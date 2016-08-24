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

// A SecretEncoder implementation that uses hexadecimal encoding. This encoding
// has 2 symbols per byte since each hex character uses 4 bits.

#include "polo/encoding/hexadecimalencoder.h"

#include <string>
#include <vector>
#include "polo/util/poloutil.h"

namespace polo {
namespace encoding {

std::string HexadecimalEncoder::EncodeToString(
    const std::vector<uint8_t>& secret) const {
  return polo::util::PoloUtil::BytesToHexString(&secret[0], secret.size());
}

std::vector<uint8_t> HexadecimalEncoder::DecodeToBytes(
    const std::string& secret) const {
  uint8_t* bytes;
  size_t length = polo::util::PoloUtil::HexStringToBytes(secret, bytes);
  std::vector<uint8_t> decoded(bytes, bytes + length);
  delete[] bytes;

  return decoded;
}

size_t HexadecimalEncoder::symbols_per_byte() const {
  return HEX_SYMBOLS_PER_BYTE;
}

}  // namespace encoding
}  // namespace polo

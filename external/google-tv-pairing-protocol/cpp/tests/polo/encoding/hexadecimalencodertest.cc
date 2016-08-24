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

#include <gtest/gtest.h>
#include <polo/encoding/hexadecimalencoder.h>

namespace polo {
namespace encoding {

TEST(HexadecimalEncoderTest, EncodeToString) {
  HexadecimalEncoder encoder;

  std::vector<unsigned char> secret(4);
  secret[0] = 0xAA;
  secret[1] = 0xBB;
  secret[2] = 0xCC;
  secret[3] = 0xDD;

  std::string result = encoder.EncodeToString(secret);

  ASSERT_EQ(std::string("AABBCCDD"), result);
}

TEST(HexadecimalEncoderTest, DecodeToBytes) {
  HexadecimalEncoder encoder;

  std::string secret("AABBCCDD");

  std::vector<unsigned char> result = encoder.DecodeToBytes(secret);

  ASSERT_EQ(4, result.size());
  ASSERT_EQ(0xAA, result[0]);
  ASSERT_EQ(0xBB, result[1]);
  ASSERT_EQ(0xCC, result[2]);
  ASSERT_EQ(0xDD, result[3]);
}

}  // namespace encoding
}  // namespace polo

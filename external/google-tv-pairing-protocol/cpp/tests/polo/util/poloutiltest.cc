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
#include <polo/util/poloutil.h>

namespace polo {
namespace util {

TEST(PoloUtilTest, BytesToHexString) {
  uint8_t bytes[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  std::string result = PoloUtil::BytesToHexString(bytes, 4);
  ASSERT_EQ(std::string("AABBCCDD"), result);
}

TEST(PoloUtilTest, BytesToHexStringLeadingNull) {
  uint8_t bytes[4] = {0x00, 0xBB, 0xCC, 0xDD};
  std::string result = PoloUtil::BytesToHexString(bytes, 4);
  ASSERT_EQ(std::string("BBCCDD"), result);
}

TEST(PoloUtilTest, HexStringToBytes) {
  uint8_t* bytes;
  size_t length = PoloUtil::HexStringToBytes(std::string("AABBCCDD"), bytes);
  ASSERT_EQ(4, length);
  ASSERT_EQ(0xAA, bytes[0]);
  ASSERT_EQ(0xBB, bytes[1]);
  ASSERT_EQ(0xCC, bytes[2]);
  ASSERT_EQ(0xDD, bytes[3]);
  delete[] bytes;
}

TEST(PoloUtilTest, IntToBigEndianBytes) {
  uint8_t* bytes;
  PoloUtil::IntToBigEndianBytes(0xAABBCCDD, bytes);
  ASSERT_EQ(0xAA, bytes[0]);
  ASSERT_EQ(0xBB, bytes[1]);
  ASSERT_EQ(0xCC, bytes[2]);
  ASSERT_EQ(0xDD, bytes[3]);
  delete[] bytes;
}

TEST(PoloUtilTest, IntToBigEndianBytes_NullBytes) {
  uint8_t* bytes;
  PoloUtil::IntToBigEndianBytes(0x00AABB00, bytes);
  ASSERT_EQ(0x00, bytes[0]);
  ASSERT_EQ(0xAA, bytes[1]);
  ASSERT_EQ(0xBB, bytes[2]);
  ASSERT_EQ(0x00, bytes[3]);
  delete[] bytes;
}

TEST(PoloUtilTest, BigEndianBytesToInt) {
  uint8_t bytes[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  const uint32_t value = PoloUtil::BigEndianBytesToInt(bytes);
  ASSERT_EQ(0xAABBCCDD, value);
}

TEST(PoloUtilTest, BigEndianBytesToInt_NullBytes) {
  uint8_t bytes[4] = {0x00, 0xAA, 0xBB, 0x00};
  const uint32_t value = PoloUtil::BigEndianBytesToInt(bytes);
  ASSERT_EQ(0x00AABB00, value);
}

TEST(PoloUtilTest, GenerateRandomBytes) {
  uint8_t* random1 = PoloUtil::GenerateRandomBytes(16);
  ASSERT_TRUE(random1);
  const std::string value1 = PoloUtil::BytesToHexString(random1, 16);
  delete[] random1;

  uint8_t* random2 = PoloUtil::GenerateRandomBytes(16);
  ASSERT_TRUE(random2);
  const std::string value2 = PoloUtil::BytesToHexString(random2, 16);
  delete[] random2;

  ASSERT_NE(value1, value2);
}

}  // namespace util
}  // namespace polo

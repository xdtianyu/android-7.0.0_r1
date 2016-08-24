//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <dhcp_client/dhcp_options_parser.h>

#include <netinet/in.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <shill/net/byte_string.h>

using shill::ByteString;

namespace {
const uint8_t kFakeUInt8Option[] = {0x02};
const uint8_t kFakeUInt8OptionLength = 1;
const uint8_t kFakeUInt16Option[] = {0x2a, 0x01};
const uint8_t kFakeUInt16OptionLength = 2;
const uint8_t kFakeUInt32Option[] = {0x01, 0x02, 0x00, 0xfa};
const uint8_t kFakeUInt32OptionLength = 4;
const uint8_t kFakeUInt8ListOption[] =
    {0x01, 0x02, 0x00, 0xfa, 0x23, 0xae, 0x1f, 0x00};
const uint8_t kFakeUInt8ListOptionLength = 8;

const uint8_t kFakeUInt16ListOption[] =
    {0xaa, 0x26, 0x4b, 0x00, 0xff, 0xc2, 0xcf, 0x0d, 0xe0, 0x01};
const uint8_t kFakeUInt16ListOptionLength = 10;

const uint8_t kFakeUInt32ListOption[] = {0x01, 0x02, 0x00, 0xfa,
                                         0x23, 0xae, 0x1f, 0x00,
                                         0x0c, 0x53, 0x33, 0x10,
                                         0x47, 0x80, 0xb3, 0xff};
const uint8_t kFakeUInt32ListOptionLength = 16;

const uint8_t kFakeUInt32PairListOption[] = {0x21, 0xa0, 0xeb, 0x73,
                                             0x01, 0x00, 0x1f, 0x10,
                                             0xc9, 0x22, 0x3a, 0x37,
                                             0xff, 0x00, 0xbe, 0xd0};
const uint8_t kFakeUInt32PairListOptionLength = 16;

const unsigned char kFakeStringOption[] =
    {'f', 'a', 'k', 'e', 's', 't', 'r', 'i', 'n', 'g'};
const uint8_t kFakeStringOptionLength = 10;

const unsigned char kFakeByteArrayOption[] =
    {'f', 'a', 'k', 'e', 'b', 'y', 't', 'e', 'a', 'r', 'r', 'a', 'y'};

const uint8_t kFakeBoolOptionEnable[] = {0x01};
const uint8_t kFakeBoolOptionDisable[] = {0x00};
const uint8_t kFakeBoolOptionLength = 1;
}  // namespace

namespace dhcp_client {

class ParserTest : public testing::Test {
 protected:
  std::unique_ptr<DHCPOptionsParser> parser_;
};

TEST_F(ParserTest, ParseUInt8) {
  parser_.reset(new UInt8Parser());
  uint8_t value;
  EXPECT_TRUE(parser_->GetOption(kFakeUInt8Option,
                                 kFakeUInt8OptionLength,
                                 &value));
  EXPECT_EQ(*kFakeUInt8Option, value);
}

TEST_F(ParserTest, ParseUInt16) {
  parser_.reset(new UInt16Parser());
  uint16_t value;
  uint16_t target_value =
      *reinterpret_cast<const uint16_t*>(kFakeUInt16Option);
  target_value = ntohs(target_value);
  EXPECT_TRUE(parser_->GetOption(kFakeUInt16Option,
                                 kFakeUInt16OptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseUInt32) {
  parser_.reset(new UInt32Parser());
  uint32_t value;
  uint32_t target_value =
      *reinterpret_cast<const uint32_t*>(kFakeUInt32Option);
  target_value = ntohl(target_value);
  EXPECT_TRUE(parser_->GetOption(kFakeUInt32Option,
                                 kFakeUInt32OptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseUInt8List) {
  parser_.reset(new UInt8ListParser());
  std::vector<uint8_t> value;
  std::vector<uint8_t> target_value;
  uint8_t length = kFakeUInt8ListOptionLength;
  const uint8_t* uint8_list =
      reinterpret_cast<const uint8_t*>(kFakeUInt8ListOption);
  target_value = std::vector<uint8_t>(uint8_list, uint8_list + length);
  EXPECT_TRUE(parser_->GetOption(kFakeUInt8ListOption,
                                 kFakeUInt8ListOptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseUInt16List) {
  parser_.reset(new UInt16ListParser());
  std::vector<uint16_t> value;
  std::vector<uint16_t> target_value;
  std::vector<uint16_t> target_value_net_order;
  int length = kFakeUInt16ListOptionLength / sizeof(uint16_t);
  const uint16_t* uint16_list =
      reinterpret_cast<const uint16_t*>(kFakeUInt16ListOption);
  target_value_net_order =
      std::vector<uint16_t>(uint16_list, uint16_list + length);
  for (uint16_t element : target_value_net_order) {
    target_value.push_back(ntohs(element));
  }
  EXPECT_TRUE(parser_->GetOption(kFakeUInt16ListOption,
                                 kFakeUInt16ListOptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseUInt32List) {
  parser_.reset(new UInt32ListParser());
  std::vector<uint32_t> value;
  std::vector<uint32_t> target_value;
  std::vector<uint32_t> target_value_net_order;
  int length = kFakeUInt32ListOptionLength / sizeof(uint32_t);
  const uint32_t* uint32_list =
      reinterpret_cast<const uint32_t*>(kFakeUInt32ListOption);
  target_value_net_order =
      std::vector<uint32_t>(uint32_list, uint32_list + length);
  for (uint32_t element : target_value_net_order) {
    target_value.push_back(ntohl(element));
  }
  EXPECT_TRUE(parser_->GetOption(kFakeUInt32ListOption,
                                 kFakeUInt32ListOptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseUInt32PairList) {
  parser_.reset(new UInt32PairListParser());
  int length = kFakeUInt32PairListOptionLength / (2 * sizeof(uint32_t));
  const uint32_t* uint32_array =
      reinterpret_cast<const uint32_t*>(kFakeUInt32PairListOption);
  std::vector<uint32_t> uint32_vector =
      std::vector<uint32_t>(uint32_array, uint32_array + length * 2);
  std::vector<std::pair<uint32_t, uint32_t>> target_value;
  for (int i = 0; i < length; i++) {
    target_value.push_back(
        std::pair<uint32_t, uint32_t>(ntohl(uint32_vector[2 * i]),
                                      ntohl(uint32_vector[2 * i + 1])));
  }
  std::vector<std::pair<uint32_t, uint32_t>> value;
  EXPECT_TRUE(parser_->GetOption(kFakeUInt32PairListOption,
                                 kFakeUInt32PairListOptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseBoolEnable) {
  parser_.reset(new BoolParser());
  bool value;
  EXPECT_TRUE(parser_->GetOption(kFakeBoolOptionEnable,
                                 kFakeBoolOptionLength,
                                 &value));
  EXPECT_TRUE(value);
}

TEST_F(ParserTest, ParseBoolDisable) {
  parser_.reset(new BoolParser());
  bool value;
  EXPECT_TRUE(parser_->GetOption(kFakeBoolOptionDisable,
                                 kFakeBoolOptionLength,
                                 &value));
  EXPECT_FALSE(value);
}

TEST_F(ParserTest, ParseString) {
  parser_.reset(new StringParser());
  std::string value;
  std::string target_value;
  target_value.assign(reinterpret_cast<const char*>(kFakeStringOption),
                      kFakeStringOptionLength);
  EXPECT_TRUE(parser_->GetOption(kFakeStringOption,
                                 kFakeStringOptionLength,
                                 &value));
  EXPECT_EQ(target_value, value);
}

TEST_F(ParserTest, ParseByteArray) {
  parser_.reset(new ByteArrayParser());
  ByteString value;
  ByteString target_value(reinterpret_cast<const char*>(kFakeByteArrayOption),
                          sizeof(kFakeByteArrayOption));
  EXPECT_TRUE(parser_->GetOption(kFakeByteArrayOption,
                                 sizeof(kFakeByteArrayOption),
                                 &value));
  EXPECT_TRUE(target_value.Equals(value));
}

}  // namespace dhcp_client

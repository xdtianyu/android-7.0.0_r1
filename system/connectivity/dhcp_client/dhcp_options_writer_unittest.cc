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

#include <dhcp_client/dhcp_options_writer.h>

#include <netinet/in.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <shill/net/byte_string.h>

#include "dhcp_client/dhcp_options.h"

using shill::ByteString;
namespace dhcp_client {

namespace {
const uint8_t kFakeOptionCode1 = 3;
const uint8_t kFakeOptionCode2 = 45;
const uint8_t kFakeOptionCode3 = 251;
}  // namespace


class DHCPOptionsWriterTest : public testing::Test {
 protected:
  DHCPOptionsWriter* options_writer_;
};

TEST_F(DHCPOptionsWriterTest, WriteUInt8) {
  const uint8_t kFakeUInt8Option = 0x22;
  const uint8_t kFakeUInt8OptionResult[] = {
      kFakeOptionCode1,
      sizeof(uint8_t),
      0x22};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteUInt8Option(&option,
                                                 kFakeOptionCode1,
                                                 kFakeUInt8Option);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt8OptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteUInt16) {
  const uint16_t kFakeUInt16Option = 0x1516;
  const uint8_t kFakeUInt16OptionResult[] = {
      kFakeOptionCode2,
      sizeof(uint16_t),
      // Use the network byte order.
      0x15, 0x16};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteUInt16Option(&option,
                                                  kFakeOptionCode2,
                                                  kFakeUInt16Option);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt16OptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteUInt32) {
  const uint32_t kFakeUInt32Option = 0x32a0bf01;
  const uint8_t kFakeUInt32OptionResult[] = {
      kFakeOptionCode3,
      sizeof(uint32_t),
      // Use the network byte order.
      0x32, 0xa0, 0xbf, 0x01};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteUInt32Option(&option,
                                                  kFakeOptionCode3,
                                                  kFakeUInt32Option);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt32OptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteUInt8List) {
  const std::vector<uint8_t> kFakeUInt8ListOption =
      {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  const uint8_t kFakeUInt8ListOptionResult[] = {
      kFakeOptionCode1,
      static_cast<uint8_t>(kFakeUInt8ListOption.size()),
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteUInt8ListOption(&option,
                                                     kFakeOptionCode1,
                                                     kFakeUInt8ListOption);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt8ListOptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteUInt16List) {
  const std::vector<uint16_t> kFakeUInt16ListOption =
      {0xb1a2, 0x0264, 0xdc03, 0x92c4, 0xa500, 0x0010};
  const uint8_t kFakeUInt16ListOptionResult[] = {
      kFakeOptionCode2,
      static_cast<uint8_t>(sizeof(uint16_t) * kFakeUInt16ListOption.size()),
      // Use the network byte order.
      0xb1, 0xa2, 0x02, 0x64, 0xdc, 0x03, 0x92, 0xc4, 0xa5, 0x00, 0x00, 0x10};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteUInt16ListOption(&option,
                                                      kFakeOptionCode2,
                                                      kFakeUInt16ListOption);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt16ListOptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteUInt32List) {
  const std::vector<uint32_t> kFakeUInt32ListOption =
      {0x03a64301, 0x03f52614, 0x7c5d9eff, 0x0138b26e};
  const uint8_t kFakeUInt32ListOptionResult[] = {
      kFakeOptionCode3,
      static_cast<uint8_t>(sizeof(uint32_t) * kFakeUInt32ListOption.size()),
      // Use the network byte order.
      0x03, 0xa6, 0x43, 0x01, 0x03, 0xf5, 0x26, 0x14,
      0x7c, 0x5d, 0x9e, 0xff, 0x01, 0x38, 0xb2, 0x6e};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteUInt32ListOption(&option,
                                                      kFakeOptionCode3,
                                                      kFakeUInt32ListOption);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt32ListOptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteUInt32PairList) {
  const std::vector<std::pair<uint32_t, uint32_t>> kFakeUInt32PairListOption =
      {{0x03b576a1, 0xfa070054}, {0x650c3d22, 0x1397e5bb}};
  const uint8_t kFakeUInt32PairListOptionResult[] = {
      kFakeOptionCode1,
      static_cast<uint8_t>
          (sizeof(uint32_t) * 2 * kFakeUInt32PairListOption.size()),
      // Use the network byte order.
      0x03, 0xb5, 0x76, 0xa1, 0xfa, 0x07, 0x00, 0x54,
      0x65, 0x0c, 0x3d, 0x22, 0x13, 0x97, 0xe5, 0xbb};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length =
      options_writer_->WriteUInt32PairListOption(&option,
                                                 kFakeOptionCode1,
                                                 kFakeUInt32PairListOption);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeUInt32PairListOptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteString) {
  const std::string kFakeStringOption = "fakestring";
  const uint8_t kFakeStringOptionResult[] = {
      kFakeOptionCode1,
      static_cast<uint8_t>(kFakeStringOption.size()),
      'f', 'a', 'k', 'e', 's', 't', 'r', 'i', 'n', 'g'};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteStringOption(&option,
                                                  kFakeOptionCode1,
                                                  kFakeStringOption);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeStringOptionResult,
                           length));
}


TEST_F(DHCPOptionsWriterTest, WriteBoolTrue) {
  const uint8_t kFakeBoolOptionTrue = true;
  const uint8_t kFakeBoolOptionResultTrue[] = {
    kFakeOptionCode1,
    sizeof(uint8_t),
    0x01};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteBoolOption(&option,
                                                kFakeOptionCode1,
                                                kFakeBoolOptionTrue);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeBoolOptionResultTrue,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteBoolFalse) {
  const uint8_t kFakeBoolOptionFalse = false;
  const uint8_t kFakeBoolOptionResultFalse[] = {
      kFakeOptionCode2,
      sizeof(uint8_t),
      0x00};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteBoolOption(&option,
                                                kFakeOptionCode2,
                                                kFakeBoolOptionFalse);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeBoolOptionResultFalse,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteByteArray) {
  const ByteString kFakeByteArrayOption =
      ByteString({0x06, 0x05, 0x04, 0x03, 0x02, 0x01});
  const uint8_t kFakeByteArrayOptionResult[] = {
      kFakeOptionCode1,
      static_cast<uint8_t>(kFakeByteArrayOption.GetLength()),
      0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteByteArrayOption(&option,
                                                     kFakeOptionCode1,
                                                     kFakeByteArrayOption);
  EXPECT_NE(-1, length);
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeByteArrayOptionResult,
                           length));
}

TEST_F(DHCPOptionsWriterTest, WriteEndTag) {
  const std::string kFakeStringOption = "fakestring1";
  const uint8_t kFakeStringOptionResult[] = {
      kFakeOptionCode1,
      static_cast<uint8_t>(kFakeStringOption.size()),
      'f', 'a', 'k', 'e', 's', 't', 'r', 'i', 'n', 'g', '1'};

  ByteString option;
  options_writer_ = DHCPOptionsWriter::GetInstance();
  int length = options_writer_->WriteStringOption(&option,
                                                  kFakeOptionCode1,
                                                  kFakeStringOption);
  EXPECT_NE(-1, length);
  EXPECT_NE(-1, options_writer_->WriteEndTag(&option));
  EXPECT_EQ(0, std::memcmp(option.GetConstData(),
                           kFakeStringOptionResult,
                           length));
  EXPECT_EQ(kDHCPOptionEnd, *(option.GetConstData() + length));
}

}  // namespace dhcp_client

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

// This file tests some public interface methods of AttributeList.
#include "shill/net/attribute_list.h"

#include <linux/netlink.h>

#include <string>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/net/byte_string.h"

using testing::_;
using testing::InSequence;
using testing::Mock;
using testing::Return;
using testing::Test;

namespace shill {

class AttributeListTest : public Test {
 public:
  MOCK_METHOD2(AttributeMethod, bool(int id, const ByteString& value));

 protected:
  static const uint16_t kHeaderLength = 4;
  static const uint16_t kType1 = 1;
  static const uint16_t kType2 = 2;
  static const uint16_t kType3 = 3;

  static ByteString MakeNetlinkAttribute(uint16_t len,
                                         uint16_t type,
                                         const std::string& payload) {
    nlattr attribute{ len, type };
    ByteString data(reinterpret_cast<const char*>(&attribute),
                    sizeof(attribute));
    data.Append(ByteString(payload, false));
    return data;
  }

  static ByteString MakePaddedNetlinkAttribute(uint16_t len,
                                               uint16_t type,
                                               const std::string& payload) {
    ByteString data(MakeNetlinkAttribute(len, type, payload));
    ByteString padding(NLA_ALIGN(data.GetLength()) - data.GetLength());
    data.Append(padding);
    return data;
  }
};

MATCHER_P(PayloadIs, payload, "") {
  return arg.Equals(ByteString(std::string(payload), false));
}

TEST_F(AttributeListTest, IterateEmptyPayload) {
  EXPECT_CALL(*this, AttributeMethod(_, _)).Times(0);
  AttributeListRefPtr list(new AttributeList());
  EXPECT_TRUE(list->IterateAttributes(
      ByteString(), 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
}

TEST_F(AttributeListTest, IteratePayload) {
  ByteString payload;
  payload.Append(MakePaddedNetlinkAttribute(
      kHeaderLength + 10, kType1, "0123456789"));
  const uint16_t kLength1 = kHeaderLength + 10 + 2;  // 2 bytes padding.
  ASSERT_EQ(kLength1, payload.GetLength());
  payload.Append(MakePaddedNetlinkAttribute(kHeaderLength + 3, kType2, "123"));
  const uint16_t kLength2 = kLength1 + kHeaderLength + 3 + 1;  // 1 byte pad.
  ASSERT_EQ(kLength2, payload.GetLength());

  payload.Append(MakeNetlinkAttribute(kHeaderLength + 5, kType3, "12345"));
  const uint16_t kLength3 = kLength2 + kHeaderLength + 5;
  ASSERT_EQ(kLength3, payload.GetLength());

  InSequence seq;
  EXPECT_CALL(*this, AttributeMethod(kType1, PayloadIs("0123456789")))
      .WillOnce(Return(true));
  EXPECT_CALL(*this, AttributeMethod(kType2, PayloadIs("123")))
      .WillOnce(Return(true));
  EXPECT_CALL(*this, AttributeMethod(kType3, PayloadIs("12345")))
      .WillOnce(Return(true));
  AttributeListRefPtr list(new AttributeList());
  EXPECT_TRUE(list->IterateAttributes(
      payload, 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // If a valid offset is provided only the attributes that follow should
  // be enumerated.
  EXPECT_CALL(*this, AttributeMethod(kType1, _)).Times(0);
  EXPECT_CALL(*this, AttributeMethod(kType2, PayloadIs("123")))
      .WillOnce(Return(true));
  EXPECT_CALL(*this, AttributeMethod(kType3, PayloadIs("12345")))
      .WillOnce(Return(true));
  EXPECT_TRUE(list->IterateAttributes(
      payload, kLength1,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // If one of the attribute methods returns false, the iteration should abort.
  EXPECT_CALL(*this, AttributeMethod(kType1, PayloadIs("0123456789")))
      .WillOnce(Return(true));
  EXPECT_CALL(*this, AttributeMethod(kType2, PayloadIs("123")))
      .WillOnce(Return(false));
  EXPECT_CALL(*this, AttributeMethod(kType3, PayloadIs("12345"))).Times(0);
  EXPECT_FALSE(list->IterateAttributes(
      payload, 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);
}

TEST_F(AttributeListTest, SmallPayloads) {
  // A payload must be at least 4 bytes long to incorporate the nlattr header.
  EXPECT_CALL(*this, AttributeMethod(_, _)).Times(0);
  AttributeListRefPtr list(new AttributeList());
  EXPECT_FALSE(list->IterateAttributes(
      MakeNetlinkAttribute(kHeaderLength - 1, kType1, "0123"), 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // This is a minimal valid payload.
  EXPECT_CALL(*this, AttributeMethod(
      kType2, PayloadIs(""))).WillOnce(Return(true));
  EXPECT_TRUE(list->IterateAttributes(
      MakeNetlinkAttribute(kHeaderLength, kType2, ""), 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // This is a minmal payload except there are not enough bytes to retrieve
  // the attribute value.
  const uint16_t kType3 = 1;
  EXPECT_CALL(*this, AttributeMethod(_, _)).Times(0);
  EXPECT_FALSE(list->IterateAttributes(
      MakeNetlinkAttribute(kHeaderLength + 1, kType3, ""), 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
}

TEST_F(AttributeListTest, TrailingGarbage) {
  // +---------+
  // | Attr #1 |
  // +-+-+-+-+-+
  // |LEN|TYP|0|
  // +-+-+-+-+-+
  // Well formed frame.
  ByteString payload(MakeNetlinkAttribute(kHeaderLength + 1, kType1, "0"));
  EXPECT_CALL(*this, AttributeMethod(kType1, PayloadIs("0")))
      .WillOnce(Return(true));
  AttributeListRefPtr list(new AttributeList());
  EXPECT_TRUE(list->IterateAttributes(
      payload, 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // +---------------+
  // | Attr #1 + pad |
  // +-+-+-+-+-+-+-+-+
  // |LEN|TYP|0|1|2|3|
  // +-+-+-+-+-+-+-+-+
  // "123" assumed to be padding for attr1.
  payload.Append(ByteString(std::string("123"), false));
  EXPECT_CALL(*this, AttributeMethod(kType1, PayloadIs("0")))
      .WillOnce(Return(true));
  EXPECT_TRUE(list->IterateAttributes(
      payload, 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // +---------------+-----+
  // | Attr #1 + pad |RUNT |
  // +-+-+-+-+-+-+-+-+-+-+-+
  // |LEN|TYP|0|1|2|3|4|5|6|
  // +-+-+-+-+-+-+-+-+-+-+-+
  // "456" is acceptable since it is not long enough to complete an netlink
  // attribute header.
  payload.Append(ByteString(std::string("456"), false));
  EXPECT_CALL(*this, AttributeMethod(kType1, PayloadIs("0")))
      .WillOnce(Return(true));
  EXPECT_TRUE(list->IterateAttributes(
      payload, 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);

  // +---------------+-------+
  // | Attr #1 + pad |Broken |
  // +-+-+-+-+-+-+-+-+-+-+-+-+
  // |LEN|TYP|0|1|2|3|4|5|6|7|
  // +-+-+-+-+-+-+-+-+-+-+-+-+
  // This is a broken frame, since '4567' can be interpreted as a complete
  // nlatter header, but is malformed since there is not enough payload to
  // satisfy the "length" parameter.
  payload.Append(ByteString(std::string("7"), false));
  EXPECT_CALL(*this, AttributeMethod(kType1, PayloadIs("0")))
      .WillOnce(Return(true));
  EXPECT_FALSE(list->IterateAttributes(
      payload, 0,
      base::Bind(&AttributeListTest::AttributeMethod, base::Unretained(this))));
  Mock::VerifyAndClearExpectations(this);
}

}  // namespace shill

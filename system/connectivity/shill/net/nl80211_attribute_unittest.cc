// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shill/net/nl80211_attribute.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/net/byte_string.h"

using testing::Test;

namespace shill {

class Nl80211AttributeTest : public Test {
};

TEST_F(Nl80211AttributeTest, RegInitiatorDecode) {
  Nl80211AttributeRegInitiator empty_attribute;
  EXPECT_FALSE(empty_attribute.InitFromValue(ByteString()));

  const uint8_t kU8Value = 123;
  ByteString u8_value(&kU8Value, 1);
  Nl80211AttributeRegInitiator u8_attribute;
  EXPECT_TRUE(u8_attribute.InitFromValue(u8_value));
  uint32_t value_from_u8_attribute;
  EXPECT_TRUE(u8_attribute.GetU32Value(&value_from_u8_attribute));
  EXPECT_EQ(kU8Value, value_from_u8_attribute);

  const uint32_t kU32Value = 123456790U;
  ByteString u32_value = ByteString::CreateFromCPUUInt32(kU32Value);
  Nl80211AttributeRegInitiator u32_attribute;
  EXPECT_TRUE(u32_attribute.InitFromValue(u32_value));
  uint32_t value_from_u32_attribute;
  EXPECT_TRUE(u32_attribute.GetU32Value(&value_from_u32_attribute));
  EXPECT_EQ(kU32Value, value_from_u32_attribute);
}

}  // namespace shill

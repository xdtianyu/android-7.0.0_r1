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

// This file provides tests for individual messages.  It tests
// NetlinkMessageFactory's ability to create specific message types and it
// tests the various NetlinkMessage types' ability to parse those
// messages.

// This file tests some public interface methods of NetlinkAttribute subclasses.
#include "shill/net/netlink_attribute.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/net/byte_string.h"

using testing::Test;

namespace shill {

class NetlinkAttributeTest : public Test { };

TEST_F(NetlinkAttributeTest, StringAttribute) {
  NetlinkStringAttribute attr(0, "string id");

  // An empty ByteString should yield an empty string.
  EXPECT_TRUE(attr.InitFromValue(ByteString()));
  std::string value;
  EXPECT_TRUE(attr.GetStringValue(&value));
  EXPECT_EQ("", value);

  // An un-terminated ByteString should yield a terminated string.
  ByteString unterminated(std::string("hello"), false);
  EXPECT_EQ(5, unterminated.GetLength());
  EXPECT_TRUE(attr.InitFromValue(unterminated));
  EXPECT_TRUE(attr.GetStringValue(&value));
  EXPECT_EQ("hello", value);
  EXPECT_EQ(5, value.size());

  // A terminated ByteString should also work correctly.
  ByteString terminated(std::string("hello"), true);
  EXPECT_EQ(6, terminated.GetLength());
  EXPECT_TRUE(attr.InitFromValue(terminated));
  EXPECT_TRUE(attr.GetStringValue(&value));
  EXPECT_EQ("hello", value);
  EXPECT_EQ(5, value.size());

  // Extra data after termination should be removed.
  terminated.Append(ByteString(3));
  EXPECT_EQ(9, terminated.GetLength());
  EXPECT_TRUE(attr.InitFromValue(terminated));
  EXPECT_TRUE(attr.GetStringValue(&value));
  EXPECT_EQ("hello", value);
  EXPECT_EQ(5, value.size());
}

}  // namespace shill

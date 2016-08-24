// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/strings/string_utils.h>

#include <list>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace brillo {

TEST(StringUtils, Split) {
  std::vector<std::string> parts;

  parts = string_utils::Split("", ",", false, false);
  EXPECT_EQ(0, parts.size());

  parts = string_utils::Split("abc", ",", false, false);
  EXPECT_EQ(1, parts.size());
  EXPECT_EQ("abc", parts[0]);

  parts = string_utils::Split(",a,bc , d,  ,e, ", ",", true, true);
  EXPECT_EQ(4, parts.size());
  EXPECT_EQ("a", parts[0]);
  EXPECT_EQ("bc", parts[1]);
  EXPECT_EQ("d", parts[2]);
  EXPECT_EQ("e", parts[3]);

  parts = string_utils::Split(",a,bc , d,  ,e, ", ",", false, true);
  EXPECT_EQ(6, parts.size());
  EXPECT_EQ("a", parts[0]);
  EXPECT_EQ("bc ", parts[1]);
  EXPECT_EQ(" d", parts[2]);
  EXPECT_EQ("  ", parts[3]);
  EXPECT_EQ("e", parts[4]);
  EXPECT_EQ(" ", parts[5]);

  parts = string_utils::Split(",a,bc , d,  ,e, ", ",", true, false);
  EXPECT_EQ(7, parts.size());
  EXPECT_EQ("", parts[0]);
  EXPECT_EQ("a", parts[1]);
  EXPECT_EQ("bc", parts[2]);
  EXPECT_EQ("d", parts[3]);
  EXPECT_EQ("", parts[4]);
  EXPECT_EQ("e", parts[5]);
  EXPECT_EQ("", parts[6]);

  parts = string_utils::Split(",a,bc , d,  ,e, ", ",", false, false);
  EXPECT_EQ(7, parts.size());
  EXPECT_EQ("", parts[0]);
  EXPECT_EQ("a", parts[1]);
  EXPECT_EQ("bc ", parts[2]);
  EXPECT_EQ(" d", parts[3]);
  EXPECT_EQ("  ", parts[4]);
  EXPECT_EQ("e", parts[5]);
  EXPECT_EQ(" ", parts[6]);

  parts = string_utils::Split("abc:=xyz", ":=", false, false);
  EXPECT_EQ(2, parts.size());
  EXPECT_EQ("abc", parts[0]);
  EXPECT_EQ("xyz", parts[1]);

  parts = string_utils::Split("abc", "", false, false);
  EXPECT_EQ(3, parts.size());
  EXPECT_EQ("a", parts[0]);
  EXPECT_EQ("b", parts[1]);
  EXPECT_EQ("c", parts[2]);
}

TEST(StringUtils, SplitAtFirst) {
  std::pair<std::string, std::string> pair;

  pair = string_utils::SplitAtFirst(" 123 : 4 : 56 : 789 ", ":", true);
  EXPECT_EQ("123", pair.first);
  EXPECT_EQ("4 : 56 : 789", pair.second);

  pair = string_utils::SplitAtFirst(" 123 : 4 : 56 : 789 ", ":", false);
  EXPECT_EQ(" 123 ", pair.first);
  EXPECT_EQ(" 4 : 56 : 789 ", pair.second);

  pair = string_utils::SplitAtFirst("", "=");
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("", pair.second);

  pair = string_utils::SplitAtFirst("=", "=");
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("", pair.second);

  pair = string_utils::SplitAtFirst("a=", "=");
  EXPECT_EQ("a", pair.first);
  EXPECT_EQ("", pair.second);

  pair = string_utils::SplitAtFirst("abc=", "=");
  EXPECT_EQ("abc", pair.first);
  EXPECT_EQ("", pair.second);

  pair = string_utils::SplitAtFirst("=a", "=");
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("a", pair.second);

  pair = string_utils::SplitAtFirst("=abc=", "=");
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("abc=", pair.second);

  pair = string_utils::SplitAtFirst("abc", "=");
  EXPECT_EQ("abc", pair.first);
  EXPECT_EQ("", pair.second);

  pair = string_utils::SplitAtFirst("abc:=xyz", ":=");
  EXPECT_EQ("abc", pair.first);
  EXPECT_EQ("xyz", pair.second);

  pair = string_utils::SplitAtFirst("abc", "");
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("abc", pair.second);
}

TEST(StringUtils, Join_String) {
  EXPECT_EQ("", string_utils::Join(",", {}));
  EXPECT_EQ("abc", string_utils::Join(",", {"abc"}));
  EXPECT_EQ("abc,,xyz", string_utils::Join(",", {"abc", "", "xyz"}));
  EXPECT_EQ("abc,defg", string_utils::Join(",", {"abc", "defg"}));
  EXPECT_EQ("1 : 2 : 3", string_utils::Join(" : ", {"1", "2", "3"}));
  EXPECT_EQ("1:2", string_utils::Join(":", std::set<std::string>{"1", "2"}));
  EXPECT_EQ("1:2", string_utils::Join(":", std::vector<std::string>{"1", "2"}));
  EXPECT_EQ("1:2", string_utils::Join(":", std::list<std::string>{"1", "2"}));
  EXPECT_EQ("123", string_utils::Join("", {"1", "2", "3"}));
}

TEST(StringUtils, Join_Pair) {
  EXPECT_EQ("ab,cd", string_utils::Join(",", "ab", "cd"));
  EXPECT_EQ("key = value", string_utils::Join(" = ", "key", "value"));
}

TEST(StringUtils, GetBytesAsString) {
  EXPECT_EQ("abc", string_utils::GetBytesAsString({'a', 'b', 'c'}));
  EXPECT_TRUE(string_utils::GetBytesAsString({}).empty());
  auto str = string_utils::GetBytesAsString({0xFF, 0x00, 0x01, 0x7F, 0x80});
  ASSERT_EQ(5, str.size());
  EXPECT_EQ('\xFF', str[0]);
  EXPECT_EQ('\x00', str[1]);
  EXPECT_EQ('\x01', str[2]);
  EXPECT_EQ('\x7F', str[3]);
  EXPECT_EQ('\x80', str[4]);
}

TEST(StringUtils, GetStringAsBytes) {
  EXPECT_EQ((std::vector<uint8_t>{'a', 'b', 'c'}),
            string_utils::GetStringAsBytes("abc"));
  EXPECT_TRUE(string_utils::GetStringAsBytes("").empty());
  auto buf = string_utils::GetStringAsBytes(std::string{"\x80\0\1\xFF", 4});
  ASSERT_EQ(4, buf.size());
  EXPECT_EQ(128, buf[0]);
  EXPECT_EQ(0, buf[1]);
  EXPECT_EQ(1, buf[2]);
  EXPECT_EQ(255, buf[3]);
}

}  // namespace brillo

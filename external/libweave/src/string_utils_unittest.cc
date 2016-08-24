// Copyright (c) 2014 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/string_utils.h"

#include <list>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace weave {

TEST(StringUtils, Split) {
  std::vector<std::string> parts;

  parts = Split("", ",", false, false);
  EXPECT_EQ(1u, parts.size());
  EXPECT_EQ("", parts[0]);

  parts = Split("", ",", false, true);
  EXPECT_EQ(0u, parts.size());

  parts = Split("abc", ",", false, false);
  EXPECT_EQ(1u, parts.size());
  EXPECT_EQ("abc", parts[0]);

  parts = Split(",a,bc , d,  ,e, ", ",", true, true);
  EXPECT_EQ(4u, parts.size());
  EXPECT_EQ("a", parts[0]);
  EXPECT_EQ("bc", parts[1]);
  EXPECT_EQ("d", parts[2]);
  EXPECT_EQ("e", parts[3]);

  parts = Split(",a,bc , d,  ,e, ", ",", false, true);
  EXPECT_EQ(6u, parts.size());
  EXPECT_EQ("a", parts[0]);
  EXPECT_EQ("bc ", parts[1]);
  EXPECT_EQ(" d", parts[2]);
  EXPECT_EQ("  ", parts[3]);
  EXPECT_EQ("e", parts[4]);
  EXPECT_EQ(" ", parts[5]);

  parts = Split(",a,bc , d,  ,e, ", ",", true, false);
  EXPECT_EQ(7u, parts.size());
  EXPECT_EQ("", parts[0]);
  EXPECT_EQ("a", parts[1]);
  EXPECT_EQ("bc", parts[2]);
  EXPECT_EQ("d", parts[3]);
  EXPECT_EQ("", parts[4]);
  EXPECT_EQ("e", parts[5]);
  EXPECT_EQ("", parts[6]);

  parts = Split(",a,bc , d,  ,e, ", ",", false, false);
  EXPECT_EQ(7u, parts.size());
  EXPECT_EQ("", parts[0]);
  EXPECT_EQ("a", parts[1]);
  EXPECT_EQ("bc ", parts[2]);
  EXPECT_EQ(" d", parts[3]);
  EXPECT_EQ("  ", parts[4]);
  EXPECT_EQ("e", parts[5]);
  EXPECT_EQ(" ", parts[6]);

  parts = Split("abc:=xyz", ":=", false, false);
  EXPECT_EQ(2u, parts.size());
  EXPECT_EQ("abc", parts[0]);
  EXPECT_EQ("xyz", parts[1]);

  parts = Split("abc", "", false, false);
  EXPECT_EQ(3u, parts.size());
  EXPECT_EQ("a", parts[0]);
  EXPECT_EQ("b", parts[1]);
  EXPECT_EQ("c", parts[2]);
}

TEST(StringUtils, SplitAtFirst) {
  std::pair<std::string, std::string> pair;

  pair = SplitAtFirst(" 123 : 4 : 56 : 789 ", ":", true);
  EXPECT_EQ("123", pair.first);
  EXPECT_EQ("4 : 56 : 789", pair.second);

  pair = SplitAtFirst(" 123 : 4 : 56 : 789 ", ":", false);
  EXPECT_EQ(" 123 ", pair.first);
  EXPECT_EQ(" 4 : 56 : 789 ", pair.second);

  pair = SplitAtFirst("", "=", true);
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("", pair.second);

  pair = SplitAtFirst("=", "=", true);
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("", pair.second);

  pair = SplitAtFirst("a=", "=", true);
  EXPECT_EQ("a", pair.first);
  EXPECT_EQ("", pair.second);

  pair = SplitAtFirst("abc=", "=", true);
  EXPECT_EQ("abc", pair.first);
  EXPECT_EQ("", pair.second);

  pair = SplitAtFirst("=a", "=", true);
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("a", pair.second);

  pair = SplitAtFirst("=abc=", "=", true);
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("abc=", pair.second);

  pair = SplitAtFirst("abc", "=", true);
  EXPECT_EQ("abc", pair.first);
  EXPECT_EQ("", pair.second);

  pair = SplitAtFirst("abc:=xyz", ":=", true);
  EXPECT_EQ("abc", pair.first);
  EXPECT_EQ("xyz", pair.second);

  pair = SplitAtFirst("abc", "", true);
  EXPECT_EQ("", pair.first);
  EXPECT_EQ("abc", pair.second);
}

TEST(StringUtils, Join_String) {
  EXPECT_EQ("", Join(",", std::vector<std::string>{}));
  EXPECT_EQ("abc", Join(",", std::vector<std::string>{"abc"}));
  EXPECT_EQ("abc,,xyz", Join(",", std::vector<std::string>{"abc", "", "xyz"}));
  EXPECT_EQ("abc,defg", Join(",", std::vector<std::string>{"abc", "defg"}));
  EXPECT_EQ("1 : 2 : 3", Join(" : ", std::vector<std::string>{"1", "2", "3"}));
  EXPECT_EQ("1:2", Join(":", std::set<std::string>{"1", "2"}));
  EXPECT_EQ("1:2", Join(":", std::vector<std::string>{"1", "2"}));
  EXPECT_EQ("1:2", Join(":", std::list<std::string>{"1", "2"}));
  EXPECT_EQ("123", Join("", std::vector<std::string>{"1", "2", "3"}));
}

TEST(StringUtils, Join_Pair) {
  EXPECT_EQ("ab,cd", Join(",", "ab", "cd"));
  EXPECT_EQ("key = value", Join(" = ", "key", "value"));
}

}  // namespace weave

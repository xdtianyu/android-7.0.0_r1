// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extents.h"

#include <gtest/gtest.h>
#include <vector>

namespace bsdiff {

// ex_t comparator used for testing.
bool operator==(const struct ex_t& lhs, const struct ex_t& rhs) {
  return lhs.off == rhs.off && lhs.len == rhs.len;
}

// PrintTo is used by gtest framework whenever it needs to print our type.
void PrintTo(const struct ex_t& extent, ::std::ostream* os) {
  *os << extent.off << ":" << extent.len;
}

class ExtentsTest : public testing::Test {
 protected:
  std::vector<ex_t> extents_;
};

TEST_F(ExtentsTest, CornerCasesHandledTest) {
  EXPECT_TRUE(ParseExtentStr("", &extents_));
  EXPECT_TRUE(extents_.empty());
}

TEST_F(ExtentsTest, SimpleCasesTest) {
  EXPECT_TRUE(ParseExtentStr("10:20,30:40", &extents_));
  std::vector<ex_t> expected_values = {{10, 20}, {30, 40}};
  EXPECT_EQ(expected_values, extents_);
}

TEST_F(ExtentsTest, MalformedExtentsTest) {
  std::vector<const char*> test_cases = {
      ":", ",", "1,2", "1:", "1,", ":2", ",2", "1,2:3", "10:-1", "-2:10"};
  for (const char* test_case : test_cases) {
    std::vector<ex_t> extents;
    EXPECT_FALSE(ParseExtentStr(test_case, &extents)) << "while testing case \""
                                                      << test_case << "\"";
    EXPECT_EQ(std::vector<ex_t>(), extents);
  }
}

TEST_F(ExtentsTest, NegativeValuesTest) {
  // |-1| is used as a special case to read zeros for that extent.
  EXPECT_TRUE(ParseExtentStr("10:20,-1:40,50:60", &extents_));
  std::vector<ex_t> expected_values = {{10, 20}, {-1, 40}, {50, 60}};
  EXPECT_EQ(expected_values, extents_);
}

}  // namespace bsdiff

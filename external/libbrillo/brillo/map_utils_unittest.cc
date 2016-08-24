// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/map_utils.h>

#include <string>

#include <gtest/gtest.h>

namespace brillo {

class MapUtilsTest : public ::testing::Test {
 public:
  void SetUp() override {
    map_ = {
        {"key1", 1}, {"key2", 2}, {"key3", 3}, {"key4", 4}, {"key5", 5},
    };
  }

  void TearDown() override { map_.clear(); }

  std::map<std::string, int> map_;
};

TEST_F(MapUtilsTest, GetMapKeys) {
  std::set<std::string> keys = GetMapKeys(map_);
  EXPECT_EQ((std::set<std::string>{"key1", "key2", "key3", "key4", "key5"}),
            keys);
}

TEST_F(MapUtilsTest, GetMapKeysAsVector) {
  std::vector<std::string> keys = GetMapKeysAsVector(map_);
  EXPECT_EQ((std::vector<std::string>{"key1", "key2", "key3", "key4", "key5"}),
            keys);
}

TEST_F(MapUtilsTest, GetMapValues) {
  std::vector<int> values = GetMapValues(map_);
  EXPECT_EQ((std::vector<int>{1, 2, 3, 4, 5}), values);
}

TEST_F(MapUtilsTest, MapToVector) {
  std::vector<std::pair<std::string, int>> elements = MapToVector(map_);
  std::vector<std::pair<std::string, int>> expected{
      {"key1", 1}, {"key2", 2}, {"key3", 3}, {"key4", 4}, {"key5", 5},
  };
  EXPECT_EQ(expected, elements);
}

TEST_F(MapUtilsTest, Empty) {
  std::map<int, double> empty_map;
  EXPECT_TRUE(GetMapKeys(empty_map).empty());
  EXPECT_TRUE(GetMapKeysAsVector(empty_map).empty());
  EXPECT_TRUE(GetMapValues(empty_map).empty());
  EXPECT_TRUE(MapToVector(empty_map).empty());
}

}  // namespace brillo

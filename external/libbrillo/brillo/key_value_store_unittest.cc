// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/key_value_store.h>

#include <map>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <brillo/map_utils.h>
#include <gtest/gtest.h>

using base::FilePath;
using base::ReadFileToString;
using std::map;
using std::string;
using std::vector;

namespace brillo {

class KeyValueStoreTest : public ::testing::Test {
 protected:
  // Returns the value from |store_| corresponding to |key|, or an empty string
  // if the key is not present. Crashes if the store returns an empty value.
  string GetNonemptyStringValue(const string& key) {
    string value;
    if (store_.GetString(key, &value))
      CHECK(!value.empty());
    return value;
  }

  KeyValueStore store_;  // KeyValueStore under test.
};

TEST_F(KeyValueStoreTest, LoadAndSaveFromFile) {
  base::ScopedTempDir temp_dir_;
  CHECK(temp_dir_.CreateUniqueTempDir());
  base::FilePath temp_file_ = temp_dir_.path().Append("temp.conf");
  base::FilePath saved_temp_file_ = temp_dir_.path().Append("saved_temp.conf");

  string blob = "A=B\n# Comment\n";
  ASSERT_EQ(blob.size(), base::WriteFile(temp_file_, blob.data(), blob.size()));
  ASSERT_TRUE(store_.Load(temp_file_));

  string value;
  EXPECT_TRUE(store_.GetString("A", &value));
  EXPECT_EQ("B", value);

  ASSERT_TRUE(store_.Save(saved_temp_file_));
  string read_blob;
  ASSERT_TRUE(ReadFileToString(FilePath(saved_temp_file_), &read_blob));
  EXPECT_EQ("A=B\n", read_blob);
}

TEST_F(KeyValueStoreTest, CommentsAreIgnored) {
  EXPECT_TRUE(store_.LoadFromString(
      "# comment\nA=B\n\n\n#another=comment\n  # leading spaces\n"));
  EXPECT_EQ("A=B\n", store_.SaveToString());
}

TEST_F(KeyValueStoreTest, EmptyTest) {
  EXPECT_TRUE(store_.LoadFromString(""));
  EXPECT_EQ("", store_.SaveToString());
}

TEST_F(KeyValueStoreTest, LoadAndReloadTest) {
  EXPECT_TRUE(store_.LoadFromString(
      "A=B\nC=\nFOO=BAR=BAZ\nBAR=BAX\nMISSING=NEWLINE"));

  map<string, string> expected = {{"A", "B"},
                                  {"C", ""},
                                  {"FOO", "BAR=BAZ"},
                                  {"BAR", "BAX"},
                                  {"MISSING", "NEWLINE"}};

  // Test expected values.
  string value;
  for (const auto& it : expected) {
    EXPECT_TRUE(store_.GetString(it.first, &value));
    EXPECT_EQ(it.second, value) << "Testing key: " << it.first;
  }

  // Save, load and test again.
  KeyValueStore new_store;
  ASSERT_TRUE(new_store.LoadFromString(store_.SaveToString()));

  for (const auto& it : expected) {
    EXPECT_TRUE(new_store.GetString(it.first, &value)) << "key: " << it.first;
    EXPECT_EQ(it.second, value) << "key: " << it.first;
  }
}

TEST_F(KeyValueStoreTest, SimpleBooleanTest) {
  bool result;
  EXPECT_FALSE(store_.GetBoolean("A", &result));

  store_.SetBoolean("A", true);
  EXPECT_TRUE(store_.GetBoolean("A", &result));
  EXPECT_TRUE(result);

  store_.SetBoolean("A", false);
  EXPECT_TRUE(store_.GetBoolean("A", &result));
  EXPECT_FALSE(result);
}

TEST_F(KeyValueStoreTest, BooleanParsingTest) {
  string blob = "TRUE=true\nfalse=false\nvar=false\nDONT_SHOUT=TRUE\n";
  EXPECT_TRUE(store_.LoadFromString(blob));

  map<string, bool> expected = {
      {"TRUE", true}, {"false", false}, {"var", false}};
  bool value;
  EXPECT_FALSE(store_.GetBoolean("DONT_SHOUT", &value));
  string str_value;
  EXPECT_TRUE(store_.GetString("DONT_SHOUT", &str_value));

  // Test expected values.
  for (const auto& it : expected) {
    EXPECT_TRUE(store_.GetBoolean(it.first, &value)) << "key: " << it.first;
    EXPECT_EQ(it.second, value) << "key: " << it.first;
  }
}

TEST_F(KeyValueStoreTest, TrimWhitespaceAroundKey) {
  EXPECT_TRUE(store_.LoadFromString("  a=1\nb  =2\n c =3\n"));

  EXPECT_EQ("1", GetNonemptyStringValue("a"));
  EXPECT_EQ("2", GetNonemptyStringValue("b"));
  EXPECT_EQ("3", GetNonemptyStringValue("c"));

  // Keys should also be trimmed when setting new values.
  store_.SetString(" foo ", "4");
  EXPECT_EQ("4", GetNonemptyStringValue("foo"));

  store_.SetBoolean(" bar ", true);
  bool value = false;
  ASSERT_TRUE(store_.GetBoolean("bar", &value));
  EXPECT_TRUE(value);
}

TEST_F(KeyValueStoreTest, IgnoreWhitespaceLine) {
  EXPECT_TRUE(store_.LoadFromString("a=1\n \t \nb=2"));

  EXPECT_EQ("1", GetNonemptyStringValue("a"));
  EXPECT_EQ("2", GetNonemptyStringValue("b"));
}

TEST_F(KeyValueStoreTest, RejectEmptyKeys) {
  EXPECT_FALSE(store_.LoadFromString("=1"));
  EXPECT_FALSE(store_.LoadFromString(" =2"));

  // Trying to set an empty (after trimming) key should fail an assert.
  EXPECT_DEATH(store_.SetString(" ", "3"), "");
  EXPECT_DEATH(store_.SetBoolean(" ", "4"), "");
}

TEST_F(KeyValueStoreTest, RejectBogusLines) {
  EXPECT_FALSE(store_.LoadFromString("a=1\nbogus\nb=2"));
}

TEST_F(KeyValueStoreTest, MultilineValue) {
  EXPECT_TRUE(store_.LoadFromString("a=foo\nb=bar\\\n  baz \\ \nc=3\n"));

  EXPECT_EQ("foo", GetNonemptyStringValue("a"));
  EXPECT_EQ("bar  baz \\ ", GetNonemptyStringValue("b"));
  EXPECT_EQ("3", GetNonemptyStringValue("c"));
}

TEST_F(KeyValueStoreTest, UnterminatedMultilineValue) {
  EXPECT_FALSE(store_.LoadFromString("a=foo\\"));
  EXPECT_FALSE(store_.LoadFromString("a=foo\\\n"));
  EXPECT_FALSE(store_.LoadFromString("a=foo\\\n\n# blah\n"));
}

TEST_F(KeyValueStoreTest, GetKeys) {
  map<string, string> entries = {
    {"1", "apple"}, {"2", "banana"}, {"3", "cherry"}
  };
  for (const auto& it : entries) {
    store_.SetString(it.first, it.second);
  }

  vector<string> keys = GetMapKeysAsVector(entries);
  EXPECT_EQ(keys, store_.GetKeys());
}

}  // namespace brillo

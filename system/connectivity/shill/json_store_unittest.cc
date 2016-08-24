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

#include "shill/json_store.h"

#include <array>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/string_util.h>
#include <gtest/gtest.h>

#include "shill/mock_log.h"

using base::FileEnumerator;
using base::FilePath;
using base::ScopedTempDir;
using std::array;
using std::pair;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::ContainsRegex;
using testing::HasSubstr;
using testing::StartsWith;
using testing::Test;

namespace shill {

class JsonStoreTest : public Test {
 public:
  JsonStoreTest()
      : kStringWithEmbeddedNulls({0, 'a', 0, 'z'}),
        kNonUtf8String("ab\xc0") {}

  virtual void SetUp() {
    ScopeLogger::GetInstance()->EnableScopesByName("+storage");
    ASSERT_FALSE(base::IsStringUTF8(kNonUtf8String));
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    test_file_ = temp_dir_.path().Append("test-json-store");
    store_.reset(new JsonStore(test_file_));
    EXPECT_CALL(log_, Log(_, _, _)).Times(AnyNumber());
  }

  virtual void TearDown() {
    ScopeLogger::GetInstance()->EnableScopesByName("-storage");
    ScopeLogger::GetInstance()->set_verbose_level(0);
  }

 protected:
  void SetVerboseLevel(int new_level);
  void SetJsonFileContents(const string& data);

  const string kStringWithEmbeddedNulls;
  const string kNonUtf8String;
  ScopedTempDir temp_dir_;
  FilePath test_file_;
  unique_ptr<JsonStore> store_;
  ScopedMockLog log_;
};

void JsonStoreTest::SetVerboseLevel(int new_level) {
  ScopeLogger::GetInstance()->set_verbose_level(new_level);
}

void JsonStoreTest::SetJsonFileContents(const string& data) {
  EXPECT_EQ(data.size(),
            base::WriteFile(test_file_, data.data(), data.size()));
}

// In memory operations: basic storage and retrieval.
TEST_F(JsonStoreTest, StringsCanBeStoredInMemory) {
  const array<string, 5> our_values{
    {"", "hello", "world\n", kStringWithEmbeddedNulls, kNonUtf8String}};
  for (const auto& our_value : our_values) {
    string value_from_store;
    EXPECT_TRUE(store_->SetString("group_a", "knob_1", our_value));
    EXPECT_TRUE(store_->GetString("group_a", "knob_1", &value_from_store));
    EXPECT_EQ(our_value, value_from_store);
  }
}

TEST_F(JsonStoreTest, BoolsCanBeStoredInMemory) {
  const array<bool, 2> our_values{{false, true}};
  for (const auto& our_value : our_values) {
    bool value_from_store;
    EXPECT_TRUE(store_->SetBool("group_a", "knob_1", our_value));
    EXPECT_TRUE(store_->GetBool("group_a", "knob_1", &value_from_store));
    EXPECT_EQ(our_value, value_from_store);
  }
}

TEST_F(JsonStoreTest, IntsCanBeStoredInMemory) {
  const array<int, 3> our_values{{
      std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max()}};
  for (const auto& our_value : our_values) {
    int value_from_store;
    EXPECT_TRUE(store_->SetInt("group_a", "knob_1", our_value));
    EXPECT_TRUE(store_->GetInt("group_a", "knob_1", &value_from_store));
    EXPECT_EQ(our_value, value_from_store);
  }
}

TEST_F(JsonStoreTest, Uint64sCanBeStoredInMemory) {
  const array<uint64_t, 3> our_values{{
      std::numeric_limits<uint64_t>::min(),
      0,
      std::numeric_limits<uint64_t>::max()}};
  for (const auto& our_value : our_values) {
    uint64_t value_from_store;
    EXPECT_TRUE(store_->SetUint64("group_a", "knob_1", our_value));
    EXPECT_TRUE(store_->GetUint64("group_a", "knob_1", &value_from_store));
    EXPECT_EQ(our_value, value_from_store);
  }
}

TEST_F(JsonStoreTest, StringListsCanBeStoredInMemory) {
  const array<vector<string>, 7> our_values{{
      vector<string>{},
      vector<string>{""},
      vector<string>{"a"},
      vector<string>{"", "a"},
      vector<string>{"a", ""},
      vector<string>{"", "a", ""},
      vector<string>{"a", "b", "c", kStringWithEmbeddedNulls, kNonUtf8String}}};
  for (const auto& our_value : our_values) {
    vector<string> value_from_store;
    EXPECT_TRUE(store_->SetStringList("group_a", "knob_1", our_value));
    EXPECT_TRUE(store_->GetStringList("group_a", "knob_1", &value_from_store));
    EXPECT_EQ(our_value, value_from_store);
  }
}

TEST_F(JsonStoreTest, CryptedStringsCanBeStoredInMemory) {
  const array<string, 5> our_values{{
      string(), string("some stuff"), kStringWithEmbeddedNulls, kNonUtf8String
  }};
  for (const auto& our_value : our_values) {
    string value_from_store;
    EXPECT_TRUE(store_->SetCryptedString("group_a", "knob_1", our_value));
    EXPECT_TRUE(
        store_->GetCryptedString("group_a", "knob_1", &value_from_store));
    EXPECT_EQ(our_value, value_from_store);
  }
}

TEST_F(JsonStoreTest, RawValuesOfCryptedStringsDifferFromOriginalValues) {
  const array<string, 3> our_values{{
      string("simple string"), kStringWithEmbeddedNulls, kNonUtf8String
  }};
  for (const auto& our_value : our_values) {
    string raw_value_from_store;
    EXPECT_TRUE(store_->SetCryptedString("group_a", "knob_1", our_value));
    EXPECT_TRUE(store_->GetString("group_a", "knob_1", &raw_value_from_store));
    EXPECT_NE(our_value, raw_value_from_store);
  }
}

TEST_F(JsonStoreTest, DifferentGroupsCanHaveDifferentValuesForSameKey) {
  store_->SetString("group_a", "knob_1", "value_1");
  store_->SetString("group_b", "knob_1", "value_2");

  string value_from_store;
  EXPECT_TRUE(store_->GetString("group_a", "knob_1", &value_from_store));
  EXPECT_EQ("value_1", value_from_store);
  EXPECT_TRUE(store_->GetString("group_b", "knob_1", &value_from_store));
  EXPECT_EQ("value_2", value_from_store);
}

// In memory operations: presence checking.
TEST_F(JsonStoreTest, CanUseNullptrToCheckPresenceOfKey) {
  SetVerboseLevel(10);

  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group"))).Times(6);
  EXPECT_FALSE(store_->GetString("group_a", "string_knob", nullptr));
  EXPECT_FALSE(store_->GetBool("group_a", "bool_knob", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "int_knob", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "uint64_knob", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "string_list_knob", nullptr));
  EXPECT_FALSE(
      store_->GetCryptedString("group_a", "crypted_string_knob", nullptr));

  ASSERT_TRUE(store_->SetString("group_a", "random_knob", "random value"));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find property"))).Times(6);
  EXPECT_FALSE(store_->GetString("group_a", "string_knob", nullptr));
  EXPECT_FALSE(store_->GetBool("group_a", "bool_knob", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "int_knob", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "uint64_knob", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "string_list_knob", nullptr));
  EXPECT_FALSE(
      store_->GetCryptedString("group_a", "crypted_string_knob", nullptr));

  ASSERT_TRUE(store_->SetString("group_a", "string_knob", "stuff goes here"));
  ASSERT_TRUE(store_->SetBool("group_a", "bool_knob", true));
  ASSERT_TRUE(store_->SetInt("group_a", "int_knob", -1));
  ASSERT_TRUE(store_->SetUint64("group_a", "uint64_knob", 1));
  ASSERT_TRUE(store_->SetStringList(
      "group_a", "string_list_knob", vector<string>{{"hello"}}));
  ASSERT_TRUE(
      store_->SetCryptedString("group_a", "crypted_string_knob", "s3kr!t"));

  EXPECT_TRUE(store_->GetString("group_a", "string_knob", nullptr));
  EXPECT_TRUE(store_->GetBool("group_a", "bool_knob", nullptr));
  EXPECT_TRUE(store_->GetInt("group_a", "int_knob", nullptr));
  EXPECT_TRUE(store_->GetUint64("group_a", "uint64_knob", nullptr));
  EXPECT_TRUE(store_->GetStringList("group_a", "string_list_knob", nullptr));
  EXPECT_TRUE(
      store_->GetCryptedString("group_a", "crypted_string_knob", nullptr));
}

// In memory operations: access to missing elements.
TEST_F(JsonStoreTest, GetFromEmptyStoreFails) {
  bool value_from_store;
  SetVerboseLevel(10);
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group")));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", &value_from_store));
}

TEST_F(JsonStoreTest, GetFromNonexistentGroupAndKeyFails) {
  bool value_from_store;
  SetVerboseLevel(10);
  EXPECT_TRUE(store_->SetBool("group_a", "knob_1", true));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group")));
  EXPECT_FALSE(store_->GetBool("group_b", "knob_1", &value_from_store));
}

TEST_F(JsonStoreTest, GetOfNonexistentPropertyFails) {
  bool value_from_store;
  SetVerboseLevel(10);
  EXPECT_TRUE(store_->SetBool("group_a", "knob_1", true));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find property")));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_2", &value_from_store));
}

TEST_F(JsonStoreTest, GetOfPropertyFromWrongGroupFails) {
  bool value_from_store;
  SetVerboseLevel(10);
  EXPECT_TRUE(store_->SetBool("group_a", "knob_1", true));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group")));
  EXPECT_FALSE(store_->GetBool("group_b", "knob_1", &value_from_store));
}

TEST_F(JsonStoreTest, GetDoesNotMatchOnValue) {
  string value_from_store;
  SetVerboseLevel(10);
  EXPECT_TRUE(store_->SetString("group_a", "knob_1", "value_1"));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find property")));
  EXPECT_FALSE(store_->GetString("group_a", "value_1", &value_from_store));
}

// In memory operations: type conversions on read.
TEST_F(JsonStoreTest, ConversionFromStringIsProhibited) {
  EXPECT_CALL(
      log_,
      Log(logging::LOG_ERROR, _,
          ContainsRegex("Can not read \\|.+\\| from \\|.+\\|"))).Times(4);
  EXPECT_TRUE(store_->SetString("group_a", "knob_1", "stuff goes here"));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "knob_1", nullptr));
  // We deliberately omit checking store_->GetCryptedString(). While
  // this "works" right now, it's not something we're committed to.
}

TEST_F(JsonStoreTest, ConversionFromBoolIsProhibited) {
  EXPECT_CALL(
      log_,
      Log(logging::LOG_ERROR, _,
          ContainsRegex("Can not read \\|.+\\| from \\|.+\\|"))).Times(5);
  EXPECT_TRUE(store_->SetBool("group_a", "knob_1", true));
  EXPECT_FALSE(store_->GetString("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetCryptedString("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, ConversionFromIntIsProhibited) {
  EXPECT_CALL(
      log_,
      Log(logging::LOG_ERROR, _,
          ContainsRegex("Can not read \\|.+\\| from \\|.+\\|"))).Times(5);
  EXPECT_TRUE(store_->SetInt("group_a", "knob_1", -1));
  EXPECT_FALSE(store_->GetString("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetCryptedString("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, ConversionFromUint64IsProhibited) {
  EXPECT_CALL(
      log_,
      Log(logging::LOG_ERROR, _,
          ContainsRegex("Can not read \\|.+\\| from \\|.+\\|"))).Times(5);
  EXPECT_TRUE(store_->SetUint64("group_a", "knob_1", 1));
  EXPECT_FALSE(store_->GetString("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetCryptedString("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, ConversionFromStringListIsProhibited) {
  EXPECT_CALL(
      log_,
      Log(logging::LOG_ERROR, _,
          ContainsRegex("Can not read \\|.+\\| from \\|.+\\|"))).Times(5);
  EXPECT_TRUE(store_->SetStringList(
      "group_a", "knob_1", vector<string>{{"hello"}}));
  EXPECT_FALSE(store_->GetString("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetCryptedString("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, ConversionFromCryptedStringIsProhibited) {
  EXPECT_CALL(
      log_,
      Log(logging::LOG_ERROR, _,
          ContainsRegex("Can not read \\|.+\\| from \\|.+\\|"))).Times(4);
  EXPECT_TRUE(store_->SetCryptedString("group_a", "knob_1", "s3kr!t"));
  // We deliberately omit checking store_->GetString(). While this
  // "works" right now, it's not something we're committed to.
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetInt("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetUint64("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetStringList("group_a", "knob_1", nullptr));
}

// In memory operations: key deletion.
TEST_F(JsonStoreTest, DeleteKeyDeletesExistingKey) {
  SetVerboseLevel(10);
  store_->SetBool("group_a", "knob_1", bool());
  EXPECT_TRUE(store_->DeleteKey("group_a", "knob_1"));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find property")));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, DeleteKeyDeletesOnlySpecifiedKey) {
  store_->SetBool("group_a", "knob_1", bool());
  store_->SetBool("group_a", "knob_2", bool());
  EXPECT_TRUE(store_->DeleteKey("group_a", "knob_1"));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_TRUE(store_->GetBool("group_a", "knob_2", nullptr));
}

TEST_F(JsonStoreTest, DeleteKeySucceedsOnMissingKey) {
  store_->SetBool("group_a", "knob_1", bool());
  EXPECT_TRUE(store_->DeleteKey("group_a", "knob_2"));
  EXPECT_TRUE(store_->GetBool("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, DeleteKeyFailsWhenGivenWrongGroup) {
  SetVerboseLevel(10);
  store_->SetBool("group_a", "knob_1", bool());
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group")));
  EXPECT_FALSE(store_->DeleteKey("group_b", "knob_1"));
  EXPECT_TRUE(store_->GetBool("group_a", "knob_1", nullptr));
}

// In memory operations: group operations.
TEST_F(JsonStoreTest, EmptyStoreReturnsNoGroups) {
  EXPECT_EQ(set<string>(), store_->GetGroups());
  EXPECT_EQ(set<string>(), store_->GetGroupsWithKey("knob_1"));
  EXPECT_EQ(set<string>(), store_->GetGroupsWithProperties(KeyValueStore()));
}

TEST_F(JsonStoreTest, GetGroupsReturnsAllGroups) {
  store_->SetBool("group_a", "knob_1", bool());
  store_->SetBool("group_b", "knob_1", bool());
  EXPECT_EQ(set<string>({"group_a", "group_b"}), store_->GetGroups());
}

TEST_F(JsonStoreTest, GetGroupsWithKeyReturnsAllMatchingGroups) {
  store_->SetBool("group_a", "knob_1", bool());
  store_->SetBool("group_b", "knob_1", bool());
  EXPECT_EQ(set<string>({"group_a", "group_b"}),
            store_->GetGroupsWithKey("knob_1"));
}

TEST_F(JsonStoreTest, GetGroupsWithKeyReturnsOnlyMatchingGroups) {
  store_->SetBool("group_a", "knob_1", bool());
  store_->SetBool("group_b", "knob_2", bool());
  EXPECT_EQ(set<string>({"group_a"}), store_->GetGroupsWithKey("knob_1"));
}

TEST_F(JsonStoreTest, GetGroupsWithPropertiesReturnsAllMatchingGroups) {
  store_->SetBool("group_a", "knob_1", true);
  store_->SetBool("group_b", "knob_1", true);

  KeyValueStore required_properties;
  required_properties.SetBool("knob_1", true);
  EXPECT_EQ(set<string>({"group_a", "group_b"}),
            store_->GetGroupsWithProperties(required_properties));
}

TEST_F(JsonStoreTest, GetGroupsWithPropertiesReturnsOnlyMatchingGroups) {
  store_->SetBool("group_a", "knob_1", true);
  store_->SetBool("group_b", "knob_1", false);

  KeyValueStore required_properties;
  required_properties.SetBool("knob_1", true);
  EXPECT_EQ(set<string>({"group_a"}),
            store_->GetGroupsWithProperties(required_properties));
}

TEST_F(JsonStoreTest, GetGroupsWithPropertiesCanMatchOnMultipleProperties) {
  store_->SetBool("group_a", "knob_1", true);
  store_->SetBool("group_a", "knob_2", true);
  store_->SetBool("group_b", "knob_1", true);
  store_->SetBool("group_b", "knob_2", false);

  KeyValueStore required_properties;
  required_properties.SetBool("knob_1", true);
  required_properties.SetBool("knob_2", true);
  EXPECT_EQ(set<string>({"group_a"}),
            store_->GetGroupsWithProperties(required_properties));
}

TEST_F(JsonStoreTest, GetGroupsWithPropertiesChecksValuesForBoolIntAndString) {
  // Documentation in StoreInterface says GetGroupsWithProperties
  // checks only Bool, Int, and String properties. For now, we interpret
  // that permissively. i.e., checking other types is not guaranteed one
  // way or the other.
  //
  // Said differently: we test that that Bool, Int, and String are
  // supported. But we don't test that other types are ignored. (In
  // fact, JsonStore supports filtering on uint64 and StringList as
  // well. JsonStore does not, however, support filtering on
  // CryptedStrings.)
  //
  // This should be fine, as StoreInterface clients currently only use
  // String value filtering.
  const brillo::VariantDictionary exact_matcher({
      {"knob_1", string("good-string")},
      {"knob_2", bool{true}},
      {"knob_3", int{1}},
    });
  store_->SetString("group_a", "knob_1", "good-string");
  store_->SetBool("group_a", "knob_2", true);
  store_->SetInt("group_a", "knob_3", 1);

  {
    KeyValueStore correct_properties;
    KeyValueStore::ConvertFromVariantDictionary(
        exact_matcher, &correct_properties);
    EXPECT_EQ(set<string>({"group_a"}),
              store_->GetGroupsWithProperties(correct_properties));
  }

  const vector<pair<string, brillo::Any>> bad_matchers({
      {"knob_1", string("bad-string")},
      {"knob_2", bool{false}},
      {"knob_3", int{2}},
    });
  for (const auto& match_key_and_value : bad_matchers) {
    const auto& match_key = match_key_and_value.first;
    const auto& match_value = match_key_and_value.second;
    brillo::VariantDictionary bad_matcher_dict(exact_matcher);
    KeyValueStore bad_properties;
    bad_matcher_dict[match_key] = match_value;
    KeyValueStore::ConvertFromVariantDictionary(
        bad_matcher_dict, &bad_properties);
    EXPECT_EQ(set<string>(), store_->GetGroupsWithProperties(bad_properties))
        << "Failing match key: " << match_key;
  }
}

TEST_F(JsonStoreTest, ContainsGroupFindsExistingGroup) {
  store_->SetBool("group_a", "knob_1", bool());
  EXPECT_TRUE(store_->ContainsGroup("group_a"));
}

TEST_F(JsonStoreTest, ContainsGroupDoesNotFabricateGroups) {
  EXPECT_FALSE(store_->ContainsGroup("group_a"));
}

TEST_F(JsonStoreTest, DeleteGroupDeletesExistingGroup) {
  SetVerboseLevel(10);
  store_->SetBool("group_a", "knob_1", bool());
  store_->SetBool("group_a", "knob_2", bool());
  EXPECT_TRUE(store_->DeleteGroup("group_a"));
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group"))).Times(2);
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_2", nullptr));
}

TEST_F(JsonStoreTest, DeleteGroupDeletesOnlySpecifiedGroup) {
  store_->SetBool("group_a", "knob_1", bool());
  store_->SetBool("group_b", "knob_1", bool());
  EXPECT_TRUE(store_->DeleteGroup("group_a"));
  EXPECT_FALSE(store_->GetBool("group_a", "knob_1", nullptr));
  EXPECT_TRUE(store_->GetBool("group_b", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, DeleteGroupSucceedsOnMissingGroup) {
  store_->SetBool("group_a", "knob_1", bool());
  EXPECT_TRUE(store_->DeleteGroup("group_b"));
  EXPECT_TRUE(store_->GetBool("group_a", "knob_1", nullptr));
}

// File open: basic file structure.
TEST_F(JsonStoreTest, OpenSucceedsOnNonExistentFile) {
  // If the file does not already exist, we assume the caller will
  // give us data later.
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnNonJsonData) {
  SetJsonFileContents("some random junk");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("Failed to parse JSON data")));
  EXPECT_FALSE(store_->Open());
}

// File open: root element handling.
TEST_F(JsonStoreTest, OpenFailsWhenRootIsNonDictionary) {
  SetJsonFileContents("\"a string\"");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("JSON value is not a dictionary")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenWarnsOnRootDictionaryWithNonStringDescription) {
  SetJsonFileContents("{\"description\": 1}");
  EXPECT_CALL(
      log_,
      Log(logging::LOG_WARNING, _, HasSubstr("|description| is not a string")));
  store_->Open();
}

TEST_F(JsonStoreTest, OpenFailsOnRootDictionaryWithoutSettings) {
  SetJsonFileContents("{}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("Property |settings| is missing")));
  EXPECT_FALSE(store_->Open());
}

// File open: settings element handling.
TEST_F(JsonStoreTest, OpenSucceedsOnEmptySettings) {
  SetJsonFileContents("{\"settings\": {}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsWhenSettingsIsNonDictionary) {
  SetJsonFileContents("{\"settings\": 1}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("Property |settings| is not a dictionary")));
  EXPECT_FALSE(store_->Open());
}

// File open: group structure.
TEST_F(JsonStoreTest, OpenSucceedsOnEmptyGroup) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {}"
      "}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsWhenGroupIsNonDictionary) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": 1"
      "}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("Group |group_a| is not a dictionary")));
  EXPECT_FALSE(store_->Open());
}

// File open: each supported property type (with selected valid
// values for each type), ordered by base::Value::Type enum.  Types
// which are not supported by base::Value are ordered as
// TYPE_DICTIONARY.
TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithBooleanValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": true"
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithMinIntegerValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": -2147483648"  // -2^31
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithMaxIntegerValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": 2147483647"  // 2^31-1
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithStringValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": \"this is \\\"a\\\" string\\n\""
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithEscapedStringValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"non_ascii_string\","
      "            \"_encoded_value\": \"0001020304\""
      "}}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithMinUint64Value) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"uint64\","
      "            \"_encoded_value\": \"0\""  // 2^64-1
      "}}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithMaxUint64Value) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"uint64\","
      "            \"_encoded_value\": \"18446744073709551615\""  // 2^64-1
      "}}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithEmptyListValue) {
  // Empty list is presumed to be an empty string list.
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": []"
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWithStringListValueWithSingleItem) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": [ \"a string\" ]"
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(
    JsonStoreTest, OpenSucceedsOnSettingWithStringListValueWithMultipleItems) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": [ \"string 1\", \"string 2\\n\" ]"
      "}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest, OpenSucceedsOnSettingWhenStringListHasEscapedItem) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": [{"
      "            \"_native_type\": \"non_ascii_string\","
      "            \"_encoded_value\": \"0001020304\""
      "}]}}}");
  EXPECT_TRUE(store_->Open());
}

TEST_F(JsonStoreTest,
       OpenSucceedsOnSettingWhenStringListHasEscapedAndUnescapedItems) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": ["
      "            {\"_native_type\": \"non_ascii_string\","
      "             \"_encoded_value\": \"0001020304\"},"
      "            \"normal string\""
      "]}}}");
  EXPECT_TRUE(store_->Open());
}

// File open: unsupported types, and invalid values. Ordered by
// base::Value::Type enum.  Types which are supported by JsonStore,
// but not directly supported by base::Value, are ordered as
// TYPE_DICTIONARY.
TEST_F(JsonStoreTest, OpenFailsOnSettingWithNullValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": null"
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  HasSubstr("has unsupported TYPE_NULL")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithBadBooleanValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": truthy"
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, StartsWith("Failed to parse JSON")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithOverlySmallInteger) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": -2147483649"  // -2^31-1
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, HasSubstr("unsupported TYPE_DOUBLE")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithOverlyLargeInteger) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": 2147483648"  // 2^31
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, HasSubstr("unsupported TYPE_DOUBLE")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithDoubleValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": 1.234"
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, HasSubstr("unsupported TYPE_DOUBLE")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithDictionaryValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {}"
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  HasSubstr("unsupported TYPE_DICTIONARY")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithOverlayLargeUint64Value) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"uint64\","
      "            \"_encoded_value\": \"18446744073709551616\""  // 2^64
      "}}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, StartsWith("Failed to parse uint64")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithOverlaySmallUint64Value) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"uint64\","
      "            \"_encoded_value\": \"-1\""
      "}}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, StartsWith("Failed to parse uint64")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsWhenSettingHasEscapedStringWithInvalidHex) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"non_ascii_string\","
      "            \"_encoded_value\": \"-1\""
      "}}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, StartsWith("Failed to decode hex")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest,
       OpenFailsWhenSettingHasEscapedStringListItemWithInvalidHex) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": [{"
      "            \"_native_type\": \"non_ascii_string\","
      "            \"_encoded_value\": \"-1\""
      "}]}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, StartsWith("Failed to decode hex")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnCoercedSettingWithBadNativeType) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": true,"
      "            \"_encoded_value\": \"1234\""
      "}}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("Property |_native_type| is not a string")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnCoercedSettingWhenEncodedValueIsNotAString) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": {"
      "            \"_native_type\": \"uint64\","
      "            \"_encoded_value\": 1234"
      "}}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  StartsWith("Property |_encoded_value| is not a string")));
  EXPECT_FALSE(store_->Open());
}

TEST_F(JsonStoreTest, OpenFailsOnSettingWithIntListValue) {
  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_1\": [ 1 ]"
      "}}}");
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _,
                  HasSubstr("instead of expected type")));
  EXPECT_FALSE(store_->Open());
}

// File open: miscellaneous.
TEST_F(JsonStoreTest, OpenClearsExistingInMemoryData) {
  store_->SetString("group_a", "knob_1", "watch me disappear");
  ASSERT_TRUE(store_->GetString("group_a", "knob_1", nullptr));

  SetJsonFileContents(
      "{\"settings\": {"
      "    \"group_a\": {"
      "        \"knob_2\": \"new stuff\""
      "}}}");
  ASSERT_TRUE(store_->Open());
  EXPECT_FALSE(store_->GetString("group_a", "knob_1", nullptr));
  EXPECT_TRUE(store_->GetString("group_a", "knob_2", nullptr));
}

TEST_F(JsonStoreTest, OpenClearsExistingInMemoryGroups) {
  store_->SetString("group_a", "knob_1", "watch me disappear");
  ASSERT_FALSE(store_->GetGroups().empty());

  // In the delete case, we're non-comittal about whether empty groups
  // are garbage collected. But, in the Open() case, we commit to
  // fully clearing in-memory data.
  SetJsonFileContents("{\"settings\": {}}");
  ASSERT_TRUE(store_->Open());
  EXPECT_TRUE(store_->GetGroups().empty());
}

// File operations: Close() basic functionality.
TEST_F(JsonStoreTest, ClosePersistsData) {
  ASSERT_FALSE(store_->IsNonEmpty());
  ASSERT_TRUE(store_->Close());

  // Verify that the file actually got written with the right name.
  FileEnumerator file_enumerator(temp_dir_.path(),
                                 false /* not recursive */,
                                 FileEnumerator::FILES);
  EXPECT_EQ(test_file_.value(), file_enumerator.Next().value());

  // Verify that the profile is a regular file, readable and writeable by the
  // owner only.
  FileEnumerator::FileInfo file_info = file_enumerator.GetInfo();
  EXPECT_EQ(S_IFREG | S_IRUSR | S_IWUSR, file_info.stat().st_mode);
}

// File operations: Flush() basics.
TEST_F(JsonStoreTest, FlushCreatesPersistentStore) {
  ASSERT_FALSE(store_->IsNonEmpty());
  ASSERT_TRUE(store_->Flush());

  // Verify that the file actually got written with the right name.
  FileEnumerator file_enumerator(temp_dir_.path(),
                                 false /* not recursive */,
                                 FileEnumerator::FILES);
  EXPECT_EQ(test_file_.value(), file_enumerator.Next().value());

  // Verify that the profile is a regular file, readable and writeable by the
  // owner only.
  FileEnumerator::FileInfo file_info = file_enumerator.GetInfo();
  EXPECT_EQ(S_IFREG | S_IRUSR | S_IWUSR, file_info.stat().st_mode);
}

TEST_F(JsonStoreTest, FlushFailsWhenPathIsNotWriteable) {
  ASSERT_TRUE(base::CreateDirectory(test_file_));
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, StartsWith("Failed to write")));
  EXPECT_FALSE(store_->Flush());
}

// File operations: writing.
//
// The ordering of groups, and the ordering of keys within a group,
// are decided by the JSON writer. Hence, we can not simply compare
// the written data to an expected literal value.
//
// Instead, we write the data out, and verify that reading the data
// yields the same groups, keys, and values.
TEST_F(JsonStoreTest, CanPersistAndRestoreHeader) {
  store_->SetHeader("rosetta stone");
  ASSERT_EQ("rosetta stone", store_->file_description_);
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->file_description_, persisted_data.file_description_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreAllTypes) {
  store_->SetString("group_a", "string_knob", "our string");
  store_->SetBool("group_a", "bool_knob", true);
  store_->SetInt("group_a", "int_knob", 1);
  store_->SetUint64(
      "group_a", "uint64_knob", std::numeric_limits<uint64_t>::max());
  store_->SetStringList(
      "group_a", "stringlist_knob", vector<string>{"a", "b", "c"});
  store_->SetCryptedString("group_a", "cryptedstring_knob", "s3kr!t");
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreNonUtf8Strings) {
  store_->SetString("group_a", "string_knob", kNonUtf8String);
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreNonUtf8StringList) {
  store_->SetStringList(
      "group_a", "string_knob", vector<string>({kNonUtf8String}));
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreStringsWithEmbeddedNulls) {
  store_->SetString("group_a", "string_knob", kStringWithEmbeddedNulls);
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreStringListWithEmbeddedNulls) {
  store_->SetStringList(
      "group_a", "string_knob", vector<string>({kStringWithEmbeddedNulls}));
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreMultipleGroups) {
  store_->SetString("group_a", "knob_1", "first string");
  store_->SetString("group_b", "knob_2", "second string");
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanPersistAndRestoreMultipleGroupsWithSameKeys) {
  store_->SetString("group_a", "knob_1", "first string");
  store_->SetString("group_a", "knob_2", "second string");
  store_->SetString("group_b", "knob_1", "frist post!");
  store_->SetStringList("group_b", "knob_2", vector<string>{"2nd try"});
  store_->Flush();

  JsonStore persisted_data(test_file_);
  persisted_data.Open();
  EXPECT_EQ(
      store_->group_name_to_settings_, persisted_data.group_name_to_settings_);
}

TEST_F(JsonStoreTest, CanDeleteKeyFromPersistedData) {
  store_->SetString("group_a", "knob_1", "first string");
  store_->Flush();

  JsonStore persisted_data_v1(test_file_);
  persisted_data_v1.Open();
  ASSERT_TRUE(persisted_data_v1.GetString("group_a", "knob_1", nullptr));
  store_->DeleteKey("group_a", "knob_1");
  store_->Flush();

  JsonStore persisted_data_v2(test_file_);
  SetVerboseLevel(10);
  // Whether an empty group is written or not is an implementation
  // detail.  Hence, we don't care if the error message is about a
  // missing group, or a missing property.
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find")));
  EXPECT_FALSE(persisted_data_v2.GetString("group_a", "knob_1", nullptr));
}

TEST_F(JsonStoreTest, CanDeleteGroupFromPersistedData) {
  store_->SetString("group_a", "knob_1", "first string");
  store_->Flush();

  JsonStore persisted_data_v1(test_file_);
  persisted_data_v1.Open();
  ASSERT_TRUE(persisted_data_v1.GetString("group_a", "knob_1", nullptr));
  store_->DeleteGroup("group_a");
  store_->Flush();

  JsonStore persisted_data_v2(test_file_);
  SetVerboseLevel(10);
  persisted_data_v2.Open();
  EXPECT_CALL(log_, Log(_, _, HasSubstr("Could not find group")));
  EXPECT_FALSE(persisted_data_v2.GetString("group_a", "knob_1", nullptr));
}

// File operations: file management.
TEST_F(JsonStoreTest, MarkAsCorruptedFailsWhenStoreHasNotBeenPersisted) {
  EXPECT_CALL(log_,
              Log(logging::LOG_ERROR, _, HasSubstr("rename failed")));
  EXPECT_FALSE(store_->MarkAsCorrupted());
}

TEST_F(JsonStoreTest, MarkAsCorruptedMovesCorruptStore) {
  store_->Flush();
  ASSERT_TRUE(store_->IsNonEmpty());
  ASSERT_TRUE(base::PathExists(test_file_));

  EXPECT_TRUE(store_->MarkAsCorrupted());
  EXPECT_FALSE(store_->IsNonEmpty());
  EXPECT_FALSE(base::PathExists(test_file_));
  EXPECT_TRUE(base::PathExists(FilePath(test_file_.value() + ".corrupted")));
}

}  // namespace shill

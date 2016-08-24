// Copyright 2015 The Android Open Source Project
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

#include <brillo/value_conversion.h>

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <gtest/gtest.h>

namespace brillo {

namespace {

std::unique_ptr<base::Value> ParseValue(std::string json) {
  std::replace(json.begin(), json.end(), '\'', '"');
  std::string message;
  std::unique_ptr<base::Value> value{
      base::JSONReader::ReadAndReturnError(json, base::JSON_PARSE_RFC,nullptr,
                                           &message)
          .release()};
  CHECK(value) << "Failed to load JSON: " << message << ", " << json;
  return value;
}

inline bool IsEqualValue(const base::Value& val1, const base::Value& val2) {
  return val1.Equals(&val2);
}

#define EXPECT_JSON_EQ(expected, actual) \
  EXPECT_PRED2(IsEqualValue, *ParseValue(expected), actual)

}  // namespace

TEST(ValueConversionTest, FromValueInt) {
  int actual;
  EXPECT_TRUE(FromValue(*ParseValue("123"), &actual));
  EXPECT_EQ(123, actual);

  EXPECT_TRUE(FromValue(*ParseValue("-123"), &actual));
  EXPECT_EQ(-123, actual);

  EXPECT_FALSE(FromValue(*ParseValue("true"), &actual));
}

TEST(ValueConversionTest, FromValueBool) {
  bool actual;
  EXPECT_TRUE(FromValue(*ParseValue("false"), &actual));
  EXPECT_FALSE(actual);

  EXPECT_TRUE(FromValue(*ParseValue("true"), &actual));
  EXPECT_TRUE(actual);

  EXPECT_FALSE(FromValue(*ParseValue("0"), &actual));
  EXPECT_FALSE(FromValue(*ParseValue("1"), &actual));
}

TEST(ValueConversionTest, FromValueDouble) {
  double actual;
  EXPECT_TRUE(FromValue(*ParseValue("12.5"), &actual));
  EXPECT_DOUBLE_EQ(12.5, actual);

  EXPECT_TRUE(FromValue(*ParseValue("-0.1"), &actual));
  EXPECT_DOUBLE_EQ(-0.1, actual);

  EXPECT_TRUE(FromValue(*ParseValue("17"), &actual));
  EXPECT_DOUBLE_EQ(17.0, actual);

  EXPECT_FALSE(FromValue(*ParseValue("'1.0'"), &actual));
}

TEST(ValueConversionTest, FromValueString) {
  std::string actual;
  EXPECT_TRUE(FromValue(*ParseValue("'foo'"), &actual));
  EXPECT_EQ("foo", actual);

  EXPECT_TRUE(FromValue(*ParseValue("'bar'"), &actual));
  EXPECT_EQ("bar", actual);

  EXPECT_TRUE(FromValue(*ParseValue("''"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("1"), &actual));
}

TEST(ValueConversionTest, FromValueListValue) {
  const base::ListValue* list = nullptr;
  auto in_value = ParseValue("[1, 2, 'foo']");
  EXPECT_TRUE(FromValue(*in_value, &list));
  EXPECT_JSON_EQ("[1, 2, 'foo']", *list);
}

TEST(ValueConversionTest, FromValueDictValue) {
  const base::DictionaryValue* dict = nullptr;
  auto in_value = ParseValue("{'foo':'bar','baz': 1}");
  EXPECT_TRUE(FromValue(*in_value, &dict));
  EXPECT_JSON_EQ("{'foo':'bar','baz': 1}", *dict);
}

TEST(ValueConversionTest, FromValueListValueUniquePtr) {
  std::unique_ptr<base::ListValue> list;
  EXPECT_TRUE(FromValue(*ParseValue("[1, 2, 'bar']"), &list));
  EXPECT_JSON_EQ("[1, 2, 'bar']", *list);
}

TEST(ValueConversionTest, FromValueDictValueUniquePtr) {
  std::unique_ptr<base::DictionaryValue> dict;
  EXPECT_TRUE(FromValue(*ParseValue("{'foo':'bar','baz': 1}"), &dict));
  EXPECT_JSON_EQ("{'foo':'bar','baz': 1}", *dict);
}

TEST(ValueConversionTest, FromValueVectorOfInt) {
  std::vector<int> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[1, 2, 3, 4]"), &actual));
  EXPECT_EQ((std::vector<int>{1, 2, 3, 4}), actual);

  EXPECT_TRUE(FromValue(*ParseValue("[]"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("[1, 2, 3, '4']"), &actual));
}

TEST(ValueConversionTest, FromValueVectorOfBool) {
  std::vector<bool> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[true, true, false]"), &actual));
  EXPECT_EQ((std::vector<bool>{true, true, false}), actual);

  EXPECT_TRUE(FromValue(*ParseValue("[]"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("[true, 0]"), &actual));
}

TEST(ValueConversionTest, FromValueVectorOfDouble) {
  std::vector<double> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[1, 2.0, 6.5, -11.2]"), &actual));
  EXPECT_EQ((std::vector<double>{1.0, 2.0, 6.5, -11.2}), actual);

  EXPECT_TRUE(FromValue(*ParseValue("[]"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("['s']"), &actual));
}

TEST(ValueConversionTest, FromValueVectorOfString) {
  std::vector<std::string> actual;
  EXPECT_TRUE(FromValue(*ParseValue("['', 'foo', 'bar']"), &actual));
  EXPECT_EQ((std::vector<std::string>{"", "foo", "bar"}), actual);

  EXPECT_TRUE(FromValue(*ParseValue("[]"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("[100]"), &actual));
}

TEST(ValueConversionTest, FromValueVectorOfVectors) {
  std::vector<std::vector<int>> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[[1,2], [], [3]]"), &actual));
  EXPECT_EQ((std::vector<std::vector<int>>{{1,2}, {}, {3}}), actual);

  EXPECT_TRUE(FromValue(*ParseValue("[]"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("[100]"), &actual));
}

TEST(ValueConversionTest, FromValueMap) {
  std::map<std::string, int> actual;
  EXPECT_TRUE(FromValue(*ParseValue("{'foo':1, 'bar':2, 'baz':3}"), &actual));
  EXPECT_EQ((std::map<std::string, int>{{"foo", 1}, {"bar", 2}, {"baz", 3}}),
            actual);

  EXPECT_TRUE(FromValue(*ParseValue("{}"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("{'foo':1, 'bar':'2'}"), &actual));
}

TEST(ValueConversionTest, FromValueMapOfVectors) {
  std::map<std::string, std::vector<int>> actual;
  EXPECT_TRUE(FromValue(*ParseValue("{'foo':[1,2], 'bar':[]}"), &actual));
  std::map<std::string, std::vector<int>> expected{
      {"foo", {1, 2}}, {"bar", {}}};
  EXPECT_EQ(expected, actual);

  EXPECT_TRUE(FromValue(*ParseValue("{}"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("{'foo':[1], 'bar':[2,'3']}"), &actual));
}

TEST(ValueConversionTest, FromValueVectorOfMaps) {
  std::vector<std::map<std::string, int>> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[{'foo':1,'bar':2},{'baz':3}]"), &actual));
  std::vector<std::map<std::string, int>> expected{
      {{"foo", 1}, {"bar", 2}}, {{"baz", 3}}};
  EXPECT_EQ(expected, actual);

  EXPECT_TRUE(FromValue(*ParseValue("[]"), &actual));
  EXPECT_TRUE(actual.empty());

  EXPECT_FALSE(FromValue(*ParseValue("[{'foo':1}, 'bar']"), &actual));
}

TEST(ValueConversionTest, FromValueVectorOfLists) {
  std::vector<std::unique_ptr<base::ListValue>> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[['foo',1],['bar',2],[true]]"), &actual));
  ASSERT_EQ(3, actual.size());
  EXPECT_JSON_EQ("['foo', 1]", *actual[0]);
  EXPECT_JSON_EQ("['bar', 2]", *actual[1]);
  EXPECT_JSON_EQ("[true]", *actual[2]);
}

TEST(ValueConversionTest, FromValueVectorOfDicts) {
  std::vector<std::unique_ptr<base::DictionaryValue>> actual;
  EXPECT_TRUE(FromValue(*ParseValue("[{'foo': 1}, {'bar': 2}]"), &actual));
  ASSERT_EQ(2, actual.size());
  EXPECT_JSON_EQ("{'foo': 1}", *actual[0]);
  EXPECT_JSON_EQ("{'bar': 2}", *actual[1]);
}

TEST(ValueConversionTest, ToValueScalar) {
  EXPECT_JSON_EQ("1234", *ToValue(1234));
  EXPECT_JSON_EQ("true", *ToValue(true));
  EXPECT_JSON_EQ("false", *ToValue(false));
  EXPECT_JSON_EQ("12.5", *ToValue(12.5));
  EXPECT_JSON_EQ("'foobar'", *ToValue("foobar"));
}

TEST(ValueConversionTest, ToValueVector) {
  EXPECT_JSON_EQ("[1, 2, 3]", *ToValue(std::vector<int>{1, 2, 3}));
  EXPECT_JSON_EQ("[]", *ToValue(std::vector<int>{}));
  EXPECT_JSON_EQ("[true, false]", *ToValue(std::vector<bool>{true, false}));
  EXPECT_JSON_EQ("['foo', 'bar']",
                 *ToValue(std::vector<std::string>{"foo", "bar"}));
  EXPECT_JSON_EQ("[[1,2],[3]]",
                 *ToValue(std::vector<std::vector<int>>{{1, 2}, {3}}));
}

TEST(ValueConversionTest, ToValueMap) {
  EXPECT_JSON_EQ("{'foo': 1, 'bar': 2}",
                 *ToValue(std::map<std::string, int>{{"foo", 1}, {"bar", 2}}));
  EXPECT_JSON_EQ("{}", *ToValue(std::map<std::string, int>{}));
  EXPECT_JSON_EQ("{'foo': true}",
                 *ToValue(std::map<std::string, bool>{{"foo", true}}));
  EXPECT_JSON_EQ("{'foo': 1.1, 'bar': 2.2}",
                 *ToValue(std::map<std::string, double>{{"foo", 1.1},
                                                        {"bar", 2.2}}));
}

}  // namespace brillo

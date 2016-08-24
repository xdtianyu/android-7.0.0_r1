// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/test/unittest_utils.h>

#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/logging.h>

namespace weave {
namespace test {

std::unique_ptr<base::Value> CreateValue(const std::string& json) {
  std::string json2(json);
  // Convert apostrophes to double-quotes so JSONReader can parse the string.
  std::replace(json2.begin(), json2.end(), '\'', '"');
  int error = 0;
  std::string message;
  std::unique_ptr<base::Value> value{
      base::JSONReader::ReadAndReturnError(json2, base::JSON_PARSE_RFC, &error,
                                           &message)
          .release()};
  CHECK(value) << "Failed to load JSON: " << message << ", " << json;
  return value;
}

std::string ValueToString(const base::Value& value) {
  std::string json;
  base::JSONWriter::WriteWithOptions(
      value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  return json;
}

std::unique_ptr<base::DictionaryValue> CreateDictionaryValue(
    const std::string& json) {
  std::unique_ptr<base::Value> value = CreateValue(json);
  base::DictionaryValue* dict = nullptr;
  value->GetAsDictionary(&dict);
  CHECK(dict) << "Value is not dictionary: " << json;
  value.release();
  return std::unique_ptr<base::DictionaryValue>(dict);
}

}  // namespace test
}  // namespace weave

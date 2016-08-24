// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/utils.h"

#include <base/bind_helpers.h>
#include <base/json/json_reader.h>

#include "src/json_error_codes.h"

namespace weave {

namespace {

// Truncates a string if it is too long. Used for error reporting with really
// long JSON strings.
std::string LimitString(const std::string& text, size_t max_len) {
  if (text.size() <= max_len)
    return text;
  return text.substr(0, max_len - 3) + "...";
}

const size_t kMaxStrLen = 1700;  // Log messages are limited to 2000 chars.

const char kErrorCodeKey[] = "code";
const char kErrorMessageKey[] = "message";

}  // anonymous namespace

namespace errors {
const char kSchemaError[] = "schema_error";
const char kInvalidCategoryError[] = "invalid_category";
const char kInvalidPackageError[] = "invalid_package";
}  // namespace errors

std::unique_ptr<base::DictionaryValue> LoadJsonDict(
    const std::string& json_string,
    ErrorPtr* error) {
  std::unique_ptr<base::DictionaryValue> result;
  std::string error_message;
  auto value = base::JSONReader::ReadAndReturnError(
      json_string, base::JSON_PARSE_RFC, nullptr, &error_message);
  if (!value) {
    Error::AddToPrintf(error, FROM_HERE, errors::json::kParseError,
                       "Error parsing JSON string '%s' (%zu): %s",
                       LimitString(json_string, kMaxStrLen).c_str(),
                       json_string.size(), error_message.c_str());
    return result;
  }
  base::DictionaryValue* dict_value = nullptr;
  if (!value->GetAsDictionary(&dict_value)) {
    Error::AddToPrintf(error, FROM_HERE, errors::json::kObjectExpected,
                       "JSON string '%s' is not a JSON object",
                       LimitString(json_string, kMaxStrLen).c_str());
    return result;
  } else {
    // |value| is now owned by |dict_value|.
    base::IgnoreResult(value.release());
  }
  result.reset(dict_value);
  return result;
}

std::unique_ptr<base::DictionaryValue> ErrorInfoToJson(const Error& error) {
  std::unique_ptr<base::DictionaryValue> output{new base::DictionaryValue};
  output->SetString(kErrorMessageKey, error.GetMessage());
  output->SetString(kErrorCodeKey, error.GetCode());
  return output;
}

}  // namespace weave

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

#include <string>
#include <vector>

namespace brillo {

bool FromValue(const base::Value& in_value,
               std::unique_ptr<base::ListValue>* out_value) {
  const base::ListValue* list = nullptr;
  if (!in_value.GetAsList(&list))
    return false;
  out_value->reset(list->DeepCopy());
  return true;
}

bool FromValue(const base::Value& in_value,
               std::unique_ptr<base::DictionaryValue>* out_value) {
  const base::DictionaryValue* dict = nullptr;
  if (!in_value.GetAsDictionary(&dict))
    return false;
  out_value->reset(dict->DeepCopy());
  return true;
}

std::unique_ptr<base::Value> ToValue(int value) {
  return std::unique_ptr<base::Value>{new base::FundamentalValue{value}};
}

std::unique_ptr<base::Value> ToValue(bool value) {
  return std::unique_ptr<base::Value>{new base::FundamentalValue{value}};
}

std::unique_ptr<base::Value> ToValue(double value) {
  return std::unique_ptr<base::Value>{new base::FundamentalValue{value}};
}

std::unique_ptr<base::Value> ToValue(const char* value) {
  return std::unique_ptr<base::Value>{new base::StringValue{value}};
}

std::unique_ptr<base::Value> ToValue(const std::string& value) {
  return std::unique_ptr<base::Value>{new base::StringValue{value}};
}

}  // namespace brillo

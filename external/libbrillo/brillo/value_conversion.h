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

#ifndef BRILLO_VALUE_CONVERSION_H_
#define BRILLO_VALUE_CONVERSION_H_

// This file provides a set of helper functions to convert between base::Value
// and native types. Apart from handling standard types such as 'int' and
// 'std::string' it also provides conversion to/from std::vector<T> (which
// converts to Base::listValue) and std::map<std::string, T> (convertible to
// base::DictionaryValue).

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/values.h>
#include <brillo/brillo_export.h>

namespace brillo {

inline bool FromValue(const base::Value& in_value, bool* out_value) {
  return in_value.GetAsBoolean(out_value);
}

inline bool FromValue(const base::Value& in_value, int* out_value) {
  return in_value.GetAsInteger(out_value);
}

inline bool FromValue(const base::Value& in_value, double* out_value) {
  return in_value.GetAsDouble(out_value);
}

inline bool FromValue(const base::Value& in_value, std::string* out_value) {
  return in_value.GetAsString(out_value);
}

inline bool FromValue(const base::Value& in_value,
                      const base::ListValue** out_value) {
  return in_value.GetAsList(out_value);
}

inline bool FromValue(const base::Value& in_value,
                      const base::DictionaryValue** out_value) {
  return in_value.GetAsDictionary(out_value);
}

BRILLO_EXPORT bool FromValue(const base::Value& in_value,
                             std::unique_ptr<base::ListValue>* out_value);
BRILLO_EXPORT bool FromValue(const base::Value& in_value,
                             std::unique_ptr<base::DictionaryValue>* out_value);

template <typename T, typename Pred, typename Alloc>
bool FromValue(const base::Value& in_value,
              std::map<std::string, T, Pred, Alloc>* out_value);

template <typename T, typename Alloc>
bool FromValue(const base::Value& in_value, std::vector<T, Alloc>* out_value) {
  const base::ListValue* list = nullptr;
  if (!in_value.GetAsList(&list))
    return false;
  out_value->clear();
  out_value->reserve(list->GetSize());
  for (const base::Value* item : *list) {
    T value{};
    if (!FromValue(*item, &value))
      return false;
    out_value->push_back(std::move(value));
  }
  return true;
}

template <typename T, typename Pred, typename Alloc>
bool FromValue(const base::Value& in_value,
              std::map<std::string, T, Pred, Alloc>* out_value) {
  const base::DictionaryValue* dict = nullptr;
  if (!in_value.GetAsDictionary(&dict))
    return false;
  out_value->clear();
  for (base::DictionaryValue::Iterator it(*dict); !it.IsAtEnd(); it.Advance()) {
    if (!FromValue(it.value(), &(*out_value)[it.key()]))
      return false;
  }
  return true;
}

template <typename T>
T FromValue(const base::Value& value) {
  T out_value{};
  CHECK(FromValue(value, &out_value));
  return out_value;
}

BRILLO_EXPORT std::unique_ptr<base::Value> ToValue(int value);
BRILLO_EXPORT std::unique_ptr<base::Value> ToValue(bool value);
BRILLO_EXPORT std::unique_ptr<base::Value> ToValue(double value);
BRILLO_EXPORT std::unique_ptr<base::Value> ToValue(const std::string& value);
// Implicit conversion of char* to 'bool' has precedence over the user-defined
// std::string conversion. Override this behavior explicitly.
BRILLO_EXPORT std::unique_ptr<base::Value> ToValue(const char* value);

template <typename T, typename Pred, typename Alloc>
std::unique_ptr<base::Value> ToValue(
    const std::map<std::string, T, Pred, Alloc>& dictionary);

template <typename T, typename Alloc>
std::unique_ptr<base::Value> ToValue(const std::vector<T, Alloc>& list) {
  std::unique_ptr<base::ListValue> result{new base::ListValue};
  for (const auto& value : list) {
    result->Append(ToValue(value).release());
  }
  return std::move(result);
}

template <typename T, typename Pred, typename Alloc>
std::unique_ptr<base::Value> ToValue(
    const std::map<std::string, T, Pred, Alloc>& dictionary) {
  std::unique_ptr<base::DictionaryValue> result{new base::DictionaryValue};
  for (const auto& pair : dictionary) {
    result->Set(pair.first, ToValue(pair.second).release());
  }
  return std::move(result);
}

}  // namespace brillo

#endif  // BRILLO_VALUE_CONVERSION_H_

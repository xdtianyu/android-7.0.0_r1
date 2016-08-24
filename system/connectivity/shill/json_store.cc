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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cinttypes>
#include <map>
#include <memory>
#include <typeinfo>
#include <vector>

#include <base/files/important_file_writer.h>
#include <base/files/file_util.h>
#include <base/json/json_string_value_serializer.h>
#include <base/memory/scoped_ptr.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <base/values.h>

#include "shill/crypto_rot47.h"
#include "shill/logging.h"
#include "shill/scoped_umask.h"

using std::map;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

namespace shill {

namespace Logging {

static auto kModuleLogScope = ScopeLogger::kStorage;
static string ObjectID(const JsonStore* j) {
  return "(unknown)";
}

}  // namespace Logging

namespace {

static const char kCorruptSuffix[] = ".corrupted";
static const char kCoercedValuePropertyEncodedValue[] = "_encoded_value";
static const char kCoercedValuePropertyNativeType[] = "_native_type";
static const char kNativeTypeNonAsciiString[] = "non_ascii_string";
static const char kNativeTypeUint64[] = "uint64";
static const char kRootPropertyDescription[] = "description";
static const char kRootPropertySettings[] = "settings";

bool DoesGroupContainProperties(
    const brillo::VariantDictionary& group,
    const brillo::VariantDictionary& required_properties) {
  for (const auto& required_property_name_and_value : required_properties) {
    const auto& required_key = required_property_name_and_value.first;
    const auto& required_value = required_property_name_and_value.second;
    const auto& group_it = group.find(required_key);
    if (group_it == group.end() || group_it->second != required_value) {
      return false;
    }
  }
  return true;
}

// Deserialization helpers.

// A coerced value is used to represent values that base::Value does
// not directly support.  A coerced value has the form
//   {'_native_type': <type-as-string>, '_encoded_value': <value-as-string>}
bool IsCoercedValue(const base::DictionaryValue& value) {
  return value.HasKey(kCoercedValuePropertyNativeType) &&
      value.HasKey(kCoercedValuePropertyEncodedValue);
}

unique_ptr<brillo::Any> DecodeCoercedValue(
    const base::DictionaryValue& coerced_value) {
  string native_type;
  if (!coerced_value.GetStringWithoutPathExpansion(
          kCoercedValuePropertyNativeType, &native_type)) {
    LOG(ERROR) << "Property |" << kCoercedValuePropertyNativeType
               << "| is not a string.";
    return nullptr;
  }

  string encoded_value;
  if (!coerced_value.GetStringWithoutPathExpansion(
          kCoercedValuePropertyEncodedValue, &encoded_value)) {
    LOG(ERROR) << "Property |" << kCoercedValuePropertyEncodedValue
               << "| is not a string.";
    return nullptr;
  }

  if (native_type == kNativeTypeNonAsciiString) {
    vector<uint8_t> native_value;
    if (base::HexStringToBytes(encoded_value, &native_value)) {
      return unique_ptr<brillo::Any>(
          new brillo::Any(string(native_value.begin(), native_value.end())));
    } else {
      LOG(ERROR) << "Failed to decode hex data from |" << encoded_value << "|.";
      return nullptr;
    }
  } else if (native_type == kNativeTypeUint64) {
    uint64_t native_value;
    if (base::StringToUint64(encoded_value, &native_value)) {
      return unique_ptr<brillo::Any>(new brillo::Any(native_value));
    } else {
      LOG(ERROR) << "Failed to parse uint64 from |" << encoded_value << "|.";
      return nullptr;
    }
  } else {
    LOG(ERROR) << "Unsupported native type |" << native_type << "|.";
    return nullptr;
  }
}

unique_ptr<string> MakeStringFromValue(const base::Value& value) {
  const auto value_type = value.GetType();

  if (value_type == base::Value::TYPE_STRING) {
    unique_ptr<string> unwrapped_string(new string());
    value.GetAsString(unwrapped_string.get());
    return unwrapped_string;
  } else if (value_type == base::Value::TYPE_DICTIONARY) {
    const base::DictionaryValue* dictionary_value;
    value.GetAsDictionary(&dictionary_value);
    unique_ptr<brillo::Any> decoded_value(
        DecodeCoercedValue(*dictionary_value));
    if (!decoded_value) {
      LOG(ERROR) << "Failed to decode coerced value.";
      return nullptr;
    }

    if (!decoded_value->IsTypeCompatible<string>()) {
      LOG(ERROR) << "Can not read |" << brillo::GetUndecoratedTypeName<string>()
                 << "| from |" << decoded_value->GetUndecoratedTypeName()
                 << ".";
      return nullptr;
    }
    return unique_ptr<string>(new string(decoded_value->Get<string>()));
  } else {
    LOG(ERROR) << "Got unexpected type |" << value_type << "|.";
    return nullptr;
  }
}

unique_ptr<vector<string>> ConvertListValueToStringVector(
    const base::ListValue& list_value) {
  const size_t list_len = list_value.GetSize();
  for (size_t i = 0; i < list_len; ++i) {
    const base::Value* list_item;
    list_value.Get(i, &list_item);
    const auto item_type = list_item->GetType();
    if (item_type != base::Value::TYPE_STRING &&
        item_type != base::Value::TYPE_DICTIONARY) {
      LOG(ERROR) << "Element " << i << " has type " << item_type << ", "
                 << "instead of expected types "
                 << base::Value::TYPE_STRING << "  or "
                 << base::Value::TYPE_DICTIONARY << ".";
      return nullptr;
    }
  }

  unique_ptr<vector<string>> result(new vector<string>);
  for (size_t i = 0; i < list_len; ++i) {
    const base::Value* list_item;
    unique_ptr<string> native_string;
    list_value.Get(i, &list_item);
    native_string = MakeStringFromValue(*list_item);
    if (!native_string) {
      LOG(ERROR) << "Failed to parse string from element " << i << ".";
      return nullptr;
    }
    result->push_back(*native_string);
  }
  return result;
}

unique_ptr<brillo::VariantDictionary>
ConvertDictionaryValueToVariantDictionary(
    const base::DictionaryValue& dictionary_value) {
  base::DictionaryValue::Iterator it(dictionary_value);
  unique_ptr<brillo::VariantDictionary> variant_dictionary(
      new brillo::VariantDictionary());
  while (!it.IsAtEnd()) {
    const string& key = it.key();
    const base::Value& value = it.value();
    switch (value.GetType()) {
      case base::Value::TYPE_NULL:
        LOG(ERROR) << "Key |" << key << "| has unsupported TYPE_NULL.";
        return nullptr;
      case base::Value::TYPE_BOOLEAN: {
        bool native_bool;
        value.GetAsBoolean(&native_bool);
        (*variant_dictionary)[key] = native_bool;
        break;
      }
      case base::Value::TYPE_INTEGER: {
        int native_int;
        value.GetAsInteger(&native_int);
        (*variant_dictionary)[key] = native_int;
        break;
      }
      case base::Value::TYPE_DOUBLE:
        LOG(ERROR) << "Key |" << key << "| has unsupported TYPE_DOUBLE.";
        return nullptr;
      case base::Value::TYPE_STRING: {
        string native_string;
        value.GetAsString(&native_string);
        (*variant_dictionary)[key] = native_string;
        break;
      }
      case base::Value::TYPE_BINARY:
        /* The JSON parser should never create Values of this type. */
        LOG(ERROR) << "Key |" << key << "| has unexpected TYPE_BINARY.";
        return nullptr;
      case base::Value::TYPE_DICTIONARY: {
        const base::DictionaryValue* dictionary_value;
        value.GetAsDictionary(&dictionary_value);
        if (!IsCoercedValue(*dictionary_value)) {
          LOG(ERROR) << "Key |" << key << "| has unsupported TYPE_DICTIONARY.";
          return nullptr;
        }
        unique_ptr<brillo::Any> decoded_coerced_value(
            DecodeCoercedValue(*dictionary_value));
        if (!decoded_coerced_value) {
          LOG(ERROR) << "Key |" << key << "| could not be decoded.";
          return nullptr;
        }
        (*variant_dictionary)[key] = *decoded_coerced_value;
        break;
      }
      case base::Value::TYPE_LIST: {  // Only string lists, for now.
        const base::ListValue* list_value;
        value.GetAsList(&list_value);

        unique_ptr<vector<string>> string_list(
            ConvertListValueToStringVector(*list_value));
        if (!string_list) {
          LOG(ERROR) << "Key |" << key << "| could not be decoded.";
          return nullptr;
        }
        (*variant_dictionary)[key] = *string_list;
        break;
      }
    }
    it.Advance();
  }
  return variant_dictionary;
}

// Serialization helpers.

scoped_ptr<base::DictionaryValue> MakeCoercedValue(
    const string& native_type, const string& encoded_value) {
  auto coerced_value(make_scoped_ptr(new base::DictionaryValue()));
  coerced_value->SetStringWithoutPathExpansion(
      kCoercedValuePropertyNativeType, native_type);
  coerced_value->SetStringWithoutPathExpansion(
      kCoercedValuePropertyEncodedValue, encoded_value);
  return coerced_value;
}

scoped_ptr<base::Value> MakeValueForString(const string& native_string) {
  // Strictly speaking, we don't need to escape non-ASCII text, if
  // that text is UTF-8.  Practically speaking, however, it'll be
  // easier to inspect config files if all non-ASCII strings are
  // presented as byte sequences. (Unicode has many code points with
  // similar-looking glyphs.)
  if (base::IsStringASCII(native_string) &&
      native_string.find('\0') == string::npos) {
    return make_scoped_ptr(new base::StringValue(native_string));
  } else {
    const string hex_encoded_string(
        base::HexEncode(native_string.data(), native_string.size()));
    return MakeCoercedValue(kNativeTypeNonAsciiString, hex_encoded_string);
  }
}

scoped_ptr<base::DictionaryValue> ConvertVariantDictionaryToDictionaryValue(
    const brillo::VariantDictionary& variant_dictionary) {
  auto dictionary_value(make_scoped_ptr(new base::DictionaryValue()));
  for (const auto& key_and_value : variant_dictionary) {
    const auto& key = key_and_value.first;
    const auto& value = key_and_value.second;
    if (value.IsTypeCompatible<bool>()) {
      dictionary_value->SetBooleanWithoutPathExpansion(key, value.Get<bool>());
    } else if (value.IsTypeCompatible<int32_t>()) {
      dictionary_value->SetIntegerWithoutPathExpansion(key, value.Get<int>());
    } else if (value.IsTypeCompatible<string>()) {
      dictionary_value->SetWithoutPathExpansion(
          key, MakeValueForString(value.Get<string>()));
    } else if (value.IsTypeCompatible<uint64_t>()) {
      const string encoded_value(
          base::StringPrintf("%" PRIu64, value.Get<uint64_t>()));
      dictionary_value->SetWithoutPathExpansion(
          key, MakeCoercedValue(kNativeTypeUint64, encoded_value));
    } else if (value.IsTypeCompatible<vector<string>>()) {
      auto list_value(make_scoped_ptr(new base::ListValue()));
      for (const auto& string_list_item : value.Get<vector<string>>()) {
        list_value->Append(MakeValueForString(string_list_item));
      }
      dictionary_value->SetWithoutPathExpansion(key, std::move(list_value));
    } else {
      LOG(ERROR) << "Failed to convert element with key |" << key << "|.";
      return nullptr;
    }
  }
  return dictionary_value;
}

}  // namespace

JsonStore::JsonStore(const base::FilePath& path)
    : path_(path) {
  CHECK(!path_.empty());
}

bool JsonStore::IsNonEmpty() const {
  int64_t file_size = 0;
  return base::GetFileSize(path_, &file_size) && file_size != 0;
}

bool JsonStore::Open() {
  if (!IsNonEmpty()) {
    LOG(INFO) << "Creating a new key file at |" << path_.value() << "|.";
    return true;
  }

  string json_string;
  if (!base::ReadFileToString(path_, &json_string)) {
    LOG(ERROR) << "Failed to read data from |" << path_.value() << "|.";
    return false;
  }

  JSONStringValueDeserializer json_deserializer(json_string);
  unique_ptr<base::Value> json_value;
  string json_error;
  json_deserializer.set_allow_trailing_comma(true);
  json_value.reset(
      json_deserializer.Deserialize(nullptr, &json_error).release());
  if (!json_value) {
    LOG(ERROR) << "Failed to parse JSON data from |" << path_.value() <<"|.";
    SLOG(this, 5) << json_error;
    return false;
  }

  const base::DictionaryValue* root_dictionary;
  if (!json_value->GetAsDictionary(&root_dictionary)) {
    LOG(ERROR) << "JSON value is not a dictionary.";
    return false;
  }

  CHECK(root_dictionary);
  if (root_dictionary->HasKey(kRootPropertyDescription) &&
      !root_dictionary->GetStringWithoutPathExpansion(
          kRootPropertyDescription, &file_description_)) {
    LOG(WARNING) << "Property |" << kRootPropertyDescription
                 << "| is not a string.";
    // Description is non-critical, so continue processing.
  }

  if (!root_dictionary->HasKey(kRootPropertySettings)) {
    LOG(ERROR) << "Property |" << kRootPropertySettings << "| is missing.";
    return false;
  }

  const base::DictionaryValue* settings_dictionary;
  if (!root_dictionary->GetDictionaryWithoutPathExpansion(
          kRootPropertySettings, &settings_dictionary)) {
    LOG(ERROR) << "Property |" << kRootPropertySettings
               << "| is not a dictionary.";
    return false;
  }

  if (!group_name_to_settings_.empty()) {
    LOG(INFO) << "Clearing existing settings on open.";
    group_name_to_settings_.clear();
  }

  base::DictionaryValue::Iterator it(*settings_dictionary);
  while (!it.IsAtEnd()) {
    const string& group_name = it.key();
    const base::DictionaryValue* group_settings_as_values;
    if (!it.value().GetAsDictionary(&group_settings_as_values)) {
      LOG(ERROR) << "Group |" << group_name << "| is not a dictionary.";
      return false;
    }

    unique_ptr<brillo::VariantDictionary> group_settings_as_variants =
        ConvertDictionaryValueToVariantDictionary(*group_settings_as_values);
    if (!group_settings_as_variants) {
      LOG(ERROR) << "Failed to convert group |" << group_name
                 << "| to variants.";
      return false;
    }

    group_name_to_settings_[group_name] = *group_settings_as_variants;
    it.Advance();
  }

  return true;
}

bool JsonStore::Close() {
  return Flush();
}

bool JsonStore::Flush() {
  auto groups(make_scoped_ptr(new base::DictionaryValue()));
  for (const auto& group_name_and_settings : group_name_to_settings_) {
    const auto& group_name = group_name_and_settings.first;
    scoped_ptr<base::DictionaryValue> group_settings(
        ConvertVariantDictionaryToDictionaryValue(
            group_name_and_settings.second));
    if (!group_settings) {
      // This class maintains the invariant that anything placed in
      // |group_settings| is convertible. So abort if conversion fails.
      LOG(FATAL) << "Failed to convert group |" << group_name << "|.";
      return false;
    }
    groups->SetWithoutPathExpansion(group_name, std::move(group_settings));
  }

  base::DictionaryValue root;
  root.SetStringWithoutPathExpansion(
      kRootPropertyDescription, file_description_);
  root.SetWithoutPathExpansion(kRootPropertySettings, std::move(groups));

  string json_string;
  JSONStringValueSerializer json_serializer(&json_string);
  json_serializer.set_pretty_print(true);
  if (!json_serializer.Serialize(root)) {
    LOG(ERROR) << "Failed to serialize to JSON.";
    return false;
  }

  ScopedUmask owner_only_umask(~(S_IRUSR | S_IWUSR) & 0777);
  if (!base::ImportantFileWriter::WriteFileAtomically(path_, json_string)) {
    LOG(ERROR) << "Failed to write JSON file: |" << path_.value() << "|.";
    return false;
  }

  return true;
}

bool JsonStore::MarkAsCorrupted() {
  LOG(INFO) << "In " << __func__ << " for " << path_.value();
  string corrupted_path = path_.value() + kCorruptSuffix;
  int ret = rename(path_.value().c_str(), corrupted_path.c_str());
  if (ret != 0) {
    PLOG(ERROR) << "File rename failed.";
    return false;
  }
  return true;
}

set<string> JsonStore::GetGroups() const {
  set<string> matching_groups;
  for (const auto& group_name_and_settings : group_name_to_settings_) {
    matching_groups.insert(group_name_and_settings.first);
  }
  return matching_groups;
}

// Returns a set so that caller can easily test whether a particular group
// is contained within this collection.
set<string> JsonStore::GetGroupsWithKey(const string& key) const {
  set<string> matching_groups;
  // iterate over groups, find ones with matching key
  for (const auto& group_name_and_settings : group_name_to_settings_) {
    const auto& group_name = group_name_and_settings.first;
    const auto& group_settings = group_name_and_settings.second;
    if (group_settings.find(key) != group_settings.end()) {
      matching_groups.insert(group_name);
    }
  }
  return matching_groups;
}

set<string> JsonStore::GetGroupsWithProperties(const KeyValueStore& properties)
    const {
  set<string> matching_groups;
  const brillo::VariantDictionary& properties_dict(properties.properties());
  for (const auto& group_name_and_settings : group_name_to_settings_) {
    const auto& group_name = group_name_and_settings.first;
    const auto& group_settings = group_name_and_settings.second;
    if (DoesGroupContainProperties(group_settings, properties_dict)) {
      matching_groups.insert(group_name);
    }
  }
  return matching_groups;
}

bool JsonStore::ContainsGroup(const string& group) const {
  const auto& it = group_name_to_settings_.find(group);
  return it != group_name_to_settings_.end();
}

bool JsonStore::DeleteKey(const string& group, const string& key) {
  const auto& group_name_and_settings = group_name_to_settings_.find(group);
  if (group_name_and_settings == group_name_to_settings_.end()) {
    LOG(ERROR) << "Could not find group |" << group << "|.";
    return false;
  }

  auto& group_settings = group_name_and_settings->second;
  auto property_it = group_settings.find(key);
  if (property_it != group_settings.end()) {
    group_settings.erase(property_it);
  }

  return true;
}

bool JsonStore::DeleteGroup(const string& group) {
  auto group_name_and_settings = group_name_to_settings_.find(group);
  if (group_name_and_settings != group_name_to_settings_.end()) {
    group_name_to_settings_.erase(group_name_and_settings);
  }
  return true;
}

bool JsonStore::SetHeader(const string& header) {
  file_description_ = header;
  return true;
}

bool JsonStore::GetString(const string& group,
                          const string& key,
                          string* value) const {
  return ReadSetting(group, key, value);
}

bool JsonStore::SetString(
    const string& group, const string& key, const string& value) {
  return WriteSetting(group, key, value);
}

bool JsonStore::GetBool(const string& group, const string& key, bool* value)
    const {
  return ReadSetting(group, key, value);
}

bool JsonStore::SetBool(const string& group, const string& key, bool value) {
  return WriteSetting(group, key, value);
}

bool JsonStore::GetInt(
    const string& group, const string& key, int* value) const {
  return ReadSetting(group, key, value);
}

bool JsonStore::SetInt(const string& group, const string& key, int value) {
  return WriteSetting(group, key, value);
}

bool JsonStore::GetUint64(
    const string& group, const string& key, uint64_t* value) const {
  return ReadSetting(group, key, value);
}

bool JsonStore::SetUint64(
    const string& group, const string& key, uint64_t value) {
  return WriteSetting(group, key, value);
}

bool JsonStore::GetStringList(
    const string& group, const string& key, vector<string>* value) const {
  return ReadSetting(group, key, value);
}

bool JsonStore::SetStringList(
    const string& group, const string& key, const vector<string>& value) {
  return WriteSetting(group, key, value);
}

bool JsonStore::GetCryptedString(
    const string& group, const string& key, string* value) {
  string encrypted_value;
  if (!GetString(group, key, &encrypted_value)) {
    return false;
  }

  // TODO(quiche): Once we've removed the glib dependency in
  // CryptoProvider, move to using CryptoProvider, instead of
  // CryptoROT47 directly. This change should be done before using
  // JsonStore in production, as the on-disk format of crypted strings
  // will change.
  CryptoROT47 rot47;
  string decrypted_value;
  if (!rot47.Decrypt(encrypted_value, &decrypted_value)) {
    LOG(ERROR) << "Failed to decrypt value for |" << group << "|"
               << ":|" << key << "|.";
    return false;
  }

  if (value) {
    *value = decrypted_value;
  }
  return true;
}

bool JsonStore::SetCryptedString(
    const string& group, const string& key, const string& value) {
  CryptoROT47 rot47;
  string encrypted_value;
  if (!rot47.Encrypt(value, &encrypted_value)) {
    LOG(ERROR) << "Failed to encrypt value for |" << group << "|"
               << ":|" << key << "|.";
    return false;
  }

  return SetString(group, key, encrypted_value);
}

// Private methods.
template<typename T>
bool JsonStore::ReadSetting(
    const string& group, const string& key, T* out) const {
  const auto& group_name_and_settings = group_name_to_settings_.find(group);
  if (group_name_and_settings == group_name_to_settings_.end()) {
    SLOG(this, 10) << "Could not find group |" << group << "|.";
    return false;
  }

  const auto& group_settings = group_name_and_settings->second;
  const auto& property_name_and_value = group_settings.find(key);
  if (property_name_and_value == group_settings.end()) {
    SLOG(this, 10) << "Could not find property |" << key << "|.";
    return false;
  }

  if (!property_name_and_value->second.IsTypeCompatible<T>()) {
    // We assume that the reader and the writer agree on the exact
    // type. So we do not allow implicit conversion.
    LOG(ERROR) << "Can not read |" << brillo::GetUndecoratedTypeName<T>()
               << "| from |"
               << property_name_and_value->second.GetUndecoratedTypeName()
               << "|.";
    return false;
  }

  if (out) {
    return property_name_and_value->second.GetValue(out);
  } else {
    return true;
  }
}

template<typename T>
bool JsonStore::WriteSetting(
    const string& group, const string& key, const T& new_value) {
  auto group_name_and_settings = group_name_to_settings_.find(group);
  if (group_name_and_settings == group_name_to_settings_.end()) {
    group_name_to_settings_[group][key] = new_value;
    return true;
  }

  auto& group_settings = group_name_and_settings->second;
  auto property_name_and_value = group_settings.find(key);
  if (property_name_and_value == group_settings.end()) {
    group_settings[key] = new_value;
    return true;
  }

  if (!property_name_and_value->second.IsTypeCompatible<T>()) {
    SLOG(this, 10) << "New type |" << brillo::GetUndecoratedTypeName<T>()
                   << "| differs from current type |"
                   << property_name_and_value->second.GetUndecoratedTypeName()
                   << "|.";
    return false;
  } else {
    property_name_and_value->second = new_value;
    return true;
  }
}

}  // namespace shill

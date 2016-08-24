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

#include "shill/fake_store.h"

#include <typeinfo>
#include <vector>

#include "shill/logging.h"

using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {

static auto kModuleLogScope = ScopeLogger::kStorage;
static string ObjectID(const FakeStore* j) {
  return "(unknown)";
}

}  // namespace Logging

namespace {

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

}  // namespace

FakeStore::FakeStore() {}

bool FakeStore::IsNonEmpty() const {
  // For now, the choice for return value is arbitrary. Revisit if we
  // find tests depend on this behaving correctly. (i.e., if any tests
  // require this to return true after a Close().)
  return false;
}

bool FakeStore::Open() {
  return true;
}

bool FakeStore::Close() {
  return true;
}

bool FakeStore::Flush() {
  return true;
}

bool FakeStore::MarkAsCorrupted() {
  return true;
}

set<string> FakeStore::GetGroups() const {
  set<string> matching_groups;
  for (const auto& group_name_and_settings : group_name_to_settings_) {
    matching_groups.insert(group_name_and_settings.first);
  }
  return matching_groups;
}

// Returns a set so that caller can easily test whether a particular group
// is contained within this collection.
set<string> FakeStore::GetGroupsWithKey(const string& key) const {
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

set<string> FakeStore::GetGroupsWithProperties(const KeyValueStore& properties)
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

bool FakeStore::ContainsGroup(const string& group) const {
  const auto& it = group_name_to_settings_.find(group);
  return it != group_name_to_settings_.end();
}

bool FakeStore::DeleteKey(const string& group, const string& key) {
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

bool FakeStore::DeleteGroup(const string& group) {
  auto group_name_and_settings = group_name_to_settings_.find(group);
  if (group_name_and_settings != group_name_to_settings_.end()) {
    group_name_to_settings_.erase(group_name_and_settings);
  }
  return true;
}

bool FakeStore::SetHeader(const string& header) {
  return true;
}

bool FakeStore::GetString(const string& group,
                          const string& key,
                          string* value) const {
  return ReadSetting(group, key, value);
}

bool FakeStore::SetString(
    const string& group, const string& key, const string& value) {
  return WriteSetting(group, key, value);
}

bool FakeStore::GetBool(const string& group, const string& key, bool* value)
    const {
  return ReadSetting(group, key, value);
}

bool FakeStore::SetBool(const string& group, const string& key, bool value) {
  return WriteSetting(group, key, value);
}

bool FakeStore::GetInt(
    const string& group, const string& key, int* value) const {
  return ReadSetting(group, key, value);
}

bool FakeStore::SetInt(const string& group, const string& key, int value) {
  return WriteSetting(group, key, value);
}

bool FakeStore::GetUint64(
    const string& group, const string& key, uint64_t* value) const {
  return ReadSetting(group, key, value);
}

bool FakeStore::SetUint64(
    const string& group, const string& key, uint64_t value) {
  return WriteSetting(group, key, value);
}

bool FakeStore::GetStringList(
    const string& group, const string& key, vector<string>* value) const {
  return ReadSetting(group, key, value);
}

bool FakeStore::SetStringList(
    const string& group, const string& key, const vector<string>& value) {
  return WriteSetting(group, key, value);
}

bool FakeStore::GetCryptedString(
    const string& group, const string& key, string* value) {
  return GetString(group, key, value);
}

bool FakeStore::SetCryptedString(
    const string& group, const string& key, const string& value) {
  return SetString(group, key, value);
}

// Private methods.
template<typename T>
bool FakeStore::ReadSetting(
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
bool FakeStore::WriteSetting(
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

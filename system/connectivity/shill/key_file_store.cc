//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/key_file_store.h"

#include <map>

#include <base/files/important_file_writer.h>
#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "shill/key_value_store.h"
#include "shill/logging.h"
#include "shill/scoped_umask.h"

using std::map;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kStorage;
static string ObjectID(const KeyFileStore* k) { return "(key_file_store)"; }
}

namespace {
string ConvertErrorToMessage(GError* error) {
  if (!error) {
    return "Unknown GLib error.";
  }
  string message =
    base::StringPrintf("GError(%d): %s", error->code, error->message);
  g_error_free(error);
  return message;
}
}  // namespace

const char KeyFileStore::kCorruptSuffix[] = ".corrupted";

KeyFileStore::KeyFileStore(const base::FilePath& path)
    : crypto_(),
      key_file_(nullptr),
      path_(path) {
  CHECK(!path_.empty());
}

KeyFileStore::~KeyFileStore() {
  ReleaseKeyFile();
}

void KeyFileStore::ReleaseKeyFile() {
  if (key_file_) {
    g_key_file_free(key_file_);
    key_file_ = nullptr;
  }
}

bool KeyFileStore::IsNonEmpty() const {
  int64_t file_size = 0;
  return base::GetFileSize(path_, &file_size) && file_size != 0;
}

bool KeyFileStore::Open() {
  CHECK(!key_file_);
  crypto_.Init();
  key_file_ = g_key_file_new();
  if (!IsNonEmpty()) {
    LOG(INFO) << "Creating a new key file at " << path_.value();
    return true;
  }
  GError* error = nullptr;
  if (g_key_file_load_from_file(
          key_file_,
          path_.value().c_str(),
          static_cast<GKeyFileFlags>(G_KEY_FILE_KEEP_COMMENTS |
                                     G_KEY_FILE_KEEP_TRANSLATIONS),
          &error)) {
    return true;
  }
  LOG(ERROR) << "Failed to load key file from " << path_.value() << ": "
             << ConvertErrorToMessage(error);
  ReleaseKeyFile();
  return false;
}

bool KeyFileStore::Close() {
  bool success = Flush();
  ReleaseKeyFile();
  return success;
}

bool KeyFileStore::Flush() {
  CHECK(key_file_);
  GError* error = nullptr;
  gsize length = 0;
  gchar* data = g_key_file_to_data(key_file_, &length, &error);

  bool success = true;
  if (!data || error) {
    LOG(ERROR) << "Failed to convert key file to string: "
               << ConvertErrorToMessage(error);
    success = false;
  }
  if (success) {
    ScopedUmask owner_only_umask(~(S_IRUSR | S_IWUSR) & 0777);
    success = base::ImportantFileWriter::WriteFileAtomically(path_, data);
    if (!success) {
      LOG(ERROR) << "Failed to store key file: " << path_.value();
    }
  }
  g_free(data);
  return success;
}

bool KeyFileStore::MarkAsCorrupted() {
  LOG(INFO) << "In " << __func__ << " for " << path_.value();
  string corrupted_path = path_.value() + kCorruptSuffix;
  int ret =  rename(path_.value().c_str(), corrupted_path.c_str());
  if (ret != 0) {
    PLOG(ERROR) << "File rename failed";
    return false;
  }
  return true;
}

set<string> KeyFileStore::GetGroups() const {
  CHECK(key_file_);
  gsize length = 0;
  gchar** groups = g_key_file_get_groups(key_file_, &length);
  if (!groups) {
    LOG(ERROR) << "Unable to obtain groups.";
    return set<string>();
  }
  set<string> group_set(groups, groups + length);
  g_strfreev(groups);
  return group_set;
}

// Returns a set so that caller can easily test whether a particular group
// is contained within this collection.
set<string> KeyFileStore::GetGroupsWithKey(const string& key) const {
  set<string> groups = GetGroups();
  set<string> groups_with_key;
  for (const auto& group : groups) {
    if (g_key_file_has_key(key_file_, group.c_str(), key.c_str(), nullptr)) {
      groups_with_key.insert(group);
    }
  }
  return groups_with_key;
}

set<string> KeyFileStore::GetGroupsWithProperties(
     const KeyValueStore& properties) const {
  set<string> groups = GetGroups();
  set<string> groups_with_properties;
  for (const auto& group : groups) {
    if (DoesGroupMatchProperties(group, properties)) {
      groups_with_properties.insert(group);
    }
  }
  return groups_with_properties;
}

bool KeyFileStore::ContainsGroup(const string& group) const {
  CHECK(key_file_);
  return g_key_file_has_group(key_file_, group.c_str());
}

bool KeyFileStore::DeleteKey(const string& group, const string& key) {
  CHECK(key_file_);
  GError* error = nullptr;
  g_key_file_remove_key(key_file_, group.c_str(), key.c_str(), &error);
  if (error && error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
    LOG(ERROR) << "Failed to delete (" << group << ":" << key << "): "
               << ConvertErrorToMessage(error);
    return false;
  }
  return true;
}

bool KeyFileStore::DeleteGroup(const string& group) {
  CHECK(key_file_);
  GError* error = nullptr;
  g_key_file_remove_group(key_file_, group.c_str(), &error);
  if (error && error->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
    LOG(ERROR) << "Failed to delete group " << group << ": "
               << ConvertErrorToMessage(error);
    return false;
  }
  return true;
}

bool KeyFileStore::SetHeader(const string& header) {
  GError* error = nullptr;
  g_key_file_set_comment(key_file_, nullptr, nullptr, header.c_str(), &error);
  if (error) {
    LOG(ERROR) << "Failed to to set header: "
               << ConvertErrorToMessage(error);
    return false;
  }
  return true;
}

bool KeyFileStore::GetString(const string& group,
                             const string& key,
                             string* value) const {
  CHECK(key_file_);
  GError* error = nullptr;
  gchar* data =
      g_key_file_get_string(key_file_, group.c_str(), key.c_str(), &error);
  if (!data) {
    string s = ConvertErrorToMessage(error);
    SLOG(this, 10) << "Failed to lookup (" << group << ":" << key << "): " << s;
    return false;
  }
  if (value) {
    *value = data;
  }
  g_free(data);
  return true;
}

bool KeyFileStore::SetString(const string& group,
                             const string& key,
                             const string& value) {
  CHECK(key_file_);
  g_key_file_set_string(key_file_, group.c_str(), key.c_str(), value.c_str());
  return true;
}

bool KeyFileStore::GetBool(const string& group,
                           const string& key,
                           bool* value) const {
  CHECK(key_file_);
  GError* error = nullptr;
  gboolean data =
      g_key_file_get_boolean(key_file_, group.c_str(), key.c_str(), &error);
  if (error) {
    string s = ConvertErrorToMessage(error);
    SLOG(this, 10) << "Failed to lookup (" << group << ":" << key << "): " << s;
    return false;
  }
  if (value) {
    *value = data;
  }
  return true;
}

bool KeyFileStore::SetBool(const string& group, const string& key, bool value) {
  CHECK(key_file_);
  g_key_file_set_boolean(key_file_,
                         group.c_str(),
                         key.c_str(),
                         value ? TRUE : FALSE);
  return true;
}

bool KeyFileStore::GetInt(
    const string& group, const string& key, int* value) const {
  CHECK(key_file_);
  GError* error = nullptr;
  gint data =
      g_key_file_get_integer(key_file_, group.c_str(), key.c_str(), &error);
  if (error) {
    string s = ConvertErrorToMessage(error);
    SLOG(this, 10) << "Failed to lookup (" << group << ":" << key << "): " << s;
    return false;
  }
  if (value) {
    *value = data;
  }
  return true;
}

bool KeyFileStore::SetInt(const string& group, const string& key, int value) {
  CHECK(key_file_);
  g_key_file_set_integer(key_file_, group.c_str(), key.c_str(), value);
  return true;
}

bool KeyFileStore::GetUint64(
    const string& group, const string& key, uint64_t* value) const {
  // Read the value in as a string and then convert to uint64_t because glib's
  // g_key_file_set_uint64 appears not to work correctly on 32-bit platforms
  // in unit tests.
  string data_string;
  if (!GetString(group, key, &data_string)) {
    return false;
  }

  uint64_t data;
  if (!base::StringToUint64(data_string, &data)) {
    SLOG(this, 10) << "Failed to convert (" << group << ":" << key << "): "
                   << "string to uint64_t conversion failed";
    return false;
  }

  if (value) {
    *value = data;
  }

  return true;
}

bool KeyFileStore::SetUint64(
    const string& group, const string& key, uint64_t value) {
  // Convert the value to a string first, then save the value because glib's
  // g_key_file_get_uint64 appears not to work on 32-bit platforms in our
  // unit tests.
  return SetString(group, key, base::Uint64ToString(value));
}

bool KeyFileStore::GetStringList(const string& group,
                                 const string& key,
                                 vector<string>* value) const {
  CHECK(key_file_);
  gsize length = 0;
  GError* error = nullptr;
  gchar** data = g_key_file_get_string_list(key_file_,
                                            group.c_str(),
                                            key.c_str(),
                                            &length,
                                            &error);
  if (!data) {
    string s = ConvertErrorToMessage(error);
    SLOG(this, 10) << "Failed to lookup (" << group << ":" << key << "): " << s;
    return false;
  }
  if (value) {
    value->assign(data, data + length);
  }
  g_strfreev(data);
  return true;
}

bool KeyFileStore::SetStringList(const string& group,
                                 const string& key,
                                 const vector<string>& value) {
  CHECK(key_file_);
  vector<const char*> list;
  for (const auto& string_entry : value) {
    list.push_back(string_entry.c_str());
  }
  g_key_file_set_string_list(key_file_,
                             group.c_str(),
                             key.c_str(),
                             list.data(),
                             list.size());
  return true;
}

bool KeyFileStore::GetCryptedString(const string& group,
                                    const string& key,
                                    string* value) {
  if (!GetString(group, key, value)) {
    return false;
  }
  if (value) {
    *value = crypto_.Decrypt(*value);
  }
  return true;
}

bool KeyFileStore::SetCryptedString(const string& group,
                                    const string& key,
                                    const string& value) {
  return SetString(group, key, crypto_.Encrypt(value));
}

bool KeyFileStore::DoesGroupMatchProperties(
    const string& group, const KeyValueStore& properties) const {
  for (const auto& property : properties.properties()) {
    if (property.second.IsTypeCompatible<bool>()) {
      bool value;
      if (!GetBool(group, property.first, &value) ||
          value != property.second.Get<bool>()) {
        return false;
      }
    } else if (property.second.IsTypeCompatible<int32_t>()) {
      int value;
      if (!GetInt(group, property.first, &value) ||
          value != property.second.Get<int32_t>()) {
        return false;
      }
    } else if (property.second.IsTypeCompatible<string>()) {
      string value;
      if (!GetString(group, property.first, &value) ||
          value != property.second.Get<string>()) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace shill

// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/key_value_store.h"

#include <map>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/files/important_file_writer.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <brillo/strings/string_utils.h>
#include <brillo/map_utils.h>

using std::map;
using std::string;
using std::vector;

namespace brillo {

namespace {

// Values used for booleans.
const char kTrueValue[] = "true";
const char kFalseValue[] = "false";

// Returns a copy of |key| with leading and trailing whitespace removed.
string TrimKey(const string& key) {
  string trimmed_key;
  base::TrimWhitespaceASCII(key, base::TRIM_ALL, &trimmed_key);
  CHECK(!trimmed_key.empty());
  return trimmed_key;
}

}  // namespace

bool KeyValueStore::Load(const base::FilePath& path) {
  string file_data;
  if (!base::ReadFileToString(path, &file_data))
    return false;
  return LoadFromString(file_data);
}

bool KeyValueStore::LoadFromString(const std::string& data) {
  // Split along '\n', then along '='.
  vector<string> lines = base::SplitString(data, "\n", base::KEEP_WHITESPACE,
                                           base::SPLIT_WANT_ALL);
  for (auto it = lines.begin(); it != lines.end(); ++it) {
    std::string line;
    base::TrimWhitespaceASCII(*it, base::TRIM_LEADING, &line);
    if (line.empty() || line.front() == '#')
      continue;

    std::string key;
    std::string value;
    if (!string_utils::SplitAtFirst(line, "=", &key, &value, false))
      return false;

    base::TrimWhitespaceASCII(key, base::TRIM_TRAILING, &key);
    if (key.empty())
      return false;

    // Append additional lines to the value as long as we see trailing
    // backslashes.
    while (!value.empty() && value.back() == '\\') {
      ++it;
      if (it == lines.end() || it->empty())
        return false;
      value.pop_back();
      value += *it;
    }

    store_[key] = value;
  }
  return true;
}

bool KeyValueStore::Save(const base::FilePath& path) const {
  return base::ImportantFileWriter::WriteFileAtomically(path, SaveToString());
}

string KeyValueStore::SaveToString() const {
  string data;
  for (const auto& key_value : store_)
    data += key_value.first + "=" + key_value.second + "\n";
  return data;
}

bool KeyValueStore::GetString(const string& key, string* value) const {
  const auto key_value = store_.find(TrimKey(key));
  if (key_value == store_.end())
    return false;
  *value = key_value->second;
  return true;
}

void KeyValueStore::SetString(const string& key, const string& value) {
  store_[TrimKey(key)] = value;
}

bool KeyValueStore::GetBoolean(const string& key, bool* value) const {
  string string_value;
  if (!GetString(key, &string_value))
    return false;

  if (string_value == kTrueValue) {
    *value = true;
    return true;
  } else if (string_value == kFalseValue) {
    *value = false;
    return true;
  }
  return false;
}

void KeyValueStore::SetBoolean(const string& key, bool value) {
  SetString(key, value ? kTrueValue : kFalseValue);
}

std::vector<std::string> KeyValueStore::GetKeys() const {
  return GetMapKeysAsVector(store_);
}

}  // namespace brillo

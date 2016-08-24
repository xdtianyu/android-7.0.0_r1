//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_STUB_STORAGE_H_
#define SHILL_STUB_STORAGE_H_

#include <set>
#include <string>
#include <vector>

#include "shill/store_interface.h"

namespace shill {

// A stub implementation of StoreInterface.
class StubStorage : public StoreInterface {
 public:
  ~StubStorage() override {}

  bool IsNonEmpty() const override { return false; }
  bool Open() override { return false; }
  bool Close() override { return false; }
  bool Flush() override { return false; }
  bool MarkAsCorrupted() override { return false; }
  std::set<std::string> GetGroups() const override { return {}; }
  std::set<std::string> GetGroupsWithKey(
      const std::string& key) const override {
    return {};
  }
  std::set<std::string> GetGroupsWithProperties(
      const KeyValueStore& properties) const override {
    return {};
  }
  bool ContainsGroup(const std::string& group) const override {
    return false;
  }
  bool DeleteKey(const std::string& group, const std::string& key)
      override { return false; }
  bool DeleteGroup(const std::string& group) override { return false; }
  bool SetHeader(const std::string& header) override { return false; }
  bool GetString(const std::string& group,
                 const std::string& key,
                 std::string* value) const override {
    return false;
  }
  bool SetString(const std::string& group,
                 const std::string& key,
                 const std::string& value) override {
    return false;
  }
  bool GetBool(const std::string& group,
               const std::string& key,
               bool* value) const override {
    return false;
  }
  bool SetBool(const std::string& group,
               const std::string& key,
               bool value) override {
    return false;
  }
  bool GetInt(const std::string& group,
              const std::string& key,
              int* value) const override {
    return false;
  }
  bool SetInt(const std::string& group,
              const std::string& key,
              int value) override {
    return false;
  }
  bool GetUint64(const std::string& group,
                 const std::string& key,
                 uint64_t* value) const override {
    return false;
  }
  bool SetUint64(const std::string& group,
                 const std::string& key,
                 uint64_t value) override {
    return false;
  }
  bool GetStringList(const std::string& group,
                     const std::string& key,
                     std::vector<std::string>* value) const override {
    return false;
  }
  bool SetStringList(const std::string& group,
                     const std::string& key,
                     const std::vector<std::string>& value) override {
    return false;
  }
  bool GetCryptedString(const std::string& group,
                        const std::string& key,
                        std::string* value) override {
    return false;
  }
  bool SetCryptedString(const std::string& group,
                        const std::string& key,
                        const std::string& value) override {
    return false;
  }
};

}  // namespace shill

#endif  // SHILL_STUB_STORAGE_H_

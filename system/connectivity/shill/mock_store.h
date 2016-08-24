//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_MOCK_STORE_H_
#define SHILL_MOCK_STORE_H_

#include <set>
#include <string>
#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/key_value_store.h"
#include "shill/store_interface.h"

namespace shill {

class MockStore : public StoreInterface {
 public:
  MockStore();
  ~MockStore() override;

  MOCK_CONST_METHOD0(IsNonEmpty, bool());
  MOCK_METHOD0(Open, bool());
  MOCK_METHOD0(Close, bool());
  MOCK_METHOD0(Flush, bool());
  MOCK_METHOD0(MarkAsCorrupted, bool());
  MOCK_CONST_METHOD0(GetGroups, std::set<std::string>());
  MOCK_CONST_METHOD1(GetGroupsWithKey,
                     std::set<std::string>(const std::string& key));
  MOCK_CONST_METHOD1(GetGroupsWithProperties,
                     std::set<std::string>(const KeyValueStore& properties));
  MOCK_CONST_METHOD1(ContainsGroup, bool(const std::string& group));
  MOCK_METHOD2(DeleteKey, bool(const std::string& group,
                               const std::string& key));
  MOCK_METHOD1(DeleteGroup, bool(const std::string& group));
  MOCK_METHOD1(SetHeader, bool(const std::string& header));
  MOCK_CONST_METHOD3(GetString, bool(const std::string& group,
                                     const std::string& key,
                                     std::string* value));
  MOCK_METHOD3(SetString, bool(const std::string& group,
                               const std::string& key,
                               const std::string& value));
  MOCK_CONST_METHOD3(GetBool, bool(const std::string& group,
                                   const std::string& key,
                                   bool* value));
  MOCK_METHOD3(SetBool, bool(const std::string& group,
                             const std::string& key,
                             bool value));
  MOCK_CONST_METHOD3(GetInt, bool(const std::string& group,
                                  const std::string& key,
                                  int* value));
  MOCK_METHOD3(SetInt, bool(const std::string& group,
                            const std::string& key,
                            int value));
  MOCK_CONST_METHOD3(GetUint64, bool(const std::string& group,
                                     const std::string& key,
                                     uint64_t* value));
  MOCK_METHOD3(SetUint64, bool(const std::string& group,
                               const std::string& key,
                               uint64_t value));
  MOCK_CONST_METHOD3(GetStringList, bool(const std::string& group,
                                         const std::string& key,
                                         std::vector<std::string>* value));
  MOCK_METHOD3(SetStringList, bool(const std::string& group,
                                   const std::string& key,
                                   const std::vector<std::string>& value));
  MOCK_METHOD3(GetCryptedString, bool(const std::string& group,
                                      const std::string& key,
                                      std::string* value));
  MOCK_METHOD3(SetCryptedString, bool(const std::string& group,
                                      const std::string& key,
                                      const std::string& value));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockStore);
};

}  // namespace shill

#endif  // SHILL_MOCK_STORE_H_

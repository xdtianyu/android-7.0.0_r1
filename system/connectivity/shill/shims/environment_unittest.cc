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

#include "shill/shims/environment.h"

#include <base/stl_util.h>
#include <gtest/gtest.h>

using std::map;
using std::string;

namespace shill {

namespace shims {

class EnvironmentTest : public testing::Test {
 public:
  EnvironmentTest() : environment_(Environment::GetInstance()) {}

 protected:
  Environment* environment_;
};

TEST_F(EnvironmentTest, GetVariable) {
  static const char* const kVarValues[] = {
    "VALUE",
    "",
  };
  static const char kVarName[] = "SHILL_SHIMS_GET_VARIABLE_TEST";
  for (size_t i = 0; i < arraysize(kVarValues); i++) {
    EXPECT_FALSE(environment_->GetVariable(kVarName, NULL));
    EXPECT_EQ(0, setenv(kVarName, kVarValues[i], 0)) << kVarValues[i];
    string value;
    EXPECT_TRUE(environment_->GetVariable(kVarName, &value)) << kVarValues[i];
    EXPECT_EQ(kVarValues[i], value);
    EXPECT_EQ(0, unsetenv(kVarName));
  }
}

TEST_F(EnvironmentTest, AsMap) {
  static const char* const kVarNames[] = {
    "SHILL_SHIMS_AS_MAP_TEST_1",
    "SHILL_SHIMS_AS_MAP_TEST_EMPTY",
    "SHILL_SHIMS_AS_MAP_TEST_2",
  };
  static const char* const kVarValues[] = {
    "VALUE 1",
    "",
    "VALUE 2",
  };
  ASSERT_EQ(arraysize(kVarNames), arraysize(kVarValues));
  for (size_t i = 0; i < arraysize(kVarNames); i++) {
    EXPECT_EQ(0, setenv(kVarNames[i], kVarValues[i], 0)) << kVarNames[i];
  }
  map<string, string> env = environment_->AsMap();
  for (size_t i = 0; i < arraysize(kVarNames); i++) {
    EXPECT_TRUE(ContainsKey(env, kVarNames[i])) << kVarNames[i];
    EXPECT_EQ(kVarValues[i], env[kVarNames[i]]) << kVarNames[i];
    EXPECT_EQ(0, unsetenv(kVarNames[i])) << kVarNames[i];
  }
}

}  // namespace shims

}  // namespace shill

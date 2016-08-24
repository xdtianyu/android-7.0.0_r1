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

#ifndef APMANAGER_MOCK_CONFIG_H_
#define APMANAGER_MOCK_CONFIG_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "apmanager/config.h"

namespace apmanager {

class MockConfig : public Config {
 public:
  MockConfig(Manager* manager);
  ~MockConfig() override;

  MOCK_METHOD2(GenerateConfigFile,
               bool(Error* error, std::string* config_str));
  MOCK_METHOD0(ClaimDevice, bool());
  MOCK_METHOD0(ReleaseDevice, bool());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockConfig);
};

}  // namespace apmanager

#endif  // APMANAGER_MOCK_CONFIG_H_

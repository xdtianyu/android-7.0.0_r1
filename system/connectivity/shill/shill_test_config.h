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

#ifndef SHILL_SHILL_TEST_CONFIG_H_
#define SHILL_SHILL_TEST_CONFIG_H_

#include <string>

#include <base/files/scoped_temp_dir.h>

#include "shill/shill_config.h"

namespace shill {

class TestConfig : public Config {
 public:
  TestConfig();
  ~TestConfig() override;

  std::string GetRunDirectory() override;
  std::string GetStorageDirectory() override;

 private:
  base::ScopedTempDir dir_;

  DISALLOW_COPY_AND_ASSIGN(TestConfig);
};

}  // namespace shill

#endif  // SHILL_SHILL_TEST_CONFIG_H_

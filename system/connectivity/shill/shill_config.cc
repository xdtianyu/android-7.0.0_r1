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

#include "shill/shill_config.h"

namespace shill {

// static
const char Config::kDefaultRunDirectory[] = RUNDIR;
// static
#if defined(__ANDROID__)
const char Config::kDefaultStorageDirectory[] = RUNDIR "/default_profiles/";
#else
const char Config::kDefaultStorageDirectory[] = "/var/cache/shill";
#endif  // __ANDROID__
// static
const char Config::kDefaultUserStorageDirectory[] = RUNDIR "/user_profiles/";

Config::Config() {}

Config::~Config() {}

std::string Config::GetRunDirectory() {
  return kDefaultRunDirectory;
}

std::string Config::GetStorageDirectory() {
  return kDefaultStorageDirectory;
}

std::string Config::GetUserStorageDirectory() {
  return kDefaultUserStorageDirectory;
}

}  // namespace shill

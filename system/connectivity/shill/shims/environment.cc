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

#include <cstdlib>
#include <unistd.h>

using std::map;
using std::string;

namespace shill {

namespace shims {

static base::LazyInstance<Environment> g_environment =
    LAZY_INSTANCE_INITIALIZER;

Environment::Environment() {}

Environment::~Environment() {}

// static
Environment* Environment::GetInstance() {
  return g_environment.Pointer();
}

bool Environment::GetVariable(const string& name, string* value) {
  char* v = getenv(name.c_str());
  if (v) {
    *value = v;
    return true;
  }
  return false;
}

map<string, string> Environment::AsMap() {
  map<string, string> env;
  for (char** var = environ; var && *var; var++) {
    string v = *var;
    size_t assign = v.find('=');
    if (assign != string::npos) {
      env[v.substr(0, assign)] = v.substr(assign + 1);
    }
  }
  return env;
}

}  // namespace shims

}  // namespace shill

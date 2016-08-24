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

#ifndef SHILL_SHIMS_ENVIRONMENT_H_
#define SHILL_SHIMS_ENVIRONMENT_H_

#include <map>
#include <string>

#include <base/lazy_instance.h>

namespace shill {

namespace shims {

// Environment access utilities.
class Environment {
 public:
  virtual ~Environment();

  // This is a singleton -- use Environment::GetInstance()->Foo().
  static Environment* GetInstance();

  // Sets |value| to the value of environment variable |name| and returns
  // true. Returns false if variable |name| is not set.
  virtual bool GetVariable(const std::string& name, std::string* value);

  // Parses and returns the environment as a name->value string map.
  virtual std::map<std::string, std::string> AsMap();

 protected:
  Environment();

 private:
  friend struct base::DefaultLazyInstanceTraits<Environment>;

  DISALLOW_COPY_AND_ASSIGN(Environment);
};

}  // namespace shims

}  // namespace shill

#endif  // SHILL_SHIMS_ENVIRONMENT_H_

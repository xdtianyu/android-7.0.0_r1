/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_NATIVEPOWER_DAEMON_SYSTEM_PROPERTY_SETTER_STUB_H_
#define SYSTEM_NATIVEPOWER_DAEMON_SYSTEM_PROPERTY_SETTER_STUB_H_

#include <map>
#include <string>

#include <base/macros.h>

#include "system_property_setter.h"

namespace android {

// A stub implementation of SystemPropertySetterInterface for use by tests.
class SystemPropertySetterStub : public SystemPropertySetterInterface {
 public:
  SystemPropertySetterStub();
  ~SystemPropertySetterStub() override;

  // Returns the value for |key|. An empty string is returned for an unset
  // property.
  std::string GetProperty(const std::string& key) const;

  // SystemPropertySetterInterface:
  bool SetProperty(const std::string& key, const std::string& value) override;

 private:
  std::map<std::string, std::string> properties_;

  DISALLOW_COPY_AND_ASSIGN(SystemPropertySetterStub);
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_DAEMON_SYSTEM_PROPERTY_SETTER_STUB_H_

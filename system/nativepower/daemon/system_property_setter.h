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

#ifndef SYSTEM_NATIVEPOWER_DAEMON_SYSTEM_PROPERTY_SETTER_H_
#define SYSTEM_NATIVEPOWER_DAEMON_SYSTEM_PROPERTY_SETTER_H_

#include <string>

#include <base/macros.h>

namespace android {

// An interface for setting Android system properties.
class SystemPropertySetterInterface {
 public:
  SystemPropertySetterInterface() {}
  virtual ~SystemPropertySetterInterface() {}

  // Sets the property named |key| to |value|, returning true on success.
  virtual bool SetProperty(const std::string& key,
                           const std::string& value) = 0;
};

// The real implementation of SystemPropertySetterInterface.
class SystemPropertySetter : public SystemPropertySetterInterface {
 public:
  SystemPropertySetter();
  ~SystemPropertySetter() override;

  // SystemPropertySetterInterface:
  bool SetProperty(const std::string& key, const std::string& value) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemPropertySetter);
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_DAEMON_SYSTEM_PROPERTY_SETTER_H_

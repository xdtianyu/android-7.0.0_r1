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

#include "system_property_setter_stub.h"

namespace android {

SystemPropertySetterStub::SystemPropertySetterStub() = default;

SystemPropertySetterStub::~SystemPropertySetterStub() = default;

std::string SystemPropertySetterStub::GetProperty(
    const std::string& key) const {
  const auto it = properties_.find(key);
  return it != properties_.end() ? it->second : std::string();
}

bool SystemPropertySetterStub::SetProperty(const std::string& key,
                                           const std::string& value) {
  properties_[key] = value;
  return true;
}

}  // namespace android

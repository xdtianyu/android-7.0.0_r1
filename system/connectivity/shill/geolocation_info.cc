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

#include "shill/geolocation_info.h"

#include <string>

namespace shill {

using std::string;

GeolocationInfo::GeolocationInfo() {
}

GeolocationInfo::~GeolocationInfo() {
}

void GeolocationInfo::AddField(const string& key,
                               const string& value) {
  properties_[key] = value;
}

const string& GeolocationInfo::GetFieldValue(
    const string& key) const {
  return properties_.find(key)->second;
}

bool GeolocationInfo::Equals(const GeolocationInfo& info) const {
  return properties_ == info.properties_;
}

}  // namespace shill

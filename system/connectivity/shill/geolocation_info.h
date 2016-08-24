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

#ifndef SHILL_GEOLOCATION_INFO_H_
#define SHILL_GEOLOCATION_INFO_H_

#include <map>
#include <string>
#include <vector>

#include <gtest/gtest_prod.h>  // for FRIEND_TEST

namespace shill {

class WiFiMainTest;

// This class stores properties (key-value pairs) for a single entity
// (e.g. a WiFi access point) that may be used for geolocation.
class GeolocationInfo {
 public:
  GeolocationInfo();
  ~GeolocationInfo();

  void AddField(const std::string& key, const std::string& value);
  const std::string& GetFieldValue(const std::string& key) const;

  const std::map<std::string, std::string> properties() const {
    return properties_;
  }

 private:
  FRIEND_TEST(WiFiMainTest, GetGeolocationObjects);

  // An equality testing helper for unit tests.
  bool Equals(const GeolocationInfo& info) const;

  std::map<std::string, std::string> properties_;
};

typedef std::vector<GeolocationInfo> GeolocationInfos;

}  // namespace shill

#endif  // SHILL_GEOLOCATION_INFO_H_

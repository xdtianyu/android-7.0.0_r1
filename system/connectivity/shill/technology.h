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

#ifndef SHILL_TECHNOLOGY_H_
#define SHILL_TECHNOLOGY_H_

#include <string>
#include <vector>

namespace shill {

class Error;

// A class that provides functions for converting between technology names
// and identifiers.
class Technology {
 public:
  enum Identifier {
    kEthernet,
    kEthernetEap,
    kWifi,
    kWiFiMonitor,
    kWiMax,
    kCellular,
    kVPN,
    kTunnel,
    kBlacklisted,
    kLoopback,
    kCDCEthernet,  // Only for internal use in DeviceInfo.
    kVirtioEthernet,  // Only for internal use in DeviceInfo.
    kNoDeviceSymlink,  // Only for internal use in DeviceInfo.
    kPPP,
    kPPPoE,
    kUnknown,
  };

  // Returns the technology identifier for a technology name in |name|,
  // or Technology::kUnknown if the technology name is unknown.
  static Identifier IdentifierFromName(const std::string& name);

  // Returns the technology name for a technology identifier in |id|,
  // or Technology::kUnknownName ("Unknown") if the technology identifier
  // is unknown.
  static std::string NameFromIdentifier(Identifier id);

  // Returns the technology identifier for a storage group identifier in
  // |group|, which should have the format of <technology name>_<suffix>,
  // or Technology::kUnknown if |group| is not prefixed with a known
  // technology name.
  static Identifier IdentifierFromStorageGroup(const std::string& group);

  // Converts the comma-separated list of technology names (with no whitespace
  // around commas) in |technologies_string| into a vector of technology
  // identifiers output in |technologies_vector|. Returns true if the
  // |technologies_string| contains a valid set of technologies with no
  // duplicate elements, false otherwise.
  static bool GetTechnologyVectorFromString(
      const std::string& technologies_string,
      std::vector<Identifier>* technologies_vector,
      Error* error);

  // Returns true if |technology| is a primary connectivity technology, i.e.
  // Ethernet, Cellular, WiFi, WiMAX, or PPPoE.
  static bool IsPrimaryConnectivityTechnology(Identifier technology);

 private:
  static const char kLoopbackName[];
  static const char kTunnelName[];
  static const char kPPPName[];
  static const char kPPPoEName[];
  static const char kUnknownName[];
};

}  // namespace shill

#endif  // SHILL_TECHNOLOGY_H_

//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/tethering.h"

#include <base/macros.h>

using std::set;
using std::vector;

namespace shill {

// static
const char Tethering::kAndroidVendorEncapsulatedOptions[] = "ANDROID_METERED";
const uint8_t Tethering::kAndroidBSSIDPrefix[] = {0x02, 0x1a, 0x11};
const uint32_t Tethering::kIosOui = 0x0017f2;
const uint8_t Tethering::kLocallyAdministratedMACBit = 0x02;

// static
bool Tethering::IsAndroidBSSID(const vector<uint8_t>& bssid) {
  vector<uint8_t> truncated_bssid = bssid;
  truncated_bssid.resize(arraysize(kAndroidBSSIDPrefix));
  return truncated_bssid == vector<uint8_t>(
      kAndroidBSSIDPrefix,
      kAndroidBSSIDPrefix + arraysize(kAndroidBSSIDPrefix));
}

// static
bool Tethering::IsLocallyAdministeredBSSID(const vector<uint8_t>& bssid) {
  return bssid.size() > 0 && (bssid[0] & kLocallyAdministratedMACBit);
}

// static
bool Tethering::HasIosOui(const set<uint32_t>& oui_set) {
  return oui_set.find(kIosOui) != oui_set.end();
}

}  // namespace shill

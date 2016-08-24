/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <VehiclePropertyAccessControlForTesting.h>

namespace android {
bool VehiclePropertyAccessControlForTesting::isHexNotation(std::string const& s) {
      return VehiclePropertyAccessControl::isHexNotation(s);
}

bool VehiclePropertyAccessControlForTesting::accessToInt(int32_t* const value,const xmlChar* property,
                   const xmlChar* uid, const xmlChar* access) {
      return VehiclePropertyAccessControl::accessToInt(value, property, uid, access);
}

bool VehiclePropertyAccessControlForTesting::updateOrCreate(int32_t uid, int32_t property, int32_t access) {
      return VehiclePropertyAccessControl::updateOrCreate(uid, property, access);
}

bool VehiclePropertyAccessControlForTesting::populate(xmlNode* a_node) {
      return VehiclePropertyAccessControl::populate(a_node);
}

bool VehiclePropertyAccessControlForTesting::process(const char* policy) {
      return VehiclePropertyAccessControl::process(policy);
}

void VehiclePropertyAccessControlForTesting::emptyAccessControlMap() {
    for (auto& i: VehiclePropertyAccessControl::mVehicleAccessControlMap) {
        delete(&i);
    }

    VehiclePropertyAccessControl::mVehicleAccessControlMap.clear();
}

bool VehiclePropertyAccessControlForTesting::getAccessToProperty(int32_t property, std::map<int32_t, int32_t>** accessMap) {
  if (mVehicleAccessControlMap.count(property) == 0) {
    return false;
  }

  *accessMap = mVehicleAccessControlMap[property];
  return true;
}

};

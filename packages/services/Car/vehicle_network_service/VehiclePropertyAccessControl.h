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

#ifndef CAR_VEHICLE_PROPERTY_ACCESS_CONTROL_H_
#define CAR_VEHICLE_PROPERTY_ACCESS_CONTROL_H_

#include <utils/String8.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <map>
#include <string>
#include <private/android_filesystem_config.h>
#include <vehicle-internal.h>

namespace android {
// This class is used to gate access to properties that are defined in an XML
// file. The property are read from /system/etc/vns/vns_policy.xml and this xml
// file must exist. If not, an error is generated. If the optional
// vendor_vns_policy.xml file is found in the same directory, properties from
// that file are also loaded to extend or override the properties from the
// vns_policy.xml file.
class VehiclePropertyAccessControl {
public:
    VehiclePropertyAccessControl();
    ~VehiclePropertyAccessControl();
    bool init();
    bool testAccess(int32_t property, int32_t uid, bool isWrite);
    void dump(String8& msg);
// protected for testing
protected:
    bool isHexNotation(std::string const& s);
    bool accessToInt(int32_t* const value,const xmlChar* property,
                   const xmlChar* uid, const xmlChar* access);
    bool updateOrCreate(int32_t uid, int32_t property, int32_t access);
    bool populate(xmlNode* a_node);
    bool process(const char* policy);
// protected for testing
protected:
    // mVehicleAccessControlMap uses "property" as a key to map to map<int,int>*
    //
    // map<int,int> uses "uid" as a key to map to an integer that represents
    // "access"
    //
    // So "property" is used to find "uid" and "uid" is used to find "access".
    std::map<int32_t, std::map<int32_t, int32_t>*> mVehicleAccessControlMap;
};

};

#endif /* CAR_VEHICLE_NETWORK_PROPERTY_CONFIG_H_ */

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

#define LOG_TAG "VehiclePropertyAccessControl"
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <IVehicleNetwork.h>
#include "VehiclePropertyAccessControl.h"
#include <hardware/vehicle.h>
#include <private/android_filesystem_config.h>
#include <vehicle-internal.h>

//#define DBG_EVENT
//#define DBG_VERBOSE
#ifdef DBG_EVENT
#define EVENT_LOG(x...) ALOGD(x)
#else
#define EVENT_LOG(x...)
#endif
#ifdef DBG_VERBOSE
#define LOG_VERBOSE(x...) ALOGD(x)
#else
#define LOG_VERBOSE(x...)
#endif


namespace android {

VehiclePropertyAccessControl::VehiclePropertyAccessControl() {
}

VehiclePropertyAccessControl::~VehiclePropertyAccessControl() {
    int index;
    int size;

    for (auto& i: mVehicleAccessControlMap) {
        delete(&i);
    }

    mVehicleAccessControlMap.clear();
}

// Returns true if the given string, s, is a hex number that starts with 0x.
// Otherwise false is returned.
bool VehiclePropertyAccessControl::isHexNotation(std::string const& s) {
    return s.compare(0, 2, "0x") == 0
            && s.size() > 2
            && s.find_first_not_of("0123456789abcdefABCDEF", 2)
            == std::string::npos;
}

// Converts the string representation, access, to an integer form and store it
// in value. true is returned if the parameter, access, is "r", "w", "rw" or
// "wr". Otherwise false is returned. The parameters property and uid are
// only used for logging in the event that the string, access, was not
// recognized.
bool VehiclePropertyAccessControl::accessToInt(int32_t* const value,
                                               const xmlChar* property,
                                               const xmlChar* uid,
                                               const xmlChar* access) {
    if (!value || !property || !uid || !access) {
        ALOGE("Internal Error\n");
        return false;
    }

    if (xmlStrcmp(access, (const xmlChar *)"r") == 0) {
        *value = VEHICLE_PROP_ACCESS_READ;
    }
    else if (xmlStrcmp(access, (const xmlChar *)"w") == 0) {
        *value = VEHICLE_PROP_ACCESS_WRITE;
    }
    else if ((xmlStrcmp(access, (const xmlChar *)"rw") == 0)
            || (xmlStrcmp(access, (const xmlChar *)"wr") == 0)) {
        *value = VEHICLE_PROP_ACCESS_READ_WRITE;
    }
    else {
        ALOGE("Unknown access tag %s for UID %s in PROPERTY %s\n",access, uid,
              property);
        return false;
    }

    return true;
}

// Adds the property/uid pair to the mVehicleAccessControlMap map if the pair
// doesn't already exist. If the pair does exist, the access is updated.
bool VehiclePropertyAccessControl::updateOrCreate(int32_t uid, int32_t property,
                                                  int32_t access) {
    // check if property exists
    if (mVehicleAccessControlMap.count(property) == 0) {
        std::map<int32_t, int32_t>* uid_access =
                new std::map<int32_t, int32_t>();
        mVehicleAccessControlMap[property] = uid_access;
    }

    // Get the propertyAccessMap
    std::map<int32_t, int32_t>* uidAccessMap =
            mVehicleAccessControlMap[property];

    // Now check if uid exists
    if (uidAccessMap->count(uid) == 0) {
        (*uidAccessMap)[uid] = access;
        // uid was not found
        return false;
    }

    // The Property, Uid pair exist. So update the access
    (*uidAccessMap)[uid] = access;

    return true;
}

// Start parsing the xml file and populating the mVehicleAccessControlMap
// map. The parameter, a_node, must point to the first <PROPERTY> tag.
// true is returned if the parsing completed else false.
bool VehiclePropertyAccessControl::populate(xmlNode * a_node) {
    xmlNode* cur_node = NULL;
    xmlNode* child = NULL;
    xmlChar* property = NULL;
    xmlChar* property_value_str = NULL;
    xmlChar* uid = NULL;
    xmlChar* uid_value_str = NULL;
    xmlChar* access = NULL;
    int32_t property_value;
    int32_t uid_value;
    int32_t access_value;

    if (!a_node) {
        ALOGE("Internal Error");
        return false;
    }

    // Loop over all the PROPERTY tags
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if ((xmlStrcmp(cur_node->name, (const xmlChar *)"PROPERTY") == 0) &&
                (cur_node->type == XML_ELEMENT_NODE)) {
            // Free the old property tag
            xmlFree(property);
            // get new property tag name attribute
            property = xmlGetProp(cur_node, (const xmlChar *)"name");
            if (!property) {
                ALOGE("PROPERTY given without name attribute");
                continue;
            }

            // get new property tag value attribute
            property_value_str = xmlGetProp(cur_node, (const xmlChar*)"value");
            if (!property_value_str) {
                ALOGE("PROPERTY given without value attribute");
                continue;
            }

            std::string tmp_str((const char*)property_value_str);
            if (isHexNotation(tmp_str)) {
                property_value = std::stoul(tmp_str, nullptr, 16);
            } else {
                property_value = std::stoul(tmp_str, nullptr, 10);
            }

            // Loop over all UID tags
            for (child = cur_node->children; child; child = child->next) {
                if ((xmlStrcmp(child->name, (const xmlChar*)"UID")==0) &&
                        (child->type == XML_ELEMENT_NODE)) {
                    if (property != NULL) {
                        // Free the old uid tag
                        xmlFree(uid);
                        // Free the old access tag
                        xmlFree(access);
                        // get new uid tag
                        uid = xmlGetProp(child, (const xmlChar*)"name");
                        // get new uid tag
                        uid_value_str = xmlGetProp(child,
                                                   (const xmlChar*)"value");
                        // get new access tag
                        access = xmlGetProp(child, (const xmlChar *)"access");

                        if (uid == NULL) {
                            ALOGE(
                                "UID tag for property %s given without name attribute\n",
                                property);
                        } else if (uid_value_str == NULL) {
                            ALOGE(
                                "UID tag for property %s given without value attribute\n",
                                property);
                        } else if (access == NULL) {
                            ALOGE(
                                "UID tag for property %s given without access attribute\n",
                                property);
                        } else {
                            std::string tmp_str((const char *)uid_value_str);
                            if (isHexNotation(tmp_str)) {
                                uid_value = std::stoul(tmp_str, nullptr, 16);
                            } else {
                                uid_value = std::stoul(tmp_str, nullptr, 10);
                            }

                            bool re1 = accessToInt(&access_value, property, uid,
                                                   access);
                            if (re1) {
                                if (!updateOrCreate(uid_value, property_value,
                                                    access_value)) {
                                    LOG_VERBOSE(
                                        "Property %08x was added: uid=%d access=%d\n",
                                        property_value, uid_value, access_value);
                                } else {
                                    LOG_VERBOSE("Property %08x was updated: uid=%d access=%d\n",
                                          property_value, uid_value, access_value);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    xmlFree(property);
    xmlFree(uid);
    xmlFree(access);

    return true;
}

// This method initializes the class by parsing the mandatory
// /system/etc/vns/vns_policy.xml file and then the optional
// /system/etc/vns/vendor_vns_policy.xml if found.
// false is returned if vns_policy.xml was not found or is
// invalid else true is returned.
bool VehiclePropertyAccessControl::init() {
    static const char* default_policy = "/system/etc/vns/vns_policy.xml";
    static const char* vendor_policy = "/system/etc/vns/vendor_vns_policy.xml";

    if (!process(default_policy)) {
        return false;
    }

    if (process(vendor_policy)) {
        ALOGE("Vendor VNS Policy was applied\n");
    }

    return true;
}

// Processes the vns_policy.xml or vendor_vns_policy.xml files
// and returns true on success else false is returned.
bool VehiclePropertyAccessControl::process(const char* policy) {
    xmlDoc* doc = NULL;
    xmlNode* root_element = NULL;

    doc = xmlReadFile(policy, NULL, 0);
    if (doc == NULL) {
        ALOGE("Could not find %s\n", policy);
        return false;
    }

    root_element = xmlDocGetRootElement(doc);
    if (!root_element) {
        ALOGE("Not a valid config file %s\n", policy);
        xmlFreeDoc(doc);
        return false;
    }

    if (xmlStrcmp(root_element->name, (const xmlChar *)"ALLOW") != 0) {
        ALOGE("Not a valid config file %s\n", policy);
        xmlFreeDoc(doc);
        return false;
    }

    bool ret = populate(root_element->children);

    xmlFreeDoc(doc);

    return ret;
}

void VehiclePropertyAccessControl::dump(String8& msg) {
    std::string perm;
    int32_t property;
    int32_t uid;
    int32_t access;
    std::map<int32_t, int32_t> *uid_access_map;

    for (auto& i: mVehicleAccessControlMap) {
        property = i.first;
        uid_access_map = mVehicleAccessControlMap[property];
        for (auto& j: *uid_access_map) {
            uid = j.first;
            access = (*uid_access_map)[uid];
            switch(access) {
                case VEHICLE_PROP_ACCESS_READ: perm = "read"; break;
                case VEHICLE_PROP_ACCESS_WRITE: perm = "write"; break;
                case VEHICLE_PROP_ACCESS_READ_WRITE: perm = "read/write"; break;
                default: perm="unknown";
            }
            msg.appendFormat("UID %d: property 0x%08x, access %s\n", uid,
                             property, perm.c_str());
        }
    }
}

// Test if the given uid has (read or write) access to the given property. If it
// does, true is returned and false is returned if it doesn't have access or the
// property or uid is unknown.
bool VehiclePropertyAccessControl::testAccess(int32_t property, int32_t uid,
                                              bool isWrite) {
    // Check if the property exists
    if (mVehicleAccessControlMap.count(property) == 0) {
        // property was not found
        return false;
    }

    // Get the uidAccessMap
    std::map<int32_t, int32_t>* uidAccessMap =
            mVehicleAccessControlMap[property];

    // Now check if uid exists
    if (uidAccessMap->count(uid) == 0) {
        // uid was not found
        return false;
    }

    // Get Access to this Property
    int32_t access = (*uidAccessMap)[uid];

    // Test if the UID has access to the property
    if (isWrite) {
        if ((access == VEHICLE_PROP_ACCESS_WRITE)
                || (access == VEHICLE_PROP_ACCESS_READ_WRITE)) {
            return true;
        } else {
            return false;
        }
    } else {
        if ((access == VEHICLE_PROP_ACCESS_READ)
                || (access == VEHICLE_PROP_ACCESS_READ_WRITE)) {
            return true;
        } else {
            return false;
        }
    }
}

};

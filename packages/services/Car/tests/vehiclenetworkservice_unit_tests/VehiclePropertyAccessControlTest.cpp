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

#include <unistd.h>

#include <gtest/gtest.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <VehiclePropertyAccessControlForTesting.h>

namespace android {

class VehiclePropertyAccessControlTest : public testing::Test {
public:
    VehiclePropertyAccessControlTest() {}
    ~VehiclePropertyAccessControlTest() {}

public:
    static std::string xmlData;
    static std::string xmlData2;
    static const int32_t prop1;
    static const int32_t prop2;
    static const int32_t prop3;
    static const int32_t uid1;
    static const int32_t uid2;
    static const int32_t uid3;

protected:
    void SetUp() {}

protected:
    xmlDoc* doc;
    VehiclePropertyAccessControlForTesting mVehiclePropertyAccessControl;
};

std::string VehiclePropertyAccessControlTest::xmlData =
                            "<ALLOW>"
                            "<PROPERTY name=\"PROP1\" value=\"0xA\">"
                            "<UID name=\"UID1\" access=\"r\" value=\"1000\"/>"
                            "</PROPERTY>"
                            "<PROPERTY name=\"PROP2\" value=\"0xB\">"
                            "<UID name=\"UID2\" access=\"w\" value=\"2000\"/>"
                            "</PROPERTY>"
                            "<PROPERTY name=\"PROP3\" value=\"0xC\">"
                            "<UID name=\"UID3\" access=\"rw\" value=\"3000\"/>"
                            "</PROPERTY>"
                            "</ALLOW>";

const int32_t VehiclePropertyAccessControlTest::prop1 = 0xa;
const int32_t VehiclePropertyAccessControlTest::prop2 = 0xb;
const int32_t VehiclePropertyAccessControlTest::prop3 = 0xc;
const int32_t VehiclePropertyAccessControlTest::uid1 = 1000;
const int32_t VehiclePropertyAccessControlTest::uid2 = 2000;
const int32_t VehiclePropertyAccessControlTest::uid3 = 3000;

TEST_F(VehiclePropertyAccessControlTest, isHexNotation) {
    static const std::string shouldPass[] =
        {"0x01234567",
         "0x01abcdef",
         "0x01ABCDEF",
         "0x0"};

    static const std::string shouldFail[] =
        {"0",
         "0x",
         "01234567",
         "ABCDEF01",
         "0xabi"};

    for(auto& h : shouldPass) {
        ASSERT_TRUE(mVehiclePropertyAccessControl.isHexNotation(h));
    }

    for(auto& h : shouldFail) {
        ASSERT_FALSE(mVehiclePropertyAccessControl.isHexNotation(h));
    }
}

TEST_F(VehiclePropertyAccessControlTest, accessToInt) {
    static const char* property = "property";
    static const char* uid = "uid";
    struct ShouldPassType {std::string str; int32_t value;};
    static const struct ShouldPassType shouldPass[] = {
            {"r", VEHICLE_PROP_ACCESS_READ},
            {"w", VEHICLE_PROP_ACCESS_WRITE},
            {"rw", VEHICLE_PROP_ACCESS_READ_WRITE},
            {"wr", VEHICLE_PROP_ACCESS_READ_WRITE}
    };
    static const char* shouldFail[] = {"rr", "ww", "rww", "rwr", "", "k"};
    int32_t value;

    for(auto& h : shouldPass) {
        ASSERT_TRUE(mVehiclePropertyAccessControl.accessToInt(&value,
            (const xmlChar*)property, (const xmlChar*)uid,
            (const xmlChar*)h.str.c_str()));
        ASSERT_EQ(h.value, value);
    }

    for(auto& h : shouldFail) {
        ASSERT_FALSE(mVehiclePropertyAccessControl.accessToInt(&value,
            (const xmlChar*)property, (const xmlChar*)uid, (const xmlChar*)h));
    }
}

TEST_F(VehiclePropertyAccessControlTest, updateOrCreate) {
    std::map<int32_t, int32_t> *accessMap;

    // Empty the map
    mVehiclePropertyAccessControl.emptyAccessControlMap();

    // Make sure the property does not exist
    ASSERT_FALSE(mVehiclePropertyAccessControl.getAccessToProperty(prop1,
                                                                   &accessMap));

    // Create the property and give uid read access
    ASSERT_FALSE(mVehiclePropertyAccessControl.updateOrCreate(uid1, prop1,
                                                 VEHICLE_PROP_ACCESS_READ));

    // Make sure the property was created
    ASSERT_TRUE(mVehiclePropertyAccessControl.getAccessToProperty(prop1,
                                                                   &accessMap));

    // Make sure uid has read access to the property
    ASSERT_EQ((*accessMap)[uid1], VEHICLE_PROP_ACCESS_READ);

    // Give uid2 read/write access to the property
    ASSERT_FALSE(mVehiclePropertyAccessControl.updateOrCreate(uid2, prop1,
                                                VEHICLE_PROP_ACCESS_READ_WRITE));

    // Get the accessMap
    ASSERT_TRUE(mVehiclePropertyAccessControl.getAccessToProperty(prop1,
                                                                   &accessMap));
    // Make sure uid2 has read/write access to the property
    ASSERT_EQ((*accessMap)[uid2], VEHICLE_PROP_ACCESS_READ_WRITE);

    // Make sure uid still has read access to the property
    ASSERT_EQ((*accessMap)[uid1], VEHICLE_PROP_ACCESS_READ);

    // Change uid access to write for property
    ASSERT_TRUE(mVehiclePropertyAccessControl.updateOrCreate(uid1, prop1,
                                                     VEHICLE_PROP_ACCESS_WRITE));

    // Get the accessMap
    ASSERT_TRUE(mVehiclePropertyAccessControl.getAccessToProperty(prop1,
                                                                   &accessMap));

    // Make sure uid has write access to property
    ASSERT_EQ((*accessMap)[uid1], VEHICLE_PROP_ACCESS_WRITE);

    // Make sure uid2 has read write access to property
    ASSERT_EQ((*accessMap)[uid2], VEHICLE_PROP_ACCESS_READ_WRITE);
}

TEST_F(VehiclePropertyAccessControlTest, populate) {
    xmlNode* root_element;
    std::map<int32_t, int32_t> *accessMap;

    // Empty the map
    mVehiclePropertyAccessControl.emptyAccessControlMap();

    doc = xmlReadMemory(xmlData.c_str(), xmlData.length(), NULL, NULL, 0);
    ASSERT_TRUE(doc != NULL);
    root_element = xmlDocGetRootElement(doc);
    ASSERT_TRUE(root_element != NULL);

    bool result = mVehiclePropertyAccessControl.populate(root_element->children);

    ASSERT_TRUE(result);

    // Get the accessMap
    ASSERT_TRUE(mVehiclePropertyAccessControl.getAccessToProperty(prop1,
                                                                  &accessMap));

    // Make sure uid still has read access to the property
    ASSERT_EQ((*accessMap)[uid1], VEHICLE_PROP_ACCESS_READ);

    // Get the accessMap
    ASSERT_TRUE(mVehiclePropertyAccessControl.getAccessToProperty(prop2,
                                                                  &accessMap));

    // Make sure uid still has write access to the property
    ASSERT_EQ((*accessMap)[uid2], VEHICLE_PROP_ACCESS_WRITE);

    ASSERT_TRUE(mVehiclePropertyAccessControl.testAccess(prop1, uid1, 0));
    ASSERT_FALSE(mVehiclePropertyAccessControl.testAccess(prop1, uid1, 1));
    ASSERT_TRUE(mVehiclePropertyAccessControl.testAccess(prop2, uid2, 1));
    ASSERT_FALSE(mVehiclePropertyAccessControl.testAccess(prop2, uid2, 0));
    ASSERT_TRUE(mVehiclePropertyAccessControl.testAccess(prop3, uid3, 1));
    ASSERT_TRUE(mVehiclePropertyAccessControl.testAccess(prop3, uid3, 0));

    static const std::string dump_msg =
            "UID 1000: property 0x0000000a, access read\n"
            "UID 2000: property 0x0000000b, access write\n"
            "UID 3000: property 0x0000000c, access read/write\n";

    String8 msg;
    mVehiclePropertyAccessControl.dump(msg);

    ASSERT_EQ(dump_msg.compare(msg.string()), 0);

}

TEST_F(VehiclePropertyAccessControlTest, init) {
    xmlFreeDoc(doc);
    xmlCleanupParser();
    // Empty the map
    mVehiclePropertyAccessControl.emptyAccessControlMap();
    ASSERT_TRUE(mVehiclePropertyAccessControl.init());
}
}; // namespace android

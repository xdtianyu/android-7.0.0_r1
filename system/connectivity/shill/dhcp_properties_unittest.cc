//
// Copyright (C) 2015 The Android Open Source Project
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

#include "shill/dhcp_properties.h"

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/mock_property_store.h"
#include "shill/mock_store.h"

using std::string;
using std::unique_ptr;
using testing::_;
using testing::DoAll;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

namespace shill {

namespace {
const char kVendorClass[] = "Chromebook";
const char kHostname[] = "TestHost";
const char kStorageID[] = "dhcp_service_id";
const char kOverrideValue[] = "override";
}

class DhcpPropertiesTest : public Test {
 public:
  DhcpPropertiesTest() { }

  virtual ~DhcpPropertiesTest() { }

 protected:
  DhcpProperties dhcp_properties_;
};

TEST_F(DhcpPropertiesTest, Ctor) {
  EXPECT_TRUE(dhcp_properties_.properties_.IsEmpty());
}

TEST_F(DhcpPropertiesTest, InitPropertyStore) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);

  Error error;
  string value_in_prop_store;
  // DHCPProperty.Hostname is a valid option.
  EXPECT_FALSE(store.GetStringProperty("DHCPProperty.Hostname",
                                       &value_in_prop_store,
                                       &error));
  EXPECT_EQ(Error::kNotFound, error.type());

  // DHCPProperty.VendorClass is a valid option.
  EXPECT_FALSE(store.GetStringProperty("DHCPProperty.VendorClass",
                                       &value_in_prop_store,
                                       &error));
  EXPECT_EQ(Error::kNotFound, error.type());

  // DhcpProperty.NotAProp is not a valid option.
  EXPECT_FALSE(store.GetStringProperty("DHCPProperty.NotAProp",
                                       &value_in_prop_store,
                                       &error));
  EXPECT_EQ(Error::kInvalidProperty, error.type());
}

TEST_F(DhcpPropertiesTest, SetMappedStringPropertyOverrideExisting) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);
  dhcp_properties_.properties_.SetString("Hostname", kHostname);

  Error error;
  EXPECT_TRUE(store.SetStringProperty("DHCPProperty.Hostname", kOverrideValue, &error));
  EXPECT_EQ(kOverrideValue, dhcp_properties_.properties_.GetString("Hostname"));
}

TEST_F(DhcpPropertiesTest, SetMappedStringPropertyNoExistingValue) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);

  Error error;
  EXPECT_TRUE(store.SetStringProperty("DHCPProperty.Hostname", kHostname, &error));
  EXPECT_EQ(kHostname, dhcp_properties_.properties_.GetString("Hostname"));
}

TEST_F(DhcpPropertiesTest, SetMappedStringPropertySameAsExistingValue) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);
  dhcp_properties_.properties_.SetString("Hostname", kHostname);

  Error error;
  EXPECT_FALSE(store.SetStringProperty("DHCPProperty.Hostname", kHostname, &error));
  EXPECT_EQ(kHostname, dhcp_properties_.properties_.GetString("Hostname"));
}

TEST_F(DhcpPropertiesTest, GetMappedStringPropertyWithSetValue) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);
  dhcp_properties_.properties_.SetString("Hostname", kHostname);

  Error error;
  string value_in_prop_store;
  store.GetStringProperty("DHCPProperty.Hostname", &value_in_prop_store, &error);
  EXPECT_EQ(kHostname, value_in_prop_store);
}

TEST_F(DhcpPropertiesTest, GetMappedStringPropertyNoExistingValue) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);

  Error error;
  string value_in_prop_store;
  store.GetStringProperty("DHCPProperty.Hostname", &value_in_prop_store, &error);
  EXPECT_EQ(Error::kNotFound, error.type());
}

TEST_F(DhcpPropertiesTest, ClearMappedStringPropertyWithSetValue) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);
  dhcp_properties_.properties_.SetString("Hostname", kHostname);

  Error error;
  string value_in_prop_store;
  store.ClearProperty("DHCPProperty.Hostname", &error);
  EXPECT_FALSE(dhcp_properties_.properties_.ContainsString("Hostname"));
}

TEST_F(DhcpPropertiesTest, ClearMappedStringPropertyNoExistingValue) {
  PropertyStore store;
  dhcp_properties_.InitPropertyStore(&store);

  Error error;
  string value_in_prop_store;
  store.ClearProperty("DHCPProperty.Hostname", &error);
  EXPECT_EQ(Error::kNotFound, error.type());
}

TEST_F(DhcpPropertiesTest, LoadEmpty) {
  MockStore storage;
  EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
      .WillOnce(Return(false));
  EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
      .WillOnce(Return(false));
  dhcp_properties_.Load(&storage, kStorageID);
  EXPECT_TRUE(dhcp_properties_.properties_.IsEmpty());
}

TEST_F(DhcpPropertiesTest, Load) {
  MockStore storage;
  EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kVendorClass)),
                      Return(true)));
  EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kHostname)),
                      Return(true)));
  dhcp_properties_.Load(&storage, kStorageID);
  EXPECT_EQ(kVendorClass,
            dhcp_properties_.properties_.GetString("VendorClass"));
  EXPECT_EQ(kHostname, dhcp_properties_.properties_.GetString("Hostname"));
}

TEST_F(DhcpPropertiesTest, LoadWithValuesSetAndClearRequired) {
  MockStore storage;
  dhcp_properties_.properties_.SetString("Hostname", kHostname);

  EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.VendorClass", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kVendorClass)),
                      Return(true)));
  EXPECT_CALL(storage, GetString(kStorageID, "DHCPProperty.Hostname", _))
      .WillOnce(Return(false));
  dhcp_properties_.Load(&storage, kStorageID);
  EXPECT_EQ(kVendorClass, dhcp_properties_.properties_.GetString("VendorClass"));
  EXPECT_FALSE(dhcp_properties_.properties_.Contains("Hostname"));
}

TEST_F(DhcpPropertiesTest, SaveWithValuesSet) {
  MockStore storage;
  dhcp_properties_.properties_.SetString("VendorClass", kVendorClass);
  dhcp_properties_.properties_.SetString("Hostname", "");

  EXPECT_CALL(storage,
              SetString(kStorageID, "DHCPProperty.VendorClass", kVendorClass))
      .WillOnce(Return(true));
  EXPECT_CALL(storage,
              SetString(kStorageID, "DHCPProperty.Hostname", ""))
      .WillOnce(Return(true));
  dhcp_properties_.Save(&storage, kStorageID);
}

TEST_F(DhcpPropertiesTest, SavePropertyNotSetShouldBeDeleted) {
  MockStore storage;
  dhcp_properties_.properties_.SetString("VendorClass", kVendorClass);

  EXPECT_CALL(storage, SetString(_, _, _)).Times(0);
  EXPECT_CALL(storage,
              SetString(kStorageID, "DHCPProperty.VendorClass", kVendorClass))
      .WillOnce(Return(true));
  EXPECT_CALL(storage,
              DeleteKey(kStorageID, "DHCPProperty.Hostname"))
      .WillOnce(Return(true));
  dhcp_properties_.Save(&storage, kStorageID);
}

TEST_F(DhcpPropertiesTest, CombineIntoEmpty) {
  DhcpProperties to_merge;
  to_merge.properties_.SetString("VendorClass", kVendorClass);
  to_merge.properties_.SetString("Hostname", kHostname);

  unique_ptr<DhcpProperties> merged_props =
      DhcpProperties::Combine(dhcp_properties_, to_merge);
  EXPECT_EQ(merged_props->properties_, to_merge.properties_);
}

TEST_F(DhcpPropertiesTest, CombineEmptyIntoExisting) {
  DhcpProperties to_merge;
  dhcp_properties_.properties_.SetString("VendorClass", kVendorClass);
  dhcp_properties_.properties_.SetString("Hostname", kHostname);

  unique_ptr<DhcpProperties> merged_props =
      DhcpProperties::Combine(dhcp_properties_, to_merge);
  EXPECT_EQ(merged_props->properties_, dhcp_properties_.properties_);
}

TEST_F(DhcpPropertiesTest, CombineConflicting) {
  DhcpProperties to_merge;
  to_merge.properties_.SetString("VendorClass", kOverrideValue);
  to_merge.properties_.SetString("Hostname", kHostname);
  dhcp_properties_.properties_.SetString("VendorClass", kVendorClass);

  unique_ptr<DhcpProperties> merged_props =
      DhcpProperties::Combine(dhcp_properties_, to_merge);
  EXPECT_EQ(kOverrideValue, merged_props->properties_.GetString("VendorClass"));
  EXPECT_EQ(kHostname, merged_props->properties_.GetString("Hostname"));
}

TEST_F(DhcpPropertiesTest, GetValueForProperty) {
  string value;
  EXPECT_FALSE(dhcp_properties_.GetValueForProperty("VendorClass", &value));
  EXPECT_FALSE(dhcp_properties_.GetValueForProperty("Hostname", &value));

  dhcp_properties_.properties_.SetString("VendorClass", kVendorClass);
  EXPECT_TRUE(dhcp_properties_.GetValueForProperty("VendorClass", &value));
  EXPECT_EQ(kVendorClass, value);
  EXPECT_FALSE(dhcp_properties_.GetValueForProperty("Hostname", &value));

  dhcp_properties_.properties_.SetString("Hostname", kHostname);
  EXPECT_TRUE(dhcp_properties_.GetValueForProperty("VendorClass", &value));
  EXPECT_EQ(kVendorClass, value);
  EXPECT_TRUE(dhcp_properties_.GetValueForProperty("Hostname", &value));
  EXPECT_EQ(kHostname, value);
}

} // namespace shill

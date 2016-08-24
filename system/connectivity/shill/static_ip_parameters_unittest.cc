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

#include "shill/static_ip_parameters.h"

#include <base/strings/string_number_conversions.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/ipconfig.h"
#include "shill/mock_store.h"
#include "shill/property_store.h"

using base::IntToString;
using std::string;
using std::vector;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {

const char kAddress[] = "10.0.0.1";
const char kGateway[] = "10.0.0.254";
const int32_t kMtu = 512;
const char kNameServer0[] = "10.0.1.253";
const char kNameServer1[] = "10.0.1.252";
const char kNameServers[] = "10.0.1.253,10.0.1.252";
const char kPeerAddress[] = "10.0.0.2";
const int32_t kPrefixLen = 24;

}  // namespace

class StaticIpParametersTest : public Test {
 public:
  StaticIpParametersTest() {}

  void ExpectEmptyIPConfig() {
    EXPECT_TRUE(ipconfig_props_.address.empty());
    EXPECT_TRUE(ipconfig_props_.gateway.empty());
    EXPECT_EQ(IPConfig::kUndefinedMTU, ipconfig_props_.mtu);
    EXPECT_TRUE(ipconfig_props_.dns_servers.empty());
    EXPECT_TRUE(ipconfig_props_.peer_address.empty());
    EXPECT_FALSE(ipconfig_props_.subnet_prefix);
  }
  // Modify an IP address string in some predictable way.  There's no need
  // for the output string to be valid from a networking perspective.
  string VersionedAddress(const string& address, int version) {
    string returned_address = address;
    CHECK(returned_address.length());
    returned_address[returned_address.length() - 1] += version;
    return returned_address;
  }
  void ExpectPopulatedIPConfigWithVersion(int version) {
    EXPECT_EQ(VersionedAddress(kAddress, version), ipconfig_props_.address);
    EXPECT_EQ(VersionedAddress(kGateway, version), ipconfig_props_.gateway);
    EXPECT_EQ(kMtu + version, ipconfig_props_.mtu);
    EXPECT_EQ(2, ipconfig_props_.dns_servers.size());
    EXPECT_EQ(VersionedAddress(kNameServer0, version),
              ipconfig_props_.dns_servers[0]);
    EXPECT_EQ(VersionedAddress(kNameServer1, version),
              ipconfig_props_.dns_servers[1]);
    EXPECT_EQ(VersionedAddress(kPeerAddress, version),
              ipconfig_props_.peer_address);
    EXPECT_EQ(kPrefixLen + version, ipconfig_props_.subnet_prefix);
  }
  void ExpectPopulatedIPConfig() { ExpectPopulatedIPConfigWithVersion(0); }
  void ExpectPropertiesWithVersion(PropertyStore* store,
                                        const string& property_prefix,
                                        int version) {
    string string_value;
    Error unused_error;
    EXPECT_TRUE(store->GetStringProperty(property_prefix + ".Address",
                                         &string_value,
                                         &unused_error));
    EXPECT_EQ(VersionedAddress(kAddress, version), string_value);
    EXPECT_TRUE(store->GetStringProperty(property_prefix + ".Gateway",
                                         &string_value,
                                         &unused_error));
    EXPECT_EQ(VersionedAddress(kGateway, version), string_value);
    int32_t int_value;
    EXPECT_TRUE(store->GetInt32Property(property_prefix + ".Mtu", &int_value,
                                        &unused_error));
    EXPECT_EQ(kMtu + version, int_value);
    EXPECT_TRUE(store->GetStringProperty(property_prefix + ".NameServers",
                                         &string_value,
                                         &unused_error));
    EXPECT_EQ(VersionedAddress(kNameServer0, version) + "," +
              VersionedAddress(kNameServer1, version),
              string_value);
    EXPECT_TRUE(store->GetStringProperty(property_prefix + ".PeerAddress",
                                         &string_value,
                                         &unused_error));
    EXPECT_EQ(VersionedAddress(kPeerAddress, version), string_value);
    EXPECT_TRUE(store->GetInt32Property(property_prefix + ".Prefixlen",
                                        &int_value,
                                        &unused_error));
    EXPECT_EQ(kPrefixLen + version, int_value);
  }
  void ExpectProperties(PropertyStore* store, const string& property_prefix) {
    ExpectPropertiesWithVersion(store, property_prefix, 0);
  }
  void PopulateIPConfig() {
    ipconfig_props_.address = kAddress;
    ipconfig_props_.gateway = kGateway;
    ipconfig_props_.mtu = kMtu;
    ipconfig_props_.dns_servers.push_back(kNameServer0);
    ipconfig_props_.dns_servers.push_back(kNameServer1);
    ipconfig_props_.peer_address = kPeerAddress;
    ipconfig_props_.subnet_prefix = kPrefixLen;
  }
  void SetStaticPropertiesWithVersion(PropertyStore* store, int version) {
    Error error;
    store->SetStringProperty(
        "StaticIP.Address", VersionedAddress(kAddress, version), &error);
    store->SetStringProperty(
        "StaticIP.Gateway", VersionedAddress(kGateway, version), &error);
    store->SetInt32Property(
        "StaticIP.Mtu", kMtu + version, &error);
    store->SetStringProperty(
        "StaticIP.NameServers",
        VersionedAddress(kNameServer0, version) + "," +
        VersionedAddress(kNameServer1, version),
        &error);
    store->SetStringProperty(
        "StaticIP.PeerAddress",
        VersionedAddress(kPeerAddress, version),
        &error);
    store->SetInt32Property("StaticIP.Prefixlen", kPrefixLen + version, &error);
  }
  void SetStaticProperties(PropertyStore* store) {
    SetStaticPropertiesWithVersion(store, 0);
  }
  void SetStaticDictPropertiesWithVersion(PropertyStore* store, int version) {
    KeyValueStore args;
    args.SetString(kAddressProperty, VersionedAddress(kAddress, version));
    args.SetString(kGatewayProperty, VersionedAddress(kGateway, version));
    args.SetInt(kMtuProperty, kMtu + version);
    vector<string> name_servers;
    name_servers.push_back(VersionedAddress(kNameServer0, version));
    name_servers.push_back(VersionedAddress(kNameServer1, version));
    args.SetStrings(kNameServersProperty, name_servers);
    args.SetString(kPeerAddressProperty,
                   VersionedAddress(kPeerAddress, version));
    args.SetInt(kPrefixlenProperty, kPrefixLen + version);
    Error error;
    store->SetKeyValueStoreProperty(kStaticIPConfigProperty, args, &error);
  }

 protected:
  StaticIPParameters static_params_;
  IPConfig::Properties ipconfig_props_;
};

TEST_F(StaticIpParametersTest, InitState) {
  ExpectEmptyIPConfig();

  // Applying an empty set of parameters on an empty set of properties should
  // be a no-op.
  static_params_.ApplyTo(&ipconfig_props_);
  ExpectEmptyIPConfig();
}

TEST_F(StaticIpParametersTest, ApplyEmptyParameters) {
  PopulateIPConfig();
  static_params_.ApplyTo(&ipconfig_props_);
  ExpectPopulatedIPConfig();
}

TEST_F(StaticIpParametersTest, ControlInterface) {
  PropertyStore store;
  static_params_.PlumbPropertyStore(&store);
  SetStaticProperties(&store);
  static_params_.ApplyTo(&ipconfig_props_);
  ExpectPopulatedIPConfig();

  EXPECT_TRUE(static_params_.ContainsAddress());
  Error unused_error;
  store.ClearProperty("StaticIP.Address", &unused_error);
  EXPECT_FALSE(static_params_.ContainsAddress());
  store.ClearProperty("StaticIP.Mtu", &unused_error);
  IPConfig::Properties props;
  const string kTestAddress("test_address");
  props.address = kTestAddress;
  const int32_t kTestMtu = 256;
  props.mtu = kTestMtu;
  static_params_.ApplyTo(&props);
  EXPECT_EQ(kTestAddress, props.address);
  EXPECT_EQ(kTestMtu, props.mtu);

  {
    Error error;
    EXPECT_FALSE(store.GetStringProperty("StaticIP.Address", nullptr, &error));
    EXPECT_EQ(Error::kNotFound, error.type());
  }
  string string_value;
  EXPECT_TRUE(store.GetStringProperty("StaticIP.Gateway", &string_value,
                                      &unused_error));
  EXPECT_EQ(kGateway, string_value);
  {
    Error error;
    EXPECT_FALSE(store.GetInt32Property("StaticIP.Mtu", nullptr, &error));
    EXPECT_EQ(Error::kNotFound, error.type());
  }
  EXPECT_TRUE(store.GetStringProperty("StaticIP.NameServers", &string_value,
                                      &unused_error));
  EXPECT_EQ(kNameServers, string_value);
  EXPECT_TRUE(store.GetStringProperty("StaticIP.PeerAddress", &string_value,
                                      &unused_error));
  EXPECT_EQ(kPeerAddress, string_value);
  int32_t int_value;
  EXPECT_TRUE(store.GetInt32Property("StaticIP.Prefixlen", &int_value,
                                     &unused_error));
  EXPECT_EQ(kPrefixLen, int_value);
}

TEST_F(StaticIpParametersTest, Profile) {
  StrictMock<MockStore> store;
  const string kID = "storage_id";
  EXPECT_CALL(store, GetString(kID, "StaticIP.Address", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kAddress)), Return(true)));
  EXPECT_CALL(store, GetString(kID, "StaticIP.Gateway", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kGateway)), Return(true)));
  EXPECT_CALL(store, GetInt(kID, "StaticIP.Mtu", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kMtu), Return(true)));
  EXPECT_CALL(store, GetString(kID, "StaticIP.NameServers", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kNameServers)),
                      Return(true)));
  EXPECT_CALL(store, GetString(kID, "StaticIP.PeerAddress", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kPeerAddress)),
                      Return(true)));
  EXPECT_CALL(store, GetInt(kID, "StaticIP.Prefixlen", _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kPrefixLen), Return(true)));
  static_params_.Load(&store, kID);
  static_params_.ApplyTo(&ipconfig_props_);
  ExpectPopulatedIPConfig();

  EXPECT_CALL(store, SetString(kID, "StaticIP.Address", kAddress))
      .WillOnce(Return(true));
  EXPECT_CALL(store, SetString(kID, "StaticIP.Gateway", kGateway))
      .WillOnce(Return(true));
  EXPECT_CALL(store, SetInt(kID, "StaticIP.Mtu", kMtu))
      .WillOnce(Return(true));
  EXPECT_CALL(store, SetString(kID, "StaticIP.NameServers", kNameServers))
      .WillOnce(Return(true));
  EXPECT_CALL(store, SetString(kID, "StaticIP.PeerAddress", kPeerAddress))
      .WillOnce(Return(true));
  EXPECT_CALL(store, SetInt(kID, "StaticIP.Prefixlen", kPrefixLen))
      .WillOnce(Return(true));
  static_params_.Save(&store, kID);
}

TEST_F(StaticIpParametersTest, SavedParameters) {
  // Calling RestoreTo() when no parameters are set should not crash or
  // add any entries.
  static_params_.RestoreTo(&ipconfig_props_);
  ExpectEmptyIPConfig();

  PopulateIPConfig();
  PropertyStore static_params_props;
  static_params_.PlumbPropertyStore(&static_params_props);
  SetStaticPropertiesWithVersion(&static_params_props, 1);
  static_params_.ApplyTo(&ipconfig_props_);

  // The version 0 properties in |ipconfig_props_| are now in SavedIP.*
  // properties, while the version 1 StaticIP parameters are now in
  // |ipconfig_props_|.
  ExpectPropertiesWithVersion(&static_params_props, "SavedIP", 0);
  ExpectPopulatedIPConfigWithVersion(1);

  // Clear all "StaticIP" parameters.
  static_params_.args_.Clear();

  // Another ApplyTo() call rotates the version 1 properties in
  // |ipconfig_props_| over to SavedIP.*.  Since there are no StaticIP
  // parameters, |ipconfig_props_| should remain populated with version 1
  // parameters.
  static_params_.ApplyTo(&ipconfig_props_);
  ExpectPropertiesWithVersion(&static_params_props, "SavedIP", 1);
  ExpectPopulatedIPConfigWithVersion(1);

  // Reset |ipconfig_props_| to version 0.
  PopulateIPConfig();

  // A RestoreTo() call moves the version 1 "SavedIP" parameters into
  // |ipconfig_props_|.
  SetStaticPropertiesWithVersion(&static_params_props, 2);
  static_params_.RestoreTo(&ipconfig_props_);
  ExpectPopulatedIPConfigWithVersion(1);

  // All "SavedIP" parameters should be cleared.
  EXPECT_TRUE(static_params_.saved_args_.IsEmpty());

  // Static IP parameters should be unchanged.
  ExpectPropertiesWithVersion(&static_params_props, "StaticIP", 2);
}

TEST_F(StaticIpParametersTest, SavedParametersDict) {
  // Calling RestoreTo() when no parameters are set should not crash or
  // add any entries.
  static_params_.RestoreTo(&ipconfig_props_);
  ExpectEmptyIPConfig();

  PopulateIPConfig();
  PropertyStore static_params_props;
  static_params_.PlumbPropertyStore(&static_params_props);
  SetStaticDictPropertiesWithVersion(&static_params_props, 1);
  static_params_.ApplyTo(&ipconfig_props_);

  // The version 0 properties in |ipconfig_props_| are now in SavedIP.*
  // properties, while the version 1 StaticIP parameters are now in
  // |ipconfig_props_|.
  ExpectPropertiesWithVersion(&static_params_props, "SavedIP", 0);
  ExpectPopulatedIPConfigWithVersion(1);

  // Clear all "StaticIP" parameters.
  static_params_.args_.Clear();

  // Another ApplyTo() call rotates the version 1 properties in
  // |ipconfig_props_| over to SavedIP.*.  Since there are no StaticIP
  // parameters, |ipconfig_props_| should remain populated with version 1
  // parameters.
  static_params_.ApplyTo(&ipconfig_props_);
  ExpectPropertiesWithVersion(&static_params_props, "SavedIP", 1);
  ExpectPopulatedIPConfigWithVersion(1);

  // Reset |ipconfig_props_| to version 0.
  PopulateIPConfig();

  // A RestoreTo() call moves the version 1 "SavedIP" parameters into
  // |ipconfig_props_|.
  SetStaticDictPropertiesWithVersion(&static_params_props, 2);
  static_params_.RestoreTo(&ipconfig_props_);
  ExpectPopulatedIPConfigWithVersion(1);

  // All "SavedIP" parameters should be cleared.
  EXPECT_TRUE(static_params_.saved_args_.IsEmpty());

  // Static IP parameters should be unchanged.
  ExpectPropertiesWithVersion(&static_params_props, "StaticIP", 2);
}

}  // namespace shill

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

#include "shill/vpn/vpn_driver.h"

#include <vector>

#include <base/stl_util.h>
#include <base/strings/string_number_conversions.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_connection.h"
#include "shill/mock_device_info.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_service.h"
#include "shill/mock_store.h"
#include "shill/nice_mock_control.h"
#include "shill/property_store.h"
#include "shill/test_event_dispatcher.h"

using std::string;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::NiceMock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {

const char kVPNHostProperty[] = "VPN.Host";
const char kOTPProperty[] = "VPN.OTP";
const char kPINProperty[] = "VPN.PIN";
const char kPSKProperty[] = "VPN.PSK";
const char kPasswordProperty[] = "VPN.Password";
const char kPortProperty[] = "VPN.Port";

const char kPIN[] = "5555";
const char kPassword[] = "random-password";
const char kPort[] = "1234";
const char kStorageID[] = "vpn_service_id";

}  // namespace

class VPNDriverUnderTest : public VPNDriver {
 public:
  VPNDriverUnderTest(EventDispatcher* dispatcher, Manager* manager);
  virtual ~VPNDriverUnderTest() {}

  // Inherited from VPNDriver.
  MOCK_METHOD2(ClaimInterface, bool(const string& link_name,
                                    int interface_index));
  MOCK_METHOD2(Connect, void(const VPNServiceRefPtr& service, Error* error));
  MOCK_METHOD0(Disconnect, void());
  MOCK_METHOD0(OnConnectionDisconnected, void());
  MOCK_CONST_METHOD0(GetProviderType, string());

 private:
  static const Property kProperties[];

  DISALLOW_COPY_AND_ASSIGN(VPNDriverUnderTest);
};

// static
const VPNDriverUnderTest::Property VPNDriverUnderTest::kProperties[] = {
  { kEapCaCertPemProperty, Property::kArray },
  { kVPNHostProperty, 0 },
  { kL2tpIpsecCaCertPemProperty, Property::kArray },
  { kOTPProperty, Property::kEphemeral },
  { kPINProperty, Property::kWriteOnly },
  { kPSKProperty, Property::kCredential },
  { kPasswordProperty, Property::kCredential },
  { kPortProperty, 0 },
  { kProviderTypeProperty, 0 },
};

VPNDriverUnderTest::VPNDriverUnderTest(
    EventDispatcher* dispatcher, Manager* manager)
    : VPNDriver(dispatcher, manager, kProperties, arraysize(kProperties)) {}

class VPNDriverTest : public Test {
 public:
  VPNDriverTest()
      : device_info_(&control_, &dispatcher_, &metrics_, &manager_),
        metrics_(&dispatcher_),
        manager_(&control_, &dispatcher_, &metrics_),
        driver_(&dispatcher_, &manager_) {}

  virtual ~VPNDriverTest() {}

 protected:
  EventDispatcher* dispatcher() { return driver_.dispatcher_; }
  void set_dispatcher(EventDispatcher* dispatcher) {
    driver_.dispatcher_ = dispatcher;
  }

  const base::CancelableClosure& connect_timeout_callback() {
    return driver_.connect_timeout_callback_;
  }

  bool IsConnectTimeoutStarted() { return driver_.IsConnectTimeoutStarted(); }
  int connect_timeout_seconds() { return driver_.connect_timeout_seconds(); }

  void StartConnectTimeout(int timeout_seconds) {
    driver_.StartConnectTimeout(timeout_seconds);
  }

  void StopConnectTimeout() { driver_.StopConnectTimeout(); }

  void SetArg(const string& arg, const string& value) {
    driver_.args()->SetString(arg, value);
  }

  void SetArgArray(const string& arg, const vector<string>& value) {
    driver_.args()->SetStrings(arg, value);
  }

  KeyValueStore* GetArgs() { return driver_.args(); }

  bool GetProviderPropertyString(const PropertyStore& store,
                                 const string& key,
                                 string* value);

  bool GetProviderPropertyStrings(const PropertyStore& store,
                                  const string& key,
                                  vector<string>* value);

  NiceMockControl control_;
  NiceMock<MockDeviceInfo> device_info_;
  EventDispatcherForTest dispatcher_;
  MockMetrics metrics_;
  MockManager manager_;
  VPNDriverUnderTest driver_;
};

bool VPNDriverTest::GetProviderPropertyString(const PropertyStore& store,
                                              const string& key,
                                              string* value) {
  KeyValueStore provider_properties;
  Error error;
  EXPECT_TRUE(store.GetKeyValueStoreProperty(
      kProviderProperty, &provider_properties, &error));
  if (!provider_properties.ContainsString(key)) {
    return false;
  }
  if (value != nullptr) {
    *value = provider_properties.GetString(key);
  }
  return true;
}

bool VPNDriverTest::GetProviderPropertyStrings(const PropertyStore& store,
                                               const string& key,
                                               vector<string>* value) {
  KeyValueStore provider_properties;
  Error error;
  EXPECT_TRUE(store.GetKeyValueStoreProperty(
      kProviderProperty, &provider_properties, &error));
  if (!provider_properties.ContainsStrings(key)) {
    return false;
  }
  if (value != nullptr) {
    *value = provider_properties.GetStrings(key);
  }
  return true;
}

TEST_F(VPNDriverTest, Load) {
  MockStore storage;
  GetArgs()->SetString(kVPNHostProperty, "1.2.3.4");
  GetArgs()->SetString(kPSKProperty, "1234");
  GetArgs()->SetStrings(kL2tpIpsecCaCertPemProperty,
                        vector<string>{ "cleared-cert0", "cleared-cert1" });
  EXPECT_CALL(storage, GetString(kStorageID, _, _))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(storage, GetStringList(kStorageID, _, _))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(storage, GetString(_, kEapCaCertPemProperty, _)).Times(0);
  EXPECT_CALL(storage, GetString(_, kOTPProperty, _)).Times(0);
  EXPECT_CALL(storage, GetCryptedString(_, kOTPProperty, _)).Times(0);
  EXPECT_CALL(storage, GetStringList(_, kOTPProperty, _)).Times(0);
  vector<string> kCaCerts{ "cert0", "cert1" };
  EXPECT_CALL(storage, GetStringList(kStorageID, kEapCaCertPemProperty, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kCaCerts), Return(true)));
  EXPECT_CALL(storage, GetString(kStorageID, kPortProperty, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kPort)), Return(true)));
  EXPECT_CALL(storage, GetString(kStorageID, kPINProperty, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kPIN)), Return(true)));
  EXPECT_CALL(storage, GetCryptedString(kStorageID, kPSKProperty, _))
      .WillOnce(Return(false));
  EXPECT_CALL(storage, GetCryptedString(kStorageID, kPasswordProperty, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(string(kPassword)), Return(true)));
  EXPECT_TRUE(driver_.Load(&storage, kStorageID));
  EXPECT_TRUE(GetArgs()->ContainsStrings(kEapCaCertPemProperty));
  if (GetArgs()->ContainsStrings(kEapCaCertPemProperty)) {
    EXPECT_EQ(kCaCerts, GetArgs()->GetStrings(kEapCaCertPemProperty));
  }
  EXPECT_EQ(kPort, GetArgs()->LookupString(kPortProperty, ""));
  EXPECT_EQ(kPIN, GetArgs()->LookupString(kPINProperty, ""));
  EXPECT_EQ(kPassword, GetArgs()->LookupString(kPasswordProperty, ""));

  // Properties missing from the persistent store should be deleted.
  EXPECT_FALSE(GetArgs()->ContainsString(kVPNHostProperty));
  EXPECT_FALSE(GetArgs()->ContainsStrings(kL2tpIpsecCaCertPemProperty));
  EXPECT_FALSE(GetArgs()->ContainsString(kPSKProperty));
}

TEST_F(VPNDriverTest, Save) {
  SetArg(kProviderTypeProperty, "");
  SetArg(kPINProperty, kPIN);
  SetArg(kPortProperty, kPort);
  SetArg(kPasswordProperty, kPassword);
  SetArg(kOTPProperty, "987654");
  const vector<string> kCaCerts{ "cert0", "cert1" };
  SetArgArray(kEapCaCertPemProperty, kCaCerts);
  MockStore storage;
  EXPECT_CALL(storage,
              SetStringList(kStorageID, kEapCaCertPemProperty, kCaCerts))
      .WillOnce(Return(true));
  EXPECT_CALL(storage,
              SetString(kStorageID, kProviderTypeProperty, ""))
      .WillOnce(Return(true));
  EXPECT_CALL(storage, SetString(kStorageID, kPortProperty, kPort))
      .WillOnce(Return(true));
  EXPECT_CALL(storage, SetString(kStorageID, kPINProperty, kPIN))
      .WillOnce(Return(true));
  EXPECT_CALL(storage,
              SetCryptedString(kStorageID, kPasswordProperty, kPassword))
      .WillOnce(Return(true));
  EXPECT_CALL(storage, SetCryptedString(_, kOTPProperty, _)).Times(0);
  EXPECT_CALL(storage, SetString(_, kOTPProperty, _)).Times(0);
  EXPECT_CALL(storage, SetString(_, kEapCaCertPemProperty, _)).Times(0);
  EXPECT_CALL(storage, DeleteKey(kStorageID, kEapCaCertPemProperty)).Times(0);
  EXPECT_CALL(storage, DeleteKey(kStorageID, kProviderTypeProperty))
      .Times(0);
  EXPECT_CALL(storage, DeleteKey(kStorageID, kL2tpIpsecCaCertPemProperty));
  EXPECT_CALL(storage, DeleteKey(kStorageID, kPSKProperty));
  EXPECT_CALL(storage, DeleteKey(kStorageID, kVPNHostProperty));
  EXPECT_TRUE(driver_.Save(&storage, kStorageID, true));
}

TEST_F(VPNDriverTest, SaveNoCredentials) {
  SetArg(kPasswordProperty, kPassword);
  SetArg(kPSKProperty, "");
  MockStore storage;
  EXPECT_CALL(storage, SetString(_, kPasswordProperty, _)).Times(0);
  EXPECT_CALL(storage, SetCryptedString(_, kPasswordProperty, _)).Times(0);
  EXPECT_CALL(storage, DeleteKey(kStorageID, _)).Times(AnyNumber());
  EXPECT_CALL(storage, DeleteKey(kStorageID, kPasswordProperty));
  EXPECT_CALL(storage, DeleteKey(kStorageID, kPSKProperty));
  EXPECT_CALL(storage, DeleteKey(kStorageID, kEapCaCertPemProperty));
  EXPECT_CALL(storage, DeleteKey(kStorageID, kL2tpIpsecCaCertPemProperty));
  EXPECT_TRUE(driver_.Save(&storage, kStorageID, false));
}

TEST_F(VPNDriverTest, UnloadCredentials) {
  SetArg(kOTPProperty, "654321");
  SetArg(kPasswordProperty, kPassword);
  SetArg(kPortProperty, kPort);
  driver_.UnloadCredentials();
  EXPECT_FALSE(GetArgs()->ContainsString(kOTPProperty));
  EXPECT_FALSE(GetArgs()->ContainsString(kPasswordProperty));
  EXPECT_EQ(kPort, GetArgs()->LookupString(kPortProperty, ""));
}

TEST_F(VPNDriverTest, InitPropertyStore) {
  // Figure out if the store is actually hooked up to the driver argument
  // KeyValueStore.
  PropertyStore store;
  driver_.InitPropertyStore(&store);

  // An un-set property should not be readable.
  {
    Error error;
    EXPECT_FALSE(store.GetStringProperty(kPortProperty, nullptr, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
  {
    Error error;
    EXPECT_FALSE(
        store.GetStringsProperty(kEapCaCertPemProperty, nullptr, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
  EXPECT_FALSE(GetProviderPropertyString(store, kPortProperty, nullptr));
  EXPECT_FALSE(
      GetProviderPropertyStrings(store, kEapCaCertPemProperty, nullptr));

  const string kProviderType = "boo";
  SetArg(kPortProperty, kPort);
  SetArg(kPasswordProperty, kPassword);
  SetArg(kProviderTypeProperty, kProviderType);
  SetArg(kVPNHostProperty, "");
  const vector<string> kCaCerts{ "cert1" };
  SetArgArray(kEapCaCertPemProperty, kCaCerts);
  SetArgArray(kL2tpIpsecCaCertPemProperty, vector<string>());

  // We should not be able to read a property out of the driver args using the
  // key to the args directly.
  {
    Error error;
    EXPECT_FALSE(store.GetStringProperty(kPortProperty, nullptr, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }
  {
    Error error;
    EXPECT_FALSE(
        store.GetStringsProperty(kEapCaCertPemProperty, nullptr, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
  }

  // We should instead be able to find it within the "Provider" stringmap.
  {
    string value;
    EXPECT_TRUE(GetProviderPropertyString(store, kPortProperty, &value));
    EXPECT_EQ(kPort, value);
  }
  {
    vector<string> value;
    EXPECT_TRUE(GetProviderPropertyStrings(store, kEapCaCertPemProperty,
                                           &value));
    EXPECT_EQ(kCaCerts, value);
  }

  // We should be able to read empty properties from the "Provider" stringmap.
  {
    string value;
    EXPECT_TRUE(GetProviderPropertyString(store, kVPNHostProperty, &value));
    EXPECT_TRUE(value.empty());
  }
  {
    vector<string> value;
    EXPECT_TRUE(GetProviderPropertyStrings(store, kL2tpIpsecCaCertPemProperty,
                                           &value));
    EXPECT_TRUE(value.empty());
  }

  // Properties that start with the prefix "Provider." should be mapped to the
  // name in the Properties dict with the prefix removed.
  {
    string value;
    EXPECT_TRUE(GetProviderPropertyString(store, kTypeProperty,
                                          &value));
    EXPECT_EQ(kProviderType, value);
  }

  // If we clear a property, we should no longer be able to find it.
  {
    Error error;
    EXPECT_TRUE(store.ClearProperty(kPortProperty, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_FALSE(GetProviderPropertyString(store, kPortProperty, nullptr));
  }
  {
    Error error;
    EXPECT_TRUE(store.ClearProperty(kEapCaCertPemProperty, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_FALSE(GetProviderPropertyStrings(store, kEapCaCertPemProperty,
                                            nullptr));
  }

  // A second attempt to clear this property should return an error.
  {
    Error error;
    EXPECT_FALSE(store.ClearProperty(kPortProperty, &error));
    EXPECT_EQ(Error::kNotFound, error.type());
  }
  {
    Error error;
    EXPECT_FALSE(store.ClearProperty(kEapCaCertPemProperty, &error));
    EXPECT_EQ(Error::kNotFound, error.type());
  }

  // Test write only properties.
  EXPECT_FALSE(GetProviderPropertyString(store, kPINProperty, nullptr));

  // Write properties to the driver args using the PropertyStore interface.
  {
    const string kValue = "some-value";
    Error error;
    EXPECT_TRUE(store.SetStringProperty(kPINProperty, kValue, &error));
    EXPECT_EQ(kValue, GetArgs()->GetString(kPINProperty));
  }
  {
    const vector<string> kValue{ "some-value" };
    Error error;
    EXPECT_TRUE(store.SetStringsProperty(kEapCaCertPemProperty, kValue,
                                         &error));
    EXPECT_EQ(kValue, GetArgs()->GetStrings(kEapCaCertPemProperty));
  }
}

TEST_F(VPNDriverTest, ConnectTimeout) {
  EXPECT_EQ(&dispatcher_, dispatcher());
  EXPECT_TRUE(connect_timeout_callback().IsCancelled());
  EXPECT_FALSE(IsConnectTimeoutStarted());
  StartConnectTimeout(0);
  EXPECT_FALSE(connect_timeout_callback().IsCancelled());
  EXPECT_TRUE(IsConnectTimeoutStarted());
  set_dispatcher(nullptr);
  StartConnectTimeout(0);  // Expect no crash.
  dispatcher_.DispatchPendingEvents();
  EXPECT_TRUE(connect_timeout_callback().IsCancelled());
  EXPECT_FALSE(IsConnectTimeoutStarted());
}

TEST_F(VPNDriverTest, StartStopConnectTimeout) {
  EXPECT_FALSE(IsConnectTimeoutStarted());
  EXPECT_EQ(0, connect_timeout_seconds());
  const int kTimeout = 123;
  StartConnectTimeout(kTimeout);
  EXPECT_TRUE(IsConnectTimeoutStarted());
  EXPECT_EQ(kTimeout, connect_timeout_seconds());
  StartConnectTimeout(kTimeout - 20);
  EXPECT_EQ(kTimeout, connect_timeout_seconds());
  StopConnectTimeout();
  EXPECT_FALSE(IsConnectTimeoutStarted());
  EXPECT_EQ(0, connect_timeout_seconds());
}

}  // namespace shill

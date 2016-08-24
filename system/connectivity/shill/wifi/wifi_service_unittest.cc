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

#include "shill/wifi/wifi_service.h"

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <base/strings/stringprintf.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/event_dispatcher.h"
#include "shill/manager.h"
#include "shill/metrics.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_certificate_file.h"
#include "shill/mock_control.h"
#include "shill/mock_eap_credentials.h"
#include "shill/mock_log.h"
#include "shill/mock_manager.h"
#include "shill/mock_profile.h"
#include "shill/mock_service.h"
#include "shill/mock_store.h"
#include "shill/property_store_unittest.h"
#include "shill/refptr_types.h"
#include "shill/service_property_change_test.h"
#include "shill/supplicant/wpa_supplicant.h"
#include "shill/technology.h"
#include "shill/tethering.h"
#include "shill/wifi/mock_wifi.h"
#include "shill/wifi/mock_wifi_provider.h"
#include "shill/wifi/wifi_endpoint.h"

using base::FilePath;
using std::map;
using std::set;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::EndsWith;
using ::testing::HasSubstr;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgumentPointee;
using ::testing::StrEq;
using ::testing::StrNe;
using ::testing::StrictMock;

namespace shill {

class WiFiServiceTest : public PropertyStoreTest {
 public:
  WiFiServiceTest()
      : mock_manager_(control_interface(), dispatcher(), metrics()),
        wifi_(
            new NiceMock<MockWiFi>(control_interface(),
                                   dispatcher(),
                                   metrics(),
                                   manager(),
                                   "wifi",
                                   fake_mac,
                                   0)),
        simple_ssid_(1, 'a'),
        simple_ssid_string_("a") {}
  virtual ~WiFiServiceTest() {}

 protected:
  static const char fake_mac[];

  MockEapCredentials* SetMockEap(
      const WiFiServiceRefPtr& service) {
    MockEapCredentials* eap = new MockEapCredentials();
    service->eap_.reset(eap);  // Passes ownership.
    return eap;
  }
  bool CheckConnectable(const string& security, const char* passphrase,
                        bool is_1x_connectable) {
    Error error;
    WiFiServiceRefPtr service = MakeSimpleService(security);
    if (passphrase)
      service->SetPassphrase(passphrase, &error);
    MockEapCredentials* eap = SetMockEap(service);
    EXPECT_CALL(*eap, IsConnectable())
        .WillRepeatedly(Return(is_1x_connectable));
    const string kKeyManagement8021x(WPASupplicant::kKeyManagementIeee8021X);
    if (security == kSecurityWep && is_1x_connectable) {
      EXPECT_CALL(*eap, key_management())
          .WillRepeatedly(ReturnRef(kKeyManagement8021x));
    }
    service->OnEapCredentialsChanged(Service::kReasonCredentialsLoaded);
    return service->connectable();
  }
  WiFiEndpoint* MakeEndpoint(const string& ssid, const string& bssid,
                             uint16_t frequency, int16_t signal_dbm,
                             bool has_wpa_property, bool has_rsn_property) {
    return WiFiEndpoint::MakeEndpoint(
        nullptr, wifi(), ssid, bssid, WPASupplicant::kNetworkModeInfrastructure,
        frequency, signal_dbm, has_wpa_property, has_rsn_property);
  }
  WiFiEndpoint* MakeOpenEndpoint(const string& ssid, const string& bssid,
                                 uint16_t frequency, int16_t signal_dbm) {
    return WiFiEndpoint::MakeOpenEndpoint(
        nullptr, wifi(), ssid, bssid, WPASupplicant::kNetworkModeInfrastructure,
        frequency, signal_dbm);
  }
  WiFiEndpoint* MakeOpenEndpointWithWiFi(WiFiRefPtr wifi,
                                         const string& ssid,
                                         const string& bssid,
                                         uint16_t frequency,
                                         int16_t signal_dbm) {
    return WiFiEndpoint::MakeOpenEndpoint(
        nullptr, wifi, ssid, bssid, WPASupplicant::kNetworkModeInfrastructure,
        frequency, signal_dbm);
  }
  WiFiServiceRefPtr MakeSimpleService(const string& security) {
    return new WiFiService(control_interface(),
                           dispatcher(),
                           metrics(),
                           manager(),
                           &provider_,
                           simple_ssid_,
                           kModeManaged,
                           security,
                           false);
  }
  WiFiServiceRefPtr MakeGenericService() {
    return MakeSimpleService(kSecurityWep);
  }
  void SetWiFi(WiFiServiceRefPtr service, WiFiRefPtr wifi) {
    service->SetWiFi(wifi);  // Has side-effects.
  }
  void SetWiFiForService(WiFiServiceRefPtr service, WiFiRefPtr wifi) {
    service->wifi_ = wifi;
  }
  WiFiServiceRefPtr MakeServiceWithWiFi(const string& security) {
    WiFiServiceRefPtr service = MakeSimpleService(security);
    SetWiFiForService(service, wifi_);
    return service;
  }
  WiFiServiceRefPtr MakeServiceWithMockManager() {
    return new WiFiService(control_interface(),
                           dispatcher(),
                           metrics(),
                           &mock_manager_,
                           &provider_,
                           simple_ssid_,
                           kModeManaged,
                           kSecurityNone,
                           false);
  }
  scoped_refptr<MockWiFi> MakeSimpleWiFi(const string& link_name) {
    return new NiceMock<MockWiFi>(control_interface(),
                                  dispatcher(),
                                  metrics(),
                                  manager(),
                                  link_name,
                                  fake_mac,
                                  0);
  }
  ServiceMockAdaptor* GetAdaptor(WiFiService* service) {
    return static_cast<ServiceMockAdaptor*>(service->adaptor());
  }
  Error::Type TestConfigurePassphrase(const string& security,
                                      const char* passphrase) {
    WiFiServiceRefPtr service = MakeSimpleService(security);
    KeyValueStore args;
    if (passphrase) {
      args.SetString(kPassphraseProperty, passphrase);
    }
    Error error;
    service->Configure(args, &error);
    return error.type();
  }
  bool SetRoamThreshold(WiFiServiceRefPtr service, uint16_t threshold) {
    return service->SetRoamThreshold(threshold, nullptr);
  }
  uint16_t GetRoamThreshold(WiFiServiceRefPtr service) const {
    return service->GetRoamThreshold(nullptr);
  }
  scoped_refptr<MockWiFi> wifi() { return wifi_; }
  MockManager* mock_manager() { return &mock_manager_; }
  MockWiFiProvider* provider() { return &provider_; }
  string GetAnyDeviceAddress() { return WiFiService::kAnyDeviceAddress; }
  const vector<uint8_t>& simple_ssid() { return simple_ssid_; }
  const string& simple_ssid_string() { return simple_ssid_string_; }

 private:
  MockManager mock_manager_;
  scoped_refptr<MockWiFi> wifi_;
  MockWiFiProvider provider_;
  const vector<uint8_t> simple_ssid_;
  const string simple_ssid_string_;
};

// static
const char WiFiServiceTest::fake_mac[] = "AaBBcCDDeeFF";

MATCHER_P3(ContainsWiFiProperties, ssid, mode, security, "") {
  string hex_ssid = base::HexEncode(ssid.data(), ssid.size());
  return
      arg.ContainsString(WiFiService::kStorageType) &&
      arg.GetString(WiFiService::kStorageType) == kTypeWifi &&
      arg.ContainsString(WiFiService::kStorageSSID) &&
      arg.GetString(WiFiService::kStorageSSID) == hex_ssid &&
      arg.ContainsString(WiFiService::kStorageMode) &&
      arg.GetString(WiFiService::kStorageMode) == mode &&
      arg.ContainsString(WiFiService::kStorageSecurityClass) &&
      arg.GetString(WiFiService::kStorageSecurityClass) == security;
}

class WiFiServiceSecurityTest : public WiFiServiceTest {
 public:
  bool TestStorageSecurityIs(WiFiServiceRefPtr wifi_service,
                             const string& security) {
    string id = wifi_service->GetStorageIdentifier();
    size_t mac_pos = id.find(base::ToLowerASCII(GetAnyDeviceAddress()));
    EXPECT_NE(mac_pos, string::npos);
    size_t mode_pos = id.find(string(kModeManaged), mac_pos);
    EXPECT_NE(mode_pos, string::npos);
    return id.find(string(security), mode_pos) != string::npos;
  }

  // Test that a service that is created with security |from_security|
  // gets by default a storage identifier with |to_security| as its
  // security component, and that when saved, it sets the Security
  // property in to |to_security| as well.
  bool TestStorageMapping(const string& from_security,
                          const string& to_security) {
    WiFiServiceRefPtr wifi_service = MakeSimpleService(from_security);
    NiceMock<MockStore> mock_store;
    EXPECT_CALL(mock_store, SetString(_, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock_store,
                SetString(_, WiFiService::kStorageSecurity, from_security))
        .Times(1);
    EXPECT_CALL(mock_store,
                SetString(_, WiFiService::kStorageSecurityClass, to_security))
        .Times(1);
    wifi_service->Save(&mock_store);
    return TestStorageSecurityIs(wifi_service, to_security);
  }

  // Test whether a service of type |service_security| can load from a
  // storage interface containing an entry for |storage_security|.
  // Make sure the result meets |expectation|.  If |expectation| is
  // true, also make sure the service storage identifier changes to
  // match |storage_security|.
  bool TestLoadMapping(const string& service_security,
                       const string& storage_security,
                       bool expectation) {
    WiFiServiceRefPtr wifi_service = MakeSimpleService(service_security);
    NiceMock<MockStore> mock_store;
    EXPECT_CALL(mock_store, GetGroupsWithProperties(_))
        .WillRepeatedly(Return(set<string>()));
    const string kStorageId = "storage_id";
    EXPECT_CALL(mock_store, ContainsGroup(kStorageId))
        .WillRepeatedly(Return(true));
    set<string> groups;
    groups.insert(kStorageId);
    EXPECT_CALL(mock_store, GetGroupsWithProperties(
        ContainsWiFiProperties(wifi_service->ssid(),
                               kModeManaged,
                               storage_security)))
        .WillRepeatedly(Return(groups));
    bool is_loadable = wifi_service->IsLoadableFrom(mock_store);
    EXPECT_EQ(expectation, is_loadable);
    bool is_loaded = wifi_service->Load(&mock_store);
    EXPECT_EQ(expectation, is_loaded);
    const string expected_identifier(expectation ? kStorageId : "");
    EXPECT_EQ(expected_identifier,
              wifi_service->GetLoadableStorageIdentifier(mock_store));

    if (expectation != is_loadable || expectation != is_loaded) {
      return false;
    } else if (!expectation) {
      return true;
    } else {
      return wifi_service->GetStorageIdentifier() == kStorageId;
    }
  }
};

class WiFiServiceUpdateFromEndpointsTest : public WiFiServiceTest {
 public:
  WiFiServiceUpdateFromEndpointsTest()
      : kOkEndpointStrength(WiFiService::SignalToStrength(kOkEndpointSignal)),
        kBadEndpointStrength(WiFiService::SignalToStrength(kBadEndpointSignal)),
        kGoodEndpointStrength(
            WiFiService::SignalToStrength(kGoodEndpointSignal)),
        service(MakeGenericService()),
        adaptor(*GetAdaptor(service.get())) {
    ok_endpoint = MakeOpenEndpoint(
        simple_ssid_string(), kOkEndpointBssId, kOkEndpointFrequency,
        kOkEndpointSignal);
    good_endpoint = MakeOpenEndpoint(
        simple_ssid_string(), kGoodEndpointBssId, kGoodEndpointFrequency,
        kGoodEndpointSignal);
    bad_endpoint = MakeOpenEndpoint(
        simple_ssid_string(), kBadEndpointBssId, kBadEndpointFrequency,
        kBadEndpointSignal);
  }

 protected:
  static const uint16_t kOkEndpointFrequency = 2422;
  static const uint16_t kBadEndpointFrequency = 2417;
  static const uint16_t kGoodEndpointFrequency = 2412;
  static const int16_t kOkEndpointSignal = -50;
  static const int16_t kBadEndpointSignal = -75;
  static const int16_t kGoodEndpointSignal = -25;
  static const char* kOkEndpointBssId;
  static const char* kGoodEndpointBssId;
  static const char* kBadEndpointBssId;
  // Can't be both static and const (because initialization requires a
  // function call). So choose to be just const.
  const uint8_t kOkEndpointStrength;
  const uint8_t kBadEndpointStrength;
  const uint8_t kGoodEndpointStrength;
  WiFiEndpointRefPtr ok_endpoint;
  WiFiEndpointRefPtr bad_endpoint;
  WiFiEndpointRefPtr good_endpoint;
  WiFiServiceRefPtr service;
  ServiceMockAdaptor& adaptor;
};

const char* WiFiServiceUpdateFromEndpointsTest::kOkEndpointBssId =
    "00:00:00:00:00:01";
const char* WiFiServiceUpdateFromEndpointsTest::kGoodEndpointBssId =
    "00:00:00:00:00:02";
const char* WiFiServiceUpdateFromEndpointsTest::kBadEndpointBssId =
    "00:00:00:00:00:03";

class WiFiServiceFixupStorageTest : public WiFiServiceTest {
 protected:
  void AddGroup(string group_name) {
    groups_.insert(group_name);
  }

  void AddServiceEntry(bool has_type, bool has_mode, bool has_security,
                       bool has_security_class) {
    int index = groups_.size();
    string id = base::StringPrintf("%s_%d_%d_%s_%s", kTypeWifi,
                                   index, index, kModeManaged,
                                   kSecurityWpa);
    AddGroup(id);
    EXPECT_CALL(store_, GetString(id, WiFiService::kStorageType, _))
        .WillOnce(Return(has_type));
    if (!has_type) {
      EXPECT_CALL(store_, SetString(id, WiFiService::kStorageType,
                                    kTypeWifi));
    }
    EXPECT_CALL(store_, GetString(id, WiFiService::kStorageMode, _))
        .WillOnce(Return(has_mode));
    if (!has_mode) {
      EXPECT_CALL(store_, SetString(id, WiFiService::kStorageMode,
                                    kModeManaged));
    }
    EXPECT_CALL(store_, GetString(id, WiFiService::kStorageSecurity, _))
        .WillOnce(Return(has_security));
    if (!has_security) {
      EXPECT_CALL(store_, SetString(id, WiFiService::kStorageSecurity,
                                    kSecurityWpa));
    }
    EXPECT_CALL(store_, GetString(id, WiFiService::kStorageSecurityClass, _))
        .WillOnce(Return(has_security_class));
    if (!has_security_class) {
      EXPECT_CALL(store_, SetString(id, WiFiService::kStorageSecurityClass,
                                    kSecurityPsk));
    }
  }

  bool FixupServiceEntries() {
    EXPECT_CALL(store_, GetGroups()).WillOnce(Return(groups_));
    return WiFiService::FixupServiceEntries(&store_);
  }

 private:
  StrictMock<MockStore> store_;
  set<string> groups_;
};

TEST_F(WiFiServiceTest, Constructor) {
  string histogram = metrics()->GetFullMetricName(
      Metrics::kMetricTimeToJoinMillisecondsSuffix, Technology::kWifi);
  EXPECT_CALL(*metrics(), AddServiceStateTransitionTimer(_, _, _, _))
      .Times(AnyNumber());
  EXPECT_CALL(*metrics(), AddServiceStateTransitionTimer(
      _, histogram, Service::kStateAssociating, Service::kStateConfiguring));
  MakeSimpleService(kSecurityNone);
}

TEST_F(WiFiServiceTest, StorageId) {
  WiFiServiceRefPtr wifi_service = MakeSimpleService(kSecurityNone);
  string id = wifi_service->GetStorageIdentifier();
  for (uint i = 0; i < id.length(); ++i) {
    EXPECT_TRUE(id[i] == '_' ||
                isxdigit(id[i]) ||
                (isalpha(id[i]) && islower(id[i])));
  }
  size_t mac_pos = id.find(base::ToLowerASCII(GetAnyDeviceAddress()));
  EXPECT_NE(mac_pos, string::npos);
  EXPECT_NE(id.find(string(kModeManaged), mac_pos), string::npos);
}

// Make sure the passphrase is registered as a write only property
// by reading and comparing all string properties returned on the store.
TEST_F(WiFiServiceTest, PassphraseWriteOnly) {
  WiFiServiceRefPtr wifi_service = MakeSimpleService(kSecurityWpa);
  ReadablePropertyConstIterator<string> it =
      (wifi_service->store()).GetStringPropertiesIter();
  for ( ; !it.AtEnd(); it.Advance())
    EXPECT_NE(it.Key(), kPassphraseProperty);
}

// Make sure setting the passphrase via D-Bus Service.SetProperty validates
// the passphrase.
TEST_F(WiFiServiceTest, PassphraseSetPropertyValidation) {
  // We only spot check two password cases here to make sure the
  // SetProperty code path does validation.  We're not going to exhaustively
  // test for all types of passwords.
  WiFiServiceRefPtr wifi_service = MakeSimpleService(kSecurityWep);
  Error error;
  EXPECT_TRUE(wifi_service->mutable_store()->SetStringProperty(
                  kPassphraseProperty, "0:abcde", &error));
  EXPECT_FALSE(wifi_service->mutable_store()->SetStringProperty(
                   kPassphraseProperty, "invalid", &error));
  EXPECT_EQ(Error::kInvalidPassphrase, error.type());
}

TEST_F(WiFiServiceTest, PassphraseSetPropertyOpenNetwork) {
  WiFiServiceRefPtr wifi_service = MakeSimpleService(kSecurityNone);
  Error error;
  EXPECT_FALSE(wifi_service->mutable_store()->SetStringProperty(
                   kPassphraseProperty, "invalid", &error));
  EXPECT_EQ(Error::kNotSupported, error.type());
}

TEST_F(WiFiServiceTest, NonUTF8SSID) {
  vector<uint8_t> ssid;

  ssid.push_back(0xff);  // not a valid UTF-8 byte-sequence
  WiFiServiceRefPtr wifi_service = new WiFiService(control_interface(),
                                                   dispatcher(),
                                                   metrics(),
                                                   manager(),
                                                   provider(),
                                                   ssid,
                                                   kModeManaged,
                                                   kSecurityNone,
                                                   false);
  brillo::VariantDictionary properties;
  // if service doesn't propertly sanitize SSID, this will generate SIGABRT.
  EXPECT_TRUE(wifi_service->store().GetProperties(&properties, nullptr));
}

MATCHER(PSKSecurityArgs, "") {
  return arg.ContainsString(WPASupplicant::kPropertySecurityProtocol) &&
      arg.GetString(WPASupplicant::kPropertySecurityProtocol) ==
          string("WPA RSN") &&
      arg.ContainsString(WPASupplicant::kPropertyPreSharedKey);
}

MATCHER_P(FrequencyArg, has_arg, "") {
  return has_arg ==
      arg.ContainsInt(WPASupplicant::kNetworkPropertyFrequency);
}

TEST_F(WiFiServiceTest, ConnectReportBSSes) {
  WiFiEndpointRefPtr endpoint1 =
      MakeOpenEndpoint("a", "00:00:00:00:00:01", 0, 0);
  WiFiEndpointRefPtr endpoint2 =
      MakeOpenEndpoint("a", "00:00:00:00:00:02", 0, 0);
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityNone);
  wifi_service->AddEndpoint(endpoint1);
  wifi_service->AddEndpoint(endpoint2);
  EXPECT_CALL(*metrics(), NotifyWifiAvailableBSSes(2));
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
}

TEST_F(WiFiServiceTest, ConnectWithPreferredDevice) {
  // Setup service, device, and endpoints.
  WiFiServiceRefPtr wifi_service = MakeServiceWithMockManager();
  const string kDeviceName1 = "test_device1";
  const string kDeviceName2 = "test_device2";
  scoped_refptr<MockWiFi> wifi1 = MakeSimpleWiFi(kDeviceName1);
  scoped_refptr<MockWiFi> wifi2 = MakeSimpleWiFi(kDeviceName2);
  WiFiEndpointRefPtr endpoint1 =
      MakeOpenEndpointWithWiFi(wifi1, "a", "00:00:00:00:00:01", 0, 0);
  WiFiEndpointRefPtr endpoint2 =
      MakeOpenEndpointWithWiFi(wifi2, "a", "00:00:00:00:00:01", 0, 0);

  wifi_service->SetPreferredDevice(kDeviceName1, nullptr);
  wifi_service->AddEndpoint(endpoint1);
  wifi_service->AddEndpoint(endpoint2);
  EXPECT_EQ(wifi1, wifi_service->wifi_);

  EXPECT_CALL(*wifi1, ConnectTo(wifi_service.get()));
  EXPECT_CALL(*wifi2, ConnectTo(_)).Times(0);
  wifi_service->Connect(nullptr, "in test");
}

TEST_F(WiFiServiceTest, ConnectTaskWPA) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityWpa);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  Error error;
  wifi_service->SetPassphrase("0:mumblemumblem", &error);
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              PSKSecurityArgs());
}

TEST_F(WiFiServiceTest, ConnectTaskRSN) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityRsn);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  Error error;
  wifi_service->SetPassphrase("0:mumblemumblem", &error);
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              PSKSecurityArgs());
}

TEST_F(WiFiServiceTest, ConnectConditions) {
  Error error;
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityNone);
  scoped_refptr<MockProfile> mock_profile(
      new NiceMock<MockProfile>(control_interface(), metrics(), manager()));
  wifi_service->set_profile(mock_profile);
  // With nothing else going on, the service should attempt to connect.
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(&error, "in test");
  Mock::VerifyAndClearExpectations(wifi().get());

  // But if we're already "connecting" or "connected" then we shouldn't attempt
  // again.
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get())).Times(0);
  wifi_service->SetState(Service::kStateAssociating);
  wifi_service->Connect(&error, "in test");
  wifi_service->SetState(Service::kStateConfiguring);
  wifi_service->Connect(&error, "in test");
  wifi_service->SetState(Service::kStateConnected);
  wifi_service->Connect(&error, "in test");
  wifi_service->SetState(Service::kStatePortal);
  wifi_service->Connect(&error, "in test");
  wifi_service->SetState(Service::kStateOnline);
  wifi_service->Connect(&error, "in test");
  Mock::VerifyAndClearExpectations(wifi().get());
}

TEST_F(WiFiServiceTest, ConnectTaskPSK) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityPsk);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  Error error;
  wifi_service->SetPassphrase("0:mumblemumblem", &error);
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              PSKSecurityArgs());
}

TEST_F(WiFiServiceTest, ConnectTask8021x) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurity8021x);
  service->mutable_eap()->set_identity("identity");
  service->mutable_eap()->set_password("mumble");
  service->OnEapCredentialsChanged(Service::kReasonCredentialsLoaded);
  EXPECT_CALL(*wifi(), ConnectTo(service.get()));
  service->Connect(nullptr, "in test");
  KeyValueStore params = service->GetSupplicantConfigurationParameters();
  EXPECT_TRUE(
      params.ContainsString(WPASupplicant::kNetworkPropertyEapIdentity));
  EXPECT_TRUE(params.ContainsString(WPASupplicant::kNetworkPropertyCaPath));
}

TEST_F(WiFiServiceTest, ConnectTask8021xWithMockEap) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurity8021x);
  MockEapCredentials* eap = SetMockEap(service);
  EXPECT_CALL(*eap, IsConnectable()).WillOnce(Return(true));
  EXPECT_CALL(*wifi(), ConnectTo(service.get()));
  service->OnEapCredentialsChanged(Service::kReasonCredentialsLoaded);
  service->Connect(nullptr, "in test");

  EXPECT_CALL(*eap, PopulateSupplicantProperties(_, _));
  // The mocked function does not actually set EAP parameters so we cannot
  // expect them to be set.
  service->GetSupplicantConfigurationParameters();
}

TEST_F(WiFiServiceTest, ConnectTaskAdHocFrequency) {
  vector<uint8_t> ssid(1, 'a');
  WiFiEndpointRefPtr endpoint_nofreq =
      MakeOpenEndpoint("a", "00:00:00:00:00:01", 0, 0);
  WiFiEndpointRefPtr endpoint_freq =
      MakeOpenEndpoint("a", "00:00:00:00:00:02", 2412, 0);

  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityNone);
  wifi_service->AddEndpoint(endpoint_freq);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");

  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              FrequencyArg(false));

  wifi_service = new WiFiService(control_interface(),
                                 dispatcher(),
                                 metrics(),
                                 manager(),
                                 provider(),
                                 ssid,
                                 kModeAdhoc,
                                 kSecurityNone,
                                 false);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  SetWiFiForService(wifi_service, wifi());
  wifi_service->Connect(nullptr, "in test");

  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              FrequencyArg(false));

  wifi_service = new WiFiService(control_interface(),
                                 dispatcher(),
                                 metrics(),
                                 manager(),
                                 provider(),
                                 ssid,
                                 kModeAdhoc,
                                 kSecurityNone,
                                 false);
  wifi_service->AddEndpoint(endpoint_nofreq);
  SetWiFiForService(wifi_service, wifi());
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");

  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              FrequencyArg(false));

  wifi_service = new WiFiService(control_interface(),
                                 dispatcher(),
                                 metrics(),
                                 manager(),
                                 provider(),
                                 ssid,
                                 kModeAdhoc,
                                 kSecurityNone,
                                 false);
  wifi_service->AddEndpoint(endpoint_freq);
  SetWiFiForService(wifi_service, wifi());
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              FrequencyArg(true));
}

TEST_F(WiFiServiceTest, ConnectTaskWPA80211w) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityPsk);
  WiFiEndpointRefPtr endpoint =
      MakeOpenEndpoint("a", "00:00:00:00:00:01", 0, 0);
  endpoint->ieee80211w_required_ = true;
  wifi_service->AddEndpoint(endpoint);
  Error error;
  wifi_service->SetPassphrase("0:mumblemumblem", &error);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");

  KeyValueStore params =
      wifi_service->GetSupplicantConfigurationParameters();
  EXPECT_TRUE(params.ContainsString(WPASupplicant::kPropertySecurityProtocol));
  EXPECT_TRUE(params.ContainsString(WPASupplicant::kPropertyPreSharedKey));
  EXPECT_TRUE(params.ContainsUint(WPASupplicant::kNetworkPropertyIeee80211w));
}

MATCHER_P(WEPSecurityArgsKeyIndex, index, "") {
  uint32_t index_u32 = index;
  return arg.ContainsString(WPASupplicant::kPropertyAuthAlg) &&
      arg.ContainsUint8s(
                  WPASupplicant::kPropertyWEPKey + base::IntToString(index)) &&
      arg.ContainsUint(WPASupplicant::kPropertyWEPTxKeyIndex) &&
      (arg.GetUint(WPASupplicant::kPropertyWEPTxKeyIndex) == index_u32);
}

TEST_F(WiFiServiceTest, ConnectTaskWEP) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityWep);
  Error error;
  wifi_service->SetPassphrase("0:abcdefghijklm", &error);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              WEPSecurityArgsKeyIndex(0));

  wifi_service->SetPassphrase("abcdefghijklm", &error);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              WEPSecurityArgsKeyIndex(0));

  wifi_service->SetPassphrase("1:abcdefghijklm", &error);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              WEPSecurityArgsKeyIndex(1));

  wifi_service->SetPassphrase("2:abcdefghijklm", &error);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              WEPSecurityArgsKeyIndex(2));

  wifi_service->SetPassphrase("3:abcdefghijklm", &error);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  EXPECT_THAT(wifi_service->GetSupplicantConfigurationParameters(),
              WEPSecurityArgsKeyIndex(3));
}

// Dynamic WEP + 802.1x.
TEST_F(WiFiServiceTest, ConnectTaskDynamicWEP) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityWep);

  wifi_service->mutable_eap()->SetKeyManagement("IEEE8021X", nullptr);
  wifi_service->mutable_eap()->set_identity("something");
  wifi_service->mutable_eap()->set_password("mumble");
  wifi_service->OnEapCredentialsChanged(Service::kReasonCredentialsLoaded);
  EXPECT_CALL(*wifi(), ConnectTo(wifi_service.get()));
  wifi_service->Connect(nullptr, "in test");
  KeyValueStore params =
      wifi_service->GetSupplicantConfigurationParameters();
  EXPECT_TRUE(
      params.ContainsString(WPASupplicant::kNetworkPropertyEapIdentity));
  EXPECT_TRUE(params.ContainsString(WPASupplicant::kNetworkPropertyCaPath));
  EXPECT_FALSE(
      params.ContainsString(WPASupplicant::kPropertySecurityProtocol));
}

TEST_F(WiFiServiceTest, SetPassphraseResetHasEverConnected) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityRsn);
  const string kPassphrase = "abcdefgh";

  Error error;
  // A changed passphrase should reset has_ever_connected_ field.
  wifi_service->has_ever_connected_ = true;
  EXPECT_TRUE(wifi_service->has_ever_connected());
  wifi_service->SetPassphrase(kPassphrase, &error);
  EXPECT_FALSE(wifi_service->has_ever_connected());
}

TEST_F(WiFiServiceTest, SetPassphraseRemovesCachedCredentials) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityRsn);

  const string kPassphrase = "abcdefgh";

  {
    Error error;
    // A changed passphrase should trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    wifi_service->SetPassphrase(kPassphrase, &error);
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }

  {
    Error error;
    // An unchanged passphrase should not trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(_)).Times(0);
    wifi_service->SetPassphrase(kPassphrase, &error);
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }

  {
    Error error;
    // A modified passphrase should trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    wifi_service->SetPassphrase(kPassphrase + "X", &error);
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }

  {
    Error error;
    // A cleared passphrase should also trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    wifi_service->ClearPassphrase(&error);
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }

  {
    Error error;
    // An invalid passphrase should not trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(_)).Times(0);
    wifi_service->SetPassphrase("", &error);
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_FALSE(error.IsSuccess());
  }

  {
    // A change to EAP parameters in a PSK (non 802.1x) service will not
    // trigger cache removal.
    wifi_service->has_ever_connected_ = true;
    EXPECT_TRUE(wifi_service->has_ever_connected());
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get())).Times(0);
    wifi_service->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
    EXPECT_TRUE(wifi_service->has_ever_connected());
    Mock::VerifyAndClearExpectations(wifi().get());
  }

  WiFiServiceRefPtr eap_wifi_service = MakeServiceWithWiFi(kSecurity8021x);

  {
    // Any change to EAP parameters (including a null one) will trigger cache
    // removal in an 802.1x service.  This is a lot less granular than the
    // passphrase checks above.
    // Changes in EAP parameters should also clear has_ever_connected_.
    eap_wifi_service->has_ever_connected_ = true;
    EXPECT_TRUE(eap_wifi_service->has_ever_connected());
    EXPECT_CALL(*wifi(), ClearCachedCredentials(eap_wifi_service.get()));
    eap_wifi_service->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
    EXPECT_FALSE(eap_wifi_service->has_ever_connected());
    Mock::VerifyAndClearExpectations(wifi().get());
  }
}

// This test is somewhat redundant, since:
//
// a) we test that generic property setters return false on a null
//    change (e.g. in PropertyAccessorTest.SignedIntCorrectness)
// b) we test that custom EAP property setters return false on a null
//    change in EapCredentialsTest.CustomSetterNoopChange
// c) we test that the various custom accessors pass through the
//    return value of custom setters
//    (e.g. PropertyAccessorTest.CustomAccessorCorrectness)
// d) we test that PropertyStore skips the change callback when a
//    property setter return false (PropertyStoreTypedTest.SetProperty)
//
// Nonetheless, I think it's worth testing the WiFi+EAP case directly.
TEST_F(WiFiServiceTest, EapAuthPropertyChangeClearsCachedCredentials) {
  WiFiServiceRefPtr wifi_service =
      MakeServiceWithWiFi(kSecurity8021x);
  PropertyStore& property_store(*wifi_service->mutable_store());

  // Property with custom accessor.
  const string kPassword = "abcdefgh";
  {
    Error error;
    // A changed passphrase should trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    EXPECT_TRUE(property_store.SetStringProperty(
        kEapPasswordProperty, kPassword, &error));
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    // An unchanged passphrase should not trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(_)).Times(0);
    EXPECT_FALSE(property_store.SetStringProperty(
        kEapPasswordProperty, kPassword, &error));
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    // A modified passphrase should trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    EXPECT_TRUE(property_store.SetStringProperty(
        kEapPasswordProperty, kPassword + "X", &error));
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }

  // Property with generic accessor.
  const string kCertId = "abcdefgh";
  {
    Error error;
    // A changed cert id should trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    EXPECT_TRUE(property_store.SetStringProperty(
        kEapCertIdProperty, kCertId, &error));
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    // An unchanged cert id should not trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(_)).Times(0);
    EXPECT_FALSE(property_store.SetStringProperty(
        kEapCertIdProperty, kCertId, &error));
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    // A modified cert id should trigger cache removal.
    EXPECT_CALL(*wifi(), ClearCachedCredentials(wifi_service.get()));
    EXPECT_TRUE(property_store.SetStringProperty(
        kEapCertIdProperty, kCertId + "X", &error));
    Mock::VerifyAndClearExpectations(wifi().get());
    EXPECT_TRUE(error.IsSuccess());
  }
}

TEST_F(WiFiServiceTest, LoadHidden) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  ASSERT_FALSE(service->hidden_ssid_);
  NiceMock<MockStore> mock_store;
  const string storage_id = service->GetStorageIdentifier();
  set<string> groups;
  groups.insert(storage_id);
  EXPECT_CALL(mock_store, ContainsGroup(StrEq(storage_id)))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_store, GetGroupsWithProperties(
      ContainsWiFiProperties(
          simple_ssid(), kModeManaged, kSecurityNone)))
      .WillRepeatedly(Return(groups));
  EXPECT_CALL(mock_store, GetBool(_, _, _))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_store,
              GetBool(StrEq(storage_id), WiFiService::kStorageHiddenSSID, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<2>(true), Return(true)));
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_TRUE(service->hidden_ssid_);
}

TEST_F(WiFiServiceTest, SetPassphraseForNonPassphraseService) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  NiceMock<MockStore> mock_store;
  const string storage_id = service->GetStorageIdentifier();
  set<string> groups;
  groups.insert(storage_id);
  EXPECT_CALL(mock_store, ContainsGroup(StrEq(storage_id)))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_store, GetGroupsWithProperties(
      ContainsWiFiProperties(
          simple_ssid(), kModeManaged, kSecurityNone)))
      .WillRepeatedly(Return(groups));

  EXPECT_TRUE(service->Load(&mock_store));
  Error error;
  EXPECT_FALSE(service->SetPassphrase("password", &error));
  EXPECT_TRUE(error.type() == Error::kNotSupported);
}

TEST_F(WiFiServiceTest, LoadMultipleMatchingGroups) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurityNone);
  set<string> groups;
  groups.insert("id0");
  groups.insert("id1");
  // Make sure we retain the first matched group in the same way that
  // WiFiService::Load() will.
  string first_group = *groups.begin();

  NiceMock<MockStore> mock_store;
  EXPECT_CALL(mock_store, GetGroupsWithProperties(
      ContainsWiFiProperties(
          simple_ssid(), kModeManaged, kSecurityNone)))
      .WillRepeatedly(Return(groups));
  EXPECT_CALL(mock_store, ContainsGroup(first_group))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(mock_store, ContainsGroup(StrNe(first_group))).Times(0);
  EXPECT_CALL(mock_store, GetBool(first_group, _, _))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_store, GetBool(StrNe(first_group), _, _)).Times(0);
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_WARNING, _,
                       EndsWith("choosing the first.")));
  EXPECT_TRUE(service->Load(&mock_store));
}

TEST_F(WiFiServiceSecurityTest, WPAMapping) {
  EXPECT_TRUE(TestStorageMapping(kSecurityRsn, kSecurityPsk));
  EXPECT_TRUE(TestStorageMapping(kSecurityWpa, kSecurityPsk));
  EXPECT_TRUE(TestStorageMapping(kSecurityPsk, kSecurityPsk));
  EXPECT_TRUE(TestStorageMapping(kSecurityWep, kSecurityWep));
  EXPECT_TRUE(TestStorageMapping(kSecurityNone, kSecurityNone));
  EXPECT_TRUE(TestStorageMapping(kSecurity8021x, kSecurity8021x));
}

TEST_F(WiFiServiceSecurityTest, LoadMapping) {
  EXPECT_TRUE(TestLoadMapping(kSecurityRsn, kSecurityPsk, true));
  EXPECT_TRUE(TestLoadMapping(kSecurityRsn, kSecurityRsn, false));
  EXPECT_TRUE(TestLoadMapping(kSecurityRsn, kSecurityWpa, false));
  EXPECT_TRUE(TestLoadMapping(kSecurityWpa, kSecurityPsk, true));
  EXPECT_TRUE(TestLoadMapping(kSecurityWpa, kSecurityWpa, false));
  EXPECT_TRUE(TestLoadMapping(kSecurityWpa, kSecurityRsn, false));
  EXPECT_TRUE(TestLoadMapping(kSecurityWep, kSecurityWep, true));
  EXPECT_TRUE(TestLoadMapping(kSecurityWep, kSecurityPsk, false));
}

TEST_F(WiFiServiceTest, LoadAndUnloadPassphrase) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityPsk);
  NiceMock<MockStore> mock_store;
  const string kStorageId = service->GetStorageIdentifier();
  EXPECT_CALL(mock_store, ContainsGroup(StrEq(kStorageId)))
      .WillRepeatedly(Return(true));
  set<string> groups;
  groups.insert(kStorageId);
  EXPECT_CALL(mock_store, GetGroupsWithProperties(
      ContainsWiFiProperties(
          simple_ssid(), kModeManaged, kSecurityPsk)))
      .WillRepeatedly(Return(groups));
  EXPECT_CALL(mock_store, GetBool(_, _, _))
      .WillRepeatedly(Return(false));
  const string kPassphrase = "passphrase";
  EXPECT_CALL(mock_store,
              GetCryptedString(StrEq(kStorageId),
                               WiFiService::kStoragePassphrase, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<2>(kPassphrase), Return(true)));
  EXPECT_CALL(mock_store,
              GetCryptedString(StrEq(kStorageId),
                               StrNe(WiFiService::kStoragePassphrase), _))
      .WillRepeatedly(Return(false));
  EXPECT_TRUE(service->need_passphrase_);
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(kPassphrase, service->passphrase_);
  EXPECT_TRUE(service->connectable());
  EXPECT_FALSE(service->need_passphrase_);
  service->Unload();
  EXPECT_EQ(string(""), service->passphrase_);
  EXPECT_FALSE(service->connectable());
  EXPECT_TRUE(service->need_passphrase_);
}

TEST_F(WiFiServiceTest, LoadPassphraseClearCredentials) {
  const string kOldPassphrase = "oldpassphrase";
  const string kPassphrase = "passphrase";

  const bool kHasEverConnected = true;
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurityPsk);
  NiceMock<MockStore> mock_store;
  const string kStorageId = service->GetStorageIdentifier();
  EXPECT_CALL(mock_store, ContainsGroup(StrEq(kStorageId)))
      .WillRepeatedly(Return(true));
  set<string> groups;
  groups.insert(kStorageId);
  EXPECT_CALL(mock_store, GetGroupsWithProperties(
      ContainsWiFiProperties(
          simple_ssid(), kModeManaged, kSecurityPsk)))
      .WillRepeatedly(Return(groups));
  EXPECT_CALL(mock_store, GetBool(_, _, _))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_store,
              GetCryptedString(StrEq(kStorageId),
                               WiFiService::kStoragePassphrase, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<2>(kPassphrase), Return(true)));
  EXPECT_CALL(mock_store,
              GetCryptedString(StrEq(kStorageId),
                               StrNe(WiFiService::kStoragePassphrase), _))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(mock_store,
              GetBool(kStorageId, Service::kStorageHasEverConnected, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<2>(kHasEverConnected),
                            Return(true)));
  // Set old passphrase for service
  EXPECT_TRUE(service->need_passphrase_);
  service->passphrase_ = kOldPassphrase;
  service->has_ever_connected_ = true;

  scoped_refptr<MockProfile> mock_profile(
      new NiceMock<MockProfile>(control_interface(), metrics(), manager()));
  service->set_profile(mock_profile);
  // Detect if the service is going to attempt to update the stored profile.
  EXPECT_CALL(*mock_profile, GetConstStorage()).Times(0);

  // The kOldPassphrase is different than the newly loaded passhprase,
  // so the credentials should be cleared.
  EXPECT_CALL(*wifi(), ClearCachedCredentials(_)).Times(1);
  EXPECT_CALL(*mock_profile, UpdateService(_)).Times(0);
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(kPassphrase, service->passphrase_);
  EXPECT_TRUE(service->has_ever_connected_);

  Mock::VerifyAndClearExpectations(wifi().get());
  Mock::VerifyAndClearExpectations(mock_profile.get());


  // Repeat Service::Load with same old and new passphrase. Since the old
  // and new passphrase match, verify the cache is not cleared during
  // profile load.
  service->set_profile(mock_profile);
  EXPECT_CALL(*mock_profile, GetConstStorage()).Times(0);
  EXPECT_CALL(*wifi(), ClearCachedCredentials(_)).Times(0);
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(kPassphrase, service->passphrase_);
  EXPECT_TRUE(service->has_ever_connected_);
}

TEST_F(WiFiServiceTest, ConfigureMakesConnectable) {
  string guid("legit_guid");
  KeyValueStore args;
  args.SetString(kEapIdentityProperty, "legit_identity");
  args.SetString(kEapPasswordProperty, "legit_password");
  args.SetString(kEapMethodProperty, "PEAP");
  args.SetString(kGuidProperty, guid);
  Error error;

  WiFiServiceRefPtr service = MakeSimpleService(kSecurity8021x);
  // Hack the GUID in so that we don't have to mess about with WiFi to regsiter
  // our service.  This way, Manager will handle the lookup itself.
  service->SetGuid(guid, nullptr);
  manager()->RegisterService(service);
  EXPECT_FALSE(service->connectable());
  EXPECT_EQ(service.get(), manager()->GetService(args, &error).get());
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(service->connectable());
}

TEST_F(WiFiServiceTest, ConfigurePassphrase) {
  EXPECT_EQ(Error::kNotSupported,
            TestConfigurePassphrase(kSecurityNone, ""));
  EXPECT_EQ(Error::kNotSupported,
            TestConfigurePassphrase(kSecurityNone, "foo"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep, nullptr));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, ""));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, "abcd"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep, "abcde"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep, "abcdefghijklm"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep, "0:abcdefghijklm"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep, "0102030405"));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, "0x0102030405"));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, "O102030405"));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, "1:O102030405"));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, "1:0xO102030405"));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWep, "0xO102030405"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep,
                                    "0102030405060708090a0b0c0d"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep,
                                    "0102030405060708090A0B0C0D"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep,
                                    "0:0102030405060708090a0b0c0d"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWep,
                                    "0:0x0102030405060708090a0b0c0d"));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWpa, nullptr));
  EXPECT_EQ(Error::kSuccess,
            TestConfigurePassphrase(kSecurityWpa, "secure password"));
  EXPECT_EQ(Error::kInvalidPassphrase,
            TestConfigurePassphrase(kSecurityWpa, ""));
  EXPECT_EQ(Error::kSuccess, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAAsciiMinLen, 'Z').c_str()));
  EXPECT_EQ(Error::kSuccess, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAAsciiMaxLen, 'Z').c_str()));
  // subtle: invalid length for hex key, but valid as ascii passphrase
  EXPECT_EQ(Error::kSuccess, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAHexLen-1, '1').c_str()));
  EXPECT_EQ(Error::kSuccess, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAHexLen, '1').c_str()));
  EXPECT_EQ(Error::kInvalidPassphrase, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAAsciiMinLen-1, 'Z').c_str()));
  EXPECT_EQ(Error::kInvalidPassphrase, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAAsciiMaxLen+1, 'Z').c_str()));
  EXPECT_EQ(Error::kInvalidPassphrase, TestConfigurePassphrase(
      kSecurityWpa,
      string(IEEE_80211::kWPAHexLen+1, '1').c_str()));
}

TEST_F(WiFiServiceTest, ConfigureRedundantProperties) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeWifi);
  args.SetString(kSSIDProperty, simple_ssid_string());
  args.SetString(kSecurityProperty, kSecurityNone);
  args.SetString(kWifiHexSsid, "This is ignored even if it is invalid hex.");
  const string kGUID = "aguid";
  args.SetString(kGuidProperty, kGUID);

  EXPECT_EQ("", service->guid());
  Error error;
  service->Configure(args, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kGUID, service->guid());
}

TEST_F(WiFiServiceTest, DisconnectWithWiFi) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurityWep);
  EXPECT_CALL(*wifi(), DisconnectFromIfActive(service.get())).Times(1);
  Error error;
  service->Disconnect(&error, "in test");
}

TEST_F(WiFiServiceTest, DisconnectWithoutWiFi) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityWep);
  EXPECT_CALL(*wifi(), DisconnectFrom(_)).Times(0);
  Error error;
  service->Disconnect(&error, "in test");
  EXPECT_EQ(Error::kOperationFailed, error.type());
}

TEST_F(WiFiServiceTest, DisconnectWithoutWiFiWhileAssociating) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityWep);
  EXPECT_CALL(*wifi(), DisconnectFrom(_)).Times(0);
  service->SetState(Service::kStateAssociating);
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_ERROR, _,
                       HasSubstr("WiFi endpoints do not (yet) exist.")));
  Error error;
  service->Disconnect(&error, "in test");
  EXPECT_EQ(Error::kOperationFailed, error.type());
}

TEST_F(WiFiServiceTest, UnloadAndClearCacheWEP) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurityWep);
  EXPECT_CALL(*wifi(), ClearCachedCredentials(service.get())).Times(1);
  EXPECT_CALL(*wifi(), DisconnectFromIfActive(service.get())).Times(1);
  service->Unload();
}

TEST_F(WiFiServiceTest, UnloadAndClearCache8021x) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurity8021x);
  EXPECT_CALL(*wifi(), ClearCachedCredentials(service.get())).Times(1);
  EXPECT_CALL(*wifi(), DisconnectFromIfActive(service.get())).Times(1);
  service->Unload();
}

TEST_F(WiFiServiceTest, ParseStorageIdentifierNone) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  const string storage_id = service->GetStorageIdentifier();
  string address;
  string mode;
  string security;
  EXPECT_TRUE(service->ParseStorageIdentifier(storage_id, &address, &mode,
                                              &security));
  EXPECT_EQ(base::ToLowerASCII(GetAnyDeviceAddress()), address);
  EXPECT_EQ(kModeManaged, mode);
  EXPECT_EQ(kSecurityNone, security);
}

TEST_F(WiFiServiceTest, ParseStorageIdentifier8021x) {
  // Do a separate test for 802.1x, since kSecurity8021x contains a "_",
  // which needs to be dealt with specially in the parser.
  WiFiServiceRefPtr service = MakeSimpleService(kSecurity8021x);
  const string storage_id = service->GetStorageIdentifier();
  string address;
  string mode;
  string security;
  EXPECT_TRUE(service->ParseStorageIdentifier(storage_id, &address, &mode,
                                              &security));
  EXPECT_EQ(base::ToLowerASCII(GetAnyDeviceAddress()), address);
  EXPECT_EQ(kModeManaged, mode);
  EXPECT_EQ(kSecurity8021x, security);
}

TEST_F(WiFiServiceFixupStorageTest, FixedEntries) {
  const string kNonWiFiId = "vpn_foo";
  const string kUnparsableWiFiId = "wifi_foo";

  AddGroup(kNonWiFiId);
  AddGroup(kUnparsableWiFiId);
  AddServiceEntry(true, true, true, true);
  AddServiceEntry(false, false, false, false);
  AddServiceEntry(true, true, true, true);
  AddServiceEntry(false, false, false, false);
  EXPECT_TRUE(FixupServiceEntries());
}

TEST_F(WiFiServiceFixupStorageTest, NoFixedEntries) {
  const string kNonWiFiId = "vpn_foo";
  const string kUnparsableWiFiId = "wifi_foo";

  AddGroup(kNonWiFiId);
  AddGroup(kUnparsableWiFiId);
  AddServiceEntry(true, true, true, true);
  EXPECT_FALSE(FixupServiceEntries());
}

TEST_F(WiFiServiceFixupStorageTest, MissingTypeProperty) {
  AddServiceEntry(false, true, true, true);
  EXPECT_TRUE(FixupServiceEntries());
}

TEST_F(WiFiServiceFixupStorageTest, MissingModeProperty) {
  AddServiceEntry(true, false, true, true);
  EXPECT_TRUE(FixupServiceEntries());
}

TEST_F(WiFiServiceFixupStorageTest, MissingSecurityProperty) {
  AddServiceEntry(true, true, false, true);
  EXPECT_TRUE(FixupServiceEntries());
}

TEST_F(WiFiServiceFixupStorageTest, MissingSecurityClassProperty) {
  AddServiceEntry(true, true, true, false);
  EXPECT_TRUE(FixupServiceEntries());
}

TEST_F(WiFiServiceTest, Connectable) {
  // Open network should be connectable.
  EXPECT_TRUE(CheckConnectable(kSecurityNone, nullptr, false));

  // Open network should remain connectable if we try to set a password on it.
  EXPECT_TRUE(CheckConnectable(kSecurityNone, "abcde", false));

  // WEP network with passphrase set should be connectable.
  EXPECT_TRUE(CheckConnectable(kSecurityWep, "abcde", false));

  // WEP network without passphrase set should NOT be connectable.
  EXPECT_FALSE(CheckConnectable(kSecurityWep, nullptr, false));

  // A bad passphrase should not make a WEP network connectable.
  EXPECT_FALSE(CheckConnectable(kSecurityWep, "a", false));

  // Similar to WEP, for WPA.
  EXPECT_TRUE(CheckConnectable(kSecurityWpa, "abcdefgh", false));
  EXPECT_FALSE(CheckConnectable(kSecurityWpa, nullptr, false));
  EXPECT_FALSE(CheckConnectable(kSecurityWpa, "a", false));

  // 802.1x without connectable EAP credentials should NOT be connectable.
  EXPECT_FALSE(CheckConnectable(kSecurity8021x, nullptr, false));

  // 802.1x with connectable EAP credentials should be connectable.
  EXPECT_TRUE(CheckConnectable(kSecurity8021x, nullptr, true));

  // Dynamic WEP + 802.1X should be connectable under the same conditions.
  EXPECT_TRUE(CheckConnectable(kSecurityWep, nullptr, true));
}

TEST_F(WiFiServiceTest, IsAutoConnectable) {
  const char* reason;
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  EXPECT_CALL(*wifi(), IsIdle())
      .WillRepeatedly(Return(true));
  EXPECT_FALSE(service->HasEndpoints());
  EXPECT_FALSE(service->IsAutoConnectable(&reason));
  EXPECT_STREQ(WiFiService::kAutoConnNoEndpoint, reason);

  reason = "";
  WiFiEndpointRefPtr endpoint =
      MakeOpenEndpoint("a", "00:00:00:00:00:01", 0, 0);
  service->AddEndpoint(endpoint);
  EXPECT_CALL(*wifi(), IsIdle())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(service->HasEndpoints());
  EXPECT_TRUE(service->IsAutoConnectable(&reason));
  EXPECT_STREQ("", reason);

  // WiFi only supports connecting to one Service at a time. So, to
  // avoid disrupting connectivity, we only allow auto-connection to
  // a WiFiService when the corresponding WiFi is idle.
  EXPECT_CALL(*wifi(), IsIdle())
      .WillRepeatedly(Return(false));
  EXPECT_TRUE(service->HasEndpoints());
  EXPECT_FALSE(service->IsAutoConnectable(&reason));
  EXPECT_STREQ(WiFiService::kAutoConnBusy, reason);
}

TEST_F(WiFiServiceTest, AutoConnect) {
  const char* reason;
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  EXPECT_FALSE(service->IsAutoConnectable(&reason));
  EXPECT_CALL(*wifi(), ConnectTo(_)).Times(0);
  service->AutoConnect();
  dispatcher()->DispatchPendingEvents();

  WiFiEndpointRefPtr endpoint =
      MakeOpenEndpoint("a", "00:00:00:00:00:01", 0, 0);
  service->AddEndpoint(endpoint);
  EXPECT_CALL(*wifi(), IsIdle())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(service->IsAutoConnectable(&reason));
  EXPECT_CALL(*wifi(), ConnectTo(_));
  service->AutoConnect();
  dispatcher()->DispatchPendingEvents();

  Error error;
  service->UserInitiatedDisconnect(&error);
  dispatcher()->DispatchPendingEvents();
  EXPECT_FALSE(service->IsAutoConnectable(&reason));
}

TEST_F(WiFiServiceTest, ClearWriteOnlyDerivedProperty) {
  WiFiServiceRefPtr wifi_service = MakeSimpleService(kSecurityWep);

  EXPECT_EQ("", wifi_service->passphrase_);

  Error error;
  const string kPassphrase = "0:abcde";
  EXPECT_TRUE(
      wifi_service->mutable_store()->SetAnyProperty(kPassphraseProperty,
                                                    brillo::Any(kPassphrase),
                                                    &error));
  EXPECT_EQ(kPassphrase, wifi_service->passphrase_);

  EXPECT_TRUE(wifi_service->mutable_store()->ClearProperty(kPassphraseProperty,
                                                           &error));
  EXPECT_EQ("", wifi_service->passphrase_);
}

TEST_F(WiFiServiceTest, SignalToStrength) {
  // Verify that our mapping is sane, in the sense that it preserves ordering.
  // We break the test into two domains, because we assume that positive
  // values aren't actually in dBm.
  for (int16_t i = std::numeric_limits<int16_t>::min(); i < 0; ++i) {
    int16_t current_mapped = WiFiService::SignalToStrength(i);
    int16_t next_mapped =  WiFiService::SignalToStrength(i+1);
    EXPECT_LE(current_mapped, next_mapped)
        << "(original values " << i << " " << i+1 << ")";
    EXPECT_GE(current_mapped, Service::kStrengthMin);
    EXPECT_LE(current_mapped, Service::kStrengthMax);
  }
  for (int16_t i = 1; i < std::numeric_limits<int16_t>::max(); ++i) {
    int16_t current_mapped = WiFiService::SignalToStrength(i);
    int16_t next_mapped =  WiFiService::SignalToStrength(i+1);
    EXPECT_LE(current_mapped, next_mapped)
        << "(original values " << i << " " << i+1 << ")";
    EXPECT_GE(current_mapped, Service::kStrengthMin);
    EXPECT_LE(current_mapped, Service::kStrengthMax);
  }
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, Strengths) {
  // If the chosen signal values don't map to distinct strength
  // values, then we can't expect our other tests to pass. So verify
  // their distinctness.
  EXPECT_TRUE(kOkEndpointStrength != kBadEndpointStrength);
  EXPECT_TRUE(kOkEndpointStrength != kGoodEndpointStrength);
  EXPECT_TRUE(kGoodEndpointStrength != kBadEndpointStrength);
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, Floating) {
  // Initial endpoint updates values.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, kOkEndpointFrequency));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, kOkEndpointBssId));
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, kOkEndpointStrength));
  EXPECT_CALL(adaptor,
              EmitUint16Changed(kWifiPhyMode, Metrics::kWiFiNetworkPhyMode11b));
  service->AddEndpoint(ok_endpoint);
  EXPECT_EQ(1, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Endpoint with stronger signal updates values.
  EXPECT_CALL(adaptor,
              EmitUint16Changed(kWifiFrequency, kGoodEndpointFrequency));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, kGoodEndpointBssId));
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, kGoodEndpointStrength));
  // However, both endpoints are 11b.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiPhyMode, _)).Times(0);
  service->AddEndpoint(good_endpoint);
  EXPECT_EQ(2, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Endpoint with lower signal does not change values.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _)).Times(0);
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiPhyMode, _)).Times(0);
  service->AddEndpoint(bad_endpoint);
  EXPECT_EQ(3, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Removing non-optimal endpoint does not change values.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _)).Times(0);
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiPhyMode, _)).Times(0);
  service->RemoveEndpoint(bad_endpoint);
  EXPECT_EQ(2, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Removing optimal endpoint updates values.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, kOkEndpointFrequency));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, kOkEndpointBssId));
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, kOkEndpointStrength));
  // However, both endpoints are 11b.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiPhyMode, _)).Times(0);
  service->RemoveEndpoint(good_endpoint);
  EXPECT_EQ(1, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Removing last endpoint updates values (and doesn't crash).
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _));
  EXPECT_CALL(adaptor, EmitUint8Changed(kSignalStrengthProperty, _));
  EXPECT_CALL(adaptor, EmitUint16Changed(
      kWifiPhyMode, Metrics::kWiFiNetworkPhyModeUndef));
  service->RemoveEndpoint(ok_endpoint);
  EXPECT_EQ(0, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, Connected) {
  EXPECT_CALL(adaptor, EmitUint16Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitStringChanged(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitUint8Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitBoolChanged(_, _)).Times(AnyNumber());
  service->AddEndpoint(bad_endpoint);
  service->AddEndpoint(ok_endpoint);
  EXPECT_EQ(2, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Setting current endpoint forces adoption of its values, even if it
  // doesn't have the highest signal.
  EXPECT_CALL(adaptor,
              EmitUint16Changed(kWifiFrequency, kBadEndpointFrequency));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, kBadEndpointBssId));
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, kBadEndpointStrength));
  service->NotifyCurrentEndpoint(bad_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Adding a better endpoint doesn't matter, when current endpoint is set.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _)).Times(0);
  service->AddEndpoint(good_endpoint);
  EXPECT_EQ(3, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Removing a better endpoint doesn't matter, when current endpoint is set.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _)).Times(0);
  service->RemoveEndpoint(good_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Removing the current endpoint is safe and sane.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, kOkEndpointFrequency));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, kOkEndpointBssId));
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, kOkEndpointStrength));
  service->RemoveEndpoint(bad_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Clearing the current endpoint (without removing it) is also safe and sane.
  service->NotifyCurrentEndpoint(ok_endpoint);
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _)).Times(0);
  service->NotifyCurrentEndpoint(nullptr);
  Mock::VerifyAndClearExpectations(&adaptor);
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, EndpointModified) {
  EXPECT_CALL(adaptor, EmitUint16Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitStringChanged(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitUint8Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitBoolChanged(_, _)).Times(AnyNumber());
  service->AddEndpoint(ok_endpoint);
  service->AddEndpoint(good_endpoint);
  EXPECT_EQ(2, service->GetEndpointCount());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Updating sub-optimal Endpoint doesn't update Service.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor,
              EmitUint8Changed(kSignalStrengthProperty, _)).Times(0);
  ok_endpoint->signal_strength_ = (kOkEndpointSignal + kGoodEndpointSignal) / 2;
  service->NotifyEndpointUpdated(ok_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Updating optimal Endpoint updates appropriate Service property.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, _)).Times(0);
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, _)).Times(0);
  EXPECT_CALL(adaptor, EmitUint8Changed(kSignalStrengthProperty, _));
  good_endpoint->signal_strength_ = kGoodEndpointSignal + 1;
  service->NotifyEndpointUpdated(good_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Change in optimal Endpoint updates Service properties.
  EXPECT_CALL(adaptor, EmitUint16Changed(kWifiFrequency, kOkEndpointFrequency));
  EXPECT_CALL(adaptor, EmitStringChanged(kWifiBSsid, kOkEndpointBssId));
  EXPECT_CALL(adaptor, EmitUint8Changed(kSignalStrengthProperty, _));
  ok_endpoint->signal_strength_ = kGoodEndpointSignal + 2;
  service->NotifyEndpointUpdated(ok_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, Ieee80211w) {
  EXPECT_CALL(adaptor, EmitUint16Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitStringChanged(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitUint8Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitBoolChanged(_, _)).Times(AnyNumber());
  service->AddEndpoint(ok_endpoint);
  EXPECT_FALSE(service->ieee80211w_required());
  good_endpoint->ieee80211w_required_ = true;
  service->AddEndpoint(good_endpoint);
  EXPECT_TRUE(service->ieee80211w_required());
  service->RemoveEndpoint(good_endpoint);
  EXPECT_TRUE(service->ieee80211w_required());
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, PhysicalMode) {
  EXPECT_CALL(adaptor, EmitUint16Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitStringChanged(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitUint8Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitBoolChanged(_, _)).Times(AnyNumber());

  // No endpoints -> undef.
  EXPECT_EQ(Metrics::kWiFiNetworkPhyModeUndef, service->physical_mode());

  // Endpoint has unknown physical mode -> undef.
  ok_endpoint->physical_mode_ = Metrics::kWiFiNetworkPhyModeUndef;
  service->AddEndpoint(ok_endpoint);
  EXPECT_EQ(Metrics::kWiFiNetworkPhyModeUndef, service->physical_mode());

  // New endpoint with 802.11a -> 802.11a.
  good_endpoint->physical_mode_ = Metrics::kWiFiNetworkPhyMode11a;
  service->AddEndpoint(good_endpoint);
  EXPECT_EQ(Metrics::kWiFiNetworkPhyMode11a, service->physical_mode());

  // Remove 802.11a endpoint -> undef.
  service->RemoveEndpoint(good_endpoint);
  EXPECT_EQ(Metrics::kWiFiNetworkPhyModeUndef, service->physical_mode());

  // Change endpoint -> take endpoint's new value.
  ok_endpoint->physical_mode_ = Metrics::kWiFiNetworkPhyMode11n;
  service->NotifyEndpointUpdated(ok_endpoint);
  EXPECT_EQ(Metrics::kWiFiNetworkPhyMode11n, service->physical_mode());

  // No endpoints -> undef.
  service->RemoveEndpoint(ok_endpoint);
  EXPECT_EQ(Metrics::kWiFiNetworkPhyModeUndef, service->physical_mode());
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, WarningOnDisconnect) {
  service->AddEndpoint(ok_endpoint);
  service->SetState(Service::kStateAssociating);
  ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(log, Log(logging::LOG_WARNING, _,
                       EndsWith("disconnect due to no remaining endpoints.")));
  service->RemoveEndpoint(ok_endpoint);
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, AddEndpointWithPreferredDevice) {
  // Setup service, device, and endpoints.
  WiFiServiceRefPtr wifi_service = MakeServiceWithMockManager();
  const string kDeviceName1 = "test_device1";
  const string kDeviceName2 = "test_device2";
  scoped_refptr<MockWiFi> wifi1 = MakeSimpleWiFi(kDeviceName1);
  scoped_refptr<MockWiFi> wifi2 = MakeSimpleWiFi(kDeviceName2);
  // Best signal for endpoint associated with the preferred device.
  const int16_t kPreferredDeviceBestSignal = -40;
  WiFiEndpointRefPtr endpoint0 =
      MakeOpenEndpointWithWiFi(wifi2, "a", "00:00:00:00:00:01", 0,
                               kPreferredDeviceBestSignal + 10);
  WiFiEndpointRefPtr endpoint1 =
      MakeOpenEndpointWithWiFi(wifi1, "a", "00:00:00:00:00:01", 0,
                               kPreferredDeviceBestSignal - 10);
  WiFiEndpointRefPtr endpoint2 =
      MakeOpenEndpointWithWiFi(wifi1, "a", "00:00:00:00:00:01", 0,
                               kPreferredDeviceBestSignal);
  WiFiEndpointRefPtr endpoint3 =
      MakeOpenEndpointWithWiFi(wifi2, "a", "00:00:00:00:00:01", 0,
                               kPreferredDeviceBestSignal + 10);

  wifi_service->SetPreferredDevice(kDeviceName1, nullptr);

  wifi_service->AddEndpoint(endpoint0);
  wifi_service->AddEndpoint(endpoint1);
  wifi_service->AddEndpoint(endpoint2);
  wifi_service->AddEndpoint(endpoint3);
  EXPECT_EQ(wifi1, wifi_service->wifi_);
  // Service should display the signal strength of the best signal endpoint
  // that's associated with the preferred device.
  EXPECT_EQ(WiFiService::SignalToStrength(kPreferredDeviceBestSignal),
            wifi_service->strength());
}

MATCHER_P(IsSetwiseEqual, expected_set, "") {
  set<uint16_t> arg_set(arg.begin(), arg.end());
  return arg_set == expected_set;
}

TEST_F(WiFiServiceUpdateFromEndpointsTest, FrequencyList) {
  EXPECT_CALL(adaptor, EmitUint16Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitStringChanged(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitUint8Changed(_, _)).Times(AnyNumber());
  EXPECT_CALL(adaptor, EmitBoolChanged(_, _)).Times(AnyNumber());

  // No endpoints -> empty list.
  EXPECT_EQ(vector<uint16_t>(), service->frequency_list());

  // Add endpoint -> endpoint's frequency in list.
  EXPECT_CALL(adaptor, EmitUint16sChanged(
      kWifiFrequencyListProperty, vector<uint16_t>{kGoodEndpointFrequency}));
  service->AddEndpoint(good_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Add another endpoint -> both frequencies in list.
  // Order doesn't matter.
  set<uint16_t> expected_frequencies{kGoodEndpointFrequency,
        kOkEndpointFrequency};
  EXPECT_CALL(adaptor, EmitUint16sChanged(
      kWifiFrequencyListProperty, IsSetwiseEqual(expected_frequencies)));
  service->AddEndpoint(ok_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Remove endpoint -> other endpoint's frequency remains.
  EXPECT_CALL(adaptor, EmitUint16sChanged(
      kWifiFrequencyListProperty, vector<uint16_t>{kOkEndpointFrequency}));
  service->RemoveEndpoint(good_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);

  // Endpoint with same frequency -> frequency remains.
  // Notification may or may not occur -- don't care.
  // Frequency may or may not be repeated in list -- don't care.
  WiFiEndpointRefPtr same_freq_as_ok_endpoint = MakeOpenEndpoint(
      simple_ssid_string(), "aa:bb:cc:dd:ee:ff", ok_endpoint->frequency(), 0);
  service->AddEndpoint(same_freq_as_ok_endpoint);
  EXPECT_THAT(service->frequency_list(),
              IsSetwiseEqual(set<uint16_t>{kOkEndpointFrequency}));
  Mock::VerifyAndClearExpectations(&adaptor);

  // Remove endpoint with same frequency -> frequency remains.
  // Notification may or may not occur -- don't care.
  service->RemoveEndpoint(ok_endpoint);
  EXPECT_EQ(vector<uint16_t>{same_freq_as_ok_endpoint->frequency()},
            service->frequency_list());
  Mock::VerifyAndClearExpectations(&adaptor);

  // Remove last endpoint. Frequency list goes empty.
  EXPECT_CALL(adaptor, EmitUint16sChanged(
      kWifiFrequencyListProperty, vector<uint16_t>{}));
  service->RemoveEndpoint(same_freq_as_ok_endpoint);
  Mock::VerifyAndClearExpectations(&adaptor);
}

TEST_F(WiFiServiceTest, SecurityFromCurrentEndpoint) {
  WiFiServiceRefPtr service(MakeSimpleService(kSecurityPsk));
  EXPECT_EQ(kSecurityPsk, service->GetSecurity(nullptr));
  WiFiEndpoint* endpoint = MakeOpenEndpoint(
        simple_ssid_string(), "00:00:00:00:00:00", 0, 0);
  service->AddEndpoint(endpoint);
  EXPECT_EQ(kSecurityPsk, service->GetSecurity(nullptr));
  service->NotifyCurrentEndpoint(endpoint);
  EXPECT_EQ(kSecurityNone, service->GetSecurity(nullptr));
  service->NotifyCurrentEndpoint(nullptr);
  EXPECT_EQ(kSecurityPsk, service->GetSecurity(nullptr));
}

TEST_F(WiFiServiceTest, UpdateSecurity) {
  // Cleartext and pre-shared-key crypto.
  {
    WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
    EXPECT_EQ(Service::kCryptoNone, service->crypto_algorithm());
    EXPECT_FALSE(service->key_rotation());
    EXPECT_FALSE(service->endpoint_auth());
  }
  {
    WiFiServiceRefPtr service = MakeSimpleService(kSecurityWep);
    EXPECT_EQ(Service::kCryptoRc4, service->crypto_algorithm());
    EXPECT_FALSE(service->key_rotation());
    EXPECT_FALSE(service->endpoint_auth());
  }
  {
    WiFiServiceRefPtr service = MakeSimpleService(kSecurityPsk);
    EXPECT_EQ(Service::kCryptoRc4, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_FALSE(service->endpoint_auth());
  }
  {
    WiFiServiceRefPtr service = MakeSimpleService(kSecurityWpa);
    EXPECT_EQ(Service::kCryptoRc4, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_FALSE(service->endpoint_auth());
  }
  {
    WiFiServiceRefPtr service = MakeSimpleService(kSecurityRsn);
    EXPECT_EQ(Service::kCryptoAes, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_FALSE(service->endpoint_auth());
  }

  // Crypto with 802.1X key management.
  {
    // WEP
    WiFiServiceRefPtr service = MakeSimpleService(kSecurityWep);
    service->SetEAPKeyManagement("IEEE8021X");
    EXPECT_EQ(Service::kCryptoRc4, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_TRUE(service->endpoint_auth());
  }
  {
    // WPA
    WiFiServiceRefPtr service = MakeSimpleService(kSecurity8021x);
    WiFiEndpointRefPtr endpoint =
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, false);
    service->AddEndpoint(endpoint);
    EXPECT_EQ(Service::kCryptoRc4, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_TRUE(service->endpoint_auth());
  }
  {
    // RSN
    WiFiServiceRefPtr service = MakeSimpleService(kSecurity8021x);
    WiFiEndpointRefPtr endpoint =
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, false, true);
    service->AddEndpoint(endpoint);
    EXPECT_EQ(Service::kCryptoAes, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_TRUE(service->endpoint_auth());
  }
  {
    // AP supports both WPA and RSN.
    WiFiServiceRefPtr service = MakeSimpleService(kSecurity8021x);
    WiFiEndpointRefPtr endpoint =
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, true);
    service->AddEndpoint(endpoint);
    EXPECT_EQ(Service::kCryptoAes, service->crypto_algorithm());
    EXPECT_TRUE(service->key_rotation());
    EXPECT_TRUE(service->endpoint_auth());
  }
}

TEST_F(WiFiServiceTest, ComputeCipher8021x) {
  // No endpoints.
  {
    const set<WiFiEndpointConstRefPtr> endpoints;
    EXPECT_EQ(Service::kCryptoNone,
              WiFiService::ComputeCipher8021x(endpoints));
  }

  // Single endpoint, various configs.
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, false, false));
    EXPECT_EQ(Service::kCryptoNone,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, false));
    EXPECT_EQ(Service::kCryptoRc4,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, false, true));
    EXPECT_EQ(Service::kCryptoAes,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, true));
    EXPECT_EQ(Service::kCryptoAes,
              WiFiService::ComputeCipher8021x(endpoints));
  }

  // Multiple endpoints.
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, false, false));
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:02", 0, 0, false, false));
    EXPECT_EQ(Service::kCryptoNone,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, false, false));
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:02", 0, 0, true, false));
    EXPECT_EQ(Service::kCryptoNone,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, false));
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:02", 0, 0, true, false));
    EXPECT_EQ(Service::kCryptoRc4,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, false));
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:02", 0, 0, false, true));
    EXPECT_EQ(Service::kCryptoRc4,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, false, true));
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:02", 0, 0, false, true));
    EXPECT_EQ(Service::kCryptoAes,
              WiFiService::ComputeCipher8021x(endpoints));
  }
  {
    set<WiFiEndpointConstRefPtr> endpoints;
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:01", 0, 0, true, true));
    endpoints.insert(
        MakeEndpoint("a", "00:00:00:00:00:02", 0, 0, true, true));
    EXPECT_EQ(Service::kCryptoAes,
              WiFiService::ComputeCipher8021x(endpoints));
  }
}

TEST_F(WiFiServiceTest, Unload) {
  WiFiServiceRefPtr service = MakeServiceWithWiFi(kSecurityNone);
  EXPECT_CALL(*wifi(), DestroyIPConfigLease(service->GetStorageIdentifier())).
    Times(1);
  service->Unload();
}

TEST_F(WiFiServiceTest, PropertyChanges) {
  WiFiServiceRefPtr service = MakeServiceWithMockManager();
  ServiceMockAdaptor* adaptor = GetAdaptor(service.get());
  TestCommonPropertyChanges(service, adaptor);
  TestAutoConnectPropertyChange(service, adaptor);

  EXPECT_CALL(*adaptor,
              EmitRpcIdentifierChanged(kDeviceProperty, _));
  SetWiFi(service, wifi());
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_CALL(*adaptor,
              EmitRpcIdentifierChanged(kDeviceProperty, _));
  service->ResetWiFi();
  Mock::VerifyAndClearExpectations(adaptor);
}

// Custom property setters should return false, and make no changes, if
// the new value is the same as the old value.
TEST_F(WiFiServiceTest, CustomSetterNoopChange) {
  WiFiServiceRefPtr service = MakeServiceWithMockManager();
  TestCustomSetterNoopChange(service, mock_manager());
}

TEST_F(WiFiServiceTest, SuspectedCredentialFailure) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityWpa);
  EXPECT_FALSE(service->has_ever_connected());
  EXPECT_EQ(0, service->suspected_credential_failures_);

  EXPECT_TRUE(service->AddSuspectedCredentialFailure());
  EXPECT_EQ(0, service->suspected_credential_failures_);

  service->has_ever_connected_ = true;
  for (int i = 0; i < WiFiService::kSuspectedCredentialFailureThreshold - 1;
       ++i) {
    EXPECT_FALSE(service->AddSuspectedCredentialFailure());
    EXPECT_EQ(i + 1, service->suspected_credential_failures_);
  }

  EXPECT_TRUE(service->AddSuspectedCredentialFailure());
  // Make sure the failure state does not reset just because we ask again.
  EXPECT_TRUE(service->AddSuspectedCredentialFailure());
  // Make sure the failure state resets because of a credential change.
  // A credential change changes the has_ever_connected to false and
  // immediately returns true when attempting to add the failure.
  Error error;
  service->SetPassphrase("Panchromatic Resonance", &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_TRUE(service->AddSuspectedCredentialFailure());
  EXPECT_EQ(0, service->suspected_credential_failures_);

  // Make sure that we still return true after resetting the failure
  // count.
  service->suspected_credential_failures_ = 3;
  EXPECT_EQ(3, service->suspected_credential_failures_);
  service->ResetSuspectedCredentialFailures();
  EXPECT_EQ(0, service->suspected_credential_failures_);
  EXPECT_TRUE(service->AddSuspectedCredentialFailure());
}

TEST_F(WiFiServiceTest, GetTethering) {
  WiFiServiceRefPtr service = MakeSimpleService(kSecurityNone);
  EXPECT_EQ(kTetheringNotDetectedState, service->GetTethering(nullptr));

  // Since the device isn't connected, we shouldn't even query the WiFi device.
  EXPECT_CALL(*wifi(), IsConnectedViaTether()).Times(0);
  SetWiFiForService(service, wifi());
  EXPECT_EQ(kTetheringNotDetectedState, service->GetTethering(nullptr));
  Mock::VerifyAndClearExpectations(wifi().get());

  scoped_refptr<MockProfile> mock_profile(
      new NiceMock<MockProfile>(control_interface(), metrics(), manager()));
  service->set_profile(mock_profile);
  service->SetState(Service::kStateConnected);

  // A connected service should return "confirmed" iff the underlying device
  // reports it is tethered.
  EXPECT_CALL(*wifi(), IsConnectedViaTether())
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_EQ(kTetheringConfirmedState, service->GetTethering(nullptr));
  EXPECT_EQ(kTetheringNotDetectedState, service->GetTethering(nullptr));
  Mock::VerifyAndClearExpectations(wifi().get());

  // Add two endpoints that have a BSSID associated with some Android devices
  // in tethering mode.
  WiFiEndpointRefPtr endpoint_android1 =
      MakeOpenEndpoint("a", "02:1a:11:00:00:01", 2412, 0);
  service->AddEndpoint(endpoint_android1);
  WiFiEndpointRefPtr endpoint_android2 =
      MakeOpenEndpoint("a", "02:1a:11:00:00:02", 2412, 0);
  service->AddEndpoint(endpoint_android2);

  // Since there are two endpoints, we should not detect tethering mode.
  EXPECT_CALL(*wifi(), IsConnectedViaTether()).WillOnce(Return(false));
  EXPECT_EQ(kTetheringNotDetectedState, service->GetTethering(nullptr));

  // If the device reports that it is tethered, this should override any
  // findings gained from examining the endpoints.
  EXPECT_CALL(*wifi(), IsConnectedViaTether()).WillOnce(Return(true));
  EXPECT_EQ(kTetheringConfirmedState, service->GetTethering(nullptr));

  // Continue in the un-tethered device case for a few more tests below.
  Mock::VerifyAndClearExpectations(wifi().get());
  EXPECT_CALL(*wifi(), IsConnectedViaTether())
      .WillRepeatedly(Return(false));

  // Removing an endpoint so we only have one should put us in the "Suspected"
  // state.
  service->RemoveEndpoint(endpoint_android1);
  EXPECT_EQ(kTetheringSuspectedState, service->GetTethering(nullptr));

  // Add a different endpoint which has a locally administered MAC address
  // but not one used by Android.
  service->RemoveEndpoint(endpoint_android2);
  WiFiEndpointRefPtr endpoint_ios =
      MakeOpenEndpoint("a", "02:00:00:00:00:01", 2412, 0);
  service->AddEndpoint(endpoint_ios);
  EXPECT_EQ(kTetheringNotDetectedState, service->GetTethering(nullptr));

  // If this endpoint reports the right vendor OUI, we should suspect
  // it to be tethered.  However since this evaluation normally only
  // happens in the endpoint constructor, we must force it to recalculate.
  endpoint_ios->vendor_information_.oui_set.insert(Tethering::kIosOui);
  endpoint_ios->CheckForTetheringSignature();
  EXPECT_EQ(kTetheringSuspectedState, service->GetTethering(nullptr));

  // If the device reports that it is tethered, this should override any
  // findings gained from examining the endpoints.
  Mock::VerifyAndClearExpectations(wifi().get());
  EXPECT_CALL(*wifi(), IsConnectedViaTether()).WillOnce(Return(true));
  EXPECT_EQ(kTetheringConfirmedState, service->GetTethering(nullptr));
}

TEST_F(WiFiServiceTest, IsVisible) {
  WiFiServiceRefPtr wifi_service = MakeServiceWithWiFi(kSecurityNone);
  ServiceMockAdaptor* adaptor = GetAdaptor(wifi_service.get());

  // Adding the first endpoint emits a change: Visible = true.
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, true));
  WiFiEndpointRefPtr endpoint =
      MakeOpenEndpoint("a", "00:00:00:00:00:01", 0, 0);
  wifi_service->AddEndpoint(endpoint);
  EXPECT_TRUE(wifi_service->IsVisible());
  Mock::VerifyAndClearExpectations(adaptor);

  // Removing the last endpoint emits a change: Visible = false.
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, false));
  wifi_service->RemoveEndpoint(endpoint);
  EXPECT_FALSE(wifi_service->IsVisible());
  Mock::VerifyAndClearExpectations(adaptor);

  // Entering the a connecting state emits a change: Visible = true
  // although the service has no endpoints.
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, true));
  wifi_service->SetState(Service::kStateAssociating);
  EXPECT_TRUE(wifi_service->IsVisible());
  Mock::VerifyAndClearExpectations(adaptor);

  // Moving between connecting / connected states does not trigger an Emit.
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, _)).Times(0);
  wifi_service->SetState(Service::kStateConfiguring);
  EXPECT_TRUE(wifi_service->IsVisible());
  Mock::VerifyAndClearExpectations(adaptor);

  // Entering the Idle state emits a change: Visible = false.
  EXPECT_CALL(*adaptor, EmitBoolChanged(kVisibleProperty, false));
  wifi_service->SetState(Service::kStateIdle);
  EXPECT_FALSE(wifi_service->IsVisible());
  Mock::VerifyAndClearExpectations(adaptor);
}

TEST_F(WiFiServiceTest, ConfigurePreferredDevice) {
  const string kDeviceName = "test_device";

  WiFiServiceRefPtr service = MakeGenericService();
  KeyValueStore args;
  args.SetString(kWifiPreferredDeviceProperty, kDeviceName);

  // With no wifi device.
  Error error;
  service->Configure(args, &error);
  EXPECT_EQ(Error::kSuccess, error.type());
  EXPECT_EQ(kDeviceName, service->preferred_device_);

  // With non-preferred wifi device.
  SetWiFiForService(service, wifi());
  service->Configure(args, &error);
  EXPECT_EQ(Error::kSuccess, error.type());
  EXPECT_EQ(nullptr, service->wifi_);
  EXPECT_EQ(kDeviceName, service->preferred_device_);

  // With preferred wifi device.
  scoped_refptr<MockWiFi> preferred_wifi = MakeSimpleWiFi(kDeviceName);
  SetWiFiForService(service, preferred_wifi);
  service->Configure(args, &error);
  EXPECT_EQ(Error::kSuccess, error.type());
  EXPECT_EQ(preferred_wifi, service->wifi_);
  EXPECT_EQ(kDeviceName, service->preferred_device_);
}

TEST_F(WiFiServiceTest, LoadAndUnloadPreferredDevice) {
  WiFiServiceRefPtr service = MakeGenericService();
  NiceMock<MockStore> mock_store;
  const string kStorageId = service->GetStorageIdentifier();
  EXPECT_CALL(mock_store, ContainsGroup(StrEq(kStorageId)))
      .WillRepeatedly(Return(true));
  set<string> groups;
  groups.insert(kStorageId);
  EXPECT_CALL(mock_store, GetGroupsWithProperties(
      ContainsWiFiProperties(
          simple_ssid(), kModeManaged, kSecurityWep)))
      .WillRepeatedly(Return(groups));
  EXPECT_CALL(mock_store, GetBool(_, _, _))
      .WillRepeatedly(Return(false));
  const string kDeviceName = "test_device";
  EXPECT_CALL(mock_store,
              GetString(StrEq(kStorageId),
                        WiFiService::kStoragePreferredDevice, _))
      .WillRepeatedly(DoAll(SetArgumentPointee<2>(kDeviceName), Return(true)));
  EXPECT_CALL(mock_store,
              GetString(StrEq(kStorageId),
                        StrNe(WiFiService::kStoragePreferredDevice), _))
      .WillRepeatedly(Return(false));

  // With no wifi device.
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(kDeviceName, service->preferred_device_);
  service->Unload();
  EXPECT_EQ("", service->preferred_device_);

  // With non-preferred wifi device.
  SetWiFiForService(service, wifi());
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(nullptr, service->wifi_);
  EXPECT_EQ(kDeviceName, service->preferred_device_);
  service->Unload();
  EXPECT_EQ("", service->preferred_device_);

  // With preferred wifi device.
  scoped_refptr<MockWiFi> preferred_wifi = MakeSimpleWiFi(kDeviceName);
  SetWiFiForService(service, preferred_wifi);
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(preferred_wifi, service->wifi_);
  EXPECT_EQ(kDeviceName, service->preferred_device_);
  service->Unload();
  EXPECT_EQ("", service->preferred_device_);
}

TEST_F(WiFiServiceTest, ChooseDevice) {
  const string kDeviceName1 = "test_device1";
  const string kDeviceName2 = "test_device2";
  scoped_refptr<MockWiFi> wifi1 = MakeSimpleWiFi(kDeviceName1);
  scoped_refptr<MockWiFi> wifi2 = MakeSimpleWiFi(kDeviceName2);
  WiFiServiceRefPtr service = MakeServiceWithMockManager();

  // No preferred device.
  EXPECT_CALL(*mock_manager(), GetEnabledDeviceByLinkName(_)).Times(0);
  EXPECT_CALL(*mock_manager(),
              GetEnabledDeviceWithTechnology(Technology::kWifi))
      .WillOnce(Return(wifi1));
  EXPECT_EQ(wifi1, service->ChooseDevice());
  Mock::VerifyAndClearExpectations(mock_manager());

  // With preferred device.
  service->SetPreferredDevice(kDeviceName2, nullptr);
  EXPECT_CALL(*mock_manager(), GetEnabledDeviceByLinkName(kDeviceName2))
      .WillOnce(Return(wifi2));
  EXPECT_CALL(*mock_manager(), GetEnabledDeviceWithTechnology(_)).Times(0);
  EXPECT_EQ(wifi2, service->ChooseDevice());
  Mock::VerifyAndClearExpectations(mock_manager());
}

TEST_F(WiFiServiceTest, RoamThresholdProperty) {
  WiFiServiceRefPtr service = MakeGenericService();
  static const uint16_t kRoamThreshold16 = 16;
  static const uint16_t kRoamThreshold32 = 32;

  EXPECT_TRUE(SetRoamThreshold(service, kRoamThreshold16));
  EXPECT_EQ(GetRoamThreshold(service), kRoamThreshold16);

  // Try a different number
  EXPECT_TRUE(SetRoamThreshold(service, kRoamThreshold32));
  EXPECT_EQ(GetRoamThreshold(service), kRoamThreshold32);
}

TEST_F(WiFiServiceTest, SaveLoadRoamThreshold) {
  WiFiServiceRefPtr service = MakeGenericService();
  NiceMock<MockStore> mock_store;
  const uint16_t kRoamThreshold = 10;
  const string kStorageId = service->GetStorageIdentifier();
  EXPECT_CALL(mock_store, ContainsGroup(StrEq(kStorageId)))
      .WillRepeatedly(Return(true));
  set<string> groups;
  groups.insert(kStorageId);
  EXPECT_CALL(mock_store, GetGroupsWithProperties(ContainsWiFiProperties(
                              simple_ssid(), kModeManaged, kSecurityWep)))
      .WillRepeatedly(Return(groups));
  EXPECT_CALL(mock_store, GetBool(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(mock_store, SetBool(_, _, _)).Times(AnyNumber());

  // First, save these values.
  service->roam_threshold_db_ = kRoamThreshold;
  service->roam_threshold_db_set_ = true;
  EXPECT_CALL(mock_store,
              SetUint64(StrEq(kStorageId), WiFiService::kStorageRoamThreshold,
                        kRoamThreshold));
  EXPECT_CALL(mock_store, SetBool(StrEq(kStorageId),
                                  WiFiService::kStorageRoamThresholdSet, true));
  EXPECT_TRUE(service->Save(&mock_store));

  // Then, load these values into the WiFiService members.
  service->roam_threshold_db_ = 0;
  service->roam_threshold_db_set_ = false;
  EXPECT_CALL(mock_store, GetUint64(StrEq(kStorageId),
                                    WiFiService::kStorageRoamThreshold, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(kRoamThreshold), Return(true)));
  EXPECT_CALL(mock_store, GetBool(StrEq(kStorageId),
                                  WiFiService::kStorageRoamThresholdSet, _))
      .WillOnce(DoAll(SetArgumentPointee<2>(true), Return(true)));
  EXPECT_TRUE(service->Load(&mock_store));
  EXPECT_EQ(kRoamThreshold, service->roam_threshold_db_);
  EXPECT_TRUE(service->roam_threshold_db_set_);
}

}  // namespace shill

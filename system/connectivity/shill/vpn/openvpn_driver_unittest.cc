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

#include "shill/vpn/openvpn_driver.h"

#include <algorithm>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/ipconfig.h"
#include "shill/logging.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_certificate_file.h"
#include "shill/mock_device_info.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_process_manager.h"
#include "shill/mock_service.h"
#include "shill/mock_store.h"
#include "shill/mock_virtual_device.h"
#include "shill/nice_mock_control.h"
#include "shill/rpc_task.h"
#include "shill/technology.h"
#include "shill/virtual_device.h"
#include "shill/vpn/mock_openvpn_management_server.h"
#include "shill/vpn/mock_vpn_service.h"
#include "shill/vpn/vpn_service.h"

using base::FilePath;
using base::WeakPtr;
using std::map;
using std::string;
using std::vector;
using testing::_;
using testing::AnyNumber;
using testing::DoAll;
using testing::ElementsAreArray;
using testing::Field;
using testing::Mock;
using testing::Ne;
using testing::NiceMock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrictMock;

namespace shill {

struct AuthenticationExpectations {
  AuthenticationExpectations()
      : remote_authentication_type(Metrics::kVpnRemoteAuthenticationTypeMax) {}
  AuthenticationExpectations(
      const string& ca_cert_in,
      const string& client_cert_in,
      const string& user_in,
      const string& otp_in,
      const string& token_in,
      Metrics::VpnRemoteAuthenticationType remote_authentication_type_in,
      const vector<Metrics::VpnUserAuthenticationType>
          &user_authentication_types_in)
      : ca_cert(ca_cert_in),
        client_cert(client_cert_in),
        user(user_in),
        otp(otp_in),
        token(token_in),
        remote_authentication_type(remote_authentication_type_in),
        user_authentication_types(user_authentication_types_in) {}
  string ca_cert;
  string client_cert;
  string user;
  string otp;
  string token;
  Metrics::VpnRemoteAuthenticationType remote_authentication_type;
  vector<Metrics::VpnUserAuthenticationType> user_authentication_types;
};

class OpenVPNDriverTest
    : public testing::TestWithParam<AuthenticationExpectations>,
      public RPCTaskDelegate {
 public:
  OpenVPNDriverTest()
      : device_info_(&control_, &dispatcher_, &metrics_, &manager_),
        metrics_(&dispatcher_),
        manager_(&control_, &dispatcher_, &metrics_),
        driver_(new OpenVPNDriver(&control_, &dispatcher_, &metrics_, &manager_,
                                  &device_info_, &process_manager_)),
        service_(new MockVPNService(&control_, &dispatcher_, &metrics_,
                                    &manager_, driver_)),
        device_(new MockVirtualDevice(
            &control_, &dispatcher_, &metrics_, &manager_,
            kInterfaceName, kInterfaceIndex, Technology::kVPN)),
        certificate_file_(new MockCertificateFile()),
        extra_certificates_file_(new MockCertificateFile()),
        management_server_(new NiceMock<MockOpenVPNManagementServer>()) {
    driver_->management_server_.reset(management_server_);
    driver_->certificate_file_.reset(certificate_file_);  // Passes ownership.
    driver_->extra_certificates_file_.reset(
        extra_certificates_file_);  // Passes ownership.
    CHECK(temporary_directory_.CreateUniqueTempDir());
    driver_->openvpn_config_directory_ =
        temporary_directory_.path().Append(kOpenVPNConfigDirectory);
  }

  virtual ~OpenVPNDriverTest() {}

  virtual void TearDown() {
    driver_->default_service_callback_tag_ = 0;
    driver_->pid_ = 0;
    driver_->device_ = nullptr;
    driver_->service_ = nullptr;
    if (!lsb_release_file_.empty()) {
      EXPECT_TRUE(base::DeleteFile(lsb_release_file_, false));
      lsb_release_file_.clear();
    }
  }

 protected:
  static const char kOption[];
  static const char kProperty[];
  static const char kValue[];
  static const char kOption2[];
  static const char kProperty2[];
  static const char kValue2[];
  static const char kGateway1[];
  static const char kNetmask1[];
  static const char kNetwork1[];
  static const char kGateway2[];
  static const char kNetmask2[];
  static const char kNetwork2[];
  static const char kInterfaceName[];
  static const int kInterfaceIndex;
  static const char kOpenVPNConfigDirectory[];

  void SetArg(const string& arg, const string& value) {
    driver_->args()->SetString(arg, value);
  }

  void SetArgArray(const string& arg, const vector<string>& value) {
    driver_->args()->SetStrings(arg, value);
  }

  KeyValueStore* GetArgs() {
    return driver_->args();
  }

  KeyValueStore GetProviderProperties(const PropertyStore& store) {
    KeyValueStore props;
    Error error;
    EXPECT_TRUE(
        store.GetKeyValueStoreProperty(kProviderProperty, &props, &error));
    return props;
  }

  void RemoveStringArg(const string& arg) {
    driver_->args()->RemoveString(arg);
  }

  const ServiceRefPtr& GetSelectedService() {
    return device_->selected_service();
  }

  bool InitManagementChannelOptions(
      vector<vector<string>>* options, Error* error) {
    return driver_->InitManagementChannelOptions(options, error);
  }

  Sockets* GetSockets() {
    return &driver_->sockets_;
  }

  void SetDevice(const VirtualDeviceRefPtr& device) {
    driver_->device_ = device;
  }

  void SetService(const VPNServiceRefPtr& service) {
    driver_->service_ = service;
  }

  VPNServiceRefPtr GetService() {
    return driver_->service_;
  }

  void OnConnectionDisconnected() {
    driver_->OnConnectionDisconnected();
  }

  void OnConnectTimeout() {
    driver_->OnConnectTimeout();
  }

  void StartConnectTimeout(int timeout_seconds) {
    driver_->StartConnectTimeout(timeout_seconds);
  }

  bool IsConnectTimeoutStarted() {
    return driver_->IsConnectTimeoutStarted();
  }

  static int GetDefaultConnectTimeoutSeconds() {
    return OpenVPNDriver::kDefaultConnectTimeoutSeconds;
  }

  static int GetReconnectOfflineTimeoutSeconds() {
    return OpenVPNDriver::kReconnectOfflineTimeoutSeconds;
  }

  static int GetReconnectTLSErrorTimeoutSeconds() {
    return OpenVPNDriver::kReconnectTLSErrorTimeoutSeconds;
  }

  static int GetReconnectTimeoutSeconds(OpenVPNDriver::ReconnectReason reason) {
    return OpenVPNDriver::GetReconnectTimeoutSeconds(reason);
  }

  void SetClientState(const string& state) {
    management_server_->state_ = state;
  }

  // Used to assert that a flag appears in the options.
  void ExpectInFlags(const vector<vector<string>>& options, const string& flag);
  void ExpectInFlags(const vector<vector<string>>& options, const string& flag,
                     const string& value);
  void ExpectInFlags(const vector<vector<string>>& options,
                     const vector<string>& arguments);
  void ExpectNotInFlags(const vector<vector<string>>& options,
                        const string& flag);

  void SetupLSBRelease();

  // Inherited from RPCTaskDelegate.
  virtual void GetLogin(string* user, string* password);
  virtual void Notify(const string& reason, const map<string, string>& dict);

  NiceMockControl control_;
  NiceMock<MockDeviceInfo> device_info_;
  MockEventDispatcher dispatcher_;
  MockMetrics metrics_;
  MockProcessManager process_manager_;
  MockManager manager_;
  OpenVPNDriver* driver_;  // Owned by |service_|.
  scoped_refptr<MockVPNService> service_;
  scoped_refptr<MockVirtualDevice> device_;
  MockCertificateFile* certificate_file_;  // Owned by |driver_|.
  MockCertificateFile* extra_certificates_file_;  // Owned by |driver_|.
  base::ScopedTempDir temporary_directory_;

  // Owned by |driver_|.
  NiceMock<MockOpenVPNManagementServer>* management_server_;

  FilePath lsb_release_file_;
};

const char OpenVPNDriverTest::kOption[] = "openvpn-option";
const char OpenVPNDriverTest::kProperty[] = "OpenVPN.SomeProperty";
const char OpenVPNDriverTest::kValue[] = "some-property-value";
const char OpenVPNDriverTest::kOption2[] = "openvpn-option2";
const char OpenVPNDriverTest::kProperty2[] = "OpenVPN.SomeProperty2";
const char OpenVPNDriverTest::kValue2[] = "some-property-value2";
const char OpenVPNDriverTest::kGateway1[] = "10.242.2.13";
const char OpenVPNDriverTest::kNetmask1[] = "255.255.255.255";
const char OpenVPNDriverTest::kNetwork1[] = "10.242.2.1";
const char OpenVPNDriverTest::kGateway2[] = "10.242.2.14";
const char OpenVPNDriverTest::kNetmask2[] = "255.255.0.0";
const char OpenVPNDriverTest::kNetwork2[] = "192.168.0.0";
const char OpenVPNDriverTest::kInterfaceName[] = "tun0";
const int OpenVPNDriverTest::kInterfaceIndex = 123;
const char OpenVPNDriverTest::kOpenVPNConfigDirectory[] = "openvpn";

void OpenVPNDriverTest::GetLogin(string* /*user*/, string* /*password*/) {}

void OpenVPNDriverTest::Notify(const string& /*reason*/,
                               const map<string, string>& /*dict*/) {}

void OpenVPNDriverTest::ExpectInFlags(const vector<vector<string>>& options,
                                      const string& flag) {
  ExpectInFlags(options, vector<string> { flag });
}

void OpenVPNDriverTest::ExpectInFlags(const vector<vector<string>>& options,
                                      const string& flag,
                                      const string& value) {
  ExpectInFlags(options, vector<string> { flag, value });
}

void OpenVPNDriverTest::ExpectInFlags(const vector<vector<string>>& options,
                                      const vector<string>& arguments) {
  EXPECT_TRUE(std::find(options.begin(), options.end(), arguments) !=
              options.end());
}

void OpenVPNDriverTest::ExpectNotInFlags(const vector<vector<string>>& options,
                                         const string& flag) {
  for (const auto& option : options) {
    EXPECT_NE(flag, option[0]);
  }
}

void OpenVPNDriverTest::SetupLSBRelease() {
  static const char kLSBReleaseContents[] =
      "\n"
      "=\n"
      "foo=\n"
      "=bar\n"
      "zoo==\n"
      "CHROMEOS_RELEASE_BOARD=x86-alex\n"
      "CHROMEOS_RELEASE_NAME=Chromium OS\n"
      "CHROMEOS_RELEASE_VERSION=2202.0\n";
  EXPECT_TRUE(base::CreateTemporaryFile(&lsb_release_file_));
  EXPECT_EQ(arraysize(kLSBReleaseContents),
            base::WriteFile(lsb_release_file_,
                            kLSBReleaseContents,
                            arraysize(kLSBReleaseContents)));
  EXPECT_EQ(OpenVPNDriver::kLSBReleaseFile, driver_->lsb_release_file_.value());
  driver_->lsb_release_file_ = lsb_release_file_;
}

TEST_F(OpenVPNDriverTest, Connect) {
  EXPECT_CALL(*service_, SetState(Service::kStateConfiguring));
  const string interface = kInterfaceName;
  EXPECT_CALL(device_info_, CreateTunnelInterface(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(interface), Return(true)));
  Error error;
  driver_->Connect(service_, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kInterfaceName, driver_->tunnel_interface_);
  EXPECT_TRUE(driver_->IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, ConnectTunnelFailure) {
  EXPECT_CALL(*service_, SetState(Service::kStateConfiguring));
  EXPECT_CALL(device_info_, CreateTunnelInterface(_)).WillOnce(Return(false));
  EXPECT_CALL(*service_, SetFailure(Service::kFailureInternal));
  Error error;
  driver_->Connect(service_, &error);
  EXPECT_EQ(Error::kInternalError, error.type());
  EXPECT_TRUE(driver_->tunnel_interface_.empty());
  EXPECT_FALSE(driver_->IsConnectTimeoutStarted());
}

namespace {
MATCHER_P(IsIPAddress, address, "") {
  IPAddress ip_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address.SetAddressFromString(address));
  return ip_address.Equals(arg);
}
}  // namespace

TEST_F(OpenVPNDriverTest, Notify) {
  map<string, string> config;
  driver_->service_ = service_;
  driver_->device_ = device_;
  StartConnectTimeout(0);
  EXPECT_CALL(*device_,
              UpdateIPConfig(Field(&IPConfig::Properties::address, "")));
  driver_->Notify("up", config);
  EXPECT_FALSE(driver_->IsConnectTimeoutStarted());
  EXPECT_TRUE(GetSelectedService().get() == service_.get());

  // Tests that existing properties are reused if no new ones provided.
  driver_->ip_properties_.address = "1.2.3.4";
  EXPECT_CALL(*device_,
              UpdateIPConfig(Field(&IPConfig::Properties::address, "1.2.3.4")));
  driver_->Notify("up", config);
}

TEST_P(OpenVPNDriverTest, NotifyUMA) {
  map<string, string> config;
  driver_->service_ = service_;
  driver_->device_ = device_;

  // Check that UMA metrics are emitted on Notify.
  EXPECT_CALL(*device_, UpdateIPConfig(_));
  EXPECT_CALL(metrics_, SendEnumToUMA(
      Metrics::kMetricVpnDriver,
      Metrics::kVpnDriverOpenVpn,
      Metrics::kMetricVpnDriverMax));
  EXPECT_CALL(metrics_, SendEnumToUMA(
      Metrics::kMetricVpnRemoteAuthenticationType,
      GetParam().remote_authentication_type,
      Metrics::kVpnRemoteAuthenticationTypeMax));
  for (const auto& authentication_type : GetParam().user_authentication_types) {
    EXPECT_CALL(metrics_, SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        authentication_type,
        Metrics::kVpnUserAuthenticationTypeMax));
  }

  Error unused_error;
  PropertyStore store;
  driver_->InitPropertyStore(&store);
  if (!GetParam().ca_cert.empty()) {
    store.SetStringsProperty(kOpenVPNCaCertPemProperty,
                             vector<string>{ GetParam().ca_cert },
                             &unused_error);
  }
  if (!GetParam().client_cert.empty()) {
    store.SetStringProperty(kOpenVPNClientCertIdProperty,
                            GetParam().client_cert,
                            &unused_error);
  }
  if (!GetParam().user.empty()) {
    store.SetStringProperty(kOpenVPNUserProperty, GetParam().user,
                            &unused_error);
  }
  if (!GetParam().otp.empty()) {
    store.SetStringProperty(kOpenVPNOTPProperty, GetParam().otp, &unused_error);
  }
  if (!GetParam().token.empty()) {
    store.SetStringProperty(kOpenVPNTokenProperty, GetParam().token,
                            &unused_error);
  }
  driver_->Notify("up", config);
  Mock::VerifyAndClearExpectations(&metrics_);
}

INSTANTIATE_TEST_CASE_P(
    OpenVPNDriverAuthenticationTypes,
    OpenVPNDriverTest,
    ::testing::Values(
      AuthenticationExpectations(
          "", "", "", "", "",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnNone }),
      AuthenticationExpectations(
          "", "client_cert", "", "", "",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnCertificate }),
      AuthenticationExpectations(
          "", "client_cert", "user", "", "",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnCertificate,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePassword }),
      AuthenticationExpectations(
          "", "", "user", "", "",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePassword }),
      AuthenticationExpectations(
          "", "client_cert", "user", "otp", "",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnCertificate,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePassword,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePasswordOtp }),
      AuthenticationExpectations(
          "", "client_cert", "user", "otp", "token",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnCertificate,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePassword,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePasswordOtp,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernameToken }),
      AuthenticationExpectations(
          "ca_cert", "client_cert", "user", "otp", "token",
          Metrics::kVpnRemoteAuthenticationTypeOpenVpnCertificate,
          vector<Metrics::VpnUserAuthenticationType> {
              Metrics::kVpnUserAuthenticationTypeOpenVpnCertificate,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePassword,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePasswordOtp,
              Metrics::kVpnUserAuthenticationTypeOpenVpnUsernameToken })));

TEST_F(OpenVPNDriverTest, NotifyFail) {
  map<string, string> dict;
  driver_->device_ = device_;
  StartConnectTimeout(0);
  EXPECT_CALL(*device_, DropConnection());
  driver_->Notify("fail", dict);
  EXPECT_TRUE(driver_->IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, GetRouteOptionEntry) {
  OpenVPNDriver::RouteOptions routes;
  EXPECT_EQ(nullptr, OpenVPNDriver::GetRouteOptionEntry("foo", "bar", &routes));
  EXPECT_TRUE(routes.empty());
  EXPECT_EQ(nullptr, OpenVPNDriver::GetRouteOptionEntry("foo", "foo", &routes));
  EXPECT_TRUE(routes.empty());
  EXPECT_EQ(nullptr,
            OpenVPNDriver::GetRouteOptionEntry("foo", "fooz", &routes));
  EXPECT_TRUE(routes.empty());
  IPConfig::Route* route =
      OpenVPNDriver::GetRouteOptionEntry("foo", "foo12", &routes);
  EXPECT_EQ(1, routes.size());
  EXPECT_EQ(route, &routes[12]);
  route = OpenVPNDriver::GetRouteOptionEntry("foo", "foo13", &routes);
  EXPECT_EQ(2, routes.size());
  EXPECT_EQ(route, &routes[13]);
}

TEST_F(OpenVPNDriverTest, ParseRouteOption) {
  OpenVPNDriver::RouteOptions routes;
  OpenVPNDriver::ParseRouteOption("foo", "bar", &routes);
  EXPECT_TRUE(routes.empty());
  OpenVPNDriver::ParseRouteOption("gateway_2", kGateway2, &routes);
  OpenVPNDriver::ParseRouteOption("netmask_2", kNetmask2, &routes);
  OpenVPNDriver::ParseRouteOption("network_2", kNetwork2, &routes);
  EXPECT_EQ(1, routes.size());
  OpenVPNDriver::ParseRouteOption("gateway_1", kGateway1, &routes);
  OpenVPNDriver::ParseRouteOption("netmask_1", kNetmask1, &routes);
  OpenVPNDriver::ParseRouteOption("network_1", kNetwork1, &routes);
  EXPECT_EQ(2, routes.size());
  EXPECT_EQ(kGateway1, routes[1].gateway);
  EXPECT_EQ(kNetmask1, routes[1].netmask);
  EXPECT_EQ(kNetwork1, routes[1].host);
  EXPECT_EQ(kGateway2, routes[2].gateway);
  EXPECT_EQ(kNetmask2, routes[2].netmask);
  EXPECT_EQ(kNetwork2, routes[2].host);
}

TEST_F(OpenVPNDriverTest, SetRoutes) {
  OpenVPNDriver::RouteOptions routes;
  routes[1].gateway = "1.2.3.4";
  routes[1].host = "1.2.3.4";
  routes[2].host = "2.3.4.5";
  routes[2].netmask = "255.0.0.0";
  routes[3].netmask = "255.0.0.0";
  routes[3].gateway = "1.2.3.5";
  routes[5].host = kNetwork2;
  routes[5].netmask = kNetmask2;
  routes[5].gateway = kGateway2;
  routes[4].host = kNetwork1;
  routes[4].netmask = kNetmask1;
  routes[4].gateway = kGateway1;
  IPConfig::Properties props;
  OpenVPNDriver::SetRoutes(routes, &props);
  ASSERT_EQ(2, props.routes.size());
  EXPECT_EQ(kGateway1, props.routes[0].gateway);
  EXPECT_EQ(kNetmask1, props.routes[0].netmask);
  EXPECT_EQ(kNetwork1, props.routes[0].host);
  EXPECT_EQ(kGateway2, props.routes[1].gateway);
  EXPECT_EQ(kNetmask2, props.routes[1].netmask);
  EXPECT_EQ(kNetwork2, props.routes[1].host);

  // Tests that the routes are not reset if no new routes are supplied.
  OpenVPNDriver::SetRoutes(OpenVPNDriver::RouteOptions(), &props);
  EXPECT_EQ(2, props.routes.size());
}

TEST_F(OpenVPNDriverTest, SplitPortFromHost) {
  string name, port;
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("", nullptr, nullptr));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com:", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost(":1234", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com:f:1234", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com:x", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com:-1", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com:+1", &name, &port));
  EXPECT_FALSE(OpenVPNDriver::SplitPortFromHost("v.com:65536", &name, &port));
  EXPECT_TRUE(OpenVPNDriver::SplitPortFromHost("v.com:0", &name, &port));
  EXPECT_EQ("v.com", name);
  EXPECT_EQ("0", port);
  EXPECT_TRUE(OpenVPNDriver::SplitPortFromHost("w.com:65535", &name, &port));
  EXPECT_EQ("w.com", name);
  EXPECT_EQ("65535", port);
  EXPECT_TRUE(OpenVPNDriver::SplitPortFromHost("x.com:12345", &name, &port));
  EXPECT_EQ("x.com", name);
  EXPECT_EQ("12345", port);
}

TEST_F(OpenVPNDriverTest, ParseForeignOption) {
  vector<string> domain_search;
  vector<string> dns_servers;
  IPConfig::Properties props;
  OpenVPNDriver::ParseForeignOption("", &domain_search, &dns_servers);
  OpenVPNDriver::ParseForeignOption(
      "dhcp-option DOMAIN", &domain_search, &dns_servers);
  OpenVPNDriver::ParseForeignOption(
      "dhcp-option DOMAIN zzz.com foo", &domain_search, &dns_servers);
  OpenVPNDriver::ParseForeignOption(
      "dhcp-Option DOmAIN xyz.com", &domain_search, &dns_servers);
  ASSERT_EQ(1, domain_search.size());
  EXPECT_EQ("xyz.com", domain_search[0]);
  OpenVPNDriver::ParseForeignOption(
      "dhcp-option DnS 1.2.3.4", &domain_search, &dns_servers);
  ASSERT_EQ(1, dns_servers.size());
  EXPECT_EQ("1.2.3.4", dns_servers[0]);
}

TEST_F(OpenVPNDriverTest, ParseForeignOptions) {
  // This also tests that std::map is a sorted container.
  map<int, string> options;
  options[5] = "dhcp-option DOMAIN five.com";
  options[2] = "dhcp-option DOMAIN two.com";
  options[8] = "dhcp-option DOMAIN eight.com";
  options[7] = "dhcp-option DOMAIN seven.com";
  options[4] = "dhcp-option DOMAIN four.com";
  options[10] = "dhcp-option dns 1.2.3.4";
  IPConfig::Properties props;
  OpenVPNDriver::ParseForeignOptions(options, &props);
  ASSERT_EQ(5, props.domain_search.size());
  EXPECT_EQ("two.com", props.domain_search[0]);
  EXPECT_EQ("four.com", props.domain_search[1]);
  EXPECT_EQ("five.com", props.domain_search[2]);
  EXPECT_EQ("seven.com", props.domain_search[3]);
  EXPECT_EQ("eight.com", props.domain_search[4]);
  ASSERT_EQ(1, props.dns_servers.size());
  EXPECT_EQ("1.2.3.4", props.dns_servers[0]);

  // Test that the DNS properties are not updated if no new DNS properties are
  // supplied.
  OpenVPNDriver::ParseForeignOptions(map<int, string>(), &props);
  EXPECT_EQ(5, props.domain_search.size());
  ASSERT_EQ(1, props.dns_servers.size());
}

TEST_F(OpenVPNDriverTest, ParseIPConfiguration) {
  map<string, string> config;
  IPConfig::Properties props;

  driver_->ParseIPConfiguration(config, &props);
  EXPECT_EQ(IPAddress::kFamilyIPv4, props.address_family);
  EXPECT_EQ(32, props.subnet_prefix);

  props.subnet_prefix = 18;
  driver_->ParseIPConfiguration(config, &props);
  EXPECT_EQ(18, props.subnet_prefix);

  // An "ifconfig_remote" parameter that looks like a netmask should be
  // applied to the subnet prefix instead of to the peer address.
  config["ifconfig_remotE"] = "255.255.0.0";
  driver_->ParseIPConfiguration(config, &props);
  EXPECT_EQ(16, props.subnet_prefix);
  EXPECT_EQ("", props.peer_address);

  config["ifconfig_loCal"] = "4.5.6.7";
  config["ifconfiG_broadcast"] = "1.2.255.255";
  config["ifconFig_netmAsk"] = "255.255.255.0";
  config["ifconfig_remotE"] = "33.44.55.66";
  config["route_vpN_gateway"] = "192.168.1.1";
  config["trusted_ip"] = "99.88.77.66";
  config["tun_mtu"] = "1000";
  config["foreign_option_2"] = "dhcp-option DNS 4.4.4.4";
  config["foreign_option_1"] = "dhcp-option DNS 1.1.1.1";
  config["foreign_option_3"] = "dhcp-option DNS 2.2.2.2";
  config["route_network_2"] = kNetwork2;
  config["route_network_1"] = kNetwork1;
  config["route_netmask_2"] = kNetmask2;
  config["route_netmask_1"] = kNetmask1;
  config["route_gateway_2"] = kGateway2;
  config["route_gateway_1"] = kGateway1;
  config["foo"] = "bar";
  driver_->ParseIPConfiguration(config, &props);
  EXPECT_EQ(IPAddress::kFamilyIPv4, props.address_family);
  EXPECT_EQ("4.5.6.7", props.address);
  EXPECT_EQ("1.2.255.255", props.broadcast_address);
  EXPECT_EQ(24, props.subnet_prefix);
  EXPECT_EQ("33.44.55.66", props.peer_address);
  EXPECT_EQ("192.168.1.1", props.gateway);
  EXPECT_EQ("99.88.77.66/32", props.exclusion_list[0]);
  EXPECT_EQ(1, props.exclusion_list.size());
  EXPECT_EQ(1000, props.mtu);
  ASSERT_EQ(3, props.dns_servers.size());
  EXPECT_EQ("1.1.1.1", props.dns_servers[0]);
  EXPECT_EQ("4.4.4.4", props.dns_servers[1]);
  EXPECT_EQ("2.2.2.2", props.dns_servers[2]);
  ASSERT_EQ(2, props.routes.size());
  EXPECT_EQ(kGateway1, props.routes[0].gateway);
  EXPECT_EQ(kNetmask1, props.routes[0].netmask);
  EXPECT_EQ(kNetwork1, props.routes[0].host);
  EXPECT_EQ(kGateway2, props.routes[1].gateway);
  EXPECT_EQ(kNetmask2, props.routes[1].netmask);
  EXPECT_EQ(kNetwork2, props.routes[1].host);
  EXPECT_FALSE(props.blackhole_ipv6);

  // If the driver is configured to ignore the gateway provided, it will
  // not set the "gateway" property for the properties, however the
  // explicitly supplied routes should still be set.
  SetArg(kOpenVPNIgnoreDefaultRouteProperty, "some value");
  IPConfig::Properties props_without_gateway;
  driver_->ParseIPConfiguration(config, &props_without_gateway);
  EXPECT_EQ(kGateway1, props_without_gateway.routes[0].gateway);
  EXPECT_EQ("", props_without_gateway.gateway);

  // A pushed redirect flag should override the IgnoreDefaultRoute property.
  config["redirect_gateway"] = "def1";
  IPConfig::Properties props_with_override;
  driver_->ParseIPConfiguration(config, &props_with_override);
  EXPECT_EQ("192.168.1.1", props_with_override.gateway);
}

TEST_F(OpenVPNDriverTest, InitOptionsNoHost) {
  Error error;
  vector<vector<string>> options;
  driver_->InitOptions(&options, &error);
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_TRUE(options.empty());
}

TEST_F(OpenVPNDriverTest, InitOptions) {
  static const char kHost[] = "192.168.2.254";
  static const char kTLSAuthContents[] = "SOME-RANDOM-CONTENTS\n";
  static const char kID[] = "TestPKCS11ID";
  static const char kKU0[] = "00";
  static const char kKU1[] = "01";
  FilePath empty_cert;
  SetArg(kProviderHostProperty, kHost);
  SetArg(kOpenVPNTLSAuthContentsProperty, kTLSAuthContents);
  SetArg(kOpenVPNClientCertIdProperty, kID);
  SetArg(kOpenVPNRemoteCertKUProperty, string(kKU0) + " " + string(kKU1));
  driver_->rpc_task_.reset(new RPCTask(&control_, this));
  driver_->tunnel_interface_ = kInterfaceName;
  EXPECT_CALL(*management_server_, Start(_, _, _)).WillOnce(Return(true));
  EXPECT_CALL(manager_, IsConnected()).WillOnce(Return(false));

  Error error;
  vector<vector<string>> options;
  driver_->InitOptions(&options, &error);
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(vector<string> { "client" }, options[0]);
  ExpectInFlags(options, "remote", kHost);
  ExpectInFlags(options, vector<string> { "setenv", kRPCTaskPathVariable,
                                          RPCTaskMockAdaptor::kRpcId });
  ExpectInFlags(options, "dev", kInterfaceName);
  ExpectInFlags(options, "group", "openvpn");
  EXPECT_EQ(kInterfaceName, driver_->tunnel_interface_);
  ASSERT_FALSE(driver_->tls_auth_file_.empty());
  ExpectInFlags(options, "tls-auth", driver_->tls_auth_file_.value());
  string contents;
  EXPECT_TRUE(base::ReadFileToString(driver_->tls_auth_file_, &contents));
  EXPECT_EQ(kTLSAuthContents, contents);
  ExpectInFlags(options, "pkcs11-id", kID);
  ExpectInFlags(options, "ca", OpenVPNDriver::kDefaultCACertificates);
  ExpectInFlags(options, "syslog");
  ExpectNotInFlags(options, "auth-user-pass");
  ExpectInFlags(options, vector<string> { "remote-cert-ku", kKU0, kKU1 });
}

TEST_F(OpenVPNDriverTest, InitOptionsHostWithPort) {
  SetArg(kProviderHostProperty, "v.com:1234");
  driver_->rpc_task_.reset(new RPCTask(&control_, this));
  driver_->tunnel_interface_ = kInterfaceName;
  EXPECT_CALL(*management_server_, Start(_, _, _)).WillOnce(Return(true));
  EXPECT_CALL(manager_, IsConnected()).WillOnce(Return(false));

  Error error;
  vector<vector<string>> options;
  driver_->InitOptions(&options, &error);
  EXPECT_TRUE(error.IsSuccess());
  ExpectInFlags(options, vector<string> { "remote", "v.com", "1234" });
}

TEST_F(OpenVPNDriverTest, InitCAOptions) {
  static const char kHost[] = "192.168.2.254";
  static const char kCaCert[] = "foo";
  static const char kCaCertNSS[] = "{1234}";

  Error error;
  vector<vector<string>> options;
  EXPECT_TRUE(driver_->InitCAOptions(&options, &error));
  EXPECT_TRUE(error.IsSuccess());
  ExpectInFlags(options, "ca", OpenVPNDriver::kDefaultCACertificates);

  options.clear();
  SetArg(kOpenVPNCaCertProperty, kCaCert);
  EXPECT_TRUE(driver_->InitCAOptions(&options, &error));
  ExpectInFlags(options, "ca", kCaCert);
  EXPECT_TRUE(error.IsSuccess());

  // We should ignore the CaCertNSS property.
  SetArg(kOpenVPNCaCertNSSProperty, kCaCertNSS);
  EXPECT_TRUE(driver_->InitCAOptions(&options, &error));
  ExpectInFlags(options, "ca", kCaCert);
  EXPECT_TRUE(error.IsSuccess());

  SetArg(kOpenVPNCaCertProperty, "");
  SetArg(kProviderHostProperty, kHost);
  FilePath empty_cert;
  error.Reset();
  EXPECT_TRUE(driver_->InitCAOptions(&options, &error));
  ExpectInFlags(options, "ca", OpenVPNDriver::kDefaultCACertificates);
  EXPECT_TRUE(error.IsSuccess());

  SetArg(kOpenVPNCaCertProperty, kCaCert);
  const vector<string> kCaCertPEM{ "---PEM CONTENTS---" };
  SetArgArray(kOpenVPNCaCertPemProperty, kCaCertPEM);
  EXPECT_FALSE(driver_->InitCAOptions(&options, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("Can't specify more than one of CACert and CACertPEM.",
            error.message());

  options.clear();
  SetArg(kOpenVPNCaCertProperty, "");
  SetArg(kProviderHostProperty, "");
  static const char kPEMCertfile[] = "/tmp/pem-cert";
  FilePath pem_cert(kPEMCertfile);
  EXPECT_CALL(*certificate_file_, CreatePEMFromStrings(kCaCertPEM))
      .WillOnce(Return(empty_cert))
      .WillOnce(Return(pem_cert));

  error.Reset();
  EXPECT_FALSE(driver_->InitCAOptions(&options, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
  EXPECT_EQ("Unable to extract PEM CA certificates.", error.message());

  error.Reset();
  options.clear();
  EXPECT_TRUE(driver_->InitCAOptions(&options, &error));
  ExpectInFlags(options, "ca", kPEMCertfile);
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(OpenVPNDriverTest, InitCertificateVerifyOptions) {
  {
    Error error;
    vector<vector<string>> options;
    // No options supplied.
    driver_->InitCertificateVerifyOptions(&options);
    EXPECT_TRUE(options.empty());
  }
  const char kName[] = "x509-name";
  {
    Error error;
    vector<vector<string>> options;
    // With Name property alone, we should have the 1-parameter version of the
    // "x509-verify-name" parameter provided.
    SetArg(kOpenVPNVerifyX509NameProperty, kName);
    driver_->InitCertificateVerifyOptions(&options);
    ExpectInFlags(options, "verify-x509-name", kName);
  }
  const char kType[] = "x509-type";
  {
    Error error;
    vector<vector<string>> options;
    // With both Name property and Type property set, we should have the
    // 2-parameter version of the "x509-verify-name" parameter provided.
    SetArg(kOpenVPNVerifyX509TypeProperty, kType);
    driver_->InitCertificateVerifyOptions(&options);
    ExpectInFlags(options, vector<string> { "verify-x509-name", kName, kType });
  }
  {
    Error error;
    vector<vector<string>> options;
    // We should ignore the Type parameter if no Name parameter is specified.
    SetArg(kOpenVPNVerifyX509NameProperty, "");
    driver_->InitCertificateVerifyOptions(&options);
    EXPECT_TRUE(options.empty());
  }
}

TEST_F(OpenVPNDriverTest, InitClientAuthOptions) {
  static const char kTestValue[] = "foo";
  vector<vector<string>> options;

  // No key or cert, assume user/password authentication.
  driver_->InitClientAuthOptions(&options);
  ExpectInFlags(options, "auth-user-pass");
  ExpectNotInFlags(options, "key");
  ExpectNotInFlags(options, "cert");

  // Cert available, no user/password.
  options.clear();
  SetArg(kOpenVPNCertProperty, kTestValue);
  driver_->InitClientAuthOptions(&options);
  ExpectNotInFlags(options, "auth-user-pass");
  ExpectNotInFlags(options, "key");
  ExpectInFlags(options, "cert", kTestValue);

  // Key available, no user/password.
  options.clear();
  SetArg(kOpenVPNKeyProperty, kTestValue);
  driver_->InitClientAuthOptions(&options);
  ExpectNotInFlags(options, "auth-user-pass");
  ExpectInFlags(options, "key", kTestValue);

  // Key available, AuthUserPass set.
  options.clear();
  SetArg(kOpenVPNAuthUserPassProperty, kTestValue);
  driver_->InitClientAuthOptions(&options);
  ExpectInFlags(options, "auth-user-pass");
  ExpectInFlags(options, "key", kTestValue);

  // Key available, User set.
  options.clear();
  RemoveStringArg(kOpenVPNAuthUserPassProperty);
  SetArg(kOpenVPNUserProperty, "user");
  driver_->InitClientAuthOptions(&options);
  ExpectInFlags(options, "auth-user-pass");
  ExpectInFlags(options, "key", kTestValue);

  // Empty PKCS11 certificate id, no user/password/cert.
  options.clear();
  RemoveStringArg(kOpenVPNKeyProperty);
  RemoveStringArg(kOpenVPNCertProperty);
  RemoveStringArg(kOpenVPNUserProperty);
  SetArg(kOpenVPNClientCertIdProperty, "");
  driver_->InitClientAuthOptions(&options);
  ExpectInFlags(options, "auth-user-pass");
  ExpectNotInFlags(options, "key");
  ExpectNotInFlags(options, "cert");
  ExpectNotInFlags(options, "pkcs11-id");

  // Non-empty PKCS11 certificate id, no user/password/cert.
  options.clear();
  SetArg(kOpenVPNClientCertIdProperty, kTestValue);
  driver_->InitClientAuthOptions(&options);
  ExpectNotInFlags(options, "auth-user-pass");
  ExpectNotInFlags(options, "key");
  ExpectNotInFlags(options, "cert");
  // The "--pkcs11-id" option is added in InitPKCS11Options(), not here.
  ExpectNotInFlags(options, "pkcs11-id");

  // PKCS11 certificate id available, AuthUserPass set.
  options.clear();
  SetArg(kOpenVPNAuthUserPassProperty, kTestValue);
  driver_->InitClientAuthOptions(&options);
  ExpectInFlags(options, "auth-user-pass");
  ExpectNotInFlags(options, "key");
  ExpectNotInFlags(options, "cert");

  // PKCS11 certificate id available, User set.
  options.clear();
  RemoveStringArg(kOpenVPNAuthUserPassProperty);
  SetArg(kOpenVPNUserProperty, "user");
  driver_->InitClientAuthOptions(&options);
  ExpectInFlags(options, "auth-user-pass");
  ExpectNotInFlags(options, "key");
  ExpectNotInFlags(options, "cert");
}

TEST_F(OpenVPNDriverTest, InitExtraCertOptions) {
  {
    Error error;
    vector<vector<string>> options;
    // No ExtraCertOptions supplied.
    EXPECT_TRUE(driver_->InitExtraCertOptions(&options, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_TRUE(options.empty());
  }
  {
    Error error;
    vector<vector<string>> options;
    SetArgArray(kOpenVPNExtraCertPemProperty, vector<string>());
    // Empty ExtraCertOptions supplied.
    EXPECT_TRUE(driver_->InitExtraCertOptions(&options, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_TRUE(options.empty());
  }
  const vector<string> kExtraCerts{ "---PEM CONTENTS---" };
  SetArgArray(kOpenVPNExtraCertPemProperty, kExtraCerts);
  static const char kPEMCertfile[] = "/tmp/pem-cert";
  FilePath pem_cert(kPEMCertfile);
  EXPECT_CALL(*extra_certificates_file_, CreatePEMFromStrings(kExtraCerts))
      .WillOnce(Return(FilePath()))
      .WillOnce(Return(pem_cert));
  // CreatePemFromStrings fails.
  {
    Error error;
    vector<vector<string>> options;
    EXPECT_FALSE(driver_->InitExtraCertOptions(&options, &error));
    EXPECT_EQ(Error::kInvalidArguments, error.type());
    EXPECT_TRUE(options.empty());
  }
  // CreatePemFromStrings succeeds.
  {
    Error error;
    vector<vector<string>> options;
    EXPECT_TRUE(driver_->InitExtraCertOptions(&options, &error));
    EXPECT_TRUE(error.IsSuccess());
    ExpectInFlags(options, "extra-certs", kPEMCertfile);
  }
}

TEST_F(OpenVPNDriverTest, InitPKCS11Options) {
  vector<vector<string>> options;
  driver_->InitPKCS11Options(&options);
  EXPECT_TRUE(options.empty());

  static const char kID[] = "TestPKCS11ID";
  SetArg(kOpenVPNClientCertIdProperty, kID);
  driver_->InitPKCS11Options(&options);
  ExpectInFlags(options, "pkcs11-id", kID);
  ExpectInFlags(options, "pkcs11-providers", "libchaps.so");

  static const char kProvider[] = "libpkcs11.so";
  SetArg(kOpenVPNProviderProperty, kProvider);
  options.clear();
  driver_->InitPKCS11Options(&options);
  ExpectInFlags(options, "pkcs11-id", kID);
  ExpectInFlags(options, "pkcs11-providers", kProvider);
}

TEST_F(OpenVPNDriverTest, InitManagementChannelOptionsServerFail) {
  vector<vector<string>> options;
  EXPECT_CALL(*management_server_, Start(&dispatcher_, GetSockets(), &options))
      .WillOnce(Return(false));
  Error error;
  EXPECT_FALSE(InitManagementChannelOptions(&options, &error));
  EXPECT_EQ(Error::kInternalError, error.type());
  EXPECT_EQ("Unable to setup management channel.", error.message());
}

TEST_F(OpenVPNDriverTest, InitManagementChannelOptionsOnline) {
  vector<vector<string>> options;
  EXPECT_CALL(*management_server_, Start(&dispatcher_, GetSockets(), &options))
      .WillOnce(Return(true));
  EXPECT_CALL(manager_, IsConnected()).WillOnce(Return(true));
  EXPECT_CALL(*management_server_, ReleaseHold());
  Error error;
  EXPECT_TRUE(InitManagementChannelOptions(&options, &error));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(OpenVPNDriverTest, InitManagementChannelOptionsOffline) {
  vector<vector<string>> options;
  EXPECT_CALL(*management_server_, Start(&dispatcher_, GetSockets(), &options))
      .WillOnce(Return(true));
  EXPECT_CALL(manager_, IsConnected()).WillOnce(Return(false));
  EXPECT_CALL(*management_server_, ReleaseHold()).Times(0);
  Error error;
  EXPECT_TRUE(InitManagementChannelOptions(&options, &error));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(OpenVPNDriverTest, InitLoggingOptions) {
  vector<vector<string>> options;
  bool vpn_logging = SLOG_IS_ON(VPN, 0);
  ScopeLogger::GetInstance()->EnableScopesByName("-vpn");
  driver_->InitLoggingOptions(&options);
  ASSERT_EQ(1, options.size());
  EXPECT_EQ(vector<string> { "syslog" }, options[0]);
  ScopeLogger::GetInstance()->EnableScopesByName("+vpn");
  options.clear();
  driver_->InitLoggingOptions(&options);
  ExpectInFlags(options, "verb", "3");
  ScopeLogger::GetInstance()->EnableScopesByName("-vpn");
  SetArg("OpenVPN.Verb", "2");
  options.clear();
  driver_->InitLoggingOptions(&options);
  ExpectInFlags(options, "verb", "2");
  ScopeLogger::GetInstance()->EnableScopesByName("+vpn");
  SetArg("OpenVPN.Verb", "1");
  options.clear();
  driver_->InitLoggingOptions(&options);
  ExpectInFlags(options, "verb", "1");
  if (!vpn_logging) {
    ScopeLogger::GetInstance()->EnableScopesByName("-vpn");
  }
}

TEST_F(OpenVPNDriverTest, AppendValueOption) {
  vector<vector<string>> options;
  EXPECT_FALSE(
      driver_->AppendValueOption("OpenVPN.UnknownProperty", kOption, &options));
  EXPECT_TRUE(options.empty());

  SetArg(kProperty, "");
  EXPECT_FALSE(driver_->AppendValueOption(kProperty, kOption, &options));
  EXPECT_TRUE(options.empty());

  SetArg(kProperty, kValue);
  SetArg(kProperty2, kValue2);
  EXPECT_TRUE(driver_->AppendValueOption(kProperty, kOption, &options));
  EXPECT_TRUE(driver_->AppendValueOption(kProperty2, kOption2, &options));
  EXPECT_EQ(2, options.size());
  vector<string> expected_value { kOption, kValue };
  EXPECT_EQ(expected_value, options[0]);
  vector<string> expected_value2 { kOption2, kValue2 };
  EXPECT_EQ(expected_value2, options[1]);
}

TEST_F(OpenVPNDriverTest, AppendDelimitedValueOption) {
  vector<vector<string>> options;
  EXPECT_FALSE(
      driver_->AppendDelimitedValueOption(
          "OpenVPN.UnknownProperty", kOption, ' ', &options));
  EXPECT_TRUE(options.empty());

  SetArg(kProperty, "");
  EXPECT_FALSE(
      driver_->AppendDelimitedValueOption(kProperty, kOption, ' ', &options));
  EXPECT_TRUE(options.empty());

  string kConcatenatedValues(string(kValue) + " " + string(kValue2));
  SetArg(kProperty, kConcatenatedValues);
  SetArg(kProperty2, kConcatenatedValues);
  EXPECT_TRUE(driver_->AppendDelimitedValueOption(
      kProperty, kOption, ':', &options));
  EXPECT_TRUE(driver_->AppendDelimitedValueOption(
      kProperty2, kOption2, ' ', &options));
  EXPECT_EQ(2, options.size());
  vector<string> expected_value { kOption, kConcatenatedValues };
  EXPECT_EQ(expected_value, options[0]);
  vector<string> expected_value2 { kOption2, kValue, kValue2 };
  EXPECT_EQ(expected_value2, options[1]);
}

TEST_F(OpenVPNDriverTest, AppendFlag) {
  vector<vector<string>> options;
  EXPECT_FALSE(
      driver_->AppendFlag("OpenVPN.UnknownProperty", kOption, &options));
  EXPECT_TRUE(options.empty());

  SetArg(kProperty, "");
  SetArg(kProperty2, kValue2);
  EXPECT_TRUE(driver_->AppendFlag(kProperty, kOption, &options));
  EXPECT_TRUE(driver_->AppendFlag(kProperty2, kOption2, &options));
  EXPECT_EQ(2, options.size());
  EXPECT_EQ(vector<string> { kOption }, options[0]);
  EXPECT_EQ(vector<string> { kOption2 }, options[1]);
}

TEST_F(OpenVPNDriverTest, ClaimInterface) {
  driver_->tunnel_interface_ = kInterfaceName;
  EXPECT_FALSE(driver_->ClaimInterface(string(kInterfaceName) + "XXX",
                                       kInterfaceIndex));
  EXPECT_FALSE(driver_->device_);

  static const char kHost[] = "192.168.2.254";
  SetArg(kProviderHostProperty, kHost);
  EXPECT_CALL(*management_server_, Start(_, _, _)).WillOnce(Return(true));
  EXPECT_CALL(manager_, IsConnected()).WillOnce(Return(false));
  EXPECT_CALL(
      process_manager_,
      StartProcess(_, _, _, _, false /* Don't exit with parent */, _))
      .WillOnce(Return(true));
  const int kServiceCallbackTag = 1;
  EXPECT_EQ(0, driver_->default_service_callback_tag_);
  EXPECT_CALL(manager_, RegisterDefaultServiceCallback(_))
      .WillOnce(Return(kServiceCallbackTag));
  EXPECT_TRUE(driver_->ClaimInterface(kInterfaceName, kInterfaceIndex));
  ASSERT_TRUE(driver_->device_);
  EXPECT_EQ(kInterfaceIndex, driver_->device_->interface_index());
  EXPECT_EQ(kServiceCallbackTag, driver_->default_service_callback_tag_);
}

TEST_F(OpenVPNDriverTest, IdleService) {
  SetService(service_);
  EXPECT_CALL(*service_, SetState(Service::kStateIdle));
  driver_->IdleService();
}

TEST_F(OpenVPNDriverTest, FailService) {
  static const char kErrorDetails[] = "Bad password.";
  SetService(service_);
  EXPECT_CALL(*service_, SetFailure(Service::kFailureConnect));
  driver_->FailService(Service::kFailureConnect, kErrorDetails);
  EXPECT_EQ(kErrorDetails, service_->error_details());
}

TEST_F(OpenVPNDriverTest, Cleanup) {
  // Ensure no crash.
  driver_->Cleanup(Service::kStateIdle,
                   Service::kFailureUnknown,
                   Service::kErrorDetailsNone);

  const int kPID = 123456;
  const int kServiceCallbackTag = 5;
  static const char kErrorDetails[] = "Certificate revoked.";
  driver_->default_service_callback_tag_ = kServiceCallbackTag;
  driver_->pid_ = kPID;
  driver_->rpc_task_.reset(new RPCTask(&control_, this));
  driver_->tunnel_interface_ = kInterfaceName;
  driver_->device_ = device_;
  driver_->service_ = service_;
  driver_->ip_properties_.address = "1.2.3.4";
  StartConnectTimeout(0);
  FilePath tls_auth_file;
  EXPECT_TRUE(base::CreateTemporaryFile(&tls_auth_file));
  EXPECT_FALSE(tls_auth_file.empty());
  EXPECT_TRUE(base::PathExists(tls_auth_file));
  driver_->tls_auth_file_ = tls_auth_file;
  // Stop will be called twice -- once by Cleanup and once by the destructor.
  EXPECT_CALL(*management_server_, Stop()).Times(2);
  // UpdateExitCallback will be called twice -- once to ignore exit,
  // and once to re-enabling monitoring of exit.
  EXPECT_CALL(process_manager_, UpdateExitCallback(kPID, _)).Times(2);
  EXPECT_CALL(manager_, DeregisterDefaultServiceCallback(kServiceCallbackTag));
  EXPECT_CALL(process_manager_, StopProcess(kPID));
  EXPECT_CALL(device_info_, DeleteInterface(_)).Times(0);
  EXPECT_CALL(*device_, DropConnection());
  EXPECT_CALL(*device_, SetEnabled(false));
  EXPECT_CALL(*service_, SetFailure(Service::kFailureInternal));
  driver_->Cleanup(
      Service::kStateFailure, Service::kFailureInternal,  kErrorDetails);
  EXPECT_EQ(0, driver_->default_service_callback_tag_);
  EXPECT_EQ(0, driver_->pid_);
  EXPECT_FALSE(driver_->rpc_task_.get());
  EXPECT_TRUE(driver_->tunnel_interface_.empty());
  EXPECT_FALSE(driver_->device_);
  EXPECT_FALSE(driver_->service_);
  EXPECT_EQ(kErrorDetails, service_->error_details());
  EXPECT_FALSE(base::PathExists(tls_auth_file));
  EXPECT_TRUE(driver_->tls_auth_file_.empty());
  EXPECT_TRUE(driver_->ip_properties_.address.empty());
  EXPECT_FALSE(driver_->IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, SpawnOpenVPN) {
  SetupLSBRelease();

  EXPECT_FALSE(driver_->SpawnOpenVPN());

  static const char kHost[] = "192.168.2.254";
  SetArg(kProviderHostProperty, kHost);
  driver_->tunnel_interface_ = "tun0";
  driver_->rpc_task_.reset(new RPCTask(&control_, this));
  EXPECT_CALL(*management_server_, Start(_, _, _))
      .Times(2)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(manager_, IsConnected()).Times(2).WillRepeatedly(Return(false));

  const int kPID = 234678;
  const map<string, string> expected_env{
    {"IV_PLAT", "Chromium OS"},
    {"IV_PLAT_REL", "2202.0"}};
  EXPECT_CALL(process_manager_, StartProcess(_, _, _, expected_env, _, _))
      .WillOnce(Return(-1))
      .WillOnce(Return(kPID));
  EXPECT_FALSE(driver_->SpawnOpenVPN());
  EXPECT_TRUE(driver_->SpawnOpenVPN());
  EXPECT_EQ(kPID, driver_->pid_);
}

TEST_F(OpenVPNDriverTest, OnOpenVPNDied) {
  const int kPID = 99999;
  driver_->device_ = device_;
  driver_->pid_ = kPID;
  EXPECT_CALL(*device_, DropConnection());
  EXPECT_CALL(*device_, SetEnabled(false));
  EXPECT_CALL(process_manager_, StopProcess(_)).Times(0);
  EXPECT_CALL(device_info_, DeleteInterface(kInterfaceIndex));
  driver_->OnOpenVPNDied(2);
  EXPECT_EQ(0, driver_->pid_);
}

TEST_F(OpenVPNDriverTest, Disconnect) {
  driver_->device_ = device_;
  driver_->service_ = service_;
  EXPECT_CALL(*device_, DropConnection());
  EXPECT_CALL(*device_, SetEnabled(false));
  EXPECT_CALL(device_info_, DeleteInterface(kInterfaceIndex));
  EXPECT_CALL(*service_, SetState(Service::kStateIdle));
  driver_->Disconnect();
  EXPECT_FALSE(driver_->device_);
  EXPECT_FALSE(driver_->service_);
}

TEST_F(OpenVPNDriverTest, OnConnectionDisconnected) {
  EXPECT_CALL(*management_server_, Restart());
  SetDevice(device_);
  SetService(service_);
  EXPECT_CALL(*device_, DropConnection());
  EXPECT_CALL(*service_, SetState(Service::kStateAssociating));
  OnConnectionDisconnected();
  EXPECT_TRUE(IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, OnConnectTimeout) {
  StartConnectTimeout(0);
  SetService(service_);
  EXPECT_CALL(*service_, SetFailure(Service::kFailureConnect));
  OnConnectTimeout();
  EXPECT_FALSE(GetService());
  EXPECT_FALSE(IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, OnConnectTimeoutResolve) {
  StartConnectTimeout(0);
  SetService(service_);
  SetClientState(OpenVPNManagementServer::kStateResolve);
  EXPECT_CALL(*service_, SetFailure(Service::kFailureDNSLookup));
  OnConnectTimeout();
  EXPECT_FALSE(GetService());
  EXPECT_FALSE(IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, OnReconnectingUnknown) {
  EXPECT_FALSE(IsConnectTimeoutStarted());
  EXPECT_CALL(dispatcher_,
              PostDelayedTask(_, GetDefaultConnectTimeoutSeconds() * 1000));
  SetDevice(device_);
  SetService(service_);
  EXPECT_CALL(*device_, DropConnection());
  EXPECT_CALL(*service_, SetState(Service::kStateAssociating));
  driver_->OnReconnecting(OpenVPNDriver::kReconnectReasonUnknown);
  EXPECT_TRUE(IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, OnReconnectingTLSError) {
  EXPECT_CALL(dispatcher_,
              PostDelayedTask(_, GetReconnectOfflineTimeoutSeconds() * 1000));
  EXPECT_CALL(dispatcher_,
              PostDelayedTask(_, GetReconnectTLSErrorTimeoutSeconds() * 1000));

  driver_->OnReconnecting(OpenVPNDriver::kReconnectReasonOffline);
  EXPECT_TRUE(IsConnectTimeoutStarted());

  // The scheduled timeout should not be affected for unknown reason.
  driver_->OnReconnecting(OpenVPNDriver::kReconnectReasonUnknown);
  EXPECT_TRUE(IsConnectTimeoutStarted());

  // Reconnect on TLS error reschedules the timeout once.
  driver_->OnReconnecting(OpenVPNDriver::kReconnectReasonTLSError);
  EXPECT_TRUE(IsConnectTimeoutStarted());
  driver_->OnReconnecting(OpenVPNDriver::kReconnectReasonTLSError);
  EXPECT_TRUE(IsConnectTimeoutStarted());
}

TEST_F(OpenVPNDriverTest, InitPropertyStore) {
  // Sanity test property store initialization.
  PropertyStore store;
  driver_->InitPropertyStore(&store);
  const string kUser = "joe";
  Error error;
  EXPECT_TRUE(store.SetStringProperty(kOpenVPNUserProperty, kUser, &error));
  EXPECT_TRUE(error.IsSuccess());
  EXPECT_EQ(kUser, GetArgs()->LookupString(kOpenVPNUserProperty, ""));
}

TEST_F(OpenVPNDriverTest, PassphraseRequired) {
  PropertyStore store;
  driver_->InitPropertyStore(&store);
  KeyValueStore props = GetProviderProperties(store);
  EXPECT_TRUE(props.LookupBool(kPassphraseRequiredProperty, false));

  SetArg(kOpenVPNPasswordProperty, "random-password");
  props = GetProviderProperties(store);
  EXPECT_FALSE(props.LookupBool(kPassphraseRequiredProperty, true));
  // This parameter should be write-only.
  EXPECT_FALSE(props.ContainsString(kOpenVPNPasswordProperty));

  SetArg(kOpenVPNPasswordProperty, "");
  props = GetProviderProperties(store);
  EXPECT_TRUE(props.LookupBool(kPassphraseRequiredProperty, false));

  SetArg(kOpenVPNTokenProperty, "random-token");
  props = GetProviderProperties(store);
  EXPECT_FALSE(props.LookupBool(kPassphraseRequiredProperty, true));
  // This parameter should be write-only.
  EXPECT_FALSE(props.ContainsString(kOpenVPNTokenProperty));
}

TEST_F(OpenVPNDriverTest, GetEnvironment) {
  SetupLSBRelease();
  const map<string, string> expected{
    {"IV_PLAT", "Chromium OS"},
    {"IV_PLAT_REL", "2202.0"}};
  ASSERT_EQ(expected, driver_->GetEnvironment());

  EXPECT_EQ(0, base::WriteFile(lsb_release_file_, "", 0));
  EXPECT_EQ(0, driver_->GetEnvironment().size());
}

TEST_F(OpenVPNDriverTest, OnOpenVPNExited) {
  const int kExitStatus = 1;
  std::unique_ptr<MockDeviceInfo> device_info(
      new MockDeviceInfo(&control_, &dispatcher_, &metrics_, &manager_));
  EXPECT_CALL(*device_info, DeleteInterface(kInterfaceIndex))
      .WillOnce(Return(true));
  WeakPtr<DeviceInfo> weak = device_info->AsWeakPtr();
  EXPECT_TRUE(weak);
  OpenVPNDriver::OnOpenVPNExited(weak, kInterfaceIndex, kExitStatus);
  device_info.reset();
  EXPECT_FALSE(weak);
  // Expect no crash.
  OpenVPNDriver::OnOpenVPNExited(weak, kInterfaceIndex, kExitStatus);
}

TEST_F(OpenVPNDriverTest, OnDefaultServiceChanged) {
  driver_->service_ = service_;

  ServiceRefPtr null_service;
  EXPECT_CALL(*management_server_, Hold());
  driver_->OnDefaultServiceChanged(null_service);

  EXPECT_CALL(*management_server_, Hold());
  driver_->OnDefaultServiceChanged(service_);

  scoped_refptr<MockService> mock_service(
      new MockService(&control_, &dispatcher_, &metrics_, &manager_));

  EXPECT_CALL(*mock_service, IsConnected()).WillOnce(Return(false));
  EXPECT_CALL(*management_server_, Hold());
  driver_->OnDefaultServiceChanged(mock_service);

  EXPECT_CALL(*mock_service, IsConnected()).WillOnce(Return(true));
  EXPECT_CALL(*management_server_, ReleaseHold());
  driver_->OnDefaultServiceChanged(mock_service);
}

TEST_F(OpenVPNDriverTest, GetReconnectTimeoutSeconds) {
  EXPECT_EQ(GetDefaultConnectTimeoutSeconds(),
            GetReconnectTimeoutSeconds(OpenVPNDriver::kReconnectReasonUnknown));
  EXPECT_EQ(GetReconnectOfflineTimeoutSeconds(),
            GetReconnectTimeoutSeconds(OpenVPNDriver::kReconnectReasonOffline));
  EXPECT_EQ(GetReconnectTLSErrorTimeoutSeconds(),
            GetReconnectTimeoutSeconds(
                OpenVPNDriver::kReconnectReasonTLSError));
}

TEST_F(OpenVPNDriverTest, WriteConfigFile) {
  const char kOption0[] = "option0";
  const char kOption1[] = "option1";
  const char kOption1Argument0[] = "option1-argument0";
  const char kOption2[] = "option2";
  const char kOption2Argument0[] = "option2-argument0\n\t\"'\\";
  const char kOption2Argument0Transformed[] = "option2-argument0 \t\\\"'\\\\";
  const char kOption2Argument1[] = "option2-argument1 space";
  vector<vector<string>> options {
      { kOption0 },
      { kOption1, kOption1Argument0 },
      { kOption2, kOption2Argument0, kOption2Argument1 }
  };
  FilePath config_directory(
      temporary_directory_.path().Append(kOpenVPNConfigDirectory));
  FilePath config_file;
  EXPECT_FALSE(base::PathExists(config_directory));
  EXPECT_TRUE(driver_->WriteConfigFile(options, &config_file));
  EXPECT_TRUE(base::PathExists(config_directory));
  EXPECT_TRUE(base::PathExists(config_file));
  EXPECT_TRUE(config_directory.IsParent(config_file));

  string config_contents;
  EXPECT_TRUE(base::ReadFileToString(config_file, &config_contents));
  string expected_config_contents = base::StringPrintf(
      "%s\n%s %s\n%s \"%s\" \"%s\"\n",
      kOption0,
      kOption1, kOption1Argument0,
      kOption2, kOption2Argument0Transformed, kOption2Argument1);
  EXPECT_EQ(expected_config_contents, config_contents);
}

}  // namespace shill

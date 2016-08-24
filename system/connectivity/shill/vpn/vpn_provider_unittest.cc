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

#include "shill/vpn/vpn_provider.h"

#include <memory>
#include <set>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/fake_store.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_device_info.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_profile.h"
#include "shill/mock_store.h"
#include "shill/nice_mock_control.h"
#include "shill/vpn/mock_vpn_driver.h"
#include "shill/vpn/mock_vpn_service.h"

using std::string;
using std::unique_ptr;
using testing::_;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StartsWith;

namespace shill {

class VPNProviderTest : public testing::Test {
 public:
  VPNProviderTest()
      : metrics_(nullptr),
        manager_(&control_, nullptr, &metrics_),
        device_info_(&control_, nullptr, &metrics_, &manager_),
        provider_(&control_, nullptr, &metrics_, &manager_) {}

  virtual ~VPNProviderTest() {}

 protected:
  static const char kHost[];
  static const char kName[];

  string GetServiceFriendlyName(const ServiceRefPtr& service) {
    return service->friendly_name();
  }

  void SetConnectState(const ServiceRefPtr& service,
                       Service::ConnectState state) {
    service->state_ = state;
  }

  void AddService(const VPNServiceRefPtr& service) {
    provider_.services_.push_back(service);
  }

  VPNServiceRefPtr GetServiceAt(int idx) {
    return provider_.services_[idx];
  }

  size_t GetServiceCount() const {
    return provider_.services_.size();
  }

  NiceMockControl control_;
  MockMetrics metrics_;
  MockManager manager_;
  MockDeviceInfo device_info_;
  VPNProvider provider_;
};

const char VPNProviderTest::kHost[] = "10.8.0.1";
const char VPNProviderTest::kName[] = "vpn-name";

TEST_F(VPNProviderTest, GetServiceNoType) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeVPN);
  ServiceRefPtr service = provider_.GetService(args, &e);
  EXPECT_EQ(Error::kNotSupported, e.type());
  EXPECT_FALSE(service);
}

TEST_F(VPNProviderTest, GetServiceUnsupportedType) {
  KeyValueStore args;
  Error e;
  args.SetString(kTypeProperty, kTypeVPN);
  args.SetString(kProviderTypeProperty, "unknown-vpn-type");
  args.SetString(kProviderHostProperty, kHost);
  args.SetString(kNameProperty, kName);
  ServiceRefPtr service = provider_.GetService(args, &e);
  EXPECT_EQ(Error::kNotSupported, e.type());
  EXPECT_FALSE(service);
}

TEST_F(VPNProviderTest, GetService) {
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeVPN);
  args.SetString(kProviderTypeProperty, kProviderOpenVpn);
  args.SetString(kProviderHostProperty, kHost);
  args.SetString(kNameProperty, kName);

  {
    Error error;
    ServiceRefPtr service = provider_.FindSimilarService(args, &error);
    EXPECT_EQ(Error::kNotFound, error.type());
    EXPECT_EQ(nullptr, service.get());
  }

  EXPECT_EQ(0, GetServiceCount());

  ServiceRefPtr service;
  {
    Error error;
    EXPECT_CALL(manager_, device_info()).WillOnce(Return(&device_info_));
    EXPECT_CALL(manager_, RegisterService(_));
    service = provider_.GetService(args, &error);
    EXPECT_TRUE(error.IsSuccess());
    ASSERT_TRUE(service);
    testing::Mock::VerifyAndClearExpectations(&manager_);
  }

  EXPECT_EQ("vpn_10_8_0_1_vpn_name", service->GetStorageIdentifier());
  EXPECT_EQ(kName, GetServiceFriendlyName(service));

  EXPECT_EQ(1, GetServiceCount());

  // Configure the service to set its properties (including Provider.Host).
  {
    Error error;
    service->Configure(args, &error);
    EXPECT_TRUE(error.IsSuccess());
  }

  // None of the calls below should cause a new service to be registered.
  EXPECT_CALL(manager_, RegisterService(_)).Times(0);

  // A second call should return the same service.
  {
    Error error;
    ServiceRefPtr get_service = provider_.GetService(args, &error);
    EXPECT_TRUE(error.IsSuccess());
    ASSERT_EQ(service, get_service);
  }

  EXPECT_EQ(1, GetServiceCount());

  // FindSimilarService should also return this service.
  {
    Error error;
    ServiceRefPtr similar_service = provider_.FindSimilarService(args, &error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_EQ(service, similar_service);
  }

  EXPECT_EQ(1, GetServiceCount());

  // However, CreateTemporaryService should create a different service.
  {
    Error error;
    ServiceRefPtr temporary_service =
        provider_.CreateTemporaryService(args, &error);
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_NE(service, temporary_service);

    // However this service will not be part of the provider.
    EXPECT_EQ(1, GetServiceCount());
  }
}

TEST_F(VPNProviderTest, OnDeviceInfoAvailable) {
  const string kInterfaceName("tun0");
  const int kInterfaceIndex = 1;

  unique_ptr<MockVPNDriver> bad_driver(new MockVPNDriver());
  EXPECT_CALL(*bad_driver.get(), ClaimInterface(_, _))
      .Times(2)
      .WillRepeatedly(Return(false));
  provider_.services_.push_back(new VPNService(&control_, nullptr, &metrics_,
                                               nullptr, bad_driver.release()));

  EXPECT_FALSE(provider_.OnDeviceInfoAvailable(kInterfaceName,
                                               kInterfaceIndex));

  unique_ptr<MockVPNDriver> good_driver(new MockVPNDriver());
  EXPECT_CALL(*good_driver.get(), ClaimInterface(_, _))
      .WillOnce(Return(true));
  provider_.services_.push_back(new VPNService(&control_, nullptr, &metrics_,
                                               nullptr, good_driver.release()));

  unique_ptr<MockVPNDriver> dup_driver(new MockVPNDriver());
  EXPECT_CALL(*dup_driver.get(), ClaimInterface(_, _))
      .Times(0);
  provider_.services_.push_back(new VPNService(&control_, nullptr, &metrics_,
                                               nullptr, dup_driver.release()));

  EXPECT_TRUE(provider_.OnDeviceInfoAvailable(kInterfaceName, kInterfaceIndex));
  provider_.services_.clear();
}

TEST_F(VPNProviderTest, RemoveService) {
  scoped_refptr<MockVPNService> service0(
      new MockVPNService(&control_, nullptr, &metrics_, nullptr, nullptr));
  scoped_refptr<MockVPNService> service1(
      new MockVPNService(&control_, nullptr, &metrics_, nullptr, nullptr));
  scoped_refptr<MockVPNService> service2(
      new MockVPNService(&control_, nullptr, &metrics_, nullptr, nullptr));

  provider_.services_.push_back(service0.get());
  provider_.services_.push_back(service1.get());
  provider_.services_.push_back(service2.get());

  ASSERT_EQ(3, provider_.services_.size());

  provider_.RemoveService(service1);

  EXPECT_EQ(2, provider_.services_.size());
  EXPECT_EQ(service0, provider_.services_[0]);
  EXPECT_EQ(service2, provider_.services_[1]);

  provider_.RemoveService(service2);

  EXPECT_EQ(1, provider_.services_.size());
  EXPECT_EQ(service0, provider_.services_[0]);

  provider_.RemoveService(service0);
  EXPECT_EQ(0, provider_.services_.size());
}

MATCHER_P(ServiceWithStorageId, storage_id, "") {
  return arg->GetStorageIdentifier() == storage_id;
}

TEST_F(VPNProviderTest, CreateServicesFromProfile) {
  FakeStore storage;
  storage.SetString("no_type", "Name", "No Type Entry");
  storage.SetString("no_vpn", "Type", "wimax");
  storage.SetString("vpn_no_provider_type", "Type", "vpn");
  storage.SetString("vpn_no_name", "Type", "vpn");
  storage.SetString("vpn_no_name", "Provider.Type", "openvpn");
  storage.SetString("vpn_no_host", "Type", "vpn");
  storage.SetString("vpn_no_host", "Provider.Type", "openvpn");
  storage.SetString("vpn_ho_host", "Name", "name");
  storage.SetString("vpn_complete", "Type", "vpn");
  storage.SetString("vpn_complete", "Provider.Type", "openvpn");
  storage.SetString("vpn_complete", "Name", "name");
  storage.SetString("vpn_complete", "Provider.Host", "1.2.3.4");

  scoped_refptr<MockProfile> profile(
      new NiceMock<MockProfile>(&control_, &metrics_, &manager_, ""));
  EXPECT_CALL(*profile, GetConstStorage()).WillRepeatedly(Return(&storage));

  EXPECT_CALL(manager_, device_info()).WillRepeatedly(Return(nullptr));
  EXPECT_CALL(manager_,
              RegisterService(ServiceWithStorageId("vpn_complete")));
  EXPECT_CALL(*profile,
              ConfigureService(ServiceWithStorageId("vpn_complete")))
      .WillOnce(Return(true));
  provider_.CreateServicesFromProfile(profile);

  GetServiceAt(0)->driver()->args()->SetString(kProviderHostProperty,
                                               "1.2.3.4");
  // Calling this again should not create any more services (checked by the
  // Times(1) above).
  provider_.CreateServicesFromProfile(profile);
}

TEST_F(VPNProviderTest, CreateService) {
  static const char kName[] = "test-vpn-service";
  static const char kStorageID[] = "test_vpn_storage_id";
  static const char* kTypes[] = {
    kProviderOpenVpn,
    kProviderL2tpIpsec,
    kProviderThirdPartyVpn
  };
  const size_t kTypesCount = arraysize(kTypes);
  EXPECT_CALL(manager_, device_info())
      .Times(kTypesCount)
      .WillRepeatedly(Return(&device_info_));
  EXPECT_CALL(manager_, RegisterService(_)).Times(kTypesCount);
  for (size_t i = 0; i < kTypesCount; i++) {
    Error error;
    VPNServiceRefPtr service =
        provider_.CreateService(kTypes[i], kName, kStorageID, &error);
    ASSERT_TRUE(service) << kTypes[i];
    ASSERT_TRUE(service->driver()) << kTypes[i];
    EXPECT_EQ(kTypes[i], service->driver()->GetProviderType());
    EXPECT_EQ(kName, GetServiceFriendlyName(service)) << kTypes[i];
    EXPECT_EQ(kStorageID, service->GetStorageIdentifier()) << kTypes[i];
    EXPECT_TRUE(error.IsSuccess()) << kTypes[i];
  }
  Error error;
  VPNServiceRefPtr unknown_service =
      provider_.CreateService("unknown-vpn-type", kName, kStorageID, &error);
  EXPECT_FALSE(unknown_service);
  EXPECT_EQ(Error::kNotSupported, error.type());
}

TEST_F(VPNProviderTest, CreateTemporaryServiceFromProfile) {
  FakeStore storage;
  storage.SetString("no_vpn", "Type", "wimax");
  storage.SetString("vpn_no_provider_type", "Type", "vpn");
  storage.SetString("vpn_no_name", "Type", "vpn");
  storage.SetString("vpn_no_name", "Provider.Type", "openvpn");
  storage.SetString("vpn_no_host", "Type", "vpn");
  storage.SetString("vpn_no_host", "Provider.Type", "openvpn");
  storage.SetString("vpn_no_host", "Name", "name");
  storage.SetString("vpn_complete", "Type", "vpn");
  storage.SetString("vpn_complete", "Provider.Type", "openvpn");
  storage.SetString("vpn_complete", "Name", "name");
  storage.SetString("vpn_complete", "Provider.Host", "1.2.3.4");

  scoped_refptr<MockProfile> profile(
      new NiceMock<MockProfile>(&control_, &metrics_, &manager_, ""));
  EXPECT_CALL(*profile, GetConstStorage()).WillRepeatedly(Return(&storage));
  Error error;

  // Non VPN entry.
  EXPECT_EQ(nullptr,
            provider_.CreateTemporaryServiceFromProfile(profile,
                                                        "no_vpn",
                                                        &error));
  EXPECT_FALSE(error.IsSuccess());
  EXPECT_THAT(error.message(),
              StartsWith("Unspecified or invalid network type"));

  // VPN type not specified.
  error.Reset();
  EXPECT_EQ(nullptr,
            provider_.CreateTemporaryServiceFromProfile(profile,
                                                        "vpn_no_provider_type",
                                                        &error));
  EXPECT_FALSE(error.IsSuccess());
  EXPECT_THAT(error.message(), StartsWith("VPN type not specified"));

  // Name not specified.
  error.Reset();
  EXPECT_EQ(nullptr,
            provider_.CreateTemporaryServiceFromProfile(profile,
                                                        "vpn_no_name",
                                                        &error));
  EXPECT_FALSE(error.IsSuccess());
  EXPECT_THAT(error.message(), StartsWith("Network name not specified"));

  // Host not specified.
  error.Reset();
  EXPECT_EQ(nullptr,
            provider_.CreateTemporaryServiceFromProfile(profile,
                                                        "vpn_no_host",
                                                        &error));
  EXPECT_FALSE(error.IsSuccess());
  EXPECT_THAT(error.message(), StartsWith("Host not specified"));

  // Valid VPN service entry.
  error.Reset();
  EXPECT_NE(nullptr,
            provider_.CreateTemporaryServiceFromProfile(profile,
                                                        "vpn_complete",
                                                        &error));
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(VPNProviderTest, HasActiveService) {
  EXPECT_FALSE(provider_.HasActiveService());

  scoped_refptr<MockVPNService> service0(
      new MockVPNService(&control_, nullptr, &metrics_, nullptr, nullptr));
  scoped_refptr<MockVPNService> service1(
      new MockVPNService(&control_, nullptr, &metrics_, nullptr, nullptr));
  scoped_refptr<MockVPNService> service2(
      new MockVPNService(&control_, nullptr, &metrics_, nullptr, nullptr));

  AddService(service0);
  AddService(service1);
  AddService(service2);
  EXPECT_FALSE(provider_.HasActiveService());

  SetConnectState(service1, Service::kStateAssociating);
  EXPECT_TRUE(provider_.HasActiveService());

  SetConnectState(service1, Service::kStateOnline);
  EXPECT_TRUE(provider_.HasActiveService());
}

}  // namespace shill

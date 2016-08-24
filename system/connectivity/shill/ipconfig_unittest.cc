//
// Copyright (C) 2011 The Android Open Source Project
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

#include "shill/ipconfig.h"

#include <sys/time.h>

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/logging.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_log.h"
#include "shill/mock_store.h"
#include "shill/net/mock_time.h"
#include "shill/static_ip_parameters.h"

using base::Bind;
using base::Unretained;
using std::string;
using testing::_;
using testing::EndsWith;
using testing::DoAll;
using testing::Mock;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;
using testing::SetArgumentPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kDeviceName[] = "testdevice";
const uint32_t kTimeNow = 10;
}  // namespace

class IPConfigTest : public Test {
 public:
  IPConfigTest() : ipconfig_(new IPConfig(&control_, kDeviceName)) {
    ipconfig_->time_ = &time_;
  }

  virtual void SetUp() {
    ScopeLogger::GetInstance()->EnableScopesByName("inet");
    ScopeLogger::GetInstance()->set_verbose_level(3);
  }

  virtual void TearDown() {
    ScopeLogger::GetInstance()->EnableScopesByName("-inet");
    ScopeLogger::GetInstance()->set_verbose_level(0);
  }

  void DropRef(const IPConfigRefPtr & /*ipconfig*/,
               bool /*new_lease_acquired*/) {
    ipconfig_ = nullptr;
  }

  MOCK_METHOD2(OnIPConfigUpdated,
               void(const IPConfigRefPtr& ipconfig, bool new_lease_acquired));
  MOCK_METHOD1(OnIPConfigFailed, void(const IPConfigRefPtr& ipconfig));
  MOCK_METHOD1(OnIPConfigRefreshed, void(const IPConfigRefPtr& ipconfig));
  MOCK_METHOD1(OnIPConfigExpired, void(const IPConfigRefPtr& ipconfig));

 protected:
  IPConfigMockAdaptor* GetAdaptor() {
    return static_cast<IPConfigMockAdaptor*>(ipconfig_->adaptor_.get());
  }

  void UpdateProperties(const IPConfig::Properties& properties) {
    ipconfig_->UpdateProperties(properties, true);
  }

  void NotifyFailure() {
    ipconfig_->NotifyFailure();
  }

  void NotifyExpiry() {
    ipconfig_->NotifyExpiry();
  }

  void ExpectPropertiesEqual(const IPConfig::Properties& properties) {
    EXPECT_EQ(properties.address, ipconfig_->properties().address);
    EXPECT_EQ(properties.subnet_prefix, ipconfig_->properties().subnet_prefix);
    EXPECT_EQ(properties.broadcast_address,
              ipconfig_->properties().broadcast_address);
    EXPECT_EQ(properties.dns_servers.size(),
              ipconfig_->properties().dns_servers.size());
    if (properties.dns_servers.size() ==
        ipconfig_->properties().dns_servers.size()) {
      for (size_t i = 0; i < properties.dns_servers.size(); ++i) {
        EXPECT_EQ(properties.dns_servers[i],
                  ipconfig_->properties().dns_servers[i]);
      }
    }
    EXPECT_EQ(properties.domain_search.size(),
              ipconfig_->properties().domain_search.size());
    if (properties.domain_search.size() ==
        ipconfig_->properties().domain_search.size()) {
      for (size_t i = 0; i < properties.domain_search.size(); ++i) {
        EXPECT_EQ(properties.domain_search[i],
                  ipconfig_->properties().domain_search[i]);
      }
    }
    EXPECT_EQ(properties.gateway, ipconfig_->properties().gateway);
    EXPECT_EQ(properties.blackhole_ipv6,
              ipconfig_->properties().blackhole_ipv6);
    EXPECT_EQ(properties.mtu, ipconfig_->properties().mtu);
  }

  MockControl control_;
  MockTime time_;
  IPConfigRefPtr ipconfig_;
};

TEST_F(IPConfigTest, DeviceName) {
  EXPECT_EQ(kDeviceName, ipconfig_->device_name());
}

TEST_F(IPConfigTest, RequestIP) {
  EXPECT_FALSE(ipconfig_->RequestIP());
}

TEST_F(IPConfigTest, RenewIP) {
  EXPECT_FALSE(ipconfig_->RenewIP());
}

TEST_F(IPConfigTest, ReleaseIP) {
  EXPECT_FALSE(ipconfig_->ReleaseIP(IPConfig::kReleaseReasonDisconnect));
}

TEST_F(IPConfigTest, UpdateProperties) {
  IPConfig::Properties properties;
  properties.address = "1.2.3.4";
  properties.subnet_prefix = 24;
  properties.broadcast_address = "11.22.33.44";
  properties.dns_servers.push_back("10.20.30.40");
  properties.dns_servers.push_back("20.30.40.50");
  properties.domain_name = "foo.org";
  properties.domain_search.push_back("zoo.org");
  properties.domain_search.push_back("zoo.com");
  properties.gateway = "5.6.7.8";
  properties.blackhole_ipv6 = true;
  properties.mtu = 700;
  UpdateProperties(properties);
  ExpectPropertiesEqual(properties);

  // We should not reset on NotifyFailure.
  NotifyFailure();
  ExpectPropertiesEqual(properties);

  // We should not reset on NotifyExpiry.
  NotifyExpiry();
  ExpectPropertiesEqual(properties);

  // We should reset if ResetProperties is called.
  ipconfig_->ResetProperties();
  ExpectPropertiesEqual(IPConfig::Properties());
}

TEST_F(IPConfigTest, Callbacks) {
  ipconfig_->RegisterUpdateCallback(
      Bind(&IPConfigTest::OnIPConfigUpdated, Unretained(this)));
  ipconfig_->RegisterFailureCallback(
      Bind(&IPConfigTest::OnIPConfigFailed, Unretained(this)));
  ipconfig_->RegisterRefreshCallback(
      Bind(&IPConfigTest::OnIPConfigRefreshed, Unretained(this)));
  ipconfig_->RegisterExpireCallback(
      Bind(&IPConfigTest::OnIPConfigExpired, Unretained(this)));

  EXPECT_CALL(*this, OnIPConfigUpdated(ipconfig_, true));
  EXPECT_CALL(*this, OnIPConfigFailed(ipconfig_)).Times(0);
  EXPECT_CALL(*this, OnIPConfigRefreshed(ipconfig_)).Times(0);
  EXPECT_CALL(*this, OnIPConfigExpired(ipconfig_)).Times(0);
  UpdateProperties(IPConfig::Properties());
  Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(*this, OnIPConfigUpdated(ipconfig_, true)).Times(0);
  EXPECT_CALL(*this, OnIPConfigFailed(ipconfig_));
  EXPECT_CALL(*this, OnIPConfigRefreshed(ipconfig_)).Times(0);
  EXPECT_CALL(*this, OnIPConfigExpired(ipconfig_)).Times(0);
  NotifyFailure();
  Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(*this, OnIPConfigUpdated(ipconfig_, true)).Times(0);
  EXPECT_CALL(*this, OnIPConfigFailed(ipconfig_)).Times(0);
  EXPECT_CALL(*this, OnIPConfigRefreshed(ipconfig_));
  EXPECT_CALL(*this, OnIPConfigExpired(ipconfig_)).Times(0);
  ipconfig_->Refresh(nullptr);
  Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(*this, OnIPConfigUpdated(ipconfig_, true)).Times(0);
  EXPECT_CALL(*this, OnIPConfigFailed(ipconfig_)).Times(0);
  EXPECT_CALL(*this, OnIPConfigRefreshed(ipconfig_)).Times(0);
  EXPECT_CALL(*this, OnIPConfigExpired(ipconfig_));
  NotifyExpiry();
  Mock::VerifyAndClearExpectations(this);
}

TEST_F(IPConfigTest, UpdatePropertiesWithDropRef) {
  // The UpdateCallback should be able to drop a reference to the
  // IPConfig object without crashing.
  ipconfig_->RegisterUpdateCallback(
      Bind(&IPConfigTest::DropRef, Unretained(this)));
  UpdateProperties(IPConfig::Properties());
}

TEST_F(IPConfigTest, PropertyChanges) {
  IPConfigMockAdaptor* adaptor = GetAdaptor();

  StaticIPParameters static_ip_params;
  EXPECT_CALL(*adaptor, EmitStringChanged(kAddressProperty, _));
  EXPECT_CALL(*adaptor, EmitStringsChanged(kNameServersProperty, _));
  ipconfig_->ApplyStaticIPParameters(&static_ip_params);
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_CALL(*adaptor, EmitStringChanged(kAddressProperty, _));
  EXPECT_CALL(*adaptor, EmitStringsChanged(kNameServersProperty, _));
  ipconfig_->RestoreSavedIPParameters(&static_ip_params);
  Mock::VerifyAndClearExpectations(adaptor);

  IPConfig::Properties ip_properties;
  EXPECT_CALL(*adaptor, EmitStringChanged(kAddressProperty, _));
  EXPECT_CALL(*adaptor, EmitStringsChanged(kNameServersProperty, _));
  UpdateProperties(ip_properties);
  Mock::VerifyAndClearExpectations(adaptor);

  // It is the callback's responsibility for resetting the IPConfig
  // properties (via IPConfig::ResetProperties()).  Since NotifyFailure
  // by itself doesn't change any properties, it should not emit any
  // property change events either.
  EXPECT_CALL(*adaptor, EmitStringChanged(_, _)).Times(0);
  EXPECT_CALL(*adaptor, EmitStringsChanged(_, _)).Times(0);
  NotifyFailure();
  Mock::VerifyAndClearExpectations(adaptor);

  // Similarly, NotifyExpiry() should have no property change side effects.
  EXPECT_CALL(*adaptor, EmitStringChanged(_, _)).Times(0);
  EXPECT_CALL(*adaptor, EmitStringsChanged(_, _)).Times(0);
  NotifyExpiry();
  Mock::VerifyAndClearExpectations(adaptor);

  EXPECT_CALL(*adaptor, EmitStringChanged(kAddressProperty, _));
  EXPECT_CALL(*adaptor, EmitStringsChanged(kNameServersProperty, _));
  ipconfig_->ResetProperties();
  Mock::VerifyAndClearExpectations(adaptor);
}

TEST_F(IPConfigTest, UpdateLeaseExpirationTime) {
  const struct timeval expected_time_now = {kTimeNow , 0};
  uint32_t lease_duration = 1;
  EXPECT_CALL(time_, GetTimeBoottime(_))
      .WillOnce(DoAll(SetArgPointee<0>(expected_time_now), Return(0)));
  ipconfig_->UpdateLeaseExpirationTime(lease_duration);
  EXPECT_EQ(kTimeNow + lease_duration,
            ipconfig_->current_lease_expiration_time_.tv_sec);
}

TEST_F(IPConfigTest, TimeToLeaseExpiry_NoDHCPLease) {
  ScopedMockLog log;
  uint32_t time_left = 0;
  // |current_lease_expiration_time_| has not been set, so expect an error.
  EXPECT_CALL(log, Log(_, _,
                       EndsWith("No current DHCP lease")));
  EXPECT_FALSE(ipconfig_->TimeToLeaseExpiry(&time_left));
  EXPECT_EQ(0, time_left);
}

TEST_F(IPConfigTest, TimeToLeaseExpiry_CurrentLeaseExpired) {
  ScopedMockLog log;
  const struct timeval time_now = {kTimeNow, 0};
  uint32_t time_left = 0;
  // Set |current_lease_expiration_time_| so it is expired (i.e. earlier than
  // current time).
  ipconfig_->current_lease_expiration_time_ = {kTimeNow - 1, 0};
  EXPECT_CALL(time_, GetTimeBoottime(_))
      .WillOnce(DoAll(SetArgPointee<0>(time_now), Return(0)));
  EXPECT_CALL(log, Log(_, _,
                       EndsWith("Current DHCP lease has already expired")));
  EXPECT_FALSE(ipconfig_->TimeToLeaseExpiry(&time_left));
  EXPECT_EQ(0, time_left);
}

TEST_F(IPConfigTest, TimeToLeaseExpiry_Success) {
  const uint32_t expected_time_to_expiry = 10;
  const struct timeval time_now = {kTimeNow, 0};
  uint32_t time_left;
  // Set |current_lease_expiration_time_| so it appears like we already
  // have obtained a DHCP lease before.
  ipconfig_->current_lease_expiration_time_ = {
      kTimeNow + expected_time_to_expiry, 0};
  EXPECT_CALL(time_, GetTimeBoottime(_))
      .WillOnce(DoAll(SetArgPointee<0>(time_now), Return(0)));
  EXPECT_TRUE(ipconfig_->TimeToLeaseExpiry(&time_left));
  EXPECT_EQ(expected_time_to_expiry, time_left);
}

}  // namespace shill

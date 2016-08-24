//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/ethernet/ethernet_eap_service.h"

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest.h>

#include "shill/ethernet/mock_ethernet_eap_provider.h"
#include "shill/mock_adaptors.h"
#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/service_property_change_test.h"
#include "shill/technology.h"

using testing::Return;

namespace shill {

class EthernetEapServiceTest : public testing::Test {
 public:
  EthernetEapServiceTest()
      : metrics_(&dispatcher_),
        manager_(&control_, &dispatcher_, &metrics_),
        service_(new EthernetEapService(&control_,
                                        &dispatcher_,
                                        &metrics_,
                                        &manager_)) {}
  virtual ~EthernetEapServiceTest() {}

 protected:
  ServiceMockAdaptor* GetAdaptor() {
    return static_cast<ServiceMockAdaptor*>(service_->adaptor());
  }

  MockControl control_;
  MockEventDispatcher dispatcher_;
  MockMetrics metrics_;
  MockManager manager_;
  MockEthernetEapProvider provider_;
  scoped_refptr<EthernetEapService> service_;
};

TEST_F(EthernetEapServiceTest, MethodOverrides) {
  EXPECT_EQ("/", service_->GetDeviceRpcId(nullptr));
  EXPECT_EQ("etherneteap_all", service_->GetStorageIdentifier());
  EXPECT_EQ(Technology::kEthernetEap, service_->technology());
  EXPECT_TRUE(service_->Is8021x());
  EXPECT_FALSE(service_->IsVisible());
}

TEST_F(EthernetEapServiceTest, OnEapCredentialsChanged) {
  service_->has_ever_connected_ = true;
  EXPECT_TRUE(service_->has_ever_connected());
  EXPECT_CALL(manager_, ethernet_eap_provider()).WillOnce(Return(&provider_));
  EXPECT_CALL(provider_, OnCredentialsChanged());
  service_->OnEapCredentialsChanged(Service::kReasonPropertyUpdate);
  EXPECT_FALSE(service_->has_ever_connected());
}

TEST_F(EthernetEapServiceTest, OnEapCredentialPropertyChanged) {
  EXPECT_CALL(manager_, ethernet_eap_provider()).WillOnce(Return(&provider_));
  EXPECT_CALL(provider_, OnCredentialsChanged());
  service_->OnPropertyChanged(kEapPasswordProperty);
}

TEST_F(EthernetEapServiceTest, Unload) {
  EXPECT_CALL(manager_, ethernet_eap_provider()).WillOnce(Return(&provider_));
  EXPECT_CALL(provider_, OnCredentialsChanged());
  EXPECT_FALSE(service_->Unload());
}

TEST_F(EthernetEapServiceTest, PropertyChanges) {
  TestCommonPropertyChanges(service_, GetAdaptor());
}

// Custom property setters should return false, and make no changes, if
// the new value is the same as the old value.
TEST_F(EthernetEapServiceTest, CustomSetterNoopChange) {
  TestCustomSetterNoopChange(service_, &manager_);
}

}  // namespace shill

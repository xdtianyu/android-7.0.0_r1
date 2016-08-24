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

#include "shill/ethernet/ethernet_eap_provider.h"

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/ethernet/mock_ethernet.h"
#include "shill/key_value_store.h"
#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"

using testing::_;
using testing::Mock;
using testing::SaveArg;

namespace shill {

class EthernetEapProviderTest : public testing::Test {
 public:
  EthernetEapProviderTest()
      : metrics_(&dispatcher_),
        manager_(&control_, &dispatcher_, &metrics_),
        provider_(&control_, &dispatcher_, &metrics_, &manager_) {}
  virtual ~EthernetEapProviderTest() {}

  MOCK_METHOD0(Callback0, void());
  MOCK_METHOD0(Callback1, void());

 protected:
  const EthernetEapProvider::CallbackMap& CallbackMap() {
    return provider_.callback_map_;
  }

  MockControl control_;
  MockEventDispatcher dispatcher_;
  MockMetrics metrics_;
  MockManager manager_;
  EthernetEapProvider provider_;
};

TEST_F(EthernetEapProviderTest, Construct) {
  EXPECT_EQ(ServiceRefPtr(), provider_.service());
  EXPECT_TRUE(CallbackMap().empty());
}

TEST_F(EthernetEapProviderTest, StartAndStop) {
  ServiceRefPtr service;
  EXPECT_CALL(manager_, RegisterService(_)).WillOnce(SaveArg<0>(&service));
  provider_.Start();
  EXPECT_NE(ServiceRefPtr(), provider_.service());
  EXPECT_EQ(service, provider_.service());

  EXPECT_CALL(manager_, DeregisterService(service));
  provider_.Stop();
  EXPECT_EQ(service, provider_.service());

  // Provider re-uses the same service on restart.
  EXPECT_CALL(manager_, RegisterService(service));
  provider_.Start();
  Mock::VerifyAndClearExpectations(&manager_);
}

TEST_F(EthernetEapProviderTest, CredentialChangeCallback) {
  EXPECT_CALL(*this, Callback0()).Times(0);
  EXPECT_CALL(*this, Callback1()).Times(0);
  provider_.OnCredentialsChanged();

  scoped_refptr<MockEthernet> device0 = new MockEthernet(&control_,
                                                         &dispatcher_,
                                                         &metrics_,
                                                         &manager_,
                                                         "eth0",
                                                         "addr0",
                                                         0);
  EthernetEapProvider::CredentialChangeCallback callback0 =
      base::Bind(&EthernetEapProviderTest::Callback0,
                 base::Unretained(this));

  provider_.SetCredentialChangeCallback(device0.get(), callback0);
  EXPECT_CALL(*this, Callback0());
  EXPECT_CALL(*this, Callback1()).Times(0);
  provider_.OnCredentialsChanged();

  scoped_refptr<MockEthernet> device1 = new MockEthernet(&control_,
                                                         &dispatcher_,
                                                         &metrics_,
                                                         &manager_,
                                                         "eth1",
                                                         "addr1",
                                                         1);
  EthernetEapProvider::CredentialChangeCallback callback1 =
      base::Bind(&EthernetEapProviderTest::Callback1,
                 base::Unretained(this));

  provider_.SetCredentialChangeCallback(device1.get(), callback1);
  EXPECT_CALL(*this, Callback0());
  EXPECT_CALL(*this, Callback1());
  provider_.OnCredentialsChanged();

  provider_.SetCredentialChangeCallback(device1.get(), callback0);
  EXPECT_CALL(*this, Callback0()).Times(2);
  EXPECT_CALL(*this, Callback1()).Times(0);
  provider_.OnCredentialsChanged();

  provider_.ClearCredentialChangeCallback(device0.get());
  EXPECT_CALL(*this, Callback0());
  EXPECT_CALL(*this, Callback1()).Times(0);
  provider_.OnCredentialsChanged();

  provider_.ClearCredentialChangeCallback(device1.get());
  EXPECT_CALL(*this, Callback0()).Times(0);
  EXPECT_CALL(*this, Callback1()).Times(0);
  provider_.OnCredentialsChanged();
}

TEST_F(EthernetEapProviderTest, ServiceConstructors) {
  ServiceRefPtr service;
  EXPECT_CALL(manager_, RegisterService(_)).WillOnce(SaveArg<0>(&service));
  provider_.Start();
  KeyValueStore args;
  args.SetString(kTypeProperty, kTypeEthernetEap);
  {
    Error error;
    EXPECT_EQ(service, provider_.GetService(args, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    EXPECT_EQ(service, provider_.FindSimilarService(args, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  {
    Error error;
    Mock::VerifyAndClearExpectations(&manager_);
    EXPECT_CALL(manager_, RegisterService(_)).Times(0);
    ServiceRefPtr temp_service = provider_.CreateTemporaryService(args, &error);
    EXPECT_TRUE(error.IsSuccess());
    // Returned service should be non-NULL but not the provider's own service.
    EXPECT_NE(ServiceRefPtr(), temp_service);
    EXPECT_NE(service, temp_service);
  }
}

}  // namespace shill

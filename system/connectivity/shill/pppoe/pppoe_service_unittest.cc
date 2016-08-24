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

#include "shill/pppoe/pppoe_service.h"

#include <map>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <base/memory/ref_counted.h>

#include "shill/ethernet/mock_ethernet.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_external_task.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_ppp_device.h"
#include "shill/mock_ppp_device_factory.h"
#include "shill/mock_process_manager.h"
#include "shill/service.h"
#include "shill/test_event_dispatcher.h"
#include "shill/testing.h"

using std::map;
using std::string;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;
using testing::_;

namespace shill {

class PPPoEServiceTest : public testing::Test {
 public:
  PPPoEServiceTest()
      : metrics_(&dispatcher_),
        manager_(&control_interface_, &dispatcher_, &metrics_),
        ethernet_(new MockEthernet(&control_interface_,
                                   &dispatcher_,
                                   &metrics_,
                                   &manager_,
                                   "ethernet",
                                   "aabbccddeeff",
                                   0)),
        device_info_(&control_interface_,
                     &dispatcher_,
                     &metrics_,
                     &manager_),
        service_(new PPPoEService(&control_interface_,
                                  &dispatcher_,
                                  &metrics_,
                                  &manager_,
                                  ethernet_->weak_ptr_factory_.GetWeakPtr())) {
    manager_.set_mock_device_info(&device_info_);
    service_->process_manager_ = &process_manager_;
  }

  ~PPPoEServiceTest() override {
    Error error;
    service_->Disconnect(&error, __func__);
    dispatcher_.DispatchPendingEvents();
  }

 protected:
  void FakeConnectionSuccess() {
    EXPECT_CALL(*ethernet_, link_up())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(process_manager_, StartProcess(_, _, _, _, _, _))
        .WillOnce(Return(0));
    Error error;
    service_->Connect(&error, "in test");
    EXPECT_TRUE(error.IsSuccess());
  }

  EventDispatcherForTest dispatcher_;
  MockMetrics metrics_;
  MockControl control_interface_;
  MockProcessManager process_manager_;
  MockManager manager_;
  scoped_refptr<MockEthernet> ethernet_;
  MockDeviceInfo device_info_;

  scoped_refptr<PPPoEService> service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PPPoEServiceTest);
};

MATCHER_P(LinkNamed, name, "") {
  return arg->link_name() == name;
}

TEST_F(PPPoEServiceTest, AuthenticationFailure) {
  EXPECT_CALL(*ethernet_, link_up()).WillRepeatedly(Return(true));
  FakeConnectionSuccess();
  map<string, string> empty_dict;
  service_->Notify(kPPPReasonAuthenticating, empty_dict);
  service_->Notify(kPPPReasonDisconnect, empty_dict);
  EXPECT_EQ(service_->state(), Service::kStateFailure);
  EXPECT_EQ(service_->failure(), Service::kFailurePPPAuth);
}

TEST_F(PPPoEServiceTest, DisconnectBeforeConnect) {
  EXPECT_CALL(*ethernet_, link_up()).WillRepeatedly(Return(true));
  FakeConnectionSuccess();
  map<string, string> empty_dict;
  service_->Notify(kPPPReasonAuthenticating, empty_dict);
  service_->Notify(kPPPReasonAuthenticated, empty_dict);
  service_->Notify(kPPPReasonDisconnect, empty_dict);
  EXPECT_EQ(service_->state(), Service::kStateFailure);
  EXPECT_EQ(service_->failure(), Service::kFailureUnknown);
}

TEST_F(PPPoEServiceTest, ConnectFailsWhenEthernetLinkDown) {
  EXPECT_CALL(*ethernet_, link_up()).WillRepeatedly(Return(false));

  Error error;
  service_->Connect(&error, "in test");
  EXPECT_FALSE(error.IsSuccess());
}

TEST_F(PPPoEServiceTest, OnPPPConnected) {
  // Setup device factory.
  MockPPPDeviceFactory* factory = MockPPPDeviceFactory::GetInstance();
  service_->ppp_device_factory_ = factory;

  static const char kLinkName[] = "ppp0";
  map<string, string> params = { {kPPPInterfaceName, kLinkName} };
  MockPPPDevice* device = new MockPPPDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_, kLinkName, 0);

  EXPECT_CALL(device_info_, GetIndex(StrEq(kLinkName))).WillOnce(Return(0));
  EXPECT_CALL(*factory, CreatePPPDevice(_, _, _, _, _, _))
      .WillOnce(Return(device));
  EXPECT_CALL(device_info_, RegisterDevice(IsRefPtrTo(device)));
  EXPECT_CALL(*device, SetEnabled(true));
  EXPECT_CALL(*device, SelectService(_));
  EXPECT_CALL(*device, UpdateIPConfigFromPPP(params, false));
#ifndef DISABLE_DHCPV6
  EXPECT_CALL(manager_, IsDHCPv6EnabledForDevice(StrEq(kLinkName)))
      .WillOnce(Return(true));
  EXPECT_CALL(*device, AcquireIPv6Config());
#endif  // DISABLE_DHCPV6
  EXPECT_CALL(manager_, OnInnerDevicesChanged());
  service_->OnPPPConnected(params);
  Mock::VerifyAndClearExpectations(&manager_);
}

TEST_F(PPPoEServiceTest, Connect) {
  static const char kLinkName[] = "ppp0";

  EXPECT_CALL(*ethernet_, link_up()).WillRepeatedly(Return(true));
  EXPECT_CALL(device_info_, GetIndex(StrEq(kLinkName))).WillOnce(Return(0));
  EXPECT_CALL(device_info_, RegisterDevice(LinkNamed(kLinkName)));

  FakeConnectionSuccess();
  map<string, string> empty_dict;
  service_->Notify(kPPPReasonAuthenticating, empty_dict);
  service_->Notify(kPPPReasonAuthenticated, empty_dict);

  map<string, string> connect_dict = {
    {kPPPInterfaceName, kLinkName},
  };
  service_->Notify(kPPPReasonConnect, connect_dict);
  EXPECT_EQ(service_->state(), Service::kStateOnline);
}

TEST_F(PPPoEServiceTest, Disconnect) {
  FakeConnectionSuccess();

  auto weak_ptr = service_->weak_ptr_factory_.GetWeakPtr();
  MockExternalTask* pppd = new MockExternalTask(
      &control_interface_, &process_manager_, weak_ptr,
      base::Bind(&PPPoEService::OnPPPDied, weak_ptr));
  service_->pppd_.reset(pppd);

  MockPPPDevice* ppp_device = new MockPPPDevice(
      &control_interface_, &dispatcher_, &metrics_, &manager_, "ppp0", 0);
  service_->ppp_device_ = ppp_device;

  EXPECT_CALL(*ppp_device, DropConnection());
  EXPECT_CALL(*pppd, OnDelete());

  {
    Error error;
    service_->Disconnect(&error, "in test");
    EXPECT_TRUE(error.IsSuccess());
  }
}

TEST_F(PPPoEServiceTest, DisconnectDuringAssociation) {
  FakeConnectionSuccess();

  Error error;
  service_->Disconnect(&error, "in test");
  EXPECT_TRUE(error.IsSuccess());

  EXPECT_EQ(service_->state(), Service::kStateIdle);
}

}  // namespace shill

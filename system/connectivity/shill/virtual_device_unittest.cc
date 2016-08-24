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

#include "shill/virtual_device.h"

#include <sys/socket.h>
#include <linux/if.h>  // NOLINT - Needs typedefs from sys/socket.h.

#include <gtest/gtest.h>

#include "shill/event_dispatcher.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_store.h"
#include "shill/net/mock_rtnl_handler.h"
#include "shill/nice_mock_control.h"
#include "shill/technology.h"

using testing::_;
using testing::StrictMock;

namespace shill {

namespace {
const char kTestDeviceName[] = "tun0";
const int kTestInterfaceIndex = 5;
}  // namespace

class VirtualDeviceTest : public testing::Test {
 public:
  VirtualDeviceTest()
      : metrics_(&dispatcher_),
        manager_(&control_, &dispatcher_, &metrics_),
        device_(new VirtualDevice(&control_,
                                  &dispatcher_,
                                  &metrics_,
                                  &manager_,
                                  kTestDeviceName,
                                  kTestInterfaceIndex,
                                  Technology::kVPN)) {}

  virtual ~VirtualDeviceTest() {}

  virtual void SetUp() {
    device_->rtnl_handler_ = &rtnl_handler_;
  }

 protected:
  NiceMockControl control_;
  EventDispatcher dispatcher_;
  MockMetrics metrics_;
  MockManager manager_;
  StrictMock<MockRTNLHandler> rtnl_handler_;

  VirtualDeviceRefPtr device_;
};

TEST_F(VirtualDeviceTest, technology) {
  EXPECT_EQ(Technology::kVPN, device_->technology());
  EXPECT_NE(Technology::kEthernet, device_->technology());
}

TEST_F(VirtualDeviceTest, Load) {
  StrictMock<MockStore> storage;
  EXPECT_CALL(storage, ContainsGroup(_)).Times(0);
  EXPECT_TRUE(device_->Load(&storage));
}

TEST_F(VirtualDeviceTest, Save) {
  StrictMock<MockStore> storage;
  EXPECT_CALL(storage, SetBool(_, _, _)).Times(0);  // Or any type, really.
  EXPECT_TRUE(device_->Save(&storage));
}

TEST_F(VirtualDeviceTest, Start) {
  Error error(Error::kOperationInitiated);
  EXPECT_CALL(rtnl_handler_, SetInterfaceFlags(_, IFF_UP, IFF_UP));
  device_->Start(&error, EnabledStateChangedCallback());
  EXPECT_TRUE(error.IsSuccess());
}

TEST_F(VirtualDeviceTest, Stop) {
  Error error(Error::kOperationInitiated);
  device_->Stop(&error, EnabledStateChangedCallback());
  EXPECT_TRUE(error.IsSuccess());
}

// TODO(quiche): Add test for UpdateIPConfig. crbug.com/266404

}  // namespace shill

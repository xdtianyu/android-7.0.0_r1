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

#include "shill/upstart/upstart.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_control.h"
#include "shill/upstart/mock_upstart_proxy.h"
#include "shill/upstart/upstart_proxy_interface.h"

using testing::_;
using testing::Test;

namespace shill {

namespace {

class FakeControl : public MockControl {
 public:
  FakeControl()
      : upstart_proxy_raw_(new MockUpstartProxy),
        upstart_proxy_(upstart_proxy_raw_) {}

  UpstartProxyInterface* CreateUpstartProxy() override {
    CHECK(upstart_proxy_);
    // Passes ownership.
    return upstart_proxy_.release();
  }

  // Can not guarantee that the returned object is alive.
  MockUpstartProxy* upstart_proxy() const {
    return upstart_proxy_raw_;
  }

 private:
  MockUpstartProxy* const upstart_proxy_raw_;
  std::unique_ptr<MockUpstartProxy> upstart_proxy_;
};

}  // namespace

class UpstartTest : public Test {
 public:
  UpstartTest()
      : upstart_(&control_),
        upstart_proxy_(control_.upstart_proxy()) {}

 protected:
  FakeControl control_;
  Upstart upstart_;
  MockUpstartProxy* const upstart_proxy_;
};

TEST_F(UpstartTest, NotifyDisconnected) {
  EXPECT_CALL(*upstart_proxy_, EmitEvent("shill-disconnected", _, false));
  upstart_.NotifyDisconnected();
}

TEST_F(UpstartTest, NotifyConnected) {
  EXPECT_CALL(*upstart_proxy_, EmitEvent("shill-connected", _, false));
  upstart_.NotifyConnected();
}

}  // namespace shill

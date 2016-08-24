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

#include "shill/net/rtnl_listener.h"

#include <linux/netlink.h>
#include <sys/socket.h>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/net/rtnl_handler.h"
#include "shill/net/rtnl_message.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using testing::_;
using testing::A;
using testing::Test;

namespace shill {

class RTNLListenerTest : public Test {
 public:
  RTNLListenerTest()
      : callback_(Bind(&RTNLListenerTest::ListenerCallback,
                       Unretained(this))) {}

  MOCK_METHOD1(ListenerCallback, void(const RTNLMessage&));

  virtual void SetUp() {
    // RTNLHandler is a singleton, there's no guarentee that it is not
    // setup/used by other unittests. Clear "listeners_" field before we run
    // tests.
    RTNLHandler::GetInstance()->listeners_.clear();
  }

 protected:
  Callback<void(const RTNLMessage&)> callback_;
};

TEST_F(RTNLListenerTest, NoRun) {
  {
    RTNLListener listener(RTNLHandler::kRequestAddr, callback_);
    EXPECT_EQ(1, RTNLHandler::GetInstance()->listeners_.size());
    RTNLMessage message;
    EXPECT_CALL(*this, ListenerCallback(_)).Times(0);
    listener.NotifyEvent(RTNLHandler::kRequestLink, message);
  }
  EXPECT_EQ(0, RTNLHandler::GetInstance()->listeners_.size());
}

TEST_F(RTNLListenerTest, Run) {
  {
    RTNLListener listener(
        RTNLHandler::kRequestLink | RTNLHandler::kRequestAddr,
        callback_);
    EXPECT_EQ(1, RTNLHandler::GetInstance()->listeners_.size());
    RTNLMessage message;
    EXPECT_CALL(*this, ListenerCallback(A<const RTNLMessage&>())).Times(1);
    listener.NotifyEvent(RTNLHandler::kRequestLink, message);
  }
  EXPECT_EQ(0, RTNLHandler::GetInstance()->listeners_.size());
}

}  // namespace shill

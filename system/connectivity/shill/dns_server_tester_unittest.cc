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

#include "shill/dns_server_tester.h"

#include <memory>
#include <string>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_dns_client.h"
#include "shill/mock_dns_client_factory.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/net/mock_time.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;
using std::vector;
using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::InSequence;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgumentPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kInterfaceName[] = "int0";
const char kDNSServer0[] = "8.8.8.8";
const char kDNSServer1[] = "8.8.4.4";
const char* kDNSServers[] = { kDNSServer0, kDNSServer1 };
}  // namespace

class DNSServerTesterTest : public Test {
 public:
  DNSServerTesterTest()
      : device_info_(
            new NiceMock<MockDeviceInfo>(&control_, nullptr, nullptr, nullptr)),
        connection_(new StrictMock<MockConnection>(device_info_.get())),
        interface_name_(kInterfaceName),
        dns_servers_(kDNSServers, kDNSServers + 2) {}

  virtual void SetUp() {
    EXPECT_CALL(*connection_.get(), interface_name())
          .WillRepeatedly(ReturnRef(interface_name_));
    dns_server_tester_.reset(
        new DNSServerTester(connection_.get(),
                            &dispatcher_,
                            dns_servers_,
                            false,
                            callback_target_.result_callback()));
  }

 protected:
  class CallbackTarget {
   public:
    CallbackTarget()
        : result_callback_(Bind(&CallbackTarget::ResultCallback,
                                Unretained(this))) {
    }

    MOCK_METHOD1(ResultCallback, void(const DNSServerTester::Status status));
    Callback<void(const DNSServerTester::Status)>& result_callback() {
      return result_callback_;
    }

   private:
    Callback<void(const DNSServerTester::Status)> result_callback_;
  };

  DNSServerTester* dns_server_tester() { return dns_server_tester_.get(); }
  MockEventDispatcher& dispatcher() { return dispatcher_; }
  CallbackTarget& callback_target() { return callback_target_; }

  void ExpectReset() {
    EXPECT_TRUE(callback_target_.result_callback().Equals(
        dns_server_tester_->dns_result_callback_));
  }

 private:
  StrictMock<MockEventDispatcher> dispatcher_;
  MockControl control_;
  std::unique_ptr<MockDeviceInfo> device_info_;
  scoped_refptr<MockConnection> connection_;
  CallbackTarget callback_target_;
  const string interface_name_;
  vector<string> dns_servers_;
  std::unique_ptr<DNSServerTester> dns_server_tester_;
};

TEST_F(DNSServerTesterTest, Constructor) {
  ExpectReset();
}

TEST_F(DNSServerTesterTest, StartAttempt) {
  // Start attempt with no delay.
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  dns_server_tester()->StartAttempt(0);

  // Start attempt with delay.
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 100));
  dns_server_tester()->StartAttempt(100);
}

TEST_F(DNSServerTesterTest, StartAttemptTask) {
  // Setup mock DNS test client.
  MockDNSClient* dns_test_client = new MockDNSClient();
  dns_server_tester()->dns_test_client_.reset(dns_test_client);

  // DNS test task started successfully.
  EXPECT_CALL(*dns_test_client, Start(_, _)).WillOnce(Return(true));
  EXPECT_CALL(callback_target(), ResultCallback(_)).Times(0);
  dns_server_tester()->StartAttemptTask();
  Mock::VerifyAndClearExpectations(dns_test_client);

  // DNS test task failed to start.
  EXPECT_CALL(*dns_test_client, Start(_, _)).WillOnce(Return(false));
  EXPECT_CALL(callback_target(),
              ResultCallback(DNSServerTester::kStatusFailure)).Times(1);
  dns_server_tester()->StartAttemptTask();
  Mock::VerifyAndClearExpectations(dns_test_client);
}

TEST_F(DNSServerTesterTest, AttemptCompleted) {
  // DNS test attempt succeed with retry_until_success_ not set.
  dns_server_tester()->retry_until_success_ = false;
  EXPECT_CALL(callback_target(),
              ResultCallback(DNSServerTester::kStatusSuccess)).Times(1);
  dns_server_tester()->CompleteAttempt(DNSServerTester::kStatusSuccess);

  // DNS test attempt succeed with retry_until_success_ being set.
  dns_server_tester()->retry_until_success_ = true;
  EXPECT_CALL(callback_target(),
              ResultCallback(DNSServerTester::kStatusSuccess)).Times(1);
  dns_server_tester()->CompleteAttempt(DNSServerTester::kStatusSuccess);

  // DNS test attempt failed with retry_until_success_ not set.
  dns_server_tester()->retry_until_success_ = false;
  EXPECT_CALL(callback_target(),
              ResultCallback(DNSServerTester::kStatusFailure)).Times(1);
  dns_server_tester()->CompleteAttempt(DNSServerTester::kStatusFailure);

  // DNS test attempt failed with retry_until_success_ being set.
  dns_server_tester()->retry_until_success_ = true;
  EXPECT_CALL(callback_target(), ResultCallback(_)).Times(0);
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, _)).Times(1);
  dns_server_tester()->CompleteAttempt(DNSServerTester::kStatusFailure);
}

TEST_F(DNSServerTesterTest, StopAttempt) {
  // Setup mock DNS test client.
  MockDNSClient* dns_test_client = new MockDNSClient();
  dns_server_tester()->dns_test_client_.reset(dns_test_client);

  // DNS test task started successfully.
  EXPECT_CALL(*dns_test_client, Start(_, _)).WillOnce(Return(true));
  EXPECT_CALL(callback_target(), ResultCallback(_)).Times(0);
  dns_server_tester()->StartAttemptTask();
  Mock::VerifyAndClearExpectations(dns_test_client);

  // Stop the DNS test attempt.
  EXPECT_CALL(*dns_test_client, Stop()).Times(1);
  EXPECT_CALL(callback_target(), ResultCallback(_)).Times(0);
  dns_server_tester()->StopAttempt();
  Mock::VerifyAndClearExpectations(dns_test_client);
}

}  // namespace shill

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

#include "shill/connectivity_trial.h"

#include <memory>
#include <string>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_http_request.h"
#include "shill/net/mock_time.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;
using std::unique_ptr;
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
const char kBadURL[] = "badurl";
const char kInterfaceName[] = "int0";
const char kURL[] = "http://www.chromium.org";
const char kDNSServer0[] = "8.8.8.8";
const char kDNSServer1[] = "8.8.4.4";
const char* kDNSServers[] = { kDNSServer0, kDNSServer1 };
}  // namespace

MATCHER_P(IsResult, result, "") {
  return (result.phase == arg.phase &&
          result.status == arg.status);
}

class ConnectivityTrialTest : public Test {
 public:
  ConnectivityTrialTest()
      : device_info_(
            new NiceMock<MockDeviceInfo>(&control_, nullptr, nullptr, nullptr)),
        connection_(new StrictMock<MockConnection>(device_info_.get())),
        connectivity_trial_(new ConnectivityTrial(
            connection_.get(), &dispatcher_, kTrialTimeout,
            callback_target_.result_callback())),
        interface_name_(kInterfaceName),
        dns_servers_(kDNSServers, kDNSServers + 2),
        http_request_(nullptr) {
    current_time_.tv_sec = current_time_.tv_usec = 0;
  }

  virtual void SetUp() {
    EXPECT_CALL(*connection_.get(), IsIPv6())
                .WillRepeatedly(Return(false));
    EXPECT_CALL(*connection_.get(), interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
    EXPECT_CALL(time_, GetTimeMonotonic(_))
        .WillRepeatedly(Invoke(this, &ConnectivityTrialTest::GetTimeMonotonic));
    EXPECT_CALL(*connection_.get(), dns_servers())
        .WillRepeatedly(ReturnRef(dns_servers_));
    EXPECT_FALSE(connectivity_trial_->request_.get());
  }

  virtual void TearDown() {
    Mock::VerifyAndClearExpectations(&http_request_);
    if (connectivity_trial_->request_.get()) {
      EXPECT_CALL(*http_request(), Stop());

      // Delete the ConnectivityTrial while expectations still exist.
      connectivity_trial_.reset();
    }
  }

 protected:
  static const int kNumAttempts;
  static const int kTrialTimeout;

  class CallbackTarget {
   public:
    CallbackTarget()
        : result_callback_(Bind(&CallbackTarget::ResultCallback,
                                Unretained(this))) {
    }

    MOCK_METHOD1(ResultCallback, void(ConnectivityTrial::Result result));
    Callback<void(ConnectivityTrial::Result)>& result_callback() {
      return result_callback_;
    }

   private:
    Callback<void(ConnectivityTrial::Result)> result_callback_;
  };

  void AssignHTTPRequest() {
    http_request_ = new StrictMock<MockHTTPRequest>(connection_);
    connectivity_trial_->request_.reset(http_request_);  // Passes ownership.
  }

  bool StartTrialWithDelay(const string& url_string, int delay) {
    bool ret = connectivity_trial_->Start(url_string, delay);
    if (ret) {
      AssignHTTPRequest();
    }
    return ret;
  }

  bool StartTrial(const string& url_string) {
    return StartTrialWithDelay(url_string, 0);
  }

  void StartTrialTask() {
    AssignHTTPRequest();
    EXPECT_CALL(*http_request(), Start(_, _, _))
        .WillOnce(Return(HTTPRequest::kResultInProgress));
    EXPECT_CALL(dispatcher(), PostDelayedTask(_, kTrialTimeout * 1000));
    connectivity_trial()->StartTrialTask();
  }

  void ExpectTrialReturn(const ConnectivityTrial::Result& result) {
    EXPECT_CALL(callback_target(), ResultCallback(IsResult(result)));

    // Expect the PortalDetector to stop the current request.
    EXPECT_CALL(*http_request(), Stop());
  }

  void TimeoutTrial() {
    connectivity_trial_->TimeoutTrialTask();
  }

  MockHTTPRequest* http_request() { return http_request_; }
  ConnectivityTrial* connectivity_trial() { return connectivity_trial_.get(); }
  MockEventDispatcher& dispatcher() { return dispatcher_; }
  CallbackTarget& callback_target() { return callback_target_; }
  ByteString& response_data() { return response_data_; }

  void ExpectReset() {
    EXPECT_TRUE(callback_target_.result_callback().
                Equals(connectivity_trial_->trial_callback_));
    EXPECT_FALSE(connectivity_trial_->request_.get());
  }

  void ExpectTrialRetry(const ConnectivityTrial::Result& result, int delay) {
    EXPECT_CALL(callback_target(), ResultCallback(IsResult(result)));

    // Expect the ConnectivityTrial to stop the current request.
    EXPECT_CALL(*http_request(), Stop());

    // Expect the ConnectivityTrial to schedule the next attempt.
    EXPECT_CALL(dispatcher(), PostDelayedTask(_, delay));
  }

  void AdvanceTime(int milliseconds) {
    struct timeval tv = { milliseconds / 1000, (milliseconds % 1000) * 1000 };
    timeradd(&current_time_, &tv, &current_time_);
  }

  void StartTrial() {
    EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
    EXPECT_TRUE(StartTrial(kURL));

    // Expect that the request will be started -- return failure.
    EXPECT_CALL(*http_request(), Start(_, _, _))
        .WillOnce(Return(HTTPRequest::kResultInProgress));
    EXPECT_CALL(dispatcher(), PostDelayedTask(
        _, kTrialTimeout * 1000));

    connectivity_trial()->StartTrialTask();
  }

  void AppendReadData(const string& read_data) {
    response_data_.Append(ByteString(read_data, false));
    connectivity_trial_->RequestReadCallback(response_data_);
  }

 private:
  int GetTimeMonotonic(struct timeval* tv) {
    *tv = current_time_;
    return 0;
  }

  StrictMock<MockEventDispatcher> dispatcher_;
  MockControl control_;
  unique_ptr<MockDeviceInfo> device_info_;
  scoped_refptr<MockConnection> connection_;
  CallbackTarget callback_target_;
  unique_ptr<ConnectivityTrial> connectivity_trial_;
  StrictMock<MockTime> time_;
  struct timeval current_time_;
  const string interface_name_;
  vector<string> dns_servers_;
  ByteString response_data_;
  MockHTTPRequest* http_request_;
};

// static
const int ConnectivityTrialTest::kNumAttempts = 0;
const int ConnectivityTrialTest::kTrialTimeout = 4;

TEST_F(ConnectivityTrialTest, Constructor) {
  ExpectReset();
}

TEST_F(ConnectivityTrialTest, InvalidURL) {
  EXPECT_FALSE(connectivity_trial()->IsActive());
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0)).Times(0);
  EXPECT_FALSE(StartTrial(kBadURL));
  ExpectReset();

  EXPECT_FALSE(connectivity_trial()->Retry(0));
  EXPECT_FALSE(connectivity_trial()->IsActive());
}

TEST_F(ConnectivityTrialTest, IsActive) {
  // Before the trial is started, should not be active.
  EXPECT_FALSE(connectivity_trial()->IsActive());

  // Once the trial is started, IsActive should return true.
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));
  StartTrialTask();
  EXPECT_TRUE(connectivity_trial()->IsActive());

  // Finish the trial, IsActive should return false.
  EXPECT_CALL(*http_request(), Stop());
  connectivity_trial()->CompleteTrial(
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                ConnectivityTrial::kStatusFailure));
  EXPECT_FALSE(connectivity_trial()->IsActive());
}

TEST_F(ConnectivityTrialTest, StartAttemptFailed) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  // Expect that the request will be started -- return failure.
  EXPECT_CALL(*http_request(), Start(_, _, _))
      .WillOnce(Return(HTTPRequest::kResultConnectionFailure));
  // Expect a failure to be relayed to the caller.
  EXPECT_CALL(callback_target(),
              ResultCallback(IsResult(
                  ConnectivityTrial::Result(
                      ConnectivityTrial::kPhaseConnection,
                      ConnectivityTrial::kStatusFailure))))
      .Times(1);

  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0)).Times(0);
  EXPECT_CALL(*http_request(), Stop());

  connectivity_trial()->StartTrialTask();
}

TEST_F(ConnectivityTrialTest, StartRepeated) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0)).Times(1);
  EXPECT_TRUE(StartTrial(kURL));

  // A second call should cancel the existing trial and set up the new one.
  EXPECT_CALL(*http_request(), Stop());
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 10)).Times(1);
  EXPECT_TRUE(StartTrialWithDelay(kURL, 10));
}

TEST_F(ConnectivityTrialTest, StartTrialAfterDelay) {
  const int kDelaySeconds = 123;
  // The trial should be delayed by kDelaySeconds.
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, kDelaySeconds));
  EXPECT_TRUE(StartTrialWithDelay(kURL, kDelaySeconds));
}

TEST_F(ConnectivityTrialTest, TrialRetry) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  // Expect that the request will be started -- return failure.
  EXPECT_CALL(*http_request(), Start(_, _, _))
      .WillOnce(Return(HTTPRequest::kResultConnectionFailure));
  EXPECT_CALL(*http_request(), Stop());
  connectivity_trial()->StartTrialTask();

  const int kRetryDelay = 7;
  EXPECT_CALL(*http_request(), Stop());
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, kRetryDelay)).Times(1);
  EXPECT_TRUE(connectivity_trial()->Retry(kRetryDelay));
}

TEST_F(ConnectivityTrialTest, TrialRetryFail) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  EXPECT_CALL(*http_request(), Stop());
  connectivity_trial()->Stop();

  EXPECT_FALSE(connectivity_trial()->Retry(0));
}

// Exactly like AttemptCount, except that the termination conditions are
// different because we're triggering a different sort of error.
TEST_F(ConnectivityTrialTest, ReadBadHeadersRetry) {
  int num_failures = 3;
  int sec_between_attempts = 3;

  // Expect ConnectivityTrial to immediately post a task for the each attempt.
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  // Expect that the request will be started and return the in progress status.
  EXPECT_CALL(*http_request(), Start(_, _, _))
      .Times(num_failures).WillRepeatedly(
          Return(HTTPRequest::kResultInProgress));

  // Each HTTP request that gets started will have a request timeout.
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, kTrialTimeout * 1000))
      .Times(num_failures);

  // Expect failures for all attempts but the last.
  EXPECT_CALL(callback_target(),
              ResultCallback(IsResult(
                  ConnectivityTrial::Result(
                      ConnectivityTrial::kPhaseContent,
                      ConnectivityTrial::kStatusFailure))))
      .Times(num_failures);

  // Expect the ConnectivityTrial to stop the current request each time, plus
  // an extra time in ConnectivityTrial::Stop().
  ByteString response_data("X", 1);

  for (int i = 0; i < num_failures; ++i) {
    connectivity_trial()->StartTrialTask();
    AdvanceTime(sec_between_attempts * 1000);
    EXPECT_CALL(*http_request(), Stop()).Times(2);
    EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0)).Times(1);
    connectivity_trial()->RequestReadCallback(response_data);
    EXPECT_TRUE(connectivity_trial()->Retry(0));
  }
}


TEST_F(ConnectivityTrialTest, ReadBadHeader) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  StartTrialTask();

  ExpectTrialReturn(ConnectivityTrial::Result(
      ConnectivityTrial::kPhaseContent,
      ConnectivityTrial::kStatusFailure));
  AppendReadData("X");
}

TEST_F(ConnectivityTrialTest, RequestTimeout) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  StartTrialTask();

  ExpectTrialReturn(ConnectivityTrial::Result(
      ConnectivityTrial::kPhaseUnknown,
      ConnectivityTrial::kStatusTimeout));

  EXPECT_CALL(*http_request(), response_data())
      .WillOnce(ReturnRef(response_data()));

  TimeoutTrial();
}

TEST_F(ConnectivityTrialTest, ReadPartialHeaderTimeout) {
  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  StartTrialTask();


  const string response_expected(ConnectivityTrial::kResponseExpected);
  const size_t partial_size = response_expected.length() / 2;
  AppendReadData(response_expected.substr(0, partial_size));

  ExpectTrialReturn(ConnectivityTrial::Result(
      ConnectivityTrial::kPhaseContent,
      ConnectivityTrial::kStatusTimeout));

  EXPECT_CALL(*http_request(), response_data())
      .WillOnce(ReturnRef(response_data()));

  TimeoutTrial();
}

TEST_F(ConnectivityTrialTest, ReadCompleteHeader) {
  const string response_expected(ConnectivityTrial::kResponseExpected);
  const size_t partial_size = response_expected.length() / 2;

  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  StartTrialTask();

  AppendReadData(response_expected.substr(0, partial_size));

  ExpectTrialReturn(ConnectivityTrial::Result(
      ConnectivityTrial::kPhaseContent,
      ConnectivityTrial::kStatusSuccess));

  AppendReadData(response_expected.substr(partial_size));
}

TEST_F(ConnectivityTrialTest, ReadMatchingHeader) {
  const string kResponse("HTTP/9.8 204");

  EXPECT_CALL(dispatcher(), PostDelayedTask(_, 0));
  EXPECT_TRUE(StartTrial(kURL));

  StartTrialTask();

  ExpectTrialReturn(ConnectivityTrial::Result(
      ConnectivityTrial::kPhaseContent,
      ConnectivityTrial::kStatusSuccess));

  AppendReadData(kResponse);
}

struct ResultMapping {
  ResultMapping() : http_result(HTTPRequest::kResultUnknown), trial_result() {}
  ResultMapping(HTTPRequest::Result in_http_result,
                const ConnectivityTrial::Result& in_trial_result)
      : http_result(in_http_result),
        trial_result(in_trial_result) {}
  HTTPRequest::Result http_result;
  ConnectivityTrial::Result trial_result;
};

class ConnectivityTrialResultMappingTest
    : public testing::TestWithParam<ResultMapping> {};

TEST_P(ConnectivityTrialResultMappingTest, MapResult) {
  ConnectivityTrial::Result trial_result =
      ConnectivityTrial::GetPortalResultForRequestResult(
          GetParam().http_result);
  EXPECT_EQ(trial_result.phase, GetParam().trial_result.phase);
  EXPECT_EQ(trial_result.status, GetParam().trial_result.status);
}

INSTANTIATE_TEST_CASE_P(
    TrialResultMappingTest,
    ConnectivityTrialResultMappingTest,
    ::testing::Values(
        ResultMapping(
            HTTPRequest::kResultUnknown,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseUnknown,
                                      ConnectivityTrial::kStatusFailure)),
        ResultMapping(
            HTTPRequest::kResultInProgress,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseUnknown,
                                      ConnectivityTrial::kStatusFailure)),
        ResultMapping(
            HTTPRequest::kResultDNSFailure,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseDNS,
                                      ConnectivityTrial::kStatusFailure)),
        ResultMapping(
            HTTPRequest::kResultDNSTimeout,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseDNS,
                                      ConnectivityTrial::kStatusTimeout)),
        ResultMapping(
            HTTPRequest::kResultConnectionFailure,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseConnection,
                                      ConnectivityTrial::kStatusFailure)),
        ResultMapping(
            HTTPRequest::kResultConnectionTimeout,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseConnection,
                                      ConnectivityTrial::kStatusTimeout)),
        ResultMapping(
            HTTPRequest::kResultRequestFailure,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseHTTP,
                                      ConnectivityTrial::kStatusFailure)),
        ResultMapping(
            HTTPRequest::kResultRequestTimeout,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseHTTP,
                                      ConnectivityTrial::kStatusTimeout)),
        ResultMapping(
            HTTPRequest::kResultResponseFailure,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseHTTP,
                                      ConnectivityTrial::kStatusFailure)),
        ResultMapping(
            HTTPRequest::kResultResponseTimeout,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseHTTP,
                                      ConnectivityTrial::kStatusTimeout)),
        ResultMapping(
            HTTPRequest::kResultSuccess,
            ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                      ConnectivityTrial::kStatusFailure))));

}  // namespace shill
